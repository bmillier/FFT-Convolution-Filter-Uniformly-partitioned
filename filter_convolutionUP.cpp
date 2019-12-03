/* Audio Library for Teensy 3.X
 * Uniformly-Partitioned convolution filter library written by Brian Millier
 * using routines adapted for the Teensy 4.0 by Frank, DD4WH that were 
 * based upon code/literature by Warren Pratt 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "filter_convolutionUP.h"


/******************************************************************/
//                A u d i o FilterConvolutionUP
// Uniformly-Partitioned Convolution Filter for Teeny 4.0
// Written by Brian Millier November 2019
// adapted from routines written for Teensy 4.0 by Frank DD4WH 
// that were based upon code/literature by Warren Pratt 
/*******************************************************************/


boolean AudioFilterConvolutionUP::begin(int status,float32_t gain, float32_t *fo, int nfor) 
{
	enabled = status;
	buffidx=0;
	k = 0;
	audio_gain = gain;
	if (nfor > 185) {
		nfor = 185;
	}
	np = nfor;   // number of 128 sample segments used
// define the pointer that is used to access the main program DMAMEM array fftout[]
	ptr_fftout = fo;
	ptr_fmask = &fmask[0][0];

	memset(ptr_fftout, 0, np*512*4);  // clear fftout array
	memset(fftin, 0,  512 * 4);  // clear fftin array
 return(true);
}

void AudioFilterConvolutionUP::passThrough(int stat)
{
  passThru=stat;
}

void AudioFilterConvolutionUP::impulse(const float32_t *guitar_cabinet_impulse, float32_t *maskgen,int size)
{
	const static arm_cfft_instance_f32* maskS;
	maskS = &arm_cfft_sR_f32_len256;
	int ptr;
	
	for (unsigned j = 0; j < np; j++)     
	{
		memset(maskgen, 0, partitionsize * 16);  // zero out maskgen
		// take part of impulse response and fill into maskgen
		for (unsigned i = 0; i < partitionsize; i++)
		{
			// THIS IS FOR REAL IMPULSE RESPONSES OR FIR COEFFICIENTS
			// the position of the impulse response coeffs (right or left aligned)
			// determines the useable part of the audio in the overlap-and-save (left or right part of the iFFT buffer)
			ptr = i + j * partitionsize;
			if (ptr < size) {
				maskgen[i * 2 + partitionsize * 2] = guitar_cabinet_impulse[ptr];
			}
			else  // Impulse is smaller than the fixed (defined) filter length (taps)
			{
				maskgen[i * 2 + partitionsize * 2] = 0.0;  // pad the last part of filter mask with zeros
			} 
		}
			// perform complex FFT on maskgen
		arm_cfft_f32(maskS, maskgen, 0, 1);
		// fill into fmask array
		for (unsigned i = 0; i < partitionsize * 4; i++)
		{
			fmask[j][i] = maskgen[i];
		}
	}
	enabled = 1;
	Serial.println("Mask file generated");
}


void AudioFilterConvolutionUP::update(void) {
	audio_block_t *blockL, *blockR;
	short* bp;
	if (enabled != 1) return;
	blockL = receiveWritable(0);   // MUST be Writable, as convolution results are written into block
	blockR = receiveWritable(1);	// ""
	if (blockL && blockR)
	{
		digitalWrite(14, HIGH);
		bp = blockL->data;
		arm_q15_to_float(bp, &float_buffer_L[0], 128); // convert int_buffer to float 32bit- 128 was buffer size
		bp = blockR->data;
		arm_q15_to_float(bp, &float_buffer_R[0], 128); // convert int_buffer to float 32bit- 128 was buffer size
		// fill FFT_buffer with last events audio samples
		for (unsigned i = 0; i < partitionsize; i++)
		{
			fftin[i * 2] = last_sample_buffer_L[i]; // real is left chan
	        fftin[i * 2 + 1] = last_sample_buffer_R[i]; // imaginary is right chan 
		// copy recent samples to last_sample_buffer for next time!
			last_sample_buffer_L[i] = float_buffer_L[i];
			last_sample_buffer_R[i] = float_buffer_R[i];
			// now fill recent audio samples into FFT_buffer (left channel: re, right channel: im)
			fftin[256 + i * 2] = float_buffer_L[i]; // real = left chan
			fftin[256 + i * 2 + 1] = float_buffer_R[i]; // imaginary = right chan
		}
		//**********************************************************************************
		//	  Complex Forward FFT
	   // calculation is performed in-place the FFT_buffer [re, im, re, im, re, im . . .]
		// complex FFT with the new library CMSIS V4.5
		// **********************************************************************************
		const static arm_cfft_instance_f32 *S;
		S = &arm_cfft_sR_f32_len256;
		arm_cfft_f32(S, fftin, 0, 1);
		int buffidx512 = buffidx * 512;
		ptr1 = ptr_fftout + (buffidx512);   // set pointer to proper segment of fftout array
		memcpy(ptr1, fftin, 2048);  // copy 512 samples from fftin to fftout (at proper segment)
		//**********************************************************************************
		//	 Complex multiplication with filter mask (precalculated coefficients subjected to an FFT)
		//	 this is taken from wdsp library by Warren Pratt firmin.c
		//  **********************************************************************************
		digitalWrite(14, HIGH);
		k = buffidx;
		memset(accum, 0, partitionsize * 16);  // clear accum array
		k512 = k * 512;  // save 7 k*512 multiplications per inner loop
		j512 = 0;
		for (unsigned j = 0; j < np; j++)      //BM np was nfor
		{

				ptr1 = ptr_fftout + k512;
				ptr2 = ptr_fmask + j512;
				// do a complex MAC (multiply/accumulate)
				arm_cmplx_mult_cmplx_f32(ptr1, ptr2, ac2, 256);  // This is the complex multiply
				for (int q = 0; q < 512; q=q+8) {    // this is the accumulate
					accum[q] += ac2[q];
					accum[q+1] += ac2[q+1];
					accum[q+2] += ac2[q+2];
					accum[q+3] += ac2[q+3];
					accum[q+4] += ac2[q+4];
					accum[q + 5] += ac2[q + 5];
					accum[q + 6] += ac2[q + 6];
					accum[q + 7] += ac2[q + 7];
				}
			k--;
			if (k < 0)
			{
				k = np - 1;
			}
			k512 = k * 512;
			j512 += 512;
		} // end np loop
		buffidx++;    
		buffidx = buffidx % np;

		//*********************************************************************************
		//	 Complex inverse FFT
		//  ********************************************************************************

		  // complex iFFT with the new library CMSIS V4.5
		const static arm_cfft_instance_f32 *iS;
		iS = &arm_cfft_sR_f32_len256;
		arm_cfft_f32(iS, accum, 1, 1);

		//**********************************************************************************
		//	  Overlap and save algorithm, which simply means yóu take only half of the buffer
		//	  and discard the other half (which contains unusable time-aliased audio).
		//	  Whether you take the left or the right part is determined by the position
		//	  of the zero-padding in the filter-mask-buffer before doing the FFT of the
		//	  impulse response coefficients
		//   *********************************************************************************

		for (unsigned i = 0; i < partitionsize; i++)
		{
			float_buffer_L[i] = accum[i * 2 + 0] * audio_gain;
			float_buffer_R[i] = accum[i * 2 + 1] * audio_gain;
		}
		bp = blockL->data;
		arm_float_to_q15(&float_buffer_L[0], bp, 128);  
		bp = blockR->data;
		arm_float_to_q15(&float_buffer_R[0], bp, 128);  

		transmit(blockL, 0);
		transmit(blockR, 1);
		release(blockL);
		release(blockR);
		digitalWrite(14, LOW);
	}

}


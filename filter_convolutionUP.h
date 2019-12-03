/* Audio Library for Teensy 3.X (only works with floating point library enabled in toolchain & tested with Teensy 3.6 only )
 * filter convolution routine  written by Brian Millier
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

#ifndef filter_convolutionUP_h_
#define filter_convolutionUP_h_

#include "Arduino.h"
#include "AudioStream.h"
#include <arm_math.h>
#include <arm_const_structs.h>
#include <utility/imxrt_hw.h> 


 /******************************************************************/
 //                A u d i o FilterConvolutionUP
 // Uniformly-Partitioned Convolution Filter for Teeny 4.0
 // Written by Brian Millier November 2019
 // adapted from routines written for Teensy 4.0 by Frank DD4WH 
 // that were based upon code/literature by Warren Pratt 
 /*******************************************************************/

class AudioFilterConvolutionUP : 
public AudioStream
{
public:
	AudioFilterConvolutionUP(void) :
		AudioStream(2, inputQueueArray)                        
	{
	}

  boolean begin(int status, float32_t gain,float32_t *fo,int nfor);
  virtual void update(void);
  void passThrough(int stat);
  void impulse(const float32_t *coefs,float32_t *maskgen,int size);
  
private:
  audio_block_t *inputQueueArray[2];  
  
  int i;
  int k;
  int passThru;
  int enabled;
  int buffidx;
  const int  partitionsize = 128;
 
  //float32_t  maskgen[512];    // now located in main program as a DMAMEM array
  float32_t  fmask[140][512]; // 
  float32_t  fftin[512];  
 // float32_t	 fftout[np][512]; // now located in main program and placed in DMARAM
  float32_t float_buffer_L[128];  
  float32_t float_buffer_R[128];   
  float32_t last_sample_buffer_L[128];  
  float32_t last_sample_buffer_R[128];  
  float32_t accum[512];    
  float32_t ac2[512];
  float32_t audio_gain;  // can be adjusted from 1.0 to 10.0 depending on the filter gain / impulse response gain
  float32_t* ptr_fmask;
  float32_t* ptr_fftout;
  float32_t temp1;
  float32_t temp2;
  float32_t temp3;
  float32_t temp4;
  float32_t* ptr1;
  float32_t* ptr2;
  int k512;
  int j512;
  int kc;
  int jc;
  int i2;
  int np;
  };

#endif

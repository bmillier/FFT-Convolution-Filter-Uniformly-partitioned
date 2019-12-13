/* Audio Library for Teensy 4.0 only
 "This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>."

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

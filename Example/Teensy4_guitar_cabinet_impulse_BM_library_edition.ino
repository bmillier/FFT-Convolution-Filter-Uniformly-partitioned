
/*

* (c) DD4WH 09/11/2019
 * 
 * Real Time PARTITIONED BLOCK CONVOLUTION FILTERING (STEREO)
 * 
 * thanks a lot to Brian Millier and Warren Pratt for your help!
 * 
 * using a guitar cabinet impulse response with up to  18000 coefficients per channel
 *
 * Brian Millier- added code for the SGTL5000 and patched out   "while(!Serial) line" (must do this if using Visual studio/Visual Micro  IDE)
   and converted the whole example to run with my convolutionUP audio library routine
* this compiles on Arduino 1.8.9

 * 
 * uses Teensy 4.0 and PJRC Audio shield REV D
 * 
 *
 *  
 * inspired by and uses code from wdsp library by Warren Pratt
 * https://github.com/g0orx/wdsp/blob/master/firmin.c
 * 
 * in the public domain GNU GPL v3
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <arm_math.h>
#include <arm_const_structs.h> // in the Teensy 4.0 audio library, the ARM CMSIS DSP lib is already a newer version
#include <utility/imxrt_hw.h> // necessary for setting the sample rate, thanks FrankB !

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USER DEFINES

// Choose only one of these impulse responses !
// BM- I have limited the number of taps on the larger IRs to 18000 (the maximum useable for library version of this filter)

//#define IR1  // 512 taps // MG impulse response from bmillier github @44.1ksps
//#define IR2  // 4096 taps // impulse response @44.1ksps
//#define IR3  // 7552 taps // impulse response @44.1ksps
//#define IR4    // 17920 taps // impulse response 400ms @44.1ksps
//#define IR5    // 21632 taps // impulse response 490ms @44.1ksps (truncated to 18000 taps)
//#define IR6 // 5760 taps, 
//#define IR7 // 22016 taps (truncated to 18000)
#define IR8 // 25552 taps, too much ! (truncated to 18000)
// 18000 taps is MAXIMUM --> about 0.4 seconds
//#define LPMINPHASE512 // 512 taps minimum phase 2.7kHz lowpass filter
//#define LPMINPHASE1024 // 1024 taps minimum phase 2.7kHz lowpass filter
//#define LPMINPHASE2048PASSTHRU // 2048 taps minimum phase 19.0kHz lowpass filter
//#define LPMINPHASE4096 // 4096 taps minimum phase 2.7kHz lowpass filter


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(IR1)
#include "impulse_response_1.h"
const int nc = 512; // number of taps for the FIR filter
#elif defined(IR2)
#include "impulse_response_2.h"
const int nc = 4096; // number of taps for the FIR filter
#elif defined(IR3)
#include "impulse_response_3.h"
const int nc = 7552; // number of taps for the FIR filter
#elif defined(IR5)
#include "impulse_response_5.h"
const int nc = 18000; // number of taps for the FIR filter 21632 truncated to 18000
#elif defined(IR6)
#include "impulse_response_6.h"
const int nc = 5760; // number of taps for the FIR filter, 
#elif defined(IR7)
#include "impulse_response_7.h"
const int nc = 18000; // number of taps for the FIR filter, 22016 truncated to 18000
#elif defined(IR8)
#include "impulse_response_8.h"
const int nc = 18000; // number of taps for the FIR filter, 25552 truncated to 18000
#elif defined(LPMINPHASE512)
#include "lp_minphase_512.h"
const int nc = 512;
#elif defined(LPMINPHASE1024)
#include "lp_minphase_1024.h"
const int nc = 1024;
#elif defined(LPMINPHASE2048PASSTHRU)
#include "lp_minphase_2048passthru.h"
const int nc = 2048;
#elif defined(LPMINPHASE4096)
#include "lp_minphase_4096.h"
const int nc = 4096;
#else
#include "impulse_response_4.h"
const int nc = 17920; // number of taps for the FIR filter 
#endif

extern "C" uint32_t set_arm_clock(uint32_t frequency);

const double PROGMEM FHiCut = 2500.0;
const double PROGMEM FLoCut = -FHiCut;
// define your sample rate
const double PROGMEM SAMPLE_RATE = 44100;  

// the latency of the filter is meant to be the same regardless of the number of taps for the filter
// partition size of 128 translates to a latency of 128/sample rate, ie. to 2.9msec with 44.1ksps
// to this 2.9 ms figure, you have to add in latency involved in collecting the audio data into a 128 sample block, as well as in sending it out again in a block

#define partitionsize 128

#define DEBUG
#define FOURPI  (2.0 * TWO_PI)
#define SIXPI   (3.0 * TWO_PI)
  
int32_t sum;  //used for elapsed time
int idx_t = 0;  //used for elapsed time
uint8_t PROGMEM FIR_filter_window = 1;
const uint32_t PROGMEM FFT_L = 2 * partitionsize; 
float32_t mean = 1;  // used for elapsed time
const uint32_t PROGMEM FFT_length = FFT_L;
const int PROGMEM nfor = nc / partitionsize; // number of partition blocks --> nfor = nc / partitionsize       ** nfor should not be greater than 140
float32_t DMAMEM maskgen[FFT_L * 2];            
float32_t DMAMEM  fftout[nfor][512];  // nfor should not exceed 140
const uint32_t N_B = FFT_L / 2 / BUFFER_SIZE;
uint32_t N_BLOCKS = N_B;


// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           
AudioFilterConvolutionUP       convolution;    // Uniformly-partitioned convolution filter    
AudioOutputI2S           i2s2;           
AudioConnection          patchCord1(i2s1, 0, convolution, 0);
AudioConnection          patchCord2(i2s1, 1, convolution, 1);
AudioConnection          patchCord3(convolution, 0, i2s2,0);
AudioConnection          patchCord4(convolution, 1, i2s2,1);
AudioControlSGTL5000     codec;     
// GUItool: end automatically generated code

void setup() {
  Serial.begin(115200);
  //while(!Serial);   // must patch out for Visual Micro
  pinMode(14, OUTPUT);  // digital signal used to measure routine's execution time on 'scope
  setupSGTL5000();
  AudioMemory(10); 
  delay(100);

  set_arm_clock(600000000);
  setI2SFreq(SAMPLE_RATE);

  convolution.begin(0,1.00,*fftout,nfor); // turn off update routine until after filter mask is generated, set Audio_gain=1.00 , point to fftout array, specify number of partitions
  

/*  for (unsigned i = 0; i < FFT_length * 2; i++)
  {
      cplxcoeffs[i] = 0.0;
  }
*/
  /****************************************************************************************
     set filter bandwidth
  ****************************************************************************************/
  // this routine does all the magic of calculating the FIR coeffs
  //calc_cplx_FIR_coeffs_interleaved (cplxcoeffs, nc, FLoCut, FHiCut, SAMPLE_RATE);
 // fir_bandpass (cplxcoeffs, nc, FLoCut, FHiCut, SAMPLE_RATE, 0, 1, 1.0);
//  fir_bandpass (cplxcoeffs, nc, FLoCut, FHiCut, SAMPLE_RATE, 0, 0, 1.0);

  


  /****************************************************************************************
     Calculate the FFT of the FIR filter coefficients once to produce the FIR filter mask
 ****************************************************************************************/
  convolution.impulse(guitar_cabinet_impulse, maskgen,nc);
    

    flexRamInfo();

    Serial.println();
    Serial.print("AUDIO_BLOCK_SAMPLES:  ");     Serial.println(AUDIO_BLOCK_SAMPLES);

    Serial.println();
    
  
} // END OF SETUP


  elapsedMillis msec = 0;


void loop() {
	yield();

	delay(1000);
  elapsedMicros usec = 0;
 
}


void setI2SFreq(int freq) {
  // thanks FrankB !
  // PLL between 27*24 = 648MHz und 54*24=1296MHz
  int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
  int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
  double C = ((double)freq * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2, true);
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
       | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
       | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f 
//Serial.printf("SetI2SFreq(%d)\n",freq);
}


void flexRamInfo(void)
{ // credit to FrankB, KurtE and defragster !
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)
  int itcm = 0;
  int dtcm = 0;
  int ocram = 0;
  Serial.print("FlexRAM-Banks: [");
  for (int i = 15; i >= 0; i--) {
    switch ((IOMUXC_GPR_GPR17 >> (i * 2)) & 0b11) {
      case 0b00: Serial.print("."); break;
      case 0b01: Serial.print("O"); ocram++; break;
      case 0b10: Serial.print("D"); dtcm++; break;
      case 0b11: Serial.print("I"); itcm++; break;
    }
  }
  Serial.print("] ITCM: ");
  Serial.print(itcm * 32);
  Serial.print(" KB, DTCM: ");
  Serial.print(dtcm * 32);
  Serial.print(" KB, OCRAM: ");
  Serial.print(ocram * 32);
#if defined(__IMXRT1062__)
  Serial.print("(+512)");
#endif
  Serial.println(" KB");
  extern unsigned long _stext;
  extern unsigned long _etext;
  extern unsigned long _sdata;
  extern unsigned long _ebss;
  extern unsigned long _flashimagelen;
  extern unsigned long _heap_start;

  Serial.println("MEM (static usage):");
  Serial.println("RAM1:");

  Serial.print("ITCM = FASTRUN:      ");
  Serial.print((unsigned)&_etext - (unsigned)&_stext);
  Serial.print("   "); Serial.print((float)((unsigned)&_etext - (unsigned)&_stext) / ((float)itcm * 32768.0) * 100.0);
  Serial.print("%  of  "); Serial.print(itcm * 32); Serial.print("kb   ");
  Serial.print("  (");
  Serial.print(itcm * 32768 - ((unsigned)&_etext - (unsigned)&_stext));
  Serial.println(" Bytes free)");
 
  Serial.print("DTCM = Variables:    ");
  Serial.print((unsigned)&_ebss - (unsigned)&_sdata);
  Serial.print("   "); Serial.print((float)((unsigned)&_ebss - (unsigned)&_sdata) / ((float)dtcm * 32768.0) * 100.0);
  Serial.print("%  of  "); Serial.print(dtcm * 32); Serial.print("kb   ");
  Serial.print("  (");
  Serial.print(dtcm * 32768 - ((unsigned)&_ebss - (unsigned)&_sdata));
  Serial.println(" Bytes free)");

  Serial.println("RAM2:");
  Serial.print("OCRAM = DMAMEM:      ");
  Serial.print((unsigned)&_heap_start - 0x20200000);
  Serial.print("   "); Serial.print((float)((unsigned)&_heap_start - 0x20200000) / ((float)512 * 1024.0) * 100.0);
  Serial.print("%  of  "); Serial.print(512); Serial.print("kb");
  Serial.print("     (");
  Serial.print(512 * 1024 - ((unsigned)&_heap_start - 0x20200000));
  Serial.println(" Bytes free)");

  Serial.print("FLASH:               ");
  Serial.print((unsigned)&_flashimagelen);
  Serial.print("   "); Serial.print(((unsigned)&_flashimagelen) / (2048.0 * 1024.0) * 100.0);
  Serial.print("%  of  "); Serial.print(2048); Serial.print("kb");
  Serial.print("    (");
  Serial.print(2048 * 1024 - ((unsigned)&_flashimagelen));
  Serial.println(" Bytes free)");
  
#endif
}

void setupSGTL5000() {
	// Enable the audio shield. select input. and enable output
	codec.enable();
	codec.adcHighPassFilterDisable(); 
	codec.inputSelect(AUDIO_INPUT_LINEIN); // AUDIO_INPUT_MIC
	codec.lineInLevel(3); //1.87 v pp
	codec.lineOutLevel(30);  // doesn't affect headphone volume
	codec.volume(0.4); //   does affect headphone volume
	// line input sensitivity
	//  0: 3.12 Volts p-p
	//  1: 2.63 Volts p-p
	//  2: 2.22 Volts p-p
	//  3: 1.87 Volts p-p
	//  4: 1.58 Volts p-p
	//  5: 1.33 Volts p-p
	//  6: 1.11 Volts p-p
	//  7: 0.94 Volts p-p
	//  8: 0.79 Volts p-p
	//  9: 0.67 Volts p-p
	// 10: 0.56 Volts p-p
	// 11: 0.48 Volts p-p
	// 12: 0.40 Volts p-p
	// 13: 0.34 Volts p-p
	// 14: 0.29 Volts p-p
	// 15: 0.24 Volts p-p
}

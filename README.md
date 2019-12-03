# FFT-Convolution-Filter-Uniformly-partitioned

Teensy Audio library object for Teensy 4.0  (tested on Arduino 1.8.9/Teensyduino 1.47):
FFT-Convolution filter with uniform partitions (exhibits a fixed latency regardless of number of taps being processed)

Possible Uses:
1) FIR filters with a high number of taps (18000 max)
2) Guitar cabinet simulation using impulse response (WAV) files (only first 18000 samples are used if file 
    is larger)
                  
 This filter operates in stereo (or mono if desired)   
 
     The uniformly-partitioned FFT convolution filter library object was derived from the original paper of Warren Pratt's by 
 Frank, DD4WH. I have adapted his code to create the Teensy Audio library object "AudioFilterConvolutionUP" 
 At present, this library only works with a Teensy 4 module, due to the large arrays needed which require the Teensy 4's
 1 Mb of SRAM memory space. If you want to use a Teensy 3.6, refer to Frank's github site for his original in-line 
 code versions of this function. Frank has provided both T4 and a T3.6 version which works with smaller number of taps. 
 
     Due to the T4 MCU's partitioning of the SRAM memory into two 512 Kb blocks- DTCM and OCRAM, my code must place one of 
 the large arrays into OCRAM (using the DMAMEM directive) and the other into DTCM (no compiler directive needed for this). 
 I have not been able to use the DMAMEM directive in the class library code- it throws an error. Therefore I place one 
 large array, FFT_out,  in DMAMEM as part of the main program code, and pass convolutionUP.begin a pointer to that array. 
 The other large array, fmask, is declared in the filter_convolutionUP.h file itself,as are all of the other, 
 smaller arrays needed by the routine.
 
 Frank DD4WH's version is a completely in-line program, and he is able to split up all of the arrays evenly between the DTCM 
 and OCRAM segments. Therefore his program can handle somewhat larger impulse response files than mine.
    
 To use this library, add the filter_convolution.h and .cpp files to the folder containing all the other audio
 library files:
    
 c:\your arduino program folder\hardware\teensy\avr\libraries\Audio
    
 Edit the Audio.h file to include the following line:
   #include "filter_convolutionUP.h"
    
 Make sure you are using Arduino 1.8.9 (or newer) and the proper Teensyduino version to work with your
 Arduino version. I tested it using Arduino 1.8.9/Teensyduino 1.47
    
  The example program is a version of Frank's example program modified to work with my library. This demo
  program is specifically written to work with a PJRC Audio Adapter board. If you wish to mount this board directly
  onto the T4 module, you must use the REV D Audio Adapter, as it has pins to match the T4 pinout. I am using 
  the REV C board as I am physically wiring it up to the T4 module and can wire up the appropriate pins.
  The MCLK line must have a 100 ohm resistor inserted inline, due to the fast rise-time of the T4 module's MCLK line.
      Frank & I supplied several guitar cabinet IR files as well as some FIR low-pass filter files.
  Note: To achieve fixed, low latency performance with FIR filters, you must generate them using the "minimum phase" 
  criteria, not linear phase. I am not certain how Frank generated the 3 low pass FIR filters included with the demo
  program.
  Pick one of the IR files listed near the beginning and un-comment it. I have limited the "nc" parameter 
  (number of taps/samples) to 18000 to suit my library, but the full file length is maintained in the various .h files
  Read through the example program to see how the library object is used.
  Although the object processes a stereo signal, it can be used for mono signals equally well. The nature of the
  complex-FFTCMSIS routines used means that processing a stereo signal is no slower than a routine written 
  for signals would be. 
  
   The execution time of this convolution filter is a bit less than 1000 us. The overhead in collecting the
   audio data in 128-sample blocks and sending it out in 128-sample blocks adds 5.8 ms
   (= 2 audio block times of 2.9 ms).
   Thus, the total latency is 6.8 ms.
   This doesn't change with the number of taps/IR samples used in the filter mask.
   
   For a lot more info on the development of this library, see the thread in the Teensy forum between Frank
   and myself, at:
   https://forum.pjrc.com/threads/57267-Fast-Convolution-Filtering-with-Teensy-4-0-and-audio-board
   
   
   
  
  

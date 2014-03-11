/*
 * $$$MODULE_NAME app_audio_alg.c
 *
 * $$$MODULE_DESC app_audio_alg.c
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  This software is licensed under the  standard terms and conditions in the Texas Instruments  Incorporated
 *  Technology and Software Publicly Available Software License Agreement , a copy of which is included in the
 *  software download.
*/

#include <std.h>
#include <stdlib.h>
#include <stdio.h>
#include "debug_uart.h" // to redirect debug_printf over UART
#include <csl_rtc.h>
#include <csl_intc.h>
#include "app_asrc.h"
#include "VirtualSense_ACIcfg.h"
#include "psp_i2s.h"
#include "lcd_osd.h"
#include "gpio_control.h"

#include "main_config.h"
#include "circular_buffer.h"
#include "rtc.h"

#include "wdt.h"

#include "ff.h"
#include "make_wav.h"

#define ICR 0x0001


//FRESULT rc;

FIL wav_file;
Uint32 step = 0;
Uint32 my_step = 0;
Uint16 file_is_open = 0;

void putDataIntoOpenFile(const void *buff, unsigned int number_of_bytes);
extern unsigned char circular_buffer[PROCESS_BUFFER_SIZE];
extern unsigned char circular_buffer2[PROCESS_BUFFER_SIZE];
unsigned char * used_buffer = circular_buffer;
//extern Uint32 bufferInIdx; //logical pointer
extern Uint32 bufferOutIdx; //logical pointer
extern Uint32 buffer2OutIdx;
extern Int32 bufferInside; //number of item in buffer
extern Uint16 in_record; //logical pointer

// PRD function. Runs every 10 minutes to start sampling a new file
void CreateNewFile(void){
	debug_printf(  "Timer executes\n");

}


void DataSaveTask(void)
{
    // display the play audio message
    //print_playaudio();
	CSL_RtcTime 	 GetTime;
	CSL_RtcDate 	 GetDate;
	FRESULT rc;

	char file_name[128];
	Uint32 burts_size_bytes = DMA_BUFFER_SZ * 2;
	Uint32 b_size = PROCESS_BUFFER_SIZE;
	Uint32 iterations = 0;
	Uint32 index2 = 0;

    //main loop
    while (1)
    {
    	//wait on semaphore released from a timer function
    	//wdt_Refresh();
    	if(seconds > 0) {//if second==0 don't save nothings

			RTC_getDate(&GetDate);
			RTC_getTime(&GetTime);
			sprintf(file_name, "%d_%d_%d__%d-%d-%d.wav",GetDate.day,GetDate.month,GetDate.year, GetTime.hours, GetTime.mins, GetTime.secs);
			debug_printf("Creating a new file %s\n",file_name);

			//rc = open_wave_file(&wav_file, file_name, FREQUENCY, SECONDS);
			rc = open_wave_file(&wav_file, file_name, frequency, seconds);
			if(rc)
				debug_printf("Error opening a new wav file %d\n",rc);
			else{
				file_is_open = 1;
				in_record = 1;
			}
			putDataIntoOpenFile((void *)circular_buffer, 468); // to fill first sector in order to increase performance
			while(file_is_open){ // should be controlled by the file size????
				while(bufferInside <= 255);//spin-lock to wait buffer samples

				putDataIntoOpenFile((void *)(used_buffer+bufferOutIdx), 512);
				bufferOutIdx = ((bufferOutIdx + 512)% b_size);
				if(bufferOutIdx == 0) // switch buffer
					used_buffer = used_buffer==circular_buffer?circular_buffer2:circular_buffer;
				bufferInside-=256; // sample number

			}
#if 0
			for(iterations = 0; iterations <= 100000/*((seconds * step_per_second)+1)*/; iterations++){
							putDataIntoOpenFile((void *)circular_buffer, 512);
							for(index2 =0; index2<100000;index2++){
								asm(" nop "); // is a wait loop
							}
			}
#endif
			// wave header is 44 bytes length
			//clear_lcd();
			//SEM_reset(&SEM_BufferFull,0);
			SEM_pend(&SEM_CloseFile, SYS_FOREVER);
			close_wave_file(&wav_file);
			file_is_open = 0;
			in_record = 0;
			//directory_listing();
			file_counter++;
			step = 0;
	        //clear_lcd();
	        debug_printf("File saved %s\n",file_name);
    	}
        //wdt_Refresh();
        // Put DSP into RTC only mode
        if(mode != MODE_ALWAYS_ON) {
        	RTC_shutdownToRTCOnlyMonde();
        } else {
        	//SEM_post(&SEM_TimerSave);
        }
     }
}

void putDataIntoOpenFile(const void *buff, unsigned int number_of_bytes){
	if(file_is_open){
		write_data_to_wave(&wav_file, buff, number_of_bytes);
		my_step++;
        //wdt_Refresh();
	}
	//if(my_step == ((SECONDS * STEP_PER_SECOND)+1)){
	if(my_step == ((seconds * step_per_second)+1)){
		file_is_open = 0;
		in_record = 0;
		my_step = 0;
		SEM_post(&SEM_CloseFile);
	}

}

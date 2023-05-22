#pragma once 
#include "board_cfg.h"
#ifdef DEV_WT588D

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <limits.h>
#include "lillib.h"
#include "device.h"

extern "C"
{

void WT588D_itoa_play_generic(void (*write)(uint8_t v), bool (*is_busy)(), int16_t v);

}

template<uint8_t... pins>
struct WT588D;

template<uint8_t kIoiData, uint8_t kIoiBusy>
struct WT588D<kIoiData, kIoiBusy>
{
	// kIoiData == P03 (K4/CLK/DATA)
	// kIoiBusy == BUSY
	
	static void init()
	{
		IOI_DIR_SET(kIoiData, 1);
		IOI_DIR_SET(kIoiBusy, 0);
	}
	
	static void write(uint8_t v)
	{
		IOI_OUT_SET(kIoiData, 0);
		delay(5);
		for (uint8_t i=0; i<8 ;i++)
		{
			IOI_OUT_SET(kIoiData, 1);
			if (v&1)
			{
				udelay(600);
				IOI_OUT_SET(kIoiData, 0);
				udelay(300);
			}
			else
			{
				udelay(300);
				IOI_OUT_SET(kIoiData, 0);
				udelay(600);
			}
			v >>= 1;
		}
		udelay(30);
		IOI_OUT_SET(kIoiData, 1);
	}
	
	static bool is_busy()
	{
		return !IOI_PIN_TST(kIoiBusy);
	}
	
	static void play_int(int16_t v)
	{
		WT588D_itoa_play_generic(write, is_busy, v);
	}
};

template<uint8_t kIoiCS, uint8_t kIoiClk, uint8_t kIoiData, uint8_t kIoiBusy>
struct WT588D<kIoiCS, kIoiClk, kIoiData, kIoiBusy>
{
	// kIoiCS   == P02 (K3/CS)
	// kIoiClk  == P03 (K4/CLK/DATA)
	// kIoiData == P01 (K2/DATA)
	// kIoiBusy == BUSY
	
	static void init()
	{
		IOI_DIR_SET(kIoiCS, 1);
		IOI_DIR_SET(kIoiClk, 1);
		IOI_DIR_SET(kIoiData, 1);
		IOI_DIR_SET(kIoiBusy, 0);
	}
	
	static void write(uint8_t v)
	{
		IOI_OUT_SET(kIoiCS, 0);
		delay(3);
		for (uint8_t i=0; i<8 ;i++)
		{
			IOI_OUT_SET(kIoiClk, 0);
			udelay(10);
			IOI_OUT_SET(kIoiData, (v&1));
			udelay(50);
			IOI_OUT_SET(kIoiClk, 1);
			udelay(50);
			v >>= 1;
		}
		udelay(30);
		IOI_OUT_SET(kIoiCS, 1);
	}
	
	static bool is_busy()
	{
		return !IOI_PIN_TST(kIoiBusy);
	}
	
	static void play_int(int16_t v)
	{
		WT588D_itoa_play_generic(write, is_busy, v);
	}
};

#endif // DEV_WT588D

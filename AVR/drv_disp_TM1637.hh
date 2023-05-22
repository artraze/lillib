#pragma once
#include "board_cfg.h"
#ifdef DEV_TM1637

#include "device.h"

#include <inttypes.h>
#include <avr/io.h>
#include "lillib.h"
#include "device.h"

extern "C"
{
void TM1637_disp_num_generic(void (*disp)(uint8_t, uint8_t, uint8_t*), int16_t v, uint8_t dots, uint8_t zeros);
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
struct TM1637
{
	static inline void init();
	
	static inline void write_1(uint8_t v);
	static inline void cmd_disp(uint8_t on, uint8_t brightness);
	static inline void disp(uint8_t pos, uint8_t len, uint8_t *data);
	static inline void disp_num(int16_t v, uint8_t dots, uint8_t zeros);

private:
	static inline uint8_t write_byte(uint8_t data);
	static inline void write_start();
	static inline void write_stop();
};

#define TM1637_CLK(v)     IOI_DIR_SET(kIoiClk, ((v) ? 0 : 1))
#define TM1637_DAT(v)     IOI_DIR_SET(kIoiDat, ((v) ? 0 : 1))

template<uint8_t kIoiClk, uint8_t kIoiDat>
uint8_t TM1637<kIoiClk, kIoiDat>::write_byte(uint8_t data)
{
	// Note that the ACK is set on the 8th falling edge and remains valid until the 9th
	// falling edge.  That means this can just 'write' a high as a 9th bit and the ACK
	// will be readable at exit (which is before the 9th falling edge).  Really, a low
	// for bit 9 could work too (and then changing to high before exit), but writing a
	// 1 isn't much overhead (this is all delays anyways) and is less fiddly, e.g.
	// doesn't need to wait for the possible rise.
	for (uint8_t i = 0; i < 9; i++)
	{
		TM1637_CLK(0);
		udelay(10);
		TM1637_DAT(data & 0x01);
		udelay(15);
		TM1637_CLK(1);
		udelay(25);
		data = 0x80 | (data >> 1);
	}
	return (IOI_PIN_TST(kIoiDat) ? 1 : 0);
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::write_start()
{
	// Start - while clock is high, set dat low
	TM1637_DAT(0);
	udelay(30);
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::write_stop()
{
	// Stop, set dat high while clk is high
	TM1637_CLK(0);
	udelay(10);
	TM1637_DAT(0);
	udelay(20);
	TM1637_CLK(1);
	udelay(10);
	TM1637_DAT(1);
	udelay(20);
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::write_1(uint8_t v)
{
	write_start();
	write_byte(v); 
	write_stop();
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::init()
{
	// Idle state is to have both clk high, which means input for OC bus
	IOI_DIR_SET(kIoiClk, 0);
	IOI_DIR_SET(kIoiDat, 0);
	udelay(10);
	// Open collector, output is always 0, pull up is on board
	IOI_OUT_SET(kIoiClk, 0);
	IOI_OUT_SET(kIoiDat, 0);
	udelay(50);
	// Ensure starting condition - clock and data are high
	TM1637_CLK(1);
	TM1637_DAT(1);
	udelay(100);
	// Init as writing to display with auto-inc address
	write_1(0x40);
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::cmd_disp(uint8_t on, uint8_t brightness)
{
	write_1(0x80 | (brightness & 0x07) | (on ? 0x08 : 0));
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::disp(uint8_t pos, uint8_t len, uint8_t *data)
{
	write_start();
	write_byte(0xC0 + pos);
	while (len-- > 0)
	{
		write_byte(*data++);
	}
	write_stop();
}

template<uint8_t kIoiClk, uint8_t kIoiDat>
void TM1637<kIoiClk, kIoiDat>::disp_num(int16_t v, uint8_t dots, uint8_t zeros)
{
	TM1637_disp_num_generic(disp, v, dots, zeros);
}

#endif // DEV_TM1637

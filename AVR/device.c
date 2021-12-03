#include "device.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include "lillib.h"


uint8_t read_fuse(uint8_t addr)
{
	// (lock:1, lfuse:0, hfuse:3, efuse:2)
	register uint8_t v = 0x09;
	__asm__ volatile (
		"   out   0x37,%2   \n"  //  SPMCSR = BLBSET + SPMEN
		"   lpm             \n"
		"   mov   %0, r0    \n"
		: "=r"(v)
		: "z"((uint16_t)addr), "r"(v)
	);
	return v;
}


static volatile uint8_t i2c_result;
static uint8_t i2c_addr;
static uint8_t i2c_count;
static uint8_t *i2c_data;
#define TWCR_ACK    0x40
#define TWCR_START  0x20
#define TWCR_STOP   0x10
ISR(TWI_vect)
{
	// By factoring out memory reads, worst case time is slightly improved and code is smaller
	// Keepeing the values <<3 prevents GCC from generating a jump table, which adds size and makes it slower.
	uint8_t cr = 0x85;
	uint8_t *d = i2c_data;
	uint8_t  c = i2c_count;
	//uint8_t  r = 0;
	switch (TWSR&0xF8)
	{
		case 0x08: // START
			TWDR = i2c_addr;
			break;
		case 0x18: // Sent Address(W); Got ACK
		case 0x28: // Sent Data; Got ACK
			if (c) TWDR = *d++, c--;
			else   cr |= TWCR_STOP, i2c_result = 0;
			break;
		
		case 0x50: // Got Data; Sent ACK
			*d++ = TWDR;
		case 0x40: // Sent Address(R); Got ACK
			if (--c) cr |= TWCR_ACK;
			break;
		
		case 0x58: // Got Data; Sent NACK
			*d++ = TWDR;
		case 0x30: // Sent Data; Got NACK
			cr |= TWCR_STOP;
			i2c_result = 0;
			break;
		
		case 0x20: // Sent Address(W); got NACK
		case 0x48: // Sent Address(R); got NACK
		default:
			i2c_result = 1;
	}
	//i2c_result = r;
	i2c_count = c;
	i2c_data = d;
	TWCR = cr;
}


void i2c_init()
{
	// SCL = CPU/(16+2*TWBR*Prescaler)
	// TWBR*Prescaler = (CPU/SCL-16)/2;   72@100k, 392@20k
	TWBR = 98;
	TWSR = 0x01;  // Prescaler=4
	TWAR = 0x55;
	TWCR = 0x05;
}


#define TWCR_TWINT           0x80
#define TWCR_TWEA            0x40
#define TWCR_TWSTA           0x20
#define TWCR_TWSTO           0x10
#define TWCR_TWWC            0x08
#define TWCR_TWEN            0x04
#define TWCR_TWIE            0x01

uint8_t i2c_write(uint8_t addr, uint8_t n, const uint8_t *data)
{
	i2c_addr   = addr<<1;
	i2c_count  = n;
	i2c_data   = (uint8_t *)data;
	i2c_result = 0xFF;
	TWCR = TWCR_TWINT | 0x25; // START
	while (i2c_result == 0xFF);
	while (TWCR&TWCR_STOP);
	return i2c_result;
}

uint8_t i2c_write_1b(uint8_t addr, uint8_t data)
{
	i2c_addr   = addr<<1;
	i2c_count  = 1;
	i2c_data   = &data;
	i2c_result = 0xFF;
	TWCR = 0x25; // START
	while (i2c_result == 0xFF);
	while (TWCR&TWCR_STOP);
	return i2c_result;
}

uint8_t i2c_read(uint8_t addr, uint8_t n, uint8_t *data)
{
	i2c_addr   = (addr<<1)|1;
	i2c_count  = n;
	i2c_data   = data;
	i2c_result = 0xFF;
	TWCR = 0x25; // START
	while (i2c_result == 0xFF);
	while (TWCR&TWCR_STOP);
	return i2c_result;
}



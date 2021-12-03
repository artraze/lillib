#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <limits.h>
#include "lillib.h"
#include "device.h"
#include <stdio.h>
#include "drv_misc_WS2812.hh"
#include "drv_disp_TM1637.hh"
#include "drv_disp_WT588D.hh"

#define FIXED_0x8_MUL(a, b)       ((uint8_t)((((uint16_t)(a)) * ((uint16_t)(b))) >> 8))

#define DDRB_NSS         0x04
#define DDRB_MOSI        0x08
#define DDRB_MISO        0x10
#define DDRB_SCK         0x20

#define SPCR_SPIE        0x80
#define SPCR_SPE         0x40
#define SPCR_DORD        0x20
#define SPCR_MSTR        0x10
#define SPCR_CPOL        0x08
#define SPCR_CPHA        0x04
#define SPCR_SPR1        0x02
#define SPCR_SPR0        0x01


#define ADMUX_REFS(n)             (((n)&0x3) << 6)
#define ADMUX_ADLAR               0x20
#define ADMUX_MUX(n)              (((n)&0xF) << 0)
#define ADCSRA_EN                 0x80
#define ADCSRA_ADSC               0x40
#define ADCSRA_ADATE              0x20
#define ADCSRA_ADIF               0x10
#define ADCSRA_ADIE               0x08
#define ADCSRA_ADPS(n)            (((n)&0x7) << 0)
#define ADCSRB_ACME               0x40
#define ADCSRB_ADTS(n)            (((n)&0x7) << 0)
#define PRR_PRADC                 0x01

/*
Pin info:
	A0 - PC0/ADC0/PCINT8
	A1 - PC1/ADC1/PCINT9
	A2 - PC2/ADC2/PCINT10
	A3 - PC3/ADC3/PCINT11
	A4 - PC4/ADC4/PCINT12/SDA
	A5 - PC5/ADC5/PCINT13/SCL
	A6 - ADC6
	A7 - ADC7
	RX - PD0/PCINT16/RXD
	TX - PD1/PCINT17/TXD
	 2 - PD2/PCINT18/INT0
	 3 - PD3/PCINT19/OC2B/INT1
	 4 - PD4/PCINT20/XCK/T0
	 5 - PD5/PCINT21/OC0B/T1
	 6 - PD6/PCINT22/OC0A/AIN0
	 7 - PD7/PCINT23/AIN1
	 8 - PB0/PCINT0/CLKO/ICP1
	 9 - PB1/PCINT1/OC1A
	10 - PB2/PCINT2/OC1B/nSS
	11 - PB3/PCINT3/MOSI/OC2A
	12 - PB4/PCINT4/MISO
	13 - PB5/PCINT5/SCK
*/

void Ultrasonic_IRQ();

ISR(PCINT0_vect)
{
	Ultrasonic_IRQ();
}

void HSV2RGB(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint8_t i, f, p, q, t;
	
	if (s == 0)
	{
		*r = v;
		*g = v;
		*b = v;
		return;
	}

	i = h / 43;
	f = (h - (i * 43)) * 6; 
	p = FIXED_0x8_MUL(v, 255 - s);
	q = FIXED_0x8_MUL(v, 255 - FIXED_0x8_MUL(s, f));
	t = FIXED_0x8_MUL(v, 255 - FIXED_0x8_MUL(s, 255 - f));

	switch (i)
	{
		case 0:  *r = v, *g = t, *b = p; break;
		case 1:  *r = q, *g = v, *b = p; break;
		case 2:  *r = p, *g = v, *b = t; break;
		case 3:  *r = p, *g = q, *b = v; break;
		case 4:  *r = t, *g = p, *b = v; break;
		default: *r = v, *g = p, *b = q; break;
	}
}

static uint8_t SPI_send(uint8_t d)
{
	(void)SPSR;
	SPDR = d;
	while (!(SPSR&0x80));
	return SPDR;
}

static void MAX7219x1_send(uint16_t v1)
{
	SPI_send(v1 >> 8);
	SPI_send(v1 & 0xFF);
	udelay(10);
	PORTB |= 0x04;
	udelay(10);
	PORTB &= ~0x04;
}

static void MAX7219x4_send(uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4)
{
	SPI_send(v4 >> 8);
	SPI_send(v4 & 0xFF);
	SPI_send(v3 >> 8);
	SPI_send(v3 & 0xFF);
	SPI_send(v2 >> 8);
	SPI_send(v2 & 0xFF);
	SPI_send(v1 >> 8);
	SPI_send(v1 & 0xFF);
	udelay(1);
	PORTB |= 0x04;
	udelay(1);
	PORTB &= ~0x04;
}

static void DRV8830_read(uint8_t addr, uint8_t n, uint8_t *data)
{
	uint8_t addrv[1]={addr};
	uint8_t r0 = i2c_write(0x64, 1, addrv);
	delay(1);
	uint8_t r1 = i2c_read(0x64, n, data);
	com_printf("I2C read; Set (r=%i) addr=%02X; Got (r=%i): ", r0, addr, r1);
	for (uint8_t i=0; i<n; i++)
		com_printf(" %02X", data[i]);
	com_putc('\n');
	delay(10);
}

void DRV8830_demo()
{
	// SCL=A5, SDA=A4
	i2c_init();
	PORTC |= 0x30; // Set pullups since the DRV8830 doesn't have any
	
	uint8_t buf[10];
	uint8_t r;
	uint8_t addrv[1]={0x01};

	r = i2c_write(0x64, 1, addrv);
	com_printf("I2C r=%i\n", r);
	r = i2c_write(0x64, 1, addrv);
	com_printf("I2C r=%i\n", r);
	

	DRV8830_read(0x00, 10, buf);
	
	buf[0] = 0x01;
	buf[1] = 0x80;
	r = i2c_write(0x64, 2, buf);
	com_printf("I2C clear fault r=%i\n", r);

	DRV8830_read(0x00, 2, buf);
	
	while (1)
	{
		for (uint8_t i=0x06; i<0x26; i++)
		{
			buf[0] = 0x00;
			buf[1] = 0x01 | (i << 2);
			r = i2c_write(0x64, 2, buf);
			com_printf("I2C write power=0x%02X (cmd=0x%02X) r=%i\n", i, buf[1], r);
			delay(1000);
		}
		DRV8830_read(0x00, 2, buf);
		buf[0] = 0x01;
		buf[1] = 0x80;
		r = i2c_write(0x64, 2, buf);
		com_printf("I2C clear fault r=%i\n", r);
		delay(2000);
	}
}

void MAX7219_8x7seg_demo()
{
	// DIN=11, CS=10, CLK=13
	// Setup SPI
	DDRB  |= DDRB_SCK|DDRB_MOSI|DDRB_NSS;
	SPCR   = 0x52;
	SPSR   = 0x00;
	PORTB &= (uint8_t)~0x04;
	
	// 7 seg display
	MAX7219x1_send(0x0F00);
	MAX7219x1_send(0x0A05);
	MAX7219x1_send(0x0B07);
	MAX7219x1_send(0x0C01);
	MAX7219x1_send(0x09FF);
	uint8_t x = 0;
	while (1)
	{
		for (uint8_t i = 0; i < 8; i++)
		{
			com_printf("%i - %i\n", x, i);
			uint16_t v = ((i + 1)<<8) | x;
			MAX7219x1_send(v);
			delay(100);
		}
		x++;
		x^=0x80;
		delay(1000);
	}
}

void MAX7219_4x64dot_demo()
{
	// DIN=11, CS=10, CLK=13
	// Setup SPI
	DDRB  |= DDRB_SCK|DDRB_MOSI|DDRB_NSS;
	SPCR   = 0x52;
	SPSR   = 0x00;
	PORTB &= (uint8_t)~0x04;

	// Blink the dot matrix bar
	MAX7219x4_send(0x0A05, 0x0A07, 0x0A09, 0x0A0B);
	MAX7219x4_send(0x0B07, 0x0B07, 0x0B07, 0x0B07);
	MAX7219x4_send(0x0C01, 0x0C01, 0x0C01, 0x0C01);
	MAX7219x4_send(0x0900, 0x0900, 0x0900, 0x0900);
	while (1)
	{
		for (uint8_t i = 0; i < 8; i++)
		{
			com_printf("%i\n", i);
			uint16_t v = ((i + 1)<<8) | (0xFF&(0xFF<<i));
			MAX7219x4_send(v&0xFFFE, v&0xFFFD, v&0xFFFB, v&0xFFF7);
			delay(100);
		}
		for (uint8_t i = 0; i < 8; i++)
		{
			com_printf("%i\n", i);
			uint16_t v = ((i + 1)<<8) | (0x00);
			MAX7219x4_send(v, v, v, v);
			delay(5);
		}
	}
}

void WT588D_demo()
{
	// P02=#10(PB2), P03=#13(PB5), P01=#11(PB3), BUSY=#12(PB4)
	using WT588D_Dev1 = WT588D<IOI(B, 2), IOI(B, 5), IOI(B, 3), IOI(B, 4)>;
	//using WT588D_Dev1 = WT588D<IOI(B, 5), IOI(B, 4)>;
	WT588D_Dev1::init();
	delay(40);
	WT588D_Dev1::play_int(6);
	while (1)
	{
		uint16_t data[] = {0, 1, 10, 11, 20, 23, 100, 113, 163, 1000, 1063, 1263, 10263, 11263, 63263};
		for (uint8_t i=0; i<sizeof(data)/sizeof(data[0]); i++)
		{
			com_printf("Play %u\n", data[i]);
			WT588D_Dev1::play_int(data[i]);
			delay(500);
		}
		delay(1000);
	}
}

void WS2812_x8_demo()
{
	// DI = A3 (PC3)
	DDRC  = 0x04;
	PORTC = 0x00;
	
	uint8_t data[3*8];

	delay(1);
	uint8_t vv = 0;
	uint8_t hh = 0;
	while (1)
	{
		vv <<= 1;
		hh +=  5;
		if (vv==0) vv = 1;
		vv = 8;
		for (uint8_t i=0; i<8; i++)
		{
			HSV2RGB(28*i + hh, 255, vv, &data[3*i+1], &data[3*i+0], &data[3*i+2]);
			// com_printf("RGB: %i %i %i\n", data[3*i+1], data[3*i+0], data[3*i+2]);
		}
		WS2812_send<IOI(C, 2)>(3*8, data);
		delay(35);
	}
}

volatile uint16_t  Ultrasonic_start;
volatile uint16_t  Ultrasonic_count = 99;

void Ultrasonic_IRQ()
{
	uint16_t t = TCNT1;
	if (PINB&0x02)
	{
		Ultrasonic_start = t;
	}
	else
	{
		Ultrasonic_count = t - Ultrasonic_start;
	}
}

void Ultrasonic_demo()
{
	// trig = 10 (PB2), echo = 9 (PB1/PCINT1)
	TCCR1A = 0x00;
//	TCCR1B = 0x05;   // div=1024, clk = 15625Hz
	TCCR1B = 0x03;   // div=64, clk = 250kHz

	// Setup input interrupt (#9 - PCINT1 / PB1)
	DDRB    = 0x04;
	PORTB  |= 0x00;
	PCICR   = 0x01; 
	PCMSK0  = 0x02;
	while (1)
	{
		PORTB |= 0x04;
		udelay(10);
		PORTB &= (uint8_t)~0x04;
		delay(100);
		// 40 65 102
		com_printf("Counts = %i (start=%i, timer=%i, pinb=%02X)\n", Ultrasonic_count, Ultrasonic_start, TCNT1, PINB);
	}
}

using TM1637_Dev1 = TM1637<IOI(B, 2), IOI(B, 3)>;
void TM1637_demo()
{
	// DIO = #11 (PB3), CLK= #10 (PB2)
	TM1637_Dev1::init();
	TM1637_Dev1::cmd_disp(1, 7);
	while (1)
	{
		for (uint16_t i=9900; i<100000; i++)
		{
			TM1637_Dev1::disp_num(i, 0, 0);
			delay(50);
			com_printf("%i\n", i);
		}
	}
}

void TC8H670_demo()
{
	// DIN = D11 (PB3/MOSI), LD = D9 (PB1), CLK = D13 (PB5/SCK), STBY = D8 (PB0)
	// Note this chip isn't really SPI and uses a "LOAD" rather than CS, but this 
	// uses the SPI peripheral for convenience which means D10 (PB2/nSS) needs to
	// be set as an output.
	// Note data samples on the falling edge, as this also wants all pins to idle
	// low so that on exiting standby the serial mode (all inputs low) is configured.
	// LSB is first (2 x 16 bit words)
	
	PORTB &= ~0x2B;
	DDRB  |= DDRB_SCK|DDRB_MOSI|DDRB_NSS|0x03;
	
	// Come out of standby
	delay(100);
	PORTB |= 0x01;
	delay(100);
	
	// Enable SPI now that mode is latched
	// FIXME: can DAT be made to idle low?  Then SPI could be enabled while leaving standby
	SPSR   = 0x00;
	SPCR   = SPCR_SPE | SPCR_MSTR | SPCR_DORD | SPCR_CPHA | SPCR_SPR1;
	
	uint16_t i = 0;
	while (1)
	{
		uint16_t a_v = 0x3FF;
		uint16_t b_v = 0x3FF;
		uint16_t a_en = 0;
		uint16_t b_en = 0;
		uint16_t a_dcy = 0;
		uint16_t b_dcy = 0;
		uint16_t opt_opd = 0;
		uint16_t opt_trq = 0;
		uint16_t cmd1;
		uint16_t cmd2;
		
		i = (i+1)&3;
		a_v = (i<<8) + 0xff;
 		com_printf("a_v: 0x%04X\n", a_v);
		
		cmd1 = (uint16_t)((uint16_t)(a_dcy&3)<<0) | (uint16_t)((uint16_t)(a_en&1)<<2) | (uint16_t)((uint16_t)(a_v&0x3FF)<<3);
		SPI_send((cmd1 >> 0) & 0xFF);
		SPI_send((cmd1 >> 8) & 0xFF);
		cmd2 = ((b_dcy&3)<<0) | ((b_en&1)<<2) | ((b_v&0x3FF)<<3) | ((opt_trq&3)<<13) | ((opt_opd&1)<<15);
		SPI_send((cmd2 >> 0) & 0xFF);
		SPI_send((cmd2 >> 8) & 0xFF);
		
		//com_printf("cmd: 0x%04X 0x%04X\n", cmd1, cmd2);
		com_printf("cmd: 0x%02X 0x%02X 0x%02X 0x%02X\n", ((cmd1>>0)&0xFF), ((cmd1>>8)&0xFF), ((cmd2>>0)&0xFF), ((cmd2>>8)&0xFF));
		PORTB |= 0x02;
		udelay(2);
		PORTB &= ~0x02;
		delay(1000);
	}
}

void MCP4725_demo()
{
	i2c_init();
	
	// Setup ADC, reference AVcc because the MCP4725 also uses Vcc for ref
	PRR   &= ~PRR_PRADC;
	ADMUX  = ADMUX_REFS(1) | ADMUX_ADLAR | ADMUX_MUX(6);
	ADCSRA = ADCSRA_EN | ADCSRA_ADPS(7);
	ADCSRB = ADCSRB_ADTS(0);
	DIDR0  = 0x00;
	
	uint8_t vout = 0;
	while (1)
	{
		vout += 2;
		
		uint8_t data[2];
		data[0] = (vout >> 4) | 0x00;
		data[1] = (vout << 4);
		uint8_t r = i2c_write(0xC0>>1, 2, data);

		ADCSRA |= ADCSRA_ADSC;
		while (ADCSRA & ADCSRA_ADSC);
		
		com_printf("0x%02X = 0x%02X [cmd vs adc] (r=%i)\n", vout, ADCH, r);
		
		delay(100);
	}
}

void AHT10_demo()
{
	uint8_t r1, r2;
	uint8_t data[6];
	
	i2c_init();
	// FIXME: This didn't work... Hardware issue with chip?  The AVR couldn't pull SCL low
	
	
	PORTC |= 0x30;
	
	delay(50);
	
// 	com_printf("Init (r=%i)\n", 44);
// 	delay(100);
// 
// 	data[0] = 0x71;
// 	r1 = i2c_write(0x38, 1, data);
// 	r2 = i2c_read(0x38, 1, data);
// 	com_printf("Init Status (r=%i,%i)\n", r1,r2, data[0]);
// 
	data[0] = 0xE1;
	data[1] = 0x08;
	data[2] = 0x00;
	r1 = i2c_write(0x38, 3, data);
	com_printf("Init (r=%i)\n", r1);
	delay(100);
	
	while (1)
	{
		data[0] = 0xAC;
		data[1] = 0x33;
		data[2] = 0x00;
		r1 = i2c_write(0x38, 3, data);
		r2 = i2c_read(0x38, 6, data);
		
		uint8_t state = data[0];
		uint32_t xvh = (((uint32_t)data[1])<<12)|(((uint32_t)data[2])<<4)|(((uint32_t)data[3])>>4);
		uint32_t xvt = (((uint32_t)(data[3]&0xF))<<16)|(((uint32_t)data[4])<<8)|(((uint32_t)data[5]));
		
		uint16_t vh = xvh >> 4;
		uint16_t vt = xvt >> 4;
		com_printf("READ (r=%i,%i) state=0x%02X,  %i, %i\n", r1, r2, state, vh, vt);

		delay(100);
	}
}

void SI7021_demo()
{
	uint8_t r1, r2;
	uint8_t data[6];
	
	i2c_init();
	
	PORTC |= 0x30;
	
	delay(50);
	
	uint8_t sna[8] = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB};
	data[0] = 0xFA;
	data[1] = 0x0F;
	r1 = i2c_write(0x40, 2, data);
	r2 = i2c_read(0x40, 8, sna);
	com_printf("S/N part1 (r=%i,%i)\n", r1, r2);
	
	uint8_t snb[6] = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB};
	data[0] = 0xFC;
	data[1] = 0xC9;
	r1 = i2c_write(0x40, 2, data);
	r2 = i2c_read(0x40, 6, snb);
	com_printf("S/N part2 (r=%i,%i)\n", r1, r2);
	
	uint8_t fwver = 0xBB;
	data[0] = 0x84;
	data[1] = 0xB8;
	r1 = i2c_write(0x40, 2, data);
	r2 = i2c_read(0x40, 1, &fwver);
	com_printf("FW (r=%i,%i)\n", r1, r2);
	
	com_printf("Firmware = %02X, S/N = %02X %02X %02X %02X %02X %02X %02X %02X\n", fwver, sna[0], sna[2], sna[4], sna[6], snb[0], snb[1], snb[3], snb[4]);
	
	while (1)
	{
		r1 = i2c_write_1b(0x40, 0xF5);
		while (i2c_read(0x40, 2, data)) udelay(1);
		uint16_t vh = (data[0]<<8) | data[1];
		
		r1 = i2c_write_1b(0x40, 0xE0);
		r2 = i2c_read(0x40, 2, data);
		uint16_t vt = (data[0]<<8) | data[1];
		
		int16_t vhn = (uint32_t)vh * 12500 / 65536 - 600;  // 100*(125*v/65536-6)
		int16_t vtn = (uint32_t)vt * 17572 / 65536 - 4685; // 100*(175.72*v/65536-46.85)
		
		com_printf("READ (r=%i,%i)  %.2$, %.2$ (%u, %u)\n", r1, r2, vhn, vtn, vh, vt);
		
		delay(100);
	}
}


void DHT11_demo()
{
	// DATA = D8 (PB0)
	uint8_t pin = IOI(B, 0);
	
	// Open collector I/O, keep PORT 0, toggle DDR
	IOI_DDR_SET(pin, 0);
	IOI_PORT_SET(pin, 0);
	
	DDRB |= 0x02; // For timing testing
	
	// Wait to stabilize.  Datasheet says 1s, but the boot has some sleeps so this needn't be a full sec. 
	delay(600);
	
	while (1)
	{
		uint8_t cnt;
		// Pulse low for at least 18ms
		IOI_DDR_SET(pin, 1);
		delay(20);
		IOI_DDR_SET(pin, 0);
		
		// Now bus should say high 20-40us until device pulls low for ACK
		// Note this needs a short delay to let the bus got high initially
		for (cnt = 0; cnt < 14 && !IOI_PIN_TST(pin); cnt++) udelay(5);
		for (       ; cnt < 14 && IOI_PIN_TST(pin); cnt++) udelay(5);
		if (cnt >= 14)
		{
			com_puts("No ACK");
			delay(250);
			continue;
		}
		
		// At this point the bus is low.  It should stay there for 80us and then go high for
		// another 80us.  Rather than explicitly watch that, this will just jump into the bit
		// reading loop directly, which will keep the timings consistent for the payload. That
		// means the loop-entry timing distortion is attached to the ACK, and this gets 41b
		// instead of the nominal 40.
		uint8_t n;
		uint8_t data0[41];
		uint8_t data1[41];
		
		// Ideally this would probably use interrupts and/or a timer so that it could be robust
		// but here we'll just mask interrupts.
		cli();

		// Jut count low / high cycles and collect those for later parsing.  Probably needn't
		// be this conservative, but oh well.
		for (n = 0; n < 41; n++)
		{
			// Timing it, udelay(4) gives closer to 5us
			for (cnt = 0; cnt < 60 && !IOI_PIN_TST(pin); cnt++) udelay(4);
			data0[n] = cnt;
			for (cnt = 0; cnt < 60 && IOI_PIN_TST(pin); cnt++) udelay(4);
			data1[n] = cnt;
		}
		
		sei();
		
		uint8_t ha = 0;
		uint8_t hb = 0;
		uint8_t ta = 0;
		uint8_t tb = 0;
		uint8_t chk = 0;
		
		com_printf("%02i-%02i\n", data0[0], data1[0]);
		for (uint8_t i=1; i<40; i+=8)
		{
			for (uint8_t j=0; j<8; j++)
				com_printf("%02i-%02i  ", data0[i+j], data1[i+j]);
			com_putc('\n');
		}
		
		n = 1;
		for (; n < 1+1*8; n++) ha = (ha<<1) | (data1[n] >= data0[n] ? 1 : 0);
		for (; n < 1+2*8; n++) hb = (hb<<1) | (data1[n] >= data0[n] ? 1 : 0);
		for (; n < 1+3*8; n++) ta = (ta<<1) | (data1[n] >= data0[n] ? 1 : 0);
		for (; n < 1+4*8; n++) tb = (tb<<1) | (data1[n] >= data0[n] ? 1 : 0);
		for (; n < 1+5*8; n++) chk = (chk<<1) | (data1[n] >= data0[n] ? 1 : 0);
		
		uint8_t sum = ha + hb + ta + tb;
		if (sum != chk)
		{
			com_puts("Sensor checksum error");
		}
		
		for (uint8_t i = 0; i < 41; i++)
		{
			if (data0[i] < 60 && data1[i] < 60) continue;
			com_printf("Sensor bus timeout at n=%i\n", i);
			break;
		}
		
		com_printf("ha=%i, hb=%i, ta=%i, tb=%i, chk=%i, sum=%i\n", ha,hb,ta,tb,chk,sum);
		uint16_t hx = ha<<8 | hb;
		uint16_t tx = ta<<8 | tb;
		com_printf("h=%i, t=%i\n", hx, tx);
		delay(500);
		
	}
} 

int main()
{
	delay(250);
	com_init();
	sei();
	com_puts("BOOT");
	
// 	DRV8830_demo();
// 	MAX7219_8x7seg_demo();
// 	MAX7219_4x64dot_demo();
// 	WT588D_demo();
// 	WS2812_x8_demo();
// 	Ultrasonic_demo();
	TM1637_demo();
// 	TC8H670_demo();
// 	MCP4725_demo();
// 	AHT10_demo();
// 	SI7021_demo();
// 	DHT11_demo();
}


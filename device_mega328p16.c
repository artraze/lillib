// Parts of lilib that need device parameters
#ifdef __AVR__
#include "lillib.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#if BOARD_CFG_FREQ == 16000000UL

void udelay(unsigned int n)
{
	register uint8_t t;
	__asm__ volatile (
		"1: ldi   %1, 0x03 \n"  // n
		"2: dec   %1       \n"  // t*n
		"   nop            \n"  // t*n
		"   brne  2b       \n"  // 2*t*n - 2*n + n
		"   sbiw  %0, 0x01 \n"  // 2*n
		"   brne  1b       \n"  // 2*n - 2 + 1
		"   nop            \n"  // 1
		: "=r" (n), "=r" (t)
		: "0" (n)
	);
}

void delay(unsigned int n)
{
	register uint16_t d;
	__asm__ volatile (
		// Inner loop: (clock / 5 - 1) cycles, 3199 for 16MHz
		"1: ldi   %B1, 0x0C \n"  //  n
		"   ldi   %A1, 0x7F \n"  //  n
		"2: sbiw  %1,  0x01 \n"  //  2*d*n
		"   nop             \n"  //  d*n
		"   brne  2b        \n"  //  2*(d-1)*n + n == 2*d*n - n
		"   sbiw  %0,  0x01 \n"  //  2*n
		"   brne  1b        \n"  //  2*(n-1) + 1   ==   2*n - 1
		"   nop             \n"  //  1
		: "=r"(n), "=w"(d)       // total: 5*d*n + 5*n
		: "0"(n)
	);
}

void sleep(unsigned int ms)
{
	delay(ms);
}

#else
#error Unsupported / unknown BOARD_CFG_FREQ
#endif



#define COM_TXBUF_SIZE   63
#define COM_RXBUF_SIZE   63
static volatile uint8_t com_rxbuf_r, com_rxbuf_w;
static volatile uint8_t com_txbuf_r, com_txbuf_w;
static volatile char com_rxbuf[COM_RXBUF_SIZE+1];
static volatile char com_txbuf[COM_TXBUF_SIZE+1];

ISR(USART_RX_vect)
{
	if ((uint8_t)((com_rxbuf_w-com_rxbuf_r)&COM_RXBUF_SIZE)<COM_RXBUF_SIZE)
	{
		com_rxbuf[com_rxbuf_w] = UDR0;
		com_rxbuf_w = (uint8_t)((com_rxbuf_w+1)&COM_RXBUF_SIZE);
	}
	else (void)UDR0; //discard
}

ISR(USART_UDRE_vect)
{
	if (com_txbuf_r!=com_txbuf_w)
	{
		UDR0 = (uint8_t)(com_txbuf[com_txbuf_r]&0x7F);
		com_txbuf_r = (uint8_t)((com_txbuf_r+1)&COM_TXBUF_SIZE);
	}
	else UCSR0B = 0x98; // Clear UDRIE
}

void com_init()
{
	// USART0 as 115200 8N1
	UBRR0H = 0x00;
	UBRR0L = 0x08;
	UCSR0A = 0x00;
	UCSR0B = 0x98;
	UCSR0C = 0x06;
}

// TODO: Most of the 'device' things really just need to access the buffer... can I fix that?
uint8_t com_poll()
{
	return (uint8_t)(com_rxbuf_w-com_rxbuf_r)&COM_RXBUF_SIZE;
}

char com_getc()
{
	char v;
	while (com_rxbuf_w==com_rxbuf_r);
	v = com_rxbuf[com_rxbuf_r];
	com_rxbuf_r = (uint8_t)((com_rxbuf_r+1)&COM_RXBUF_SIZE);
	return v;
}

void com_putc(char c)
{
	//while (!(UCSR0A&0x20));
	//UDR0  = c;
	uint8_t w = com_txbuf_w;
	while ((uint8_t)((w-com_txbuf_r)&COM_TXBUF_SIZE)==COM_TXBUF_SIZE);
	com_txbuf[w] = c;
	com_txbuf_w = (uint8_t)((w+1)&COM_TXBUF_SIZE);
	UCSR0B = 0xB8; // Set UDRIE
}

#endif // __AVR__

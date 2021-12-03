#include "drv_disp_WT588D.hh"
#ifdef DEV_WT588D

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <limits.h>
#include "lillib.h"
#include "device.h"
extern "C"
{

// keys should be at least length 8, 7 for the voice
// and 1 for the terminating 255.
static void WT588D_itoa(uint16_t v, uint8_t *keys)
{
	uint16_t t = v;
	uint8_t n = 0;
	uint8_t digits[5];
	if (v == 0)
	{
		keys[0] = 0;
		keys[1] = 0xFF;
		return;
	}
	do
	{
		digits[n++] = t%10;
		t /= 10;
	} while (t);
	do
	{
		uint8_t a = digits[--n];
		if (a == 0)
		{
			continue;
		}
		uint8_t p = n%3;
		if (p == 2)
		{
			*keys++ = a;
			*keys++ = 0x1C;
		}
		else if (p == 1)
		{
			if (a == 1)
			{
				*keys++ = digits[n-1] + 10;
				n--;
			}
			else
			{
				*keys++ = a + 20 - 2;
			}
		}
		else
		{
			*keys++ = a;
		}
		if (n == 3)
		{
			*keys++ = 0x1D;
		}
	} while (n);
	*keys++ = 0xFF;
}

void WT588D_itoa_play_generic(void (*write)(uint8_t v), bool (*is_busy)(), int16_t v)
{
	uint8_t keys[10];
	WT588D_itoa(v, keys);
	for (uint8_t i = 0; keys[i] != 0xFF; i++)
	{
		write(keys[i]);
		delay(50);
		// TODO: Here we need to wait for BUSY to clear, but it doesn't start immediately after the command so we need to wait for it to got low (start playing) then got high (finished) but there's no way of knowing (?) if it will play so this can lock up.  How to resolve?  Maybe just timeout the "wait for start" step?  Or maybe using interrupts to issue the next command kinda resolves this...
		//if (!is_busy()) while (!is_busy());
		while (is_busy());
	}
}

}
#endif // DEV_WT588D

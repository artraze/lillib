#include "drv_disp_TM1637.hh"
#ifdef DEV_TM1637

extern "C"
{

static const uint8_t TM1637_font_E     = 0b01111001;
static const uint8_t TM1637_font_minus = 0b01000000;
static const uint8_t TM1637_font_digits[10] = {
	0b00111111,
	0b00000110,
	0b01011011,
	0b01001111,
	0b01100110,
	0b01101101,
	0b01111101,
	0b00000111,
	0b01111111,
	0b01101111,
};

void TM1637_disp_num_generic(void (*disp)(uint8_t, uint8_t, uint8_t*), int16_t v, uint8_t dots, uint8_t zeros)
{
	uint8_t data[4];
	if (v < -999)
	{
		data[0] = 0b00111000;
		data[1] = 0b00001000;
		data[2] = 0b00001000;
		data[3] = 0b00001110;
	}
	else if (v > 9999)
	{
		data[0] = 0b00110001;
		data[1] = 0b00000001;
		data[2] = 0b00000001;
		data[3] = 0b00000111;
	}
	else
	{
		uint8_t n = 4;
		uint8_t *d = data + 4;
		if (v < 0)
		{
			v = -v;
			data[0] = TM1637_font_minus;
			n = 3;
		}
		do
		{
			*--d = TM1637_font_digits[v%10];
			v /= 10;
		} while (--n && v);
		while (n--)
		{
			*--d = (zeros ? TM1637_font_digits[0] : 0);
		}
	}
	disp(0, 4, data);
}

}

#endif // DEV_TM1637

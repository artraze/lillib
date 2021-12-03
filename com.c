#ifdef STM32_TESTING
#include <stdio.h>
#endif
#include "lillib.h"

void com_puts(const char *str)
{
	for ( ; *str; str++) com_putc(*str);
	com_putc('\n');
}

void com_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	strf_print(com_putc, fmt, args);
	va_end(args);
}

void com_print_buf(const char *title, uint8_t *buf, int n, int wrap)
{
	int i,w;
	com_printf("%s :", title);
	if (wrap>0) com_putc('\n');
	for (i=0,w=wrap; i<n; i++)
	{
		if (--w==0) com_printf("\n%03X :", i), w=wrap;
		com_printf(" %02X", buf[i]);
	}
	com_putc('\n');
}

char *com_gets(char *buf, uint8_t size, uint8_t echo)
{
	char c;
	char *str = buf;
	for ( ; size>1; size--)
	{
		c = com_getc();
		if      (c==0x04) break; // EOT char ends line
		else if (c==0x7F) { if (str!=buf) str--, size+=2; } // Backspace
		else *str++ = c;
		if (echo) com_putc(c);
		if (c=='\n') break;
	}
	*str = 0;
	return (str==buf ? NULL : buf); // If transmission ended with no data, we hit EOF
}

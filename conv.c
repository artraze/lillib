#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include "lillib.h"

void qsprintf(char *buf, int size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	strf_sprint(buf, size, fmt, args);
	va_end(args);
}

char *itoa2(char *rbuf, unsigned int v, char radix, uint8_t flags)
{
	// Simpler and cooler itoa, mostly for printf support
	// rbuf should point to the end of a buffer (i.e. buffer+len) with at least 34 (base=2: 32bits, sign, null) bytes
	char a = ((flags&ITOA2_UCASE) ? 'A' : 'a');
	char n = ((flags&ITOA2_SIGNED) && (v&INT_MIN) ? 1 : 0);
	char d;
	if (n) v = -v;
	*--rbuf = 0;
	do {
		d = v%radix;
		v = v/radix;
		*--rbuf = (d<=9 ? '0'+d : a+d-10);
	} while (v);
	if (n && !(flags&ITOA2_NOSIGN)) *--rbuf = '-';
	return rbuf;
}

static char *strf_itoa(char *rbuffer, va_list *args, char radix, uint8_t iaflag, char *sign, int *length)
{
	unsigned int v;
	char *string;
	v = va_arg(*args, unsigned int);
	string = itoa2(rbuffer, v, radix, iaflag);
	if (string[0]=='-') *sign='-', string++;
	*length = rbuffer-string-1;
	return string;
}

/*
char *strf_ftoa(char *rbuffer, va_list *args, int flags, char *sign, int *length)
{
	unsigned int v;
	char *string;
	v = va_arg(*args, double);
	string = itoa2(rbuffer, v, radix, flags);
	if (string[0]=='-') *sign='-', string++;
	*length = rbuffer-string-1;
	return string;
}
*/

int qstrtol(const char *str, const char **end, char radix)
{
	unsigned int maxv; char maxd;
	int v = 0;
	char c, n = 0, f = 0;
	if (end) *end = str;
	while (isspace((uint8_t)(c=*str))) str++;
	if (c=='+') str++; else if (c=='-') n=1, str++;
	if ((radix==0 || radix==16) && str[0]=='0' && (str[1]=='x' || str[1]=='X')) radix=16, str+=2;
	else if (radix==0) radix = (str[0]=='0' ? 8 : 10);
	maxv = (n ? -(unsigned int)INT_MIN : INT_MAX);
	maxd = maxv % radix;
	maxv = maxv / radix;
	while (1)
	{
		c = *str++;
		if      (c>='0' && c<='9') c = c - '0';
		else if (c>='a' && c<='z') c = c - 'a' + 10;
		else if (c>='A' && c<='Z') c = c - 'A' + 10;
		else break;
		if (c>=radix) break;
		if (f==2) continue;
		if (v<maxv || (v==maxv && c<=maxd)) f=1, v*=radix, v+=c; else f=2;
	}
	if (f && end) *end = str-1;
	return (f==2 ? (n ? INT_MIN : INT_MAX) : (n ? -v : v));
}

int strtol_b10(const char *str, const char **end)
{
	unsigned int maxv=INT_MAX/10; char maxd=7;
	int v = 0;
	char c, n = 0, f = 0;
	if (end) *end = str;
	while (isspace((uint8_t)(c=*str))) str++;
	if (c=='+') str++; else if (c=='-') n=1, str++, maxd=8;
	while (1)
	{
		c = *str++;
		if (c>='0' && c<='9') c = c-'0'; else break;
		if (f==2) continue;
		if (v<maxv || (v==maxv && c<=maxd)) f=1, v*=10, v+=c; else f=2;
	}
	if (f && end) *end = str-1;
	return (f==2 ? (n ? INT_MIN : INT_MAX) : (n ? -v : v));
}

int strtol_b10u(const char *str, const char **end)
{
	unsigned int maxv=INT_MAX/10; char maxd=7;
	int v = 0;
	char c, f = 0;
	if (end) *end = str;
	while (isspace((uint8_t)(*str))) str++;
	while (1)
	{
		c = *str++;
		if (c>='0' && c<='9') c = c-'0'; else break;
		if (f==2) continue;
		if (v<maxv || (v==maxv && c<=maxd)) f=1, v*=10, v+=c; else f=2;
	}
	if (f && end) *end = str-1;
	return (f==2 ? INT_MAX : v);
}

// strtol micro: base 10, unsigned, end!=NULL, no overflow check, no leading space skip
int strtol_b10u_micro(const char *str, const char **end)
{
	int v = 0;
	while (1)
	{
		char c = *str++;
		if (c<'0' || c>'9') break;
		v = 10*v + c-'0';
	}
	*end = str-1;
	return v;
}

void strf_print(strf_putc putc, const char *fmt, va_list args)
{
	char buffer[34];
	while (*fmt)
	{
		if (fmt[0]!='%') putc(*fmt++);
		else
		{
			// Specifiers
			char fill  = ' ';
			char align = '>';
			char sign  = 0;
			char type  = 0;
			int prec  = -1;
			int width = -1;
			// Data
			const char *content = buffer;
			int length;
			// Read spec
			fmt++;
			if (*fmt && (fmt[1]=='-' || fmt[1]=='<' || fmt[1]=='>' || fmt[1]=='=' || fmt[1]=='^')) fill = *fmt++, align = *fmt++;
			else if (*fmt=='-' || *fmt=='<' || *fmt=='>' || *fmt=='=' || *fmt=='^')                align = *fmt++;
			if (*fmt=='+' || *fmt==' ')                     sign = *fmt++;
			if (*fmt=='0')                                  fill = '0', align = '=', fmt++;
			if (*fmt>='0' && *fmt<='9')                     width = strtol_b10u_micro(fmt, &fmt);
			else if (*fmt=='*')                             width = va_arg(args, int), fmt+=1;
			if (fmt[0]=='.' && fmt[1]>='0' && fmt[1]<='9')  prec = strtol_b10u_micro(fmt+1, &fmt);
			else if (fmt[0]=='.' && fmt[1]=='*')            prec = va_arg(args, int), fmt+=2;
			else if (fmt[0]=='.')                           prec = 0, fmt+=1;
			type = *fmt++;
			// Apply format
			switch (type)
			{
				// Integer
				case 'd':
				case 'i': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 10, ITOA2_SIGNED, &sign, &length); break;
				case 'u': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 10,            0, &sign, &length); break;
				case 'x': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 16,            0, &sign, &length); break;
				case 'X': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 16,  ITOA2_UCASE, &sign, &length); break;
				case 'o': content = strf_itoa(buffer+34, VA_LIST_PTR(args),  8,            0, &sign, &length); break;
				case 'b': content = strf_itoa(buffer+34, VA_LIST_PTR(args),  2,            0, &sign, &length); break;
				case 'p': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 16,  ITOA2_UCASE, &sign, &length); break;
				case '$':
				{
					int i;
					if (prec<0)  prec = 2;
					if (prec>32) prec = 32;
					content = strf_itoa(buffer+34-1, VA_LIST_PTR(args), 10, ITOA2_SIGNED, &sign, &length);
					while (length<prec) *(char*)(--content)='0', length++;
					for (i=0;i<=prec;i++) buffer[33-i]=buffer[32-i];
					buffer[32-prec]='.', length++;
					break;
				}
				// Float
				case 'f':
				case 'F':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					va_arg(args, double);
					content = "<float?>";
					length  = 8;
					break;
				// String
				case 's':
					content = va_arg(args, const char*);
					length  = qstrlen(content);
					sign    = 0;
					if (prec>=0 && prec<length) length = prec;
					break;
				case 'c':
					buffer[0] = (char)va_arg(args, int);
					buffer[1] = 0;
					length    = 1;
					sign      = 0;
					break;
				
				case '%': length=1; buffer[0]='%'; break;
				case  0 : length=2; buffer[0]='%'; buffer[1]='?';  fmt--; break;
				default : length=2; buffer[0]='%'; buffer[1]=type; break;
			}
			// Pad and write
			{
				int lpad=0, cpad=0, rpad=0;
				if (sign) length++;
				if (length<width)
				{
					if (align=='<' || align=='-') rpad = (width-length);
					if (align=='=')               cpad = (width-length);
					if (align=='>')               lpad = (width-length);
					if (align=='^')               lpad = (width-length)/2, rpad = (width-length-lpad);
				}
				while (lpad--  ) putc(fill);
				if    (sign    ) putc(sign);
				while (cpad--  ) putc(fill);
				while (*content && length--) putc(*content++);
				while (rpad--  ) putc(fill);
			}
		}
	}
}

void strf_sprint(char *buf, int size, const char *fmt, va_list args)
{
	char buffer[34];
	while (*fmt)
	{
		if (fmt[0]!='%') { if (size) *buf++ = *fmt++, size--; }
		else
		{
			// Specifiers
			char fill  = ' ';
			char align = '>';
			char sign  = 0;
			char type  = 0;
			int prec  = -1;
			int width = -1;
			// Data
			const char *content = buffer;
			int length;
			// Read spec
			fmt++;
			if (*fmt && (fmt[1]=='-' || fmt[1]=='<' || fmt[1]=='>' || fmt[1]=='=' || fmt[1]=='^')) fill = *fmt++, align = *fmt++;
			else if (*fmt=='-' || *fmt=='<' || *fmt=='>' || *fmt=='=' || *fmt=='^')                align = *fmt++;
			if (*fmt=='+' || *fmt==' ')                     sign = *fmt++;
			if (*fmt=='0')                                  fill = '0', align = '=', fmt++;
			if (*fmt>='0' && *fmt<='9')                     width = strtol_b10u_micro(fmt, &fmt);
			else if (*fmt=='*')                             width = va_arg(args, int), fmt+=1;
			if (fmt[0]=='.' && fmt[1]>='0' && fmt[1]<='9')  prec = strtol_b10u_micro(fmt+1, &fmt);
			else if (fmt[0]=='.' && fmt[1]=='*')            prec = va_arg(args, int), fmt+=2;
			else if (fmt[0]=='.')                           prec = 0, fmt+=1;
			type = *fmt++;
			// Apply format
			switch (type)
			{
				// Integer
				case 'd':
				case 'i': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 10, ITOA2_SIGNED, &sign, &length); break;
				case 'u': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 10,            0, &sign, &length); break;
				case 'x': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 16,            0, &sign, &length); break;
				case 'X': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 16,  ITOA2_UCASE, &sign, &length); break;
				case 'o': content = strf_itoa(buffer+34, VA_LIST_PTR(args),  8,            0, &sign, &length); break;
				case 'b': content = strf_itoa(buffer+34, VA_LIST_PTR(args),  2,            0, &sign, &length); break;
				case 'p': content = strf_itoa(buffer+34, VA_LIST_PTR(args), 16,  ITOA2_UCASE, &sign, &length); break;
				case '$':
				{
					int i;
					if (prec<0)  prec = 2;
					if (prec>32) prec = 32;
					content = strf_itoa(buffer+34-1, VA_LIST_PTR(args), 10, ITOA2_SIGNED, &sign, &length);
					while (length<prec) *(char*)(--content)='0', length++;
					for (i=0;i<=prec;i++) buffer[33-i]=buffer[32-i];
					buffer[32-prec]='.', length++;
					break;
				}
				// Float
				case 'f':
				case 'F':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					va_arg(args, double);
					content = "<float?>";
					length  = 8;
					break;
				// String
				case 's':
					content = va_arg(args, const char*);
					length  = qstrlen(content);
					sign    = 0;
					if (prec>=0 && prec<length) length = prec;
					break;
				case 'c':
					buffer[0] = (char)va_arg(args, int);
					buffer[1] = 0;
					length    = 1;
					sign      = 0;
					break;
				
				case '%': length=1; buffer[0]='%'; break;
				case  0 : length=2; buffer[0]='%'; buffer[1]='?';  fmt--; break;
				default : length=2; buffer[0]='%'; buffer[1]=type; break;
			}
			// Pad and write
			{
				int lpad=0, cpad=0, rpad=0;
				if (sign) length++;
				if (length<width)
				{
					if (align=='<' || align=='-') rpad = (width-length);
					if (align=='=')               cpad = (width-length);
					if (align=='>')               lpad = (width-length);
					if (align=='^')               lpad = (width-length)/2, rpad = (width-length-lpad);
				}
				while (lpad--  && size) *buf++ = fill, size--;
				if    (sign    && size) *buf++ = sign, size--;
				while (cpad--  && size) *buf++ = fill, size--;
				while (*content && length-- && size) *buf++ = *content++, size--;
				while (rpad--  && size) *buf++ = fill, size--;
			}
		}
	}
	if (size) *buf++ = 0;
}


#include <ctype.h>
#include "lillib.h"

static const char *b16_chars = "0123456789ABCDEF";
static const char *b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b16_encode(const uint8_t *in, int isize, char *out, int osize, int linewidth)
{
	int n=0, w=0;
	while (n<osize && isize--)
	{
		uint8_t v = *in++;
		out[n++] = b16_chars[v>>4];
		if (linewidth>0 && ++w>=linewidth && n<osize) out[n++]='\n', w=0;
		if (n>=osize) break;
		out[n++] = b16_chars[v&0xF];
		if (linewidth>0 && ++w>=linewidth && n<osize) out[n++]='\n', w=0;
	}
	if (n<osize) out[n] = 0;
	return n;
}

int b16_decode(const char *in, int isize, uint8_t *out, int osize)
{
	char c;
	int n=0;
	uint8_t v=0, b=0, t;
	while ((c=*in++)!=0 && n<osize && isize--)
	{
		if      (c>='A' && c<='Z') t = c-'A'+10;
		else if (c>='a' && c<='z') t = c-'a'+10;
		else if (c>='0' && c<='9') t = c-'0';
		else if (isspace((uint8_t)c)) continue;
		else break;
		b += 4;
		v  = (v<<4)|t;
		if (b>=8)
		{
			b -= 8;
			*out++ = v;
			n++;
		}
	}
	return n;
}

int b64_encode_size(int isize, int linewidth)
{
	int n = 4*((isize+2)/3);
	if (linewidth>0) n += n / linewidth;
	return n;
}

int b64_encode(const char *in, int isize, uint8_t *out, int osize, int linewidth)
{
	int n=0, w=0;
	uint8_t b=0;
	uint8_t pad = (isize%3 ? 3-isize%3 : 0);
	uint32_t v=0;
	while (n<osize && isize--)
	{
		v = (v<<8)|((uint8_t)(*in++)), b+=8;
		while (b>=6 && n<osize)
		{
			out[n++] = b64_chars[(v>>(b-=6))&0x3F];
			if (linewidth>0 && ++w>=linewidth && n<osize) out[n++]='\n', w=0;
		}
	}
	if (b && n<osize)
	{
		out[n++] = b64_chars[(v<<(6-b))&0x3F];
		if (linewidth>0 && ++w>=linewidth && n<osize) out[n++]='\n', w=0;
	}
	while (pad-- && n<osize)
	{
		out[n++] = '=';
		if (linewidth>0 && ++w>=linewidth && n<osize) out[n++]='\n', w=0;
	}
	if (n<osize) out[n] = 0;
	return n;
}

int b64_decode(const uint8_t *in, int isize, char *out, int osize)
{
	char c;
	int n=0;
	uint8_t b=0, t;
	uint32_t v=0;
	while ((c=*in++)!=0 && n<osize && isize--)
	{
		if      (c>='A' && c<='Z') t = c-'A';
		else if (c>='a' && c<='z') t = c-'a'+26;
		else if (c>='0' && c<='9') t = c-'0'+52;
		else if (c=='+') t = 62;
		else if (c=='/') t = 63;
		else if (c=='=') break;
		else if (isspace((uint8_t)c)) continue;
		else break;
		b += 6;
		v = (v<<6)|t;
		if (b>=8)
		{
			b-=8;
			*out++ = (uint8_t)(v>>b);
			n++;
		}
	}
	return n;
}


#include "lillib.h"

int qstrlen(const char *str)
{
	const char *start = str;
	while (*str) str++;
	return str - start;
}

char qstrcmp(const char *a, const char *b)
{
	while (1)
	{
		char ca=*a++, cb=*b++;
		if (ca!=cb) return ca-cb;
		if (ca==0) return 0;
	}
}

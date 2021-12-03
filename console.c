#include <ctype.h>
#include "lillib.h"
// A return of 0 is considered success.

// Called to print prompt; may return non-zero to terminate
char __attribute__((weak)) console_prompt(void *state)
{
	com_printf("\n> ");
	return 0;
}

char __attribute__((weak)) console_readline(char *buf, uint8_t size)
{
	// like gets, but with echo and special char handling
	char c;
	uint8_t i;
	size--; //NULL char
	for (i=0; i<size; )
	{
		c = com_getc();
		if (c==0x04) break; // EOT char ends line
		if (c==0x7F) { if (i) buf--, i--, com_putc(c); continue; } // BS char.  Ignore or backspace
		if (c==0x1B) { // ESC key sequence: http://en.wikipedia.org/wiki/Control_Sequence_Introducer#Sequence_elements
			// wut to do here
		}
		*buf++ = c;
		i++;
		//if (isgraph(c) || c=='\n') com_putc(c); //Echo!
		//else com_printf("<%02X>", c);
		com_putc(c); //Echo!
		if (c=='\n') break;
	}
	*buf = 0;
	return i; // If transmission ended with no data, we hit EOF
}

static uint8_t arg_split(char *buf, char **argv)
{
	uint8_t n = 0;
	while (n<LILLIB_CFG_CONSOLE_MAX_ARGV)
	{
		//Strip leading spaces
		while (*buf && isspace((uint8_t)*buf)) buf++;
		if (!*buf) break;
		//Save text pointer
		*argv++ = buf;
		n++;
		//Find end of text
		while (*buf && !isspace((uint8_t)*buf)) buf++;
		if (!*buf) break;
		//Terminate arg and move to next
		*buf++ = 0;
	}
	return n;
}


int console(const _console_cmd *commands, LILLIB_CFG_CONSOLE_STATE_TYPE *state)
{
	const _console_cmd *c;
	char buf[LILLIB_CFG_CONSOLE_LINEBUFSIZE];
	char *argv[LILLIB_CFG_CONSOLE_MAX_ARGV];
	uint8_t argc;
	int ret;
	//Go
	while (1)
	{
		if ((ret=console_prompt(state))!=0) return ret;
		if (!console_readline(buf, LILLIB_CFG_CONSOLE_LINEBUFSIZE)) break;
		argc = arg_split(buf, argv);

		//Print commands on no input
		if (!argc)
		{
			com_puts("Available Commands:");
			for (c=commands; c->name; c++) com_printf("  %<16s%s\n", c->name, (c->usage ? c->usage : "?"));
			com_puts("type \"help COMMAND\" for help");
			continue;
		}
		/*
		if (!qstrcmp(argv[0], "help"))
		{
			if (c->usage)              com_printf("USAGE: %s\n", c->usage);
			if (c->desc)               com_puts(c->desc);
			if (!c->usage && !c->desc) com_puts("No help available");
			continue;
		}
		*/
		//Dispatch command
		for (c=commands; c->name; c++)
		{
			if (qstrcmp(argv[0], c->name)) continue;
			ret = c->func(argc, argv, state);
			if (ret) return ret;
			break;
		}
		if (!c->name) com_puts("Command not found");
	}
	return 0;
}

#ifndef _LILLIB_H_
#define _LILLIB_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "board_cfg.h"

// Types
#if defined __x86_64__
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#define VA_LIST_PTR(v) ((va_list *)(v))
#elif defined __arm__
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#define VA_LIST_PTR(v) (&(v))
#elif defined __AVR__
typedef signed char int8_t;
typedef signed int int16_t;
typedef signed long int int32_t;
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long int uint32_t;
#define VA_LIST_PTR(v) (&(v))
#else
#error Unknown ARCH
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

//Config
#define LILLIB_CFG_AES_AVR_ASM
#define LILLIB_CFG_AES_AVR_ASM_TEST
#define LILLIB_CFG_AES_SBOX_TABLES
// #define LILLIB_CFG_AES_ENCRYPT
// #define LILLIB_CFG_AES_DECRYPT
// #define LILLIB_CFG_AES_128
// #define LILLIB_CFG_AES_256

//Flags/constants
#define ITOA2_UCASE    0x01  // Upper case letters for radix>10
#define ITOA2_SIGNED   0x02  // Number is signed
#define ITOA2_NOSIGN   0x04  // Supress displaying sign

//Functions
void udelay(unsigned int us);
void delay(unsigned int ms);
#ifndef TESTING
void sleep(unsigned int ms);
#endif

int qstrlen(const char *str);
char qstrcmp(const char *a, const char *b);

typedef void (*strf_putc)(char c);
void qsprintf(char *buf, int size, const char *fmt, ...);
char *itoa2(char *rbuf, unsigned int v, char radix, uint8_t flags);
int qstrtol(const char *str, const char **end, char radix);
int strtol_b10(const char *str, const char **end);
int strtol_b10u(const char *str, const char **end);
int strtol_b10u_micro(const char *str, const char **end);
void strf_print(strf_putc putc, const char *fmt, va_list args);
void strf_sprint(char *buf, int size, const char *fmt, va_list args);


void com_init();
void com_flush();
uint8_t com_poll();
char com_getc();
char *com_gets(char *buf, uint8_t size, uint8_t echo);
void com_putc(char c);
void com_puts(const char *str);
void com_printf(const char *fmt, ...);
void com_print_buf(const char *title, uint8_t *buf, int n, int wrap);


int b16_encode(const uint8_t *in, int isize, char *out, int osize, int linewidth);
int b16_decode(const char *in, int isize, uint8_t *out, int osize);
int b64_encode_size(int isize, int linewidth);
int b64_encode(const char *in, int isize, uint8_t *out, int osize, int linewidth);
int b64_decode(const uint8_t *in, int isize, char *out, int osize);


#ifdef LILLIB_CFG_AES_AVR_ASM
void aes128_avr_expand_key(const uint8_t key[16], uint8_t exkey[176]);
void aes128_avr_encrypt_ctr(const uint8_t key[176], uint8_t ctr[16], uint8_t *data, uint8_t n);
#ifdef LILLIB_CFG_AES_AVR_ASM_TEST
void aes128_avr_test_ctr();
#endif
#endif

#ifdef LILLIB_CFG_AES_128
#ifdef LILLIB_CFG_AES_ENCRYPT
void aes128_encrypt_ecb(const uint8_t enckey[16], uint8_t plaintext[16]);
#ifdef LILLIB_CFG_AES_DECRYPT
void aes128_deckey(const uint8_t enckey[16], uint8_t deckey[16]);
void aes128_decrypt_ecb(const uint8_t deckey[16], uint8_t cyphertext[16]);
#endif
#endif
#endif
#ifdef LILLIB_CFG_AES_256
#ifdef LILLIB_CFG_AES_ENCRYPT
void aes256_encrypt_ecb(const uint8_t enckey[32], uint8_t plaintext[16]);
#ifdef LILLIB_CFG_AES_DECRYPT
void aes256_deckey(const uint8_t enckey[32], uint8_t deckey[32]);
void aes256_decrypt_ecb(const uint8_t deckey[32], uint8_t cyphertext[16]);
#endif
#endif
#endif



#define LILLIB_CFG_CONSOLE_MAX_ARGV        10
#define LILLIB_CFG_CONSOLE_LINEBUFSIZE     250
#define LILLIB_CFG_CONSOLE_STATE_TYPE      void
//The list of commands is terminated by a command with name==NULL
typedef struct
{
	const char *name;
	int (*func)(uint8_t argc, char *argv[], LILLIB_CFG_CONSOLE_STATE_TYPE *state);
	const char *usage;
	const char *desc;
} _console_cmd;

int console(const _console_cmd *commands, LILLIB_CFG_CONSOLE_STATE_TYPE *state);




typedef struct _GlyphData { int8_t x, y, w, h, dx; const uint16_t data[]; } GlyphData;
typedef struct _FontData { int8_t w, h; const GlyphData *glyphs[128]; } FontData;
extern const FontData fontVB_LucidaCon8;
extern const FontData fontVB_LucidaCon9;
extern const FontData fontVB_LucidaCon10;
extern const FontData fontVB_LucidaCon12;
extern const FontData fontVB_Times9;
extern const FontData fontVB_Times10;
extern const FontData fontVB_Times12;
extern const FontData fontVB_Arial9;
extern const FontData fontVB_Arial10;
extern const FontData fontVB_Arial12;
extern const FontData fontVB_ArialN9;
extern const FontData fontVB_ArialN10;
extern const FontData fontVB_ArialN12;
extern const FontData fontVB_Silkscreen;
extern const FontData fontVB_PixelSixL;
extern const FontData fontVB_PixelSixU;
extern const FontData fontVB_PixelSixUN;
extern const FontData fontVB_04B03;
extern const FontData fontVB_04B03B;
extern const FontData fontVB_04B08;
extern const FontData fontVB_04B11;
extern const FontData fontVB_04B24U;
extern const FontData fontVB_04B25;
extern const FontData fontVB_FixedV0;
extern const FontData fontVB_piXelize;


#ifdef __cplusplus
} // extern "C"
#endif
#endif // _LILLIB_H_

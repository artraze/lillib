#include "lillib.h"
#if defined(__AVR__) && defined(LILLIB_CFG_AES_AVR_ASM)

// The AVR benefits a lot (50% improvement) with inline asm as GCC code gen really doesn't deliver in a lot of cases
// In order to keep things clean, I'm making the AVR inline ASM encrypt-only, sbox-table stuff this separate file.
typedef uint8_t index_t;
#include <avr/pgmspace.h>

// The AVR will want the tables in program memory (at small speed cost) as they take up 1/4 of the RAM!
// Also aligning the tables is a small cost and a huge speedup
// Piggyback on C AES impl tables if in use, otherwise, define the table here
// Note that this could just require SBOX_TABLES, but there could be cases where the fast encrypt is needed
// but table-less decrypt is also wanted (and must be pulled from C impl).
// I forget why this is marked volatile...
#ifdef LILLIB_CFG_AES_SBOX_TABLES
extern const uint8_t _rj_sbox_f[256] PROGMEM __attribute__((aligned(256)));
#else
static const volatile uint8_t _rj_sbox_f[256] PROGMEM __attribute__((aligned(256)));
#endif


#define SBOXi(reg)               \
	"   mov   r30, r"#reg"   \n" \
	"   lpm   r"#reg", Z     \n"
#define ADDKEYi(reg)             \
	"   ld    r20, Y+        \n" \
	"   eor   r"#reg", r20   \n"
#define ADDKEYSBOXi(reg)         \
	"   ld    r30, Y+        \n" \
	"   eor   r30, r"#reg"   \n" \
	"   lpm   r"#reg", Z     \n"

#define SBOX()         SBOXi(0) SBOXi(1) SBOXi(2) SBOXi(3) SBOXi(4) SBOXi(5) SBOXi(6) SBOXi(7) SBOXi(8) SBOXi(9) SBOXi(10) SBOXi(11) SBOXi(12) SBOXi(13) SBOXi(14) SBOXi(15)
#define ADDKEY()       ADDKEYi(0) ADDKEYi(1) ADDKEYi(2) ADDKEYi(3) ADDKEYi(4) ADDKEYi(5) ADDKEYi(6) ADDKEYi(7) ADDKEYi(8) ADDKEYi(9) ADDKEYi(10) ADDKEYi(11) ADDKEYi(12) ADDKEYi(13) ADDKEYi(14) ADDKEYi(15)
#define ADDKEYSBOX()   ADDKEYSBOXi(0) ADDKEYSBOXi(1) ADDKEYSBOXi(2) ADDKEYSBOXi(3) ADDKEYSBOXi(4) ADDKEYSBOXi(5) ADDKEYSBOXi(6) ADDKEYSBOXi(7) ADDKEYSBOXi(8) ADDKEYSBOXi(9) ADDKEYSBOXi(10) ADDKEYSBOXi(11) ADDKEYSBOXi(12) ADDKEYSBOXi(13) ADDKEYSBOXi(14) ADDKEYSBOXi(15)

#define SHIFTROWS()           \
	"   mov   r20,  r1    \n" \
	"   mov    r1,  r5    \n" \
	"   mov    r5,  r9    \n" \
	"   mov    r9, r13    \n" \
	"   mov   r13, r20    \n" \
	"   mov   r20,  r3    \n" \
	"   mov    r3, r15    \n" \
	"   mov   r15, r11    \n" \
	"   mov   r11,  r7    \n" \
	"   mov    r7, r20    \n" \
	"   mov   r20, r10    \n" \
	"   mov   r10,  r2    \n" \
	"   mov    r2, r20    \n" \
	"   mov   r20, r14    \n" \
	"   mov   r14,  r6    \n" \
	"   mov    r6, r20    \n"

#define MIXCOLSi(a,b,c,d)        \
	"   mov   r20, r"#a"     \n" \
	"   mov   r21, r"#b"     \n" \
	"   mov   r22, r"#c"     \n" \
	"   mov   r23, r"#d"     \n" \
	"   eor   r20, r"#b"     \n" \
	"   eor   r21, r"#c"     \n" \
	"   eor   r22, r"#d"     \n" \
	"   eor   r23, r"#a"     \n" \
	"   mov   r19, r20       \n" \
	"   eor   r19, r22       \n" \
	\
	"   lsl   r20            \n" \
	"   brcc  10f            \n" \
	"   eor   r20,  r17      \n" \
	"10:eor   r20,  r19      \n" \
	"   eor   r"#a", r20     \n" \
	\
	"   lsl   r21            \n" \
	"   brcc  10f            \n" \
	"   eor   r21,  r17      \n" \
	"10:eor   r21,  r19      \n" \
	"   eor   r"#b", r21     \n" \
	\
	"   lsl   r22            \n" \
	"   brcc  10f            \n" \
	"   eor   r22,  r17      \n" \
	"10:eor   r22,  r19      \n" \
	"   eor   r"#c", r22     \n" \
	\
	"   lsl   r23            \n" \
	"   brcc  10f            \n" \
	"   eor   r23,  r17      \n" \
	"10:eor   r23,  r19      \n" \
	"   eor   r"#d", r23     \n"

#define CRYPT_STORE(reg) \
	"   ld    r18,   Z       \n" \
	"   eor   r18,  r"#reg"  \n" \
	"   st     Z+,  r18      \n"

void aes128_avr_encrypt_ctr(const uint8_t key[176], uint8_t ctr[16], uint8_t *data, uint8_t n)
{
	__asm__  volatile (
	// Load data into registers
		"                     \n"
		"   mov   r16, r18    \n" // r16 = n
		"   movw  r26, r22    \n" // X = ctr
		"   movw  r28, r24    \n" // Y = key
		"   movw  r24, r20    \n" // r25:r24 = data
		"   ldi   r17, 0x1B   \n"
		"1:                   \n"
		"   ldi   r18, 0x03   \n"
		"   ldi   r31, hi8(_rj_sbox_f)\n"
		"   ld     r0,  X+    \n"
		"   ld     r1,  X+    \n"
		"   ld     r2,  X+    \n"
		"   ld     r3,  X+    \n"
		"   ld     r4,  X+    \n"
		"   ld     r5,  X+    \n"
		"   ld     r6,  X+    \n"
		"   ld     r7,  X+    \n"
		"   ld     r8,  X+    \n"
		"   ld     r9,  X+    \n"
		"   ld    r10,  X+    \n"
		"   ld    r11,  X+    \n"
		"   ld    r12,  X+    \n"
		"   ld    r13,  X+    \n"
		"   ld    r14,  X+    \n"
		"   ld    r15,  X+    \n"
		
		"   movw  r20, r12    \n"
		"   movw  r22, r14    \n"
		"   subi  r23, 0xFF   \n"
		"   sbci  r22, 0xFF   \n"
		"   sbci  r21, 0xFF   \n"
		"   sbci  r20, 0xFF   \n"
		"   st     -X, r23    \n"
		"   st     -X, r22    \n"
		"   st     -X, r21    \n"
		"   st     -X, r20    \n"
		
		"   rjmp  3f          \n"
		"2:                   \n"
		MIXCOLSi( 0,  9,  2, 11)
		MIXCOLSi( 4, 13,  6, 15)
		MIXCOLSi( 8,  1, 10,  3)
		MIXCOLSi(12,  5, 14,  7)
		ADDKEYSBOX()
		MIXCOLSi( 0, 13, 10,  7)
		MIXCOLSi( 4,  1, 14, 11)
		MIXCOLSi( 8,  5,  2, 15)
		MIXCOLSi(12,  9,  6,  3)
		ADDKEYSBOX()
		MIXCOLSi( 0,  1,  2,  3)
		MIXCOLSi( 4,  5,  6,  7)
		MIXCOLSi( 8,  9, 10, 11)
		MIXCOLSi(12, 13, 14, 15)
		"3:                   \n"
		ADDKEYSBOX()
		MIXCOLSi( 0,  5, 10, 15)
		MIXCOLSi( 4,  9, 14,  3)
		MIXCOLSi( 8, 13,  2,  7)
		MIXCOLSi(12,  1,  6, 11)
		ADDKEYSBOX()
		// End
		"   dec   r18         \n"
		"   breq   4f         \n" //Must do it this way as branch can only do +/- 64
		"   rjmp   2b         \n"
		"4:                   \n"
		ADDKEY()
		"   movw  r30, r24    \n"
		CRYPT_STORE(0)
		CRYPT_STORE(9)
		CRYPT_STORE(2)
		CRYPT_STORE(11)
		CRYPT_STORE(4)
		CRYPT_STORE(13)
		CRYPT_STORE(6)
		CRYPT_STORE(15)
		CRYPT_STORE(8)
		CRYPT_STORE(1)
		CRYPT_STORE(10)
		CRYPT_STORE(3)
		CRYPT_STORE(12)
		CRYPT_STORE(5)
		CRYPT_STORE(14)
		CRYPT_STORE(7)
		"   dec   r16         \n"
		"   breq   5f         \n"
		"   subi  r28, 176    \n" // Move the key pointer back
		"   sbci  r29,   0    \n" // ...
		"   sbiw  r26,  12    \n" // Move the ctr pointer back
		"   movw  r24, r30    \n" // Store data pointer
		"   rjmp   1b         \n"
		"5:                   \n"
		"   eor    r1,  r1    \n" // GCC doesn't seem to listen the 'clobbering r1' so restore to 0 manually
	: : : "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","r13","r14","r15","r16","r17","r18","r19","r20","r21","r22","r23","r24","r25","r26","r27","r28","r29","r30","r31");
}




#define NEXT_KEY()            \
	"   mov   r30, r13    \n" \
	"   lpm   r30,   Z    \n" \
	"   eor    r0, r30    \n" \
	"   eor    r0, r20    \n" \
	"   mov   r30, r14    \n" \
	"   lpm   r30,   Z    \n" \
	"   eor    r1, r30    \n" \
	"   mov   r30, r15    \n" \
	"   lpm   r30,   Z    \n" \
	"   eor    r2, r30    \n" \
	"   mov   r30, r12    \n" \
	"   lpm   r30,   Z    \n" \
	"   eor    r3, r30    \n" \
	"   eor    r4,  r0    \n" \
	"   eor    r5,  r1    \n" \
	"   eor    r6,  r2    \n" \
	"   eor    r7,  r3    \n" \
	"   eor    r8,  r4    \n" \
	"   eor    r9,  r5    \n" \
	"   eor   r10,  r6    \n" \
	"   eor   r11,  r7    \n" \
	"   eor   r12,  r8    \n" \
	"   eor   r13,  r9    \n" \
	"   eor   r14, r10    \n" \
	"   eor   r15, r11    \n"
#define NEXT_RCON()           \
	"   lsl  r20          \n" \
	"   brcc 5f           \n" \
	"   eor  r20, r19     \n" \
	"5:                   \n" \

void aes128_avr_expand_key(const uint8_t key[16], uint8_t exkey[176])
{
	// Expands and shuffles key
	__asm__ volatile (
		"   ldi   r31, hi8(_rj_sbox_f)\n"
		"   ldi   r18, 0x03   \n"
		"   ldi   r19, 0x1B   \n"
		"   ldi   r20, 0x01   \n"
		"   ld    r0,  Y+     \n"
		"   ld    r1,  Y+     \n"
		"   ld    r2,  Y+     \n"
		"   ld    r3,  Y+     \n"
		"   ld    r4,  Y+     \n"
		"   ld    r5,  Y+     \n"
		"   ld    r6,  Y+     \n"
		"   ld    r7,  Y+     \n"
		"   ld    r8,  Y+     \n"
		"   ld    r9,  Y+     \n"
		"   ld   r10,  Y+     \n"
		"   ld   r11,  Y+     \n"
		"   ld   r12,  Y+     \n"
		"   ld   r13,  Y+     \n"
		"   ld   r14,  Y+     \n"
		"   ld   r15,  Y+     \n"
		"   rjmp 2f           \n"
		"1:                   \n"
		NEXT_RCON()
		NEXT_KEY()
		NEXT_RCON()
		"st X+,r0\n st X+,r5\n st X+,r10\n st X+,r15\n st X+,r4\n st X+,r9\n st X+,r14\n st X+,r3\n st X+,r8\n st X+,r13\n st X+,r2\n st X+,r7\n st X+,r12\n st X+,r1\n st X+,r6\n st X+,r11\n"
		NEXT_KEY()
		NEXT_RCON()
		"2:                   \n"
		"st X+,r0\n st X+,r1\n st X+,r2\n st X+,r3\n st X+,r4\n st X+,r5\n st X+,r6\n st X+,r7\n st X+,r8\n st X+,r9\n st X+,r10\n st X+,r11\n st X+,r12\n st X+,r13\n st X+,r14\n st X+,r15\n"
		NEXT_KEY()
		NEXT_RCON()
		"st X+,r0\n st X+,r13\n st X+,r10\n st X+,r7\n st X+,r4\n st X+,r1\n st X+,r14\n st X+,r11\n st X+,r8\n st X+,r5\n st X+,r2\n st X+,r15\n st X+,r12\n st X+,r9\n st X+,r6\n st X+,r3\n"
		NEXT_KEY()
		"st X+,r0\n st X+,r9\n st X+,r2\n st X+,r11\n st X+,r4\n st X+,r13\n st X+,r6\n st X+,r15\n st X+,r8\n st X+,r1\n st X+,r10\n st X+,r3\n st X+,r12\n st X+,r5\n st X+,r14\n st X+,r7\n"
		"   dec  r18          \n"
		"   breq 4f           \n"
		"   rjmp 1b           \n"
		"4:                   \n"
		"   eor   r1, r1      \n" // GCC doesn't seem to listen the 'clobbering r1' so restore to 0 manually
	: "=&x"(exkey) : "0"(exkey), "y"(key) : "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","r13","r14","r15","r19","r30","r31");
}


#ifdef LILLIB_CFG_AES_AVR_ASM_TEST
#include <avr/interrupt.h>

static int bytecmp(const uint8_t *a, const uint8_t *b, int len)
{
	while (len--) if (*a++!=*b++) return 0;
	return 1;
}

void aes128_avr_test_ctr()
{
	volatile uint16_t t0, t1;
	uint8_t i;
	uint8_t key_exp[176];
	const uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
	const uint8_t refc[16] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xff, 0x03};
	const uint8_t pt[16*4] = {
		0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
		0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
		0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
		0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
	};
	const uint8_t ct[16*4] = {
		0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26, 0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce,
		0x98, 0x06, 0xf6, 0x6b, 0x79, 0x70, 0xfd, 0xff, 0x86, 0x17, 0x18, 0x7b, 0xb9, 0xff, 0xfd, 0xff,
		0x5a, 0xe4, 0xdf, 0x3e, 0xdb, 0xd5, 0xd3, 0x5e, 0x5b, 0x4f, 0x09, 0x02, 0x0d, 0xb0, 0x3e, 0xab,
		0x1e, 0x03, 0x1d, 0xda, 0x2f, 0xbe, 0x03, 0xd1, 0x79, 0x21, 0x70, 0xa0, 0xf3, 0x00, 0x9c, 0xee
	};
	uint8_t ctr[16] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};
	uint8_t buf[16*4];
	
	// Setup timer1 (performance counter)
	TCCR1A = 0x00;
	TCCR1B = 0x01;
	TIMSK1 = 0x00;
	
	cli(); t0 = TCNT1; aes128_avr_expand_key(key, key_exp); t1 = TCNT1; sei();
	com_printf("Expanded, shuffled key (time: %iclk):\n", t1-t0);
	for (i=0; i<176; i++)
	{
		if ((i&0xF)==0xF) com_printf("0x%02X\n", key_exp[i]);
		else              com_printf("0x%02X, ", key_exp[i]);
	}
	
	for (i=0; i<4*16; i++) buf[i] = pt[i];
	cli(); t0 = TCNT1; aes128_avr_encrypt_ctr(key_exp, ctr, buf, 4); t1 = TCNT1; sei();
	com_printf("\nCyphertext (time: %iclk, %iclock/block):\n", t1-t0, (t1-t0)/4);
	for (i=0; i<4*16; i++) 
	{
		com_printf("%02X ", buf[i]);
		if ((i&0xF)==0xF) com_printf("-- %s\n", (bytecmp(ct+i-15,buf+i-15,16) ? "OKAY" : "FAIL"));
	}
	
	ctr[14]=0xFE, ctr[15]=0xFF;
	cli(); t0 = TCNT1; aes128_avr_encrypt_ctr(key_exp, ctr, buf, 4); t1 = TCNT1; sei();
	com_printf("\nPlaintext (time: %iclk, %iclock/block):\n", t1-t0, (t1-t0)/4);
	for (i=0; i<4*16; i++) 
	{
		com_printf("%02X ", buf[i]);
		if ((i&0xF)==0xF) com_printf("-- %s\n", (bytecmp(pt+i-15,buf+i-15,16) ? "OKAY" : "FAIL"));
	}
	com_printf("\nCounter result:\n");
	for (i=0; i<16; i++) com_printf("%02X ", ctr[i]);
	com_printf("-- %s\n", (bytecmp(refc,ctr,16) ? "OKAY" : "FAIL"));

	cli(); t0 = TCNT1; aes128_avr_encrypt_ctr(key_exp, ctr, buf, 1); t1 = TCNT1; sei(); uint16_t tt1 = t1-t0;
	cli(); t0 = TCNT1; aes128_avr_encrypt_ctr(key_exp, ctr, buf, 2); t1 = TCNT1; sei(); uint16_t tt2 = t1-t0;
	cli(); t0 = TCNT1; aes128_avr_encrypt_ctr(key_exp, ctr, buf, 3); t1 = TCNT1; sei(); uint16_t tt3 = t1-t0;
	cli(); t0 = TCNT1; aes128_avr_encrypt_ctr(key_exp, ctr, buf, 4); t1 = TCNT1; sei(); uint16_t tt4 = t1-t0;
	com_printf("\n\nTimings:\n");
	com_printf("   1 block : %5i - %5i/blk\n", tt1, tt1/1);
	com_printf("   2 block : %5i - %5i/blk, %+6i\n", tt2, tt2/2, tt2-tt1);
	com_printf("   3 block : %5i - %5i/blk, %+6i\n", tt3, tt3/3, tt3-tt2);
	com_printf("   4 block : %5i - %5i/blk, %+6i\n", tt4, tt4/4, tt4-tt3);
}
#endif // LILLIB_CFG_AES_AVR_ASM_TEST

#ifndef LILLIB_CFG_AES_SBOX_TABLES
static const volatile uint8_t _rj_sbox_f[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};
#endif // LILLIB_CFG_AES_SBOX_TABLES
#endif // defined(__AVR__) && defined(LILLIB_CFG_AES_AVR_ASM)

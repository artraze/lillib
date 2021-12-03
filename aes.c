#include "lillib.h"

#if (defined(LILLIB_CFG_AES_ENCRYPT) || defined(LILLIB_CFG_AES_DECRYPT))
#if (defined(LILLIB_CFG_AES_128) || defined(LILLIB_CFG_AES_256))
#define LILLIB_AES_COMMON
#else
#error If LILLIB_CFG AES_ENCRYPT or AES_DECRYPT is set, at least one of AES_128 or AES_256 should be set as well
#endif
#endif

// Define a small iterator type.
// Using uint8 on ARM occationally makes bad code, and int32 on AVR is wasteful
#if defined __AVR__
typedef unsigned char index_t;
#else
typedef unsigned int index_t;
#endif


#ifdef LILLIB_AES_COMMON

static uint8_t rj_times2(uint8_t x)
{
	return ((x&0x80) ? (x<<1)^0x1b : (x<<1));
}

#endif

#ifdef LILLIB_CFG_AES_SBOX_TABLES

#ifdef __AVR__
// The AVR will want the tables in program memory (at small speed cost) as they take up 1/4 of the RAM!
// Also aligning the tables is a small cost and a huge speedup
#include <avr/pgmspace.h>
#ifdef LILLIB_AES_COMMON
#define rj_sbox_f(x) pgm_read_byte(&(_rj_sbox_f[(x)]))
const uint8_t _rj_sbox_f[256] PROGMEM __attribute__((aligned(256)));
#endif // LILLIB_AES_COMMON
#ifdef LILLIB_CFG_AES_DECRYPT
#define rj_sbox_i(x) pgm_read_byte(&(_rj_sbox_i[(x)]))
const uint8_t _rj_sbox_i[256] PROGMEM __attribute__((aligned(256)));
#endif // LILLIB_CFG_AES_DECRYPT

#else // Not AVR

#ifdef LILLIB_AES_COMMON
#define rj_sbox_f(x) _rj_sbox_f[(x)]
const uint8_t _rj_sbox_f[256];
#endif // LILLIB_AES_COMMON
#ifdef LILLIB_CFG_AES_DECRYPT
#define rj_sbox_i(x) _rj_sbox_i[(x)]
const uint8_t _rj_sbox_i[256];
#endif // LILLIB_CFG_AES_DECRYPT

#endif // __AVR__

#else // LILLIB_CFG_AES_SBOX_TABLES

#ifdef LILLIB_AES_COMMON

static uint8_t gf_alog(uint8_t x)
{
	uint8_t atb = 1;
	while (x--) atb ^= rj_times2(atb);
	return atb;
}

static uint8_t gf_log(uint8_t x)
{
	uint8_t atb, i;
	for (i=0,atb=1; atb!=x; i++) atb ^= rj_times2(atb);
	return i;
}

static uint8_t gf_inv(uint8_t x)
{
	return (x ? gf_alog(0xFF - gf_log(x)) : 0);
}

static uint8_t rj_sbox_f(uint8_t x)
{
	uint8_t s = gf_inv(x);
	uint8_t y = s;
	s ^= (y = (y<<1)|(y>>7));
	s ^= (y = (y<<1)|(y>>7));
	s ^= (y = (y<<1)|(y>>7));
	s ^= (y = (y<<1)|(y>>7));
	return s^0x63;
}

#endif // LILLIB_AES_COMMON

#ifdef LILLIB_CFG_AES_DECRYPT

static uint8_t rj_sbox_i(uint8_t x)
{
	uint8_t y = x^0x63;
	uint8_t s;
	s  = (y = (y<<1)|(y>>7));
	s ^= (y = (y<<2)|(y>>6));
	s ^= (y = (y<<3)|(y>>5));
	return gf_inv(s);
}

#endif // LILLIB_CFG_AES_DECRYPT
#endif // LILLIB_CFG_AES_SBOX_TABLES



#ifdef LILLIB_AES_COMMON

static void add_key(uint8_t *buf, const uint8_t *key)
{
	index_t i;
	for (i=0; i<16; i++) buf[i] ^= key[i];
}

#endif // LILLIB_AES_COMMON

#ifdef LILLIB_CFG_AES_ENCRYPT

static void sbox_f(uint8_t *buf)
{
	index_t i;
	for (i=0; i<16; i++) buf[i] = rj_sbox_f(buf[i]);
}

static void shift_rows_f(uint8_t *buf)
{
	uint8_t t;
	t = buf[ 1]; buf[ 1] = buf[ 5]; buf[ 5] = buf[ 9]; buf[ 9] = buf[13]; buf[13] = t;
	t = buf[ 3]; buf[ 3] = buf[15]; buf[15] = buf[11]; buf[11] = buf[ 7]; buf[ 7] = t;
	t = buf[10]; buf[10] = buf[ 2]; buf[ 2] = t;
	t = buf[14]; buf[14] = buf[ 6]; buf[ 6] = t;
}

static void mix_columns_f(uint8_t *buf)
{
	index_t i;
	uint8_t a, b, c, d, t;
	for (i=0; i<4; i++, buf+=4)
	{
		a = buf[0];
		b = buf[1];
		c = buf[2];
		d = buf[3];
		t = a ^ b ^ c ^ d;
		buf[0] = a ^ t ^ rj_times2(a^b);
		buf[1] = b ^ t ^ rj_times2(b^c);
		buf[2] = c ^ t ^ rj_times2(c^d);
		buf[3] = d ^ t ^ rj_times2(d^a);
	}
}

#endif // LILLIB_CFG_AES_ENCRYPT

#ifdef LILLIB_CFG_AES_DECRYPT

static uint8_t rj_div2(uint8_t x)
{
	return ((x&0x01) ? 0x8d^(x>>1) : (x>>1));
}

static void sbox_i(uint8_t *buf)
{
	index_t i;
	for (i=0; i<16; i++) buf[i] = rj_sbox_i(buf[i]);
}

static void shift_rows_i(uint8_t *buf)
{
	uint8_t t;
	t = buf[ 1]; buf[ 1] = buf[13]; buf[13] = buf[ 9]; buf[ 9] = buf[ 5]; buf[ 5] = t;
	t = buf[ 3]; buf[ 3] = buf[ 7]; buf[ 7] = buf[11]; buf[11] = buf[15]; buf[15] = t;
	t = buf[ 2]; buf[ 2] = buf[10]; buf[10] = t;
	t = buf[ 6]; buf[ 6] = buf[14]; buf[14] = t;
}

static void mix_columns_i(uint8_t *buf)
{
	index_t i;
	uint8_t a, b, c, d, t1, t2, t4ac2, t4bd2;

	for (i=0; i<4; i++, buf+=4)
	{
		a = buf[0];
		b = buf[1];
		c = buf[2];
		d = buf[3];
		t1 = a ^ b ^ c ^ d;
		t2 = rj_times2(t1);
		t4ac2 = rj_times2(t2^a^c);
		t4bd2 = rj_times2(t2^b^d);
		buf[0] = a ^ t1 ^ rj_times2(a^b^t4ac2);
		buf[1] = b ^ t1 ^ rj_times2(b^c^t4bd2);
		buf[2] = c ^ t1 ^ rj_times2(c^d^t4ac2);
		buf[3] = d ^ t1 ^ rj_times2(d^a^t4bd2);
	}
}

#endif // LILLIB_CFG_AES_DECRYPT



#ifdef LILLIB_CFG_AES_128

#ifdef LILLIB_AES_COMMON

static void next_key_128(uint8_t *k, uint8_t rcon)
{
	k[ 0] ^= rj_sbox_f(k[13]) ^ rcon;
	k[ 1] ^= rj_sbox_f(k[14]);
	k[ 2] ^= rj_sbox_f(k[15]);
	k[ 3] ^= rj_sbox_f(k[12]);
	k[ 4]^=k[ 0], k[ 5]^=k[ 1], k[ 6]^=k[ 2], k[ 7]^=k[ 3];
	k[ 8]^=k[ 4], k[ 9]^=k[ 5], k[10]^=k[ 6], k[11]^=k[ 7];
	k[12]^=k[ 8], k[13]^=k[ 9], k[14]^=k[10], k[15]^=k[11];
}

#endif // LILLIB_AES_COMMON

#ifdef LILLIB_CFG_AES_ENCRYPT

void aes128_encrypt_ecb(const uint8_t enckey[16], uint8_t buf[16])
{
	uint8_t key[16];
	uint8_t rcon = 0x01;
	index_t r, i;

	for (i=0; i<16; i++) key[i] = enckey[i];
	for (r=0; r<10; r++)
	{
		if (r) mix_columns_f(buf);
		add_key(buf, key);
		sbox_f(buf);
		shift_rows_f(buf);
		next_key_128(key, rcon);
		rcon = rj_times2(rcon);
	}
	add_key(buf, key);
}

#endif // LILLIB_CFG_AES_ENCRYPT

#ifdef LILLIB_CFG_AES_DECRYPT

static void prev_key_128(uint8_t *k, uint8_t rcon)
{
	k[12]^=k[ 8], k[13]^=k[ 9], k[14]^=k[10], k[15]^=k[11];
	k[ 8]^=k[ 4], k[ 9]^=k[ 5], k[10]^=k[ 6], k[11]^=k[ 7];
	k[ 4]^=k[ 0], k[ 5]^=k[ 1], k[ 6]^=k[ 2], k[ 7]^=k[ 3];
	k[ 0] ^= rj_sbox_f(k[13]) ^ rcon;
	k[ 1] ^= rj_sbox_f(k[14]);
	k[ 2] ^= rj_sbox_f(k[15]);
	k[ 3] ^= rj_sbox_f(k[12]);
}

void aes128_deckey(const uint8_t enckey[16], uint8_t deckey[16])
{
	index_t i;
	uint8_t rcon = 0x01;
	for (i=0; i<16; i++) deckey[i] = enckey[i];
	for (i=0; i<10; i++) next_key_128(deckey, rcon), rcon = rj_times2(rcon);
}

void aes128_decrypt_ecb(const uint8_t deckey[16], uint8_t buf[16])
{
	uint8_t key[16];
	uint8_t rcon = 0x6C;
	index_t r, i;

	for (i=0; i<16; i++) key[i] = deckey[i];
	add_key(buf, key);
	for (r=0; r<10; r++)
	{
		if (r) mix_columns_i(buf);
		shift_rows_i(buf);
		sbox_i(buf);
		rcon = rj_div2(rcon);
		prev_key_128(key, rcon);
		add_key(buf, key);
	}
}

#endif // LILLIB_CFG_AES_DECRYPT
#endif // LILLIB_CFG_AES_128





#ifdef LILLIB_CFG_AES_256

#ifdef LILLIB_AES_COMMON

static void next_key_256(uint8_t *k, uint8_t rcon)
{
	k[ 0] ^= rj_sbox_f(k[29]) ^ rcon;
	k[ 1] ^= rj_sbox_f(k[30]);
	k[ 2] ^= rj_sbox_f(k[31]);
	k[ 3] ^= rj_sbox_f(k[28]);
	k[ 4]^=k[ 0], k[ 5]^=k[ 1], k[ 6]^=k[ 2], k[ 7]^=k[ 3];
	k[ 8]^=k[ 4], k[ 9]^=k[ 5], k[10]^=k[ 6], k[11]^=k[ 7];
	k[12]^=k[ 8], k[13]^=k[ 9], k[14]^=k[10], k[15]^=k[11];
	k[16] ^= rj_sbox_f(k[12]);
	k[17] ^= rj_sbox_f(k[13]);
	k[18] ^= rj_sbox_f(k[14]);
	k[19] ^= rj_sbox_f(k[15]);
	k[20]^=k[16], k[21]^=k[17], k[22]^=k[18], k[23]^=k[19];
	k[24]^=k[20], k[25]^=k[21], k[26]^=k[22], k[27]^=k[23];
	k[28]^=k[24], k[29]^=k[25], k[30]^=k[26], k[31]^=k[27];
}

#endif // LILLIB_AES_COMMON

#ifdef LILLIB_CFG_AES_ENCRYPT

void aes256_encrypt_ecb(const uint8_t enckey[32], uint8_t buf[16])
{
	uint8_t key[32];
	uint8_t rcon = 0x01;
	index_t r, i;

	for (i=0; i<32; i++) key[i] = enckey[i];
	for (r=0; r<14; r+=2)
	{
		if (r) mix_columns_f(buf);
		add_key(buf, key);
		sbox_f(buf);
		shift_rows_f(buf);
		mix_columns_f(buf);
		add_key(buf, key+16);
		sbox_f(buf);
		shift_rows_f(buf);
		next_key_256(key, rcon), rcon = rj_times2(rcon);
	}
	add_key(buf, key);
}

#endif // LILLIB_CFG_AES_ENCRYPT

#ifdef LILLIB_CFG_AES_DECRYPT

static void prev_key_256(uint8_t *k, uint8_t rcon)
{
	k[28]^=k[24], k[29]^=k[25], k[30]^=k[26], k[31]^=k[27];
	k[24]^=k[20], k[25]^=k[21], k[26]^=k[22], k[27]^=k[23];
	k[20]^=k[16], k[21]^=k[17], k[22]^=k[18], k[23]^=k[19];
	k[16] ^= rj_sbox_f(k[12]);
	k[17] ^= rj_sbox_f(k[13]);
	k[18] ^= rj_sbox_f(k[14]);
	k[19] ^= rj_sbox_f(k[15]);
	k[12]^=k[ 8], k[13]^=k[ 9], k[14]^=k[10], k[15]^=k[11];
	k[ 8]^=k[ 4], k[ 9]^=k[ 5], k[10]^=k[ 6], k[11]^=k[ 7];
	k[ 4]^=k[ 0], k[ 5]^=k[ 1], k[ 6]^=k[ 2], k[ 7]^=k[ 3];
	k[ 0] ^= rj_sbox_f(k[29]) ^ rcon;
	k[ 1] ^= rj_sbox_f(k[30]);
	k[ 2] ^= rj_sbox_f(k[31]);
	k[ 3] ^= rj_sbox_f(k[28]);
}

void aes256_deckey(const uint8_t enckey[32], uint8_t deckey[32])
{
	index_t i;
	uint8_t rcon = 0x01;
	for (i=0; i<32; i++) deckey[i] = enckey[i];
	for (i=0; i< 7; i++) next_key_256(deckey, rcon), rcon = rj_times2(rcon);
}

void aes256_decrypt_ecb(const uint8_t deckey[32], uint8_t buf[16])
{
	uint8_t key[32];
	uint8_t rcon = 0x80;
	index_t r, i;

	for (i=0; i<32; i++) key[i] = deckey[i];
	add_key(buf, key);
	for (r=0; r<14; r+=2)
	{
		if (r) mix_columns_i(buf);
		shift_rows_i(buf);
		sbox_i(buf);
		rcon = rj_div2(rcon), prev_key_256(key, rcon);
		add_key(buf, key+16);
		mix_columns_i(buf);
		shift_rows_i(buf);
		sbox_i(buf);
		add_key(buf, key);
	}
}

#endif // LILLIB_CFG_AES_DECRYPT
#endif // LILLIB_CFG_AES_256



#ifdef LILLIB_CFG_AES_SBOX_TABLES
#ifdef LILLIB_AES_COMMON
const uint8_t _rj_sbox_f[256] = {
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
#endif // LILLIB_AES_COMMON
#ifdef LILLIB_CFG_AES_DECRYPT
const uint8_t _rj_sbox_i[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
    0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
    0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
    0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
    0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
    0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
    0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
    0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
    0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
    0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
    0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
    0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};
#endif // LILLIB_CFG_AES_DECRYPT
#endif // LILLIB_CFG_AES_SBOX_TABLES

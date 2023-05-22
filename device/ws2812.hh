#pragma once
#include "board_cfg.h"
#ifdef DEV_WS2812

#include "device.h"

#if BOARD_CFG_FREQ == 16000000

template<uint8_t kIoi>
void WS2812_send(uint8_t n, uint8_t *grb)
{
	// 0:  400ns( 6.4) high, 850ns(13.6) low
	// 1:  800ns(12.8) high, 450ns( 7.2) low
	
	// Note that writing a bit to the PIN register toggles the bit in the PORT register
	// which only takes 1 cycle versus, e.g., SBI which is 2 cycles
	constexpr uint8_t out_reg = (kIoi >> 4);    // PIN reg for IOI
	constexpr uint8_t out_pin = IOI_MASK(kIoi); // bits to toggle
	
	uint8_t v, bit;
	
	IOI_OUT_SET(kIoi, 0);
	
	__asm__ volatile (
		"   cli            \n"  /* Disable interrupts!                                                             */ \
		"   ld    %2, %a0+ \n"  /* v   = *grb++                                                                    */ \
		"   ldi   %3, 0x08 \n"  /* bit = 8                                                                         */ \
		/* High side, bit: 0=400ns(6.4=7), 1=800ns(12.8=13)                                                        */ \
		"1: nop            \n"  /* 1                                                                               */ \
		"   out   %7, %6   \n"  /* 1    PINC, toggle high                                                          */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   lsl   %2       \n"  /* 1    Shift MSB into carry                                                       */ \
		"   brcc  3f       \n"  /* 1/2  Skip the delay if bit was 0                                                */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"2: nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		/* Low side: 0=850ns(13.6=14), 1=450ns(7.2=7)                                                              */ \
		"3: out   %7, %6   \n"  /* 1    OUT LOW                                                                    */ \
		"   brcs  4f       \n"  /* 1/2  Skip the delay if bit was 1                                                */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"   nop            \n"  /* 1                                                                               */ \
		"4: dec   %3       \n"  /* 1    bit--                                                                      */ \
		"   brne  1b       \n"  /* 1/2  while(bit)                                                                 */ \
		"   dec   %1       \n"  /* 1    n--                                                                        */ \
		"   breq  9f       \n"  /* 1    if (!n) break                                                              */ \
		/* High side, byte: 0=400ns(6.4=7), 1=800ns(12.8=13)                                                       */ \
		"   out   %7, %6   \n"  /* 1    OUT HIGH                                                                   */ \
		"   ld    %2, %a0+ \n"  /* 2    v = *grb++ (2 clocks?  The docs would say 1, but maybe the post-inc?)      */ \
		"   ldi   %3, 0x08 \n"  /* 1    bit = 8                                                                    */ \
		"   lsl   %2       \n"  /* 1    Shift MSB into carry                                                       */ \
		"   brcc  3b       \n"  /* 1/2  Jump to long-low if bit was 0 as byte setup took all the short-high clocks */ \
		"   rjmp  2b       \n"  /* 2    Jump into long-high delay to burn the remaining long-high clocks           */ \
		"9: sei            \n"  /* End, restore interrupts                                                         */
		: "=e"(grb), "=r"(n), "=&r"(v), "=&d"(bit)
		: "0"(grb), "1"(n), "r"(out_pin), "M"(out_reg)
	);
}

#else
#error Unsupported CPU clock
#endif

#endif // DEV_WS2812

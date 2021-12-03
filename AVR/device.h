#ifndef _AVR_DEVICE_H_
#define _AVR_DEVICE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "board_cfg.h"

// Macros for defining I/O Indices so an int8 can describe a Port+Pin
// They are disappointingly inefficient, since optimized AVR I/O goes through
// things that require hardcoded register addresses (IN, OUT, SBI, CBI), but
// oh well.  Arduino seems to get by doing something even worse AFAICT but I
// would maybe like to redesign these device functions to be inline with pins
// defined in board_cfg.h (e.g. for WT588D), but that also come with some
// challenges...

#define IOI(port, bit)      ((3 * (0x##port##0 - 0xA0)) | 0x0##bit)

#define IOI_PIN_ADDR(ioi)   (((ioi)>>4) + 0x00)
#define IOI_DDR_ADDR(ioi)   (((ioi)>>4) + 0x01)
#define IOI_PORT_ADDR(ioi)  (((ioi)>>4) + 0x02)

#define IOI_PIN(ioi)        _SFR_IO8(IOI_PIN_ADDR(ioi))
#define IOI_DDR(ioi)        _SFR_IO8(IOI_DDR_ADDR(ioi))
#define IOI_PORT(ioi)       _SFR_IO8(IOI_PORT_ADDR(ioi))
#define IOI_MASK(ioi)       (1 << ((ioi) & 0x07))

#define IOI_PIN_TST(ioi)           (IOI_PIN(ioi) & IOI_MASK(ioi))
#define IOI_PIN_GET(ioi)           ((IOI_PIN(ioi) >> ((ioi) & 0x07)) & 0x01)
#define IOI_DDR_SET(ioi, is_out)   ((is_out) ? (IOI_DDR(ioi)  |= IOI_MASK(ioi)) : (IOI_DDR(ioi)  &= ~IOI_MASK(ioi)))
#define IOI_PORT_SET(ioi, value)   ((value)  ? (IOI_PORT(ioi) |= IOI_MASK(ioi)) : (IOI_PORT(ioi) &= ~IOI_MASK(ioi)))

// AVR specific implementations for lillib internals

uint8_t read_fuse(uint8_t addr);

void i2c_init();
uint8_t i2c_write(uint8_t addr, uint8_t n, const uint8_t *data);
uint8_t i2c_write_1b(uint8_t addr, uint8_t data);
uint8_t i2c_read(uint8_t addr, uint8_t n, uint8_t *data);

// Various device interfaces

// #ifdef DEV_TM1637
// void TM1637_init(uint8_t ioi_clk, uint8_t ioi_dat);
// void TM1637_write_1(uint8_t ioi_clk, uint8_t ioi_dat, uint8_t v);
// void TM1637_cmd_disp(uint8_t ioi_clk, uint8_t ioi_dat, uint8_t on, uint8_t brightness);
// void TM1637_disp(uint8_t ioi_clk, uint8_t ioi_dat, uint8_t pos, uint8_t len, uint8_t *data);
// void TM1637_disp_num(uint8_t ioi_clk, uint8_t ioi_dat, int16_t v, uint8_t dots, uint8_t zeros);
// #endif // DEV_TM1637

#ifdef DEV_WT588D
void WT588D_write(uint8_t v);
void WT588D_itoa_play(int16_t v);
#endif // DEV_WT588D

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _AVR_DEVICE_H_

#ifndef YM2151_H
#define YM2151_H

#include <stdint.h>
#include "ym2151_definitions.h"

extern uint32_t g_ym2151_base_addr;

#define YM2151_READ(x)          (*((volatile uint8_t *) g_ym2151_base_addr + x))
#define YM2151_WRITE(x, y)      (*((uint8_t *) g_ym2151_base_addr + x) = y)
#define YM2151_WRITEWORD(x, y)  (*((uint16_t *) (g_ym2151_base_addr + x)) = y)

#define YM2151_WRITE_REG        0x00        // Register address port (W)
#define YM2151_STATUS           0x02        // Status register (R)
#define YM2151_DATA             0x02        // Data port (W)
#define YM2151_CLOCK            0x80        // Clock set port (W)

static inline uint8_t ym2151_readreg(uint8_t reg)
{
    return YM2151_READ(reg);
}

static inline void ym2151_wait_ready(void)
{
    volatile uint8_t status;
    status = ym2151_readreg(YM2151_STATUS);
    while ((status & YM2151_STATUS_BUSY) != 0)
        status = ym2151_readreg(YM2151_STATUS);
}

void ym2151_writereg(uint8_t reg, uint8_t data);

void ym2151_set_base_addr(uint32_t addr);
void ym2151_set_clock(uint32_t freq);

void ym2151_init_lfo();

void ym2151_set_tone(uint8_t ch, uint8_t keycode, int16_t kf);
void ym2151_set_volume(uint8_t ch, uint8_t volume, uint16_t offset);
void ym2151_note_on(uint8_t ch);
void ym2151_note_off(uint8_t ch);
void ym2151_load_timbre(uint8_t ch, uint8_t *prog_addr);
void ym2151_load_separation_timbre(uint8_t ch, uint8_t *prog_addr);
void ym2151_dump_timbre(uint8_t *prog_addr);
void ym2151_set_panpot(uint8_t ch,uint8_t pan);

typedef struct
{
	uint8_t RegFLCON[8];
	uint8_t	RegSLOTMASK[8];
	uint8_t	CarrierSlot[8];
	uint8_t	RegTL[8][4];
} YM2151;

#endif

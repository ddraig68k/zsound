#include <stdint.h>

#include "../../include/zsmplayer68k.h"

#define YM2151_REG_PORT     (*(volatile uint8_t *)0x00B00000u)
#define YM2151_DATA_PORT    (*(volatile uint8_t *)0x00B00002u)
#define YM2151_STATUS_PORT  (*(volatile uint8_t *)0x00B00002u)
#define YM2151_BUSY_MASK    0x80u

#define TEST_VOICE          0u
#define CPU_BURN_PER_FRAME  50000u

static void spin_cycles(volatile uint32_t cycles)
{
    while (cycles-- != 0) {
    }
}

static void board_wait_frame(void)
{
    spin_cycles(CPU_BURN_PER_FRAME);
}

static void ym2151_wait_ready(void)
{
    while ((YM2151_STATUS_PORT & YM2151_BUSY_MASK) != 0u) {
    }
}

static void ym2151_write(uint8_t reg, uint8_t value)
{
    ym2151_wait_ready();
    YM2151_REG_PORT = reg;
    ym2151_wait_ready();
    YM2151_DATA_PORT = value;
}

static uint8_t ym2151_voice_reg(uint8_t reg, uint8_t voice)
{
    if (reg >= 0x20u && reg <= 0x27u) {
        return (uint8_t)((reg & 0xf8u) | voice);
    }
    if (reg >= 0x28u && reg <= 0x2fu) {
        return (uint8_t)((reg & 0xf8u) | voice);
    }
    if (reg >= 0x30u && reg <= 0x37u) {
        return (uint8_t)((reg & 0xf8u) | voice);
    }
    if (reg >= 0x38u && reg <= 0x3fu) {
        return (uint8_t)((reg & 0xf8u) | voice);
    }
    if (reg >= 0x40u && reg <= 0xffu) {
        return (uint8_t)((reg & 0xf8u) | voice);
    }
    return reg;
}

static uint8_t ym2151_voice_data(uint8_t reg, uint8_t value, uint8_t voice)
{
    if (reg == 0x08u) {
        return (uint8_t)((value & 0xf8u) | voice);
    }
    return value;
}

static void ym2151_key_off_voice(uint8_t voice)
{
    ym2151_write(0x08u, voice);
    ym2151_write((uint8_t)(0x20u + voice), 0x00u);
}

static void play_zfx_triplets(const uint8_t *seq, uint8_t voice)
{
    for (;;) {
        uint8_t reg = seq[0];
        uint8_t value = seq[1];
        uint8_t delay = seq[2];

        ym2151_write(ym2151_voice_reg(reg, voice),
                     ym2151_voice_data(reg, value, voice));

        seq += 3;

        if (delay == 0xffu) {
            break;
        }

        while (delay-- != 0u) {
            board_wait_frame();
        }
    }
}

int main(void)
{
    zsm_c_init_player();

    /* Stage 1: confirm raw YM register access and patch loading. */
    zsm_c_patch_ym_voice(TEST_VOICE, ymp_ding);
    play_zfx_triplets(zfx_ding, TEST_VOICE);
    ym2151_key_off_voice(TEST_VOICE);

    /*
     * Stage 2: once you have a ZSM blob linked into memory, start it here
     * and drive zsm_c_play_frame_60hz() from your frame interrupt.
     *
     * Example:
     *   extern const uint8_t test_song[];
     *   zsm_c_start_music(test_song);
     */
    for (;;) {
        board_wait_frame();
        zsm_c_play_frame_60hz();
    }
}


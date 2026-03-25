#include <stdint.h>
#include <string.h>

#include "ym2151.h"
#include "zfxfm68k.h"

typedef struct {
    const uint8_t *ptr[8];
    uint16_t delay[8];
    uint8_t active_mask;
    uint8_t allowed_mask;
} zfx_fm_state_t;

static zfx_fm_state_t g_zfx_fm;

void zfx_fm_c_init(void)
{
    memset(&g_zfx_fm, 0, sizeof(g_zfx_fm));
    g_zfx_fm.allowed_mask = 0xffu;
}

void zfx_fm_c_set_allowed_mask(uint8_t mask)
{
    g_zfx_fm.allowed_mask = mask;
}

uint8_t zfx_fm_c_get_allowed_mask(void)
{
    return g_zfx_fm.allowed_mask;
}

int zfx_fm_c_play(uint8_t voice, const uint8_t *sequence)
{
    if (voice >= 8u || sequence == NULL) {
        return -1;
    }

    if ((g_zfx_fm.allowed_mask & (1u << voice)) == 0u) {
        return -1;
    }

    g_zfx_fm.ptr[voice] = sequence;
    g_zfx_fm.delay[voice] = 1u;
    g_zfx_fm.active_mask |= (uint8_t)(1u << voice);
    return 0;
}

int zfx_fm_c_play_any(const uint8_t *sequence)
{
    uint8_t available_mask = (uint8_t)(g_zfx_fm.allowed_mask & (uint8_t)~g_zfx_fm.active_mask);

    if (sequence == NULL || available_mask == 0u) {
        return -1;
    }

    for (uint8_t voice = 0; voice < 8u; ++voice) {
        if ((available_mask & (1u << voice)) != 0u) {
            if (zfx_fm_c_play(voice, sequence) == 0) {
                return voice;
            }
            break;
        }
    }

    return -1;
}

void zfx_fm_c_stop(uint8_t voice)
{
    if (voice >= 8u) {
        return;
    }

    g_zfx_fm.active_mask &= (uint8_t)~(1u << voice);
    g_zfx_fm.delay[voice] = 0u;
    ym2151_writereg(0x08u, voice);
}

int zfx_fm_c_is_active(uint8_t voice)
{
    if (voice >= 8u) {
        return 0;
    }

    return (g_zfx_fm.active_mask & (1u << voice)) != 0u;
}

static void zfx_fm_process_voice(uint8_t voice)
{
    const uint8_t *p = g_zfx_fm.ptr[voice];

    for (;;) {
        uint8_t reg = *p++;
        uint8_t value = *p++;
        uint8_t delay = *p++;

        if (reg == 0x08u) {
            value = (uint8_t)((value & 0xf8u) | voice);
        } else if (reg >= 0x20u) {
            reg = (uint8_t)((reg & 0xf8u) | voice);
        }

        ym2151_writereg(reg, value);

        if (delay == 0xffu) {
            g_zfx_fm.active_mask &= (uint8_t)~(1u << voice);
            g_zfx_fm.delay[voice] = 0u;
            return;
        }

        if (delay != 0u) {
            g_zfx_fm.ptr[voice] = p;
            g_zfx_fm.delay[voice] = delay;
            return;
        }
    }
}

void zfx_fm_c_update(void)
{
    for (uint8_t voice = 0; voice < 8u; ++voice) {
        if ((g_zfx_fm.active_mask & (1u << voice)) == 0u) {
            continue;
        }

        if (--g_zfx_fm.delay[voice] != 0u) {
            continue;
        }

        zfx_fm_process_voice(voice);
    }
}

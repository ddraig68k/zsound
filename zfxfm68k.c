#include <stdint.h>
#include <string.h>

#include "ym2151.h"
#include "zfxfm68k.h"
#include "zsmplayer68k.h"

typedef struct {
    const uint8_t *ptr[8];
    const uint8_t *patch[8];
    uint16_t delay[8];
    uint8_t active_mask;
    uint8_t allowed_mask;
    uint8_t reserved_mask;
    uint8_t priority[8];
} zfx_fm_state_t;

static zfx_fm_state_t g_zfx_fm;

static int zfx_fm_start_voice(uint8_t voice, const uint8_t *patch, const uint8_t *sequence, uint8_t priority)
{
    if (voice >= 8u || sequence == NULL) {
        return -1;
    }

    if ((g_zfx_fm.allowed_mask & (1u << voice)) == 0u) {
        return -1;
    }

    if (patch != NULL && g_zfx_fm.patch[voice] != patch) {
        zsm_patch_ym_voice(voice, patch);
        g_zfx_fm.patch[voice] = patch;
    }

    g_zfx_fm.ptr[voice] = sequence;
    g_zfx_fm.delay[voice] = 1u;
    g_zfx_fm.priority[voice] = priority;
    g_zfx_fm.active_mask |= (uint8_t)(1u << voice);
    return 0;
}

static int zfx_fm_find_dynamic_voice(uint8_t priority)
{
    uint8_t dynamic_mask = (uint8_t)(g_zfx_fm.allowed_mask & (uint8_t)~g_zfx_fm.reserved_mask);
    int best_voice = -1;
    uint8_t best_priority = 0xffu;

    if (dynamic_mask == 0u) {
        return -1;
    }

    for (uint8_t voice = 0; voice < 8u; ++voice) {
        if ((dynamic_mask & (1u << voice)) == 0u) {
            continue;
        }

        if ((g_zfx_fm.active_mask & (1u << voice)) == 0u) {
            return voice;
        }

        if (g_zfx_fm.priority[voice] >= priority) {
            continue;
        }

        if (best_voice < 0 || g_zfx_fm.priority[voice] < best_priority) {
            best_voice = voice;
            best_priority = g_zfx_fm.priority[voice];
        }
    }

    return best_voice;
}

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

void zfx_fm_c_set_reserved_mask(uint8_t mask)
{
    g_zfx_fm.reserved_mask = (uint8_t)(mask & g_zfx_fm.allowed_mask);
}

uint8_t zfx_fm_c_get_reserved_mask(void)
{
    return g_zfx_fm.reserved_mask;
}

int zfx_fm_c_play(uint8_t voice, const uint8_t *sequence)
{
    return zfx_fm_start_voice(voice, NULL, sequence, 0u);
}

int zfx_fm_c_play_any(const uint8_t *sequence)
{
    return zfx_fm_c_play_any_cached(NULL, sequence, 0u);
}

int zfx_fm_c_play_fixed(uint8_t voice, const uint8_t *patch, const uint8_t *sequence, uint8_t priority)
{
    return zfx_fm_start_voice(voice, patch, sequence, priority);
}

int zfx_fm_c_play_any_cached(const uint8_t *patch, const uint8_t *sequence, uint8_t priority)
{
    int voice;

    if (sequence == NULL) {
        return -1;
    }

    voice = zfx_fm_find_dynamic_voice(priority);
    if (voice < 0) {
        return -1;
    }

    if ((g_zfx_fm.active_mask & (1u << voice)) != 0u) {
        zfx_fm_c_stop((uint8_t)voice);
    }

    if (zfx_fm_start_voice((uint8_t)voice, patch, sequence, priority) != 0) {
        return -1;
    }

    return voice;
}

void zfx_fm_c_bind_patch(uint8_t voice, const uint8_t *patch)
{
    if (voice >= 8u || patch == NULL) {
        return;
    }

    if ((g_zfx_fm.allowed_mask & (1u << voice)) == 0u) {
        return;
    }

    if (g_zfx_fm.patch[voice] == patch) {
        return;
    }

    zsm_patch_ym_voice(voice, patch);
    g_zfx_fm.patch[voice] = patch;
}

void zfx_fm_c_stop(uint8_t voice)
{
    if (voice >= 8u) {
        return;
    }

    g_zfx_fm.active_mask &= (uint8_t)~(1u << voice);
    g_zfx_fm.delay[voice] = 0u;
    g_zfx_fm.priority[voice] = 0u;
    ym2151_writereg(0x08u, voice);
}

int zfx_fm_c_is_active(uint8_t voice)
{
    if (voice >= 8u) {
        return 0;
    }

    return (g_zfx_fm.active_mask & (1u << voice)) != 0u;
}

const uint8_t *zfx_fm_c_get_patch(uint8_t voice)
{
    if (voice >= 8u) {
        return NULL;
    }

    return g_zfx_fm.patch[voice];
}

uint8_t zfx_fm_c_get_priority(uint8_t voice)
{
    if (voice >= 8u) {
        return 0u;
    }

    return g_zfx_fm.priority[voice];
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ym2151.h"
#include "zsmplayer68k.h"

#define ZSM_HDR_SIZE 16u
#define ZSM_EOF      0x80u

typedef struct {
    const uint8_t *song_base;
    const uint8_t *data_ptr;
    const uint8_t *loop_ptr;
    uint16_t tick_rate;
    uint16_t delay;
    uint16_t saved_delay;
    uint16_t loop_count;
    uint16_t loop_defined;
    uint16_t loop_enabled;
    uint16_t fm_mask;
    uint16_t frame_int;
    uint16_t frame_frac;
    uint16_t frame_accum;
    uint8_t rlfbcon_shadow[8];
} zsm_state_t;

typedef struct {
    const void *data;
    uint32_t length;
    uint16_t sample_rate;
    uint16_t format;
    uint16_t active;
    const uint8_t *last_ext_ptr;
    uint16_t last_ext_len;
    uint16_t pending;
} zsm_pcm_state_t;

static zsm_state_t g_zsm;
static zsm_pcm_state_t g_zsm_pcm;

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le24(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static void calc_60hz_rate(void)
{
    uint32_t rate = g_zsm.tick_rate != 0u ? g_zsm.tick_rate : 60u;
    g_zsm.frame_int = (uint16_t)(rate / 60u);
    g_zsm.frame_frac = (uint16_t)(((rate % 60u) << 16) / 60u);
    g_zsm.frame_accum = 0u;
}

static void zsm_pcm_on_extcmd0(const uint8_t *ptr, uint16_t len)
{
    g_zsm_pcm.last_ext_ptr = ptr;
    g_zsm_pcm.last_ext_len = len;
    g_zsm_pcm.pending = 1u;
}

void zsm_c_pcm_init(void)
{
    memset(&g_zsm_pcm, 0, sizeof(g_zsm_pcm));
}

int zsm_c_pcm_play(const void *data, uint32_t length, uint16_t sample_rate, uint16_t format)
{
    g_zsm_pcm.data = data;
    g_zsm_pcm.length = length;
    g_zsm_pcm.sample_rate = sample_rate;
    g_zsm_pcm.format = format;
    g_zsm_pcm.active = 1u;
    return 0;
}

void zsm_c_pcm_stop(void)
{
    g_zsm_pcm.data = NULL;
    g_zsm_pcm.length = 0u;
    g_zsm_pcm.sample_rate = 0u;
    g_zsm_pcm.format = 0u;
    g_zsm_pcm.active = 0u;
}

int zsm_c_pcm_is_active(void)
{
    return g_zsm_pcm.active != 0u;
}

void zsm_c_pcm_service(void)
{
}

void zsm_c_init_player(void)
{
    memset(&g_zsm, 0, sizeof(g_zsm));
    zsm_c_pcm_init();
}

void zsm_c_patch_ym_voice(uint8_t voice, const uint8_t *patch)
{
    uint8_t reg = (uint8_t)(0x20u + voice);
    ym2151_writereg(reg, *patch++);

    reg = (uint8_t)(0x38u + voice);
    ym2151_writereg(reg, *patch++);

    reg = (uint8_t)(0x40u + voice);
    for (int i = 0; i < 24; ++i) {
        ym2151_writereg(reg, *patch++);
        reg = (uint8_t)(reg + 8u);
    }
}

int zsm_c_start_music(const void *zsm)
{
    const uint8_t *base = (const uint8_t *)zsm;
    uint32_t loop_offset;

    zsm_c_stop_music();

    if (base == NULL || base[0] != 'z' || base[1] != 'm') {
        zsm_c_init_player();
        return -1;
    }

    if (base[2] < 1u || base[2] > 1u) {
        zsm_c_init_player();
        return -1;
    }

    g_zsm.song_base = base;
    g_zsm.fm_mask = base[9];

    loop_offset = read_le24(base + 3);
    if (loop_offset != 0u) {
        g_zsm.loop_ptr = base + loop_offset;
        g_zsm.loop_defined = 1u;
        g_zsm.loop_enabled = 1u;
    } else {
        g_zsm.loop_ptr = base + ZSM_HDR_SIZE;
        g_zsm.loop_defined = 0u;
        g_zsm.loop_enabled = 0u;
    }

    g_zsm.loop_count = 0u;
    g_zsm.tick_rate = read_le16(base + 12);
    if (g_zsm.tick_rate == 0u) {
        g_zsm.tick_rate = 60u;
    }
    calc_60hz_rate();

    g_zsm.data_ptr = base + ZSM_HDR_SIZE;
    g_zsm.delay = 1u;
    g_zsm.saved_delay = 0u;
    return 0;
}

void zsm_c_stop_music(void)
{
    zsm_c_pause_music();
    g_zsm.song_base = NULL;
    g_zsm.data_ptr = NULL;
    g_zsm.loop_ptr = NULL;
    g_zsm.tick_rate = 0u;
    g_zsm.delay = 0u;
    g_zsm.saved_delay = 0u;
    g_zsm.loop_count = 0u;
    g_zsm.loop_defined = 0u;
    g_zsm.loop_enabled = 0u;
    g_zsm.fm_mask = 0u;
    g_zsm.frame_int = 0u;
    g_zsm.frame_frac = 0u;
    g_zsm.frame_accum = 0u;
    zsm_c_pcm_stop();
}

void zsm_c_pause_music(void)
{
    g_zsm.saved_delay = g_zsm.delay;
    g_zsm.delay = 0u;

    for (uint8_t voice = 0; voice < 8u; ++voice) {
        if ((g_zsm.fm_mask & (1u << voice)) == 0u) {
            continue;
        }
        ym2151_writereg(0x08u, voice);
        ym2151_writereg((uint8_t)(0x20u + voice), 0x00u);
    }
}

void zsm_c_resume_music(void)
{
    if (g_zsm.saved_delay == 0u) {
        return;
    }

    g_zsm.delay = g_zsm.saved_delay;
    g_zsm.saved_delay = 0u;

    for (uint8_t voice = 0; voice < 8u; ++voice) {
        if ((g_zsm.fm_mask & (1u << voice)) == 0u) {
            continue;
        }
        ym2151_writereg((uint8_t)(0x20u + voice), g_zsm.rlfbcon_shadow[voice]);
    }
}

void zsm_c_force_loop(uint8_t repeats)
{
    g_zsm.loop_count = repeats;
    g_zsm.loop_enabled = 1u;
}

void zsm_c_set_loop(uint8_t repeats)
{
    if (g_zsm.loop_defined != 0u) {
        g_zsm.loop_count = repeats;
    }
}

void zsm_c_disable_loop(void)
{
    g_zsm.loop_enabled = 0u;
}

uint16_t zsm_c_get_tick_rate(void)
{
    return g_zsm.tick_rate;
}

int zsm_c_step_music(void)
{
    if (g_zsm.delay == 0u || g_zsm.data_ptr == NULL) {
        return 1;
    }

    if (--g_zsm.delay != 0u) {
        return 0;
    }

    for (;;) {
        uint8_t cmd = *g_zsm.data_ptr++;

        if (cmd == ZSM_EOF) {
            if (g_zsm.loop_enabled == 0u || g_zsm.loop_ptr == NULL) {
                zsm_c_stop_music();
                return 1;
            }

            if (g_zsm.loop_count != 0u) {
                --g_zsm.loop_count;
                if (g_zsm.loop_count == 0u) {
                    g_zsm.loop_enabled = 0u;
                    zsm_c_stop_music();
                    return 1;
                }
            }

            g_zsm.data_ptr = g_zsm.loop_ptr;
            continue;
        }

        if ((cmd & 0x80u) != 0u) {
            g_zsm.delay = (uint16_t)(cmd & 0x7fu);
            return 0;
        }

        if ((cmd & 0x40u) == 0u) {
            g_zsm.data_ptr += 1;
            continue;
        }

        if ((cmd & 0x3fu) == 0u) {
            uint8_t ext = *g_zsm.data_ptr++;
            uint8_t len = ext & 0x3fu;
            uint8_t channel = ext >> 6;
            if (channel == 0u) {
                zsm_pcm_on_extcmd0(g_zsm.data_ptr, len);
            }
            g_zsm.data_ptr += len;
            continue;
        }

        for (uint8_t count = (uint8_t)(cmd & 0x3fu); count != 0u; --count) {
            uint8_t reg = *g_zsm.data_ptr++;
            uint8_t value = *g_zsm.data_ptr++;
            ym2151_writereg(reg, value);

            if (reg >= 0x20u && reg <= 0x27u) {
                g_zsm.rlfbcon_shadow[reg & 7u] = value;
            }
        }
    }
}

void zsm_c_play_frame_60hz(void)
{
    uint16_t steps = g_zsm.frame_int;
    g_zsm.frame_accum = (uint16_t)(g_zsm.frame_accum + g_zsm.frame_frac);
    if (g_zsm.frame_accum < g_zsm.frame_frac) {
        ++steps;
    }

    while (steps-- != 0u) {
        zsm_c_step_music();
    }
}


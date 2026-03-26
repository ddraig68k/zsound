#ifndef ZSMPLAYER68K_H
#define ZSMPLAYER68K_H

#include <stdint.h>

void zsm_init_player(void);
int zsm_start_music(const void *zsm);
int zsm_step_music(void);
void zsm_play_frame_60hz(void);
void zsm_stop_music(void);
void zsm_pause_music(void);
void zsm_resume_music(void);
void zsm_force_loop(uint8_t repeats);
void zsm_set_loop(uint8_t repeats);
void zsm_disable_loop(void);
uint16_t zsm_get_tick_rate(void);
void zsm_set_fm_channel_mask(uint8_t mask);
uint8_t zsm_get_fm_channel_mask(void);
void zsm_patch_ym_voice(uint8_t voice, const uint8_t *patch);
void zsm_pcm_init(void);
int zsm_pcm_play(const void *data, uint32_t length, uint16_t sample_rate, uint16_t format);
void zsm_pcm_stop(void);
int zsm_pcm_is_active(void);
void zsm_pcm_service(void);

extern const uint8_t ymp_ding[26];
extern const uint8_t zfx_ding[21];

#endif

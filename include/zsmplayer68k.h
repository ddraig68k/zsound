#ifndef ZSMPLAYER68K_H
#define ZSMPLAYER68K_H

#include <stdint.h>

void zsm_c_init_player(void);
int zsm_c_start_music(const void *zsm);
int zsm_c_step_music(void);
void zsm_c_play_frame_60hz(void);
void zsm_c_stop_music(void);
void zsm_c_pause_music(void);
void zsm_c_resume_music(void);
void zsm_c_force_loop(uint8_t repeats);
void zsm_c_set_loop(uint8_t repeats);
void zsm_c_disable_loop(void);
uint16_t zsm_c_get_tick_rate(void);
void zsm_c_patch_ym_voice(uint8_t voice, const uint8_t *patch);
void zsm_c_pcm_init(void);
int zsm_c_pcm_play(const void *data, uint32_t length, uint16_t sample_rate, uint16_t format);
void zsm_c_pcm_stop(void);
int zsm_c_pcm_is_active(void);
void zsm_c_pcm_service(void);

extern const uint8_t ymp_ding[26];
extern const uint8_t zfx_ding[21];

#endif

#ifndef ZFXFM68K_H
#define ZFXFM68K_H

#include <stdint.h>

void zfx_fm_c_init(void);
void zfx_fm_c_set_allowed_mask(uint8_t mask);
uint8_t zfx_fm_c_get_allowed_mask(void);
void zfx_fm_c_set_reserved_mask(uint8_t mask);
uint8_t zfx_fm_c_get_reserved_mask(void);
int zfx_fm_c_play(uint8_t voice, const uint8_t *sequence);
int zfx_fm_c_play_any(const uint8_t *sequence);
int zfx_fm_c_play_fixed(uint8_t voice, const uint8_t *patch, const uint8_t *sequence, uint8_t priority);
int zfx_fm_c_play_any_cached(const uint8_t *patch, const uint8_t *sequence, uint8_t priority);
void zfx_fm_c_bind_patch(uint8_t voice, const uint8_t *patch);
void zfx_fm_c_update(void);
void zfx_fm_c_stop(uint8_t voice);
int zfx_fm_c_is_active(uint8_t voice);
const uint8_t *zfx_fm_c_get_patch(uint8_t voice);
uint8_t zfx_fm_c_get_priority(uint8_t voice);

#endif

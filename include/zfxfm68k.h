#ifndef ZFXFM68K_H
#define ZFXFM68K_H

#include <stdint.h>

void zfx_fm_c_init(void);
int zfx_fm_c_play(uint8_t voice, const uint8_t *sequence);
void zfx_fm_c_update(void);
void zfx_fm_c_stop(uint8_t voice);
int zfx_fm_c_is_active(uint8_t voice);

#endif

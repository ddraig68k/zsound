YM2151 68000 hardware smoke test
================================

Files:

- `main.c`: simple C-side bring-up program
- `../../src/zsmplayer68k_gcc.S`: GCC/GAS YM-only ZSM core
- `../../src/zsmplayer68k_c_wrappers.S`: C ABI wrappers
- `../../src/ym2151_ding68k.S`: YM patch and ding test sequence

What this test does:

1. Initializes the player state
2. Loads the `ymp_ding` patch into YM voice 0
3. Plays the `zfx_ding` sequence directly from C
4. Enters a loop that calls `zsm_c_play_frame_60hz()`

Why this is useful:

- If you hear the ding, your YM port addresses and basic write timing are very likely correct.
- Once that works, the next layer to verify is linked-in ZSM playback.

Notes:

- `board_wait_frame()` is just a crude spin delay placeholder. Replace it with your board's real frame wait or timer tick.
- For proper music playback in a game, call `zsm_c_play_frame_60hz()` from your VBL / periodic interrupt instead of a busy loop.
- The C test writes YM registers directly for the ding sequence so you can separate "YM hardware path works" from "ZSM stream player works".

Next suggested test:

1. Link a known-good YM-only ZSM as a binary blob
2. Call `zsm_c_start_music(song_ptr)`
3. Call `zsm_c_play_frame_60hz()` from your interrupt
4. If the music is unstable, calibrate the timer and verify the YM busy/status behaviour on your hardware

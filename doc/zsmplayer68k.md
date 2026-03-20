# 68000 YM2151 ZSM player

`src/zsmplayer68k.asm` is a 68000-oriented port of the X16 `zsmplayer.asm` core with the non-YM parts stripped out for game use.

`src/zsmplayer68k_gcc.S` is a GNU `as` / GCC-oriented variant of the same player.
`src/zsmplayer68k_c_wrappers.S` adds thin C ABI wrappers for GCC-built C code.
`src/zfxfm68k_gcc.S` is a lightweight FM-sequence helper for ZFX-style triplet data.

What it currently does:

- Parses the standard 16-byte ZSM header
- Plays YM2151 register/value streams from ZSM
- Skips PSG writes
- Skips EXTCMD blocks
- Supports pause, resume, stop, and loop handling
- Provides a `zsm_play_frame_60hz` helper if you want to drive playback from a 60 Hz interrupt
- Exposes placeholder PCM entry points so the future MSM6258 API shape is already in place

What it intentionally does not do yet:

- Real PCM / MSM6258 playback
- EXTCMD callbacks

## Suggested usage

If your machine already has a timer that can fire at the song tick rate, use the lightest path:

1. Call `zsm_init_player`
2. Call `zsm_start_music` with `A0` pointing at the ZSM header in memory
3. Call `zsm_step_music` once per song tick from your ISR

If you only have a 60 Hz frame interrupt, call `zsm_play_frame_60hz` once per frame instead.

## Important hardware assumptions

The source has placeholder YM2151 port addresses at the top of the file:

- `YM2151_REG_PORT`
- `YM2151_DATA_PORT`
- `YM2151_STATUS_PORT`

You should replace those with the real addresses for your board.

The wait loop assumes YM2151 busy status is bit 7 of the status port, matching normal YM2151 behaviour.

## GCC note

If you are calling from assembly, use the core entry points directly.

If you are calling from C, use the wrapper layer declared in `include/zsmplayer68k.h`:

- `zsm_c_init_player()`
- `zsm_c_start_music()`
- `zsm_c_play_frame_60hz()`
- `zsm_c_patch_ym_voice()`
- `zsm_c_pcm_play()`

## YM test assets

`src/ym2151_ding68k.S` contains the `ding.inc` YM test data in GNU assembler form:

- `ymp_ding`: 26-byte YMP patch
- `zfx_ding`: the short ding sequence

`src/zsmplayer68k_gcc.S` also exports `zsm_patch_ym_voice`, and the C wrapper exposes that as `zsm_c_patch_ym_voice()`.

## FM sequence helper

For simple one-shot YM sound effects, the repo now also includes:

- `src/zfxfm68k_gcc.S`
- `src/zfxfm68k_c_wrappers.S`
- `include/zfxfm68k.h`

This helper plays the same `reg, value, delay` FM triplet format used by `zfxdemo`'s `ding.inc`, with per-voice register translation handled at playback time.

## PCM placeholder

The GCC player now also exports placeholder PCM routines:

- `zsm_pcm_init`
- `zsm_pcm_play`
- `zsm_pcm_stop`
- `zsm_pcm_is_active`
- `zsm_pcm_service`

The C wrapper exposes those as `zsm_c_pcm_*`.

At the moment they only store request state and mark EXTCMD channel 0 payloads for later handling. No MSM6258 hardware writes are performed yet.

## C hardware test

`examples/ym2151test/main.c` is a small smoke test for real hardware bring-up:

- it loads the ding patch
- plays the ding sequence directly with raw YM writes
- then enters a loop that can drive the ZSM player once you link in a song blob

## Notes on ZSM parsing

ZSM stores header fields as little-endian values. The 68000 is big-endian, so the port reads those fields byte-by-byte instead of doing direct word loads.

On EOF:

- if the ZSM defines a loop point, looping is enabled by default
- if no loop point exists, playback stops

That mirrors the behaviour of the reference player closely enough to be useful in a game ISR without bringing over the X16-specific machinery.

/* C ABI wrappers for the register-based 68000 ZSM/YM player core. */

        .globl  zsm_c_init_player
        .globl  zsm_c_start_music
        .globl  zsm_c_step_music
        .globl  zsm_c_play_frame_60hz
        .globl  zsm_c_stop_music
        .globl  zsm_c_pause_music
        .globl  zsm_c_resume_music
        .globl  zsm_c_force_loop
        .globl  zsm_c_set_loop
        .globl  zsm_c_disable_loop
        .globl  zsm_c_get_tick_rate
        .globl  zsm_c_patch_ym_voice
        .globl  zsm_c_pcm_init
        .globl  zsm_c_pcm_play
        .globl  zsm_c_pcm_stop
        .globl  zsm_c_pcm_is_active
        .globl  zsm_c_pcm_service

        .globl  zsm_init_player
        .globl  zsm_start_music
        .globl  zsm_step_music
        .globl  zsm_play_frame_60hz
        .globl  zsm_stop_music
        .globl  zsm_pause_music
        .globl  zsm_resume_music
        .globl  zsm_force_loop
        .globl  zsm_set_loop
        .globl  zsm_disable_loop
        .globl  zsm_get_tick_rate
        .globl  zsm_patch_ym_voice
        .globl  zsm_pcm_init
        .globl  zsm_pcm_play
        .globl  zsm_pcm_stop
        .globl  zsm_pcm_is_active
        .globl  zsm_pcm_service

        .text

zsm_c_init_player:
        jmp     zsm_init_player

zsm_c_start_music:
        movea.l 4(sp),a0
        jmp     zsm_start_music

zsm_c_step_music:
        jmp     zsm_step_music

zsm_c_play_frame_60hz:
        jmp     zsm_play_frame_60hz

zsm_c_stop_music:
        jmp     zsm_stop_music

zsm_c_pause_music:
        jmp     zsm_pause_music

zsm_c_resume_music:
        jmp     zsm_resume_music

zsm_c_force_loop:
        move.l  4(sp),d0
        jmp     zsm_force_loop

zsm_c_set_loop:
        move.l  4(sp),d0
        jmp     zsm_set_loop

zsm_c_disable_loop:
        jmp     zsm_disable_loop

zsm_c_get_tick_rate:
        jmp     zsm_get_tick_rate

zsm_c_patch_ym_voice:
        move.l  4(sp),d0
        movea.l 8(sp),a0
        jmp     zsm_patch_ym_voice

zsm_c_pcm_init:
        jmp     zsm_pcm_init

zsm_c_pcm_play:
        movea.l 4(sp),a0
        move.l  8(sp),d0
        move.l  12(sp),d1
        move.l  16(sp),d2
        jmp     zsm_pcm_play

zsm_c_pcm_stop:
        jmp     zsm_pcm_stop

zsm_c_pcm_is_active:
        jmp     zsm_pcm_is_active

zsm_c_pcm_service:
        jmp     zsm_pcm_service

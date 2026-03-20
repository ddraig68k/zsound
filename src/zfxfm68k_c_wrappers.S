/* C ABI wrappers for the FM sequence helper. */

        .globl  zfx_fm_c_init
        .globl  zfx_fm_c_play
        .globl  zfx_fm_c_update
        .globl  zfx_fm_c_stop
        .globl  zfx_fm_c_is_active

        .globl  zfx_fm_init
        .globl  zfx_fm_play
        .globl  zfx_fm_update
        .globl  zfx_fm_stop
        .globl  zfx_fm_is_active

        .text

zfx_fm_c_init:
        jmp     zfx_fm_init

zfx_fm_c_play:
        move.l  4(sp),d0
        movea.l 8(sp),a0
        jmp     zfx_fm_play

zfx_fm_c_update:
        jmp     zfx_fm_update

zfx_fm_c_stop:
        move.l  4(sp),d0
        jmp     zfx_fm_stop

zfx_fm_c_is_active:
        move.l  4(sp),d0
        jmp     zfx_fm_is_active


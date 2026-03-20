/* Lightweight YM2151 FM sequence helper for 68000 / GCC.
 *
 * Sequence format matches the FM example in ding.inc:
 *   reg, value, delay
 *   ...
 *   reg, value, 0xff   ; end of sequence
 *
 * Registers that are voice-relative ($20-$ff except global $08 handling) are
 * adjusted for the selected voice at playback time.
 */

        .globl  zfx_fm_init
        .globl  zfx_fm_play
        .globl  zfx_fm_update
        .globl  zfx_fm_stop
        .globl  zfx_fm_is_active
        .globl  g_ym2151_base_addr

        .text

zfx_fm_init:
        clr.w   zfx_fm_active_mask
        lea     zfx_fm_delay,a0
        moveq   #7,d0
zfx_fm_init_clear_delay:
        clr.w   (a0)+
        dbra    d0,zfx_fm_init_clear_delay

        lea     zfx_fm_ptr,a0
        moveq   #7,d0
zfx_fm_init_clear_ptr:
        clr.l   (a0)+
        dbra    d0,zfx_fm_init_clear_ptr
        rts

/* int zfx_fm_play(unsigned voice, const uint8_t *seq) */
zfx_fm_play:
        cmpi.l  #7,d0
        bhi     zfx_fm_play_fail

        move.w  d0,d3
        add.w   d3,d3
        lea     zfx_fm_ptr,a1
        add.w   d3,d3
        move.l  a0,0(a1,d3.w)

        move.w  d0,d3
        add.w   d3,d3
        lea     zfx_fm_delay,a1
        move.w  #1,0(a1,d3.w)

        bset    d0,zfx_fm_active_mask
        moveq   #0,d0
        rts

zfx_fm_play_fail:
        moveq   #-1,d0
        rts

zfx_fm_update:
        movem.l d0-d7/a0-a1,-(sp)
        moveq   #0,d7

zfx_fm_update_voice_loop:
        cmpi.w  #8,d7
        beq     zfx_fm_update_done

        btst    d7,zfx_fm_active_mask
        beq     zfx_fm_update_next_voice

        move.w  d7,d6
        add.w   d6,d6
        lea     zfx_fm_delay,a1
        subq.w  #1,0(a1,d6.w)
        bgt     zfx_fm_update_next_voice

        add.w   d6,d6
        lea     zfx_fm_ptr,a1
        movea.l 0(a1,d6.w),a0
        bsr     zfx_fm_process_voice

zfx_fm_update_next_voice:
        addq.w  #1,d7
        bra     zfx_fm_update_voice_loop

zfx_fm_update_done:
        movem.l (sp)+,d0-d7/a0-a1
        rts

/* void zfx_fm_stop(unsigned voice) */
zfx_fm_stop:
        cmpi.l  #7,d0
        bhi     zfx_fm_stop_done
        bclr    d0,zfx_fm_active_mask
        move.w  d0,d3
        add.w   d3,d3
        lea     zfx_fm_delay,a1
        clr.w   0(a1,d3.w)
        move.b  d0,d1
        moveq   #8,d0
        bsr     zfx_fm_ym_write
zfx_fm_stop_done:
        rts

/* int zfx_fm_is_active(unsigned voice) */
zfx_fm_is_active:
        cmpi.l  #7,d0
        bhi     zfx_fm_is_active_inactive
        btst    d0,zfx_fm_active_mask
        beq     zfx_fm_is_active_inactive
        moveq   #1,d0
        rts
zfx_fm_is_active_inactive:
        moveq   #0,d0
        rts

zfx_fm_process_voice:
zfx_fm_process_voice_loop:
        moveq   #0,d0
        moveq   #0,d1
        moveq   #0,d2
        move.b  (a0)+,d0
        move.b  (a0)+,d1
        move.b  (a0)+,d2

        cmpi.b  #0x08,d0
        beq     zfx_fm_process_voice_key_reg
        cmpi.b  #0x20,d0
        blo     zfx_fm_process_voice_write_reg
        andi.b  #0xf8,d0
        or.b    d7,d0
        bra     zfx_fm_process_voice_write_reg

zfx_fm_process_voice_key_reg:
        andi.b  #0xf8,d1
        or.b    d7,d1

zfx_fm_process_voice_write_reg:
        bsr     zfx_fm_ym_write

        cmpi.b  #0xff,d2
        beq     zfx_fm_process_voice_finish
        tst.b   d2
        beq     zfx_fm_process_voice_loop

        move.w  d7,d6
        add.w   d6,d6
        lea     zfx_fm_ptr,a1
        add.w   d6,d6
        move.l  a0,0(a1,d6.w)
        move.w  d7,d6
        add.w   d6,d6
        lea     zfx_fm_delay,a1
        move.w  d2,0(a1,d6.w)
        rts

zfx_fm_process_voice_finish:
        bclr    d7,zfx_fm_active_mask
        move.w  d7,d6
        add.w   d6,d6
        lea     zfx_fm_delay,a1
        clr.w   0(a1,d6.w)
        rts

zfx_fm_ym_wait:
zfx_fm_ym_wait_loop:
        movea.l g_ym2151_base_addr,a1
        moveq   #0,d2
        move.b  2(a1),d2
        btst    #7,d2
        bne     zfx_fm_ym_wait_loop
        rts

zfx_fm_ym_write:
        bsr     zfx_fm_ym_wait
        movea.l g_ym2151_base_addr,a1
        move.b  d0,(a1)
        bsr     zfx_fm_ym_wait
        movea.l g_ym2151_base_addr,a1
        move.b  d1,2(a1)
        rts

        .bss
        .align  2

zfx_fm_active_mask:    .space 2
zfx_fm_delay:          .space 16
zfx_fm_ptr:            .space 32

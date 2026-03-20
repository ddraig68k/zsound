        cpu 68000

; ---------------------------------------------------------------------------
; ZSM YM2151 player for 68000 systems
;
; This is a lightweight 68000 port of the Commander X16 ZSM player core,
; trimmed down to YM2151 playback only:
;   - PSG stream commands are skipped
;   - EXTCMD blocks are skipped
;   - PCM support is intentionally not implemented yet
;
; The code is written in a plain Motorola-style syntax and is intended as a
; practical starting point. You will likely need small syntax adjustments for
; your chosen assembler.
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Hardware configuration
; Replace these with the correct addresses for your machine.
; ---------------------------------------------------------------------------
YM2151_REG_PORT         equ     $00B00000
YM2151_DATA_PORT        equ     $00B00002
YM2151_STATUS_PORT      equ     $00B00002
YM2151_BUSY_MASK        equ     $80

; ---------------------------------------------------------------------------
; ZSM constants
; ---------------------------------------------------------------------------
ZSM_HDR_SIZE            equ     16
ZSM_EOF                 equ     $80
ZSM_MINVER              equ     1
ZSM_MAXVER              equ     1

; ---------------------------------------------------------------------------
; Public symbols
; ---------------------------------------------------------------------------
        xdef    zsm_init_player
        xdef    zsm_start_music
        xdef    zsm_step_music
        xdef    zsm_play_frame_60hz
        xdef    zsm_stop_music
        xdef    zsm_pause_music
        xdef    zsm_resume_music
        xdef    zsm_force_loop
        xdef    zsm_set_loop
        xdef    zsm_disable_loop
        xdef    zsm_get_tick_rate

; ---------------------------------------------------------------------------
; Calling convention
;
; zsm_start_music:
;   in : A0 = pointer to start of ZSM header in memory
;   out: D0 = 0 on success, -1 on failure
;
; zsm_step_music:
;   in : none
;   out: D0 = 0 while active, 1 if stopped/idle
;
; zsm_play_frame_60hz:
;   call once per 60 Hz frame if you do not have a dedicated song timer.
;
; zsm_force_loop / zsm_set_loop:
;   in : D0.b = repeat count, 0 means infinite
; ---------------------------------------------------------------------------

        section .text

zsm_init_player:
        clr.l   zsm_song_base
        clr.l   zsm_data_ptr
        clr.l   zsm_loop_ptr
        clr.w   zsm_tick_rate
        clr.w   zsm_delay
        clr.w   zsm_saved_delay
        clr.w   zsm_loop_count
        clr.w   zsm_loop_defined
        clr.w   zsm_loop_enabled
        clr.w   zsm_fm_mask
        clr.w   zsm_frame_int
        clr.w   zsm_frame_frac
        clr.w   zsm_frame_accum

        lea     zsm_rlfbcon_shadow,a0
        moveq   #7,d0
.clear_shadow:
        clr.b   (a0)+
        dbra    d0,.clear_shadow

        rts

zsm_start_music:
        movem.l d1-d7/a1-a2,-(sp)

        bsr     zsm_stop_music

        move.l  a0,zsm_song_base

        cmpi.b  #'z',(a0)
        bne.s   .fail
        cmpi.b  #'m',1(a0)
        bne.s   .fail

        moveq   #0,d0
        move.b  2(a0),d0
        cmpi.b  #ZSM_MINVER,d0
        blo.s   .fail
        cmpi.b  #ZSM_MAXVER,d0
        bhi.s   .fail

        moveq   #0,d0
        move.b  9(a0),d0
        move.w  d0,zsm_fm_mask

        lea     3(a0),a1
        bsr     zsm_read_le24
        tst.l   d0
        beq.s   .no_loop
        movea.l zsm_song_base,a1
        adda.l  d0,a1
        move.l  a1,zsm_loop_ptr
        move.w  #1,zsm_loop_defined
        move.w  #1,zsm_loop_enabled
        clr.w   zsm_loop_count
        bra.s   .loop_done

.no_loop:
        movea.l zsm_song_base,a1
        adda.w  #ZSM_HDR_SIZE,a1
        move.l  a1,zsm_loop_ptr
        clr.w   zsm_loop_defined
        clr.w   zsm_loop_enabled
        clr.w   zsm_loop_count

.loop_done:
        lea     12(a0),a1
        bsr     zsm_read_le16
        tst.w   d0
        bne.s   .have_rate
        move.w  #60,d0
.have_rate:
        move.w  d0,zsm_tick_rate
        bsr     zsm_calc_60hz_rate

        movea.l zsm_song_base,a1
        adda.w  #ZSM_HDR_SIZE,a1
        move.l  a1,zsm_data_ptr
        move.w  #1,zsm_delay
        clr.w   zsm_saved_delay

        moveq   #0,d0
        bra.s   .done

.fail:
        bsr     zsm_init_player
        moveq   #-1,d0

.done:
        movem.l (sp)+,d1-d7/a1-a2
        rts

zsm_step_music:
        movem.l d1-d7/a0-a2,-(sp)

        tst.w   zsm_delay
        beq     .idle

        subq.w  #1,zsm_delay
        bne     .active

        movea.l zsm_data_ptr,a0

.next_cmd:
        moveq   #0,d0
        move.b  (a0)+,d0

        cmpi.b  #ZSM_EOF,d0
        beq     .handle_eof
        bcc     .delay_cmd

        btst    #6,d0
        bne     .ym_or_ext

        addq.l  #1,a0
        bra     .next_cmd

.ym_or_ext:
        andi.w  #$003F,d0
        beq     .skip_ext

        move.w  d0,d7
        subq.w  #1,d7

.ym_loop:
        moveq   #0,d1
        moveq   #0,d2
        move.b  (a0)+,d1
        move.b  (a0)+,d2
        bsr     zsm_ym_write

        cmpi.b  #$20,d1
        blo.s   .next_ym
        cmpi.b  #$27,d1
        bhi.s   .next_ym

        lea     zsm_rlfbcon_shadow,a1
        andi.w  #7,d1
        move.b  d2,0(a1,d1.w)

.next_ym:
        dbra    d7,.ym_loop
        bra     .next_cmd

.skip_ext:
        moveq   #0,d1
        move.b  (a0)+,d1
        andi.w  #$003F,d1
        adda.w  d1,a0
        bra     .next_cmd

.delay_cmd:
        andi.w  #$007F,d0
        move.w  d0,zsm_delay
        move.l  a0,zsm_data_ptr
        bra     .active

.handle_eof:
        tst.w   zsm_loop_enabled
        beq     .stop_now

        movea.l zsm_loop_ptr,a0
        tst.l   a0
        beq     .stop_now

        tst.w   zsm_loop_count
        beq.s   .do_loop
        subq.w  #1,zsm_loop_count
        beq.s   .stop_after_pass

.do_loop:
        move.l  a0,zsm_data_ptr
        bra     .next_cmd

.stop_after_pass:
        clr.w   zsm_loop_enabled
        bra     .stop_now

.stop_now:
        bsr     zsm_stop_music
.idle:
        moveq   #1,d0
        bra.s   .done

.active:
        moveq   #0,d0

.done:
        movem.l (sp)+,d1-d7/a0-a2
        rts

zsm_play_frame_60hz:
        movem.l d1-d3,-(sp)

        moveq   #0,d3
        move.w  zsm_frame_int,d3

        moveq   #0,d0
        move.w  zsm_frame_frac,d0
        add.w   d0,zsm_frame_accum
        bcc.s   .have_steps
        addq.w  #1,d3

.have_steps:
        tst.w   d3
        beq.s   .done

        subq.w  #1,d3
.step_loop:
        bsr     zsm_step_music
        dbra    d3,.step_loop

.done:
        movem.l (sp)+,d1-d3
        rts

zsm_stop_music:
        movem.l d0-d4/a0,-(sp)

        bsr     zsm_pause_music

        clr.l   zsm_song_base
        clr.l   zsm_data_ptr
        clr.l   zsm_loop_ptr
        clr.w   zsm_tick_rate
        clr.w   zsm_delay
        clr.w   zsm_saved_delay
        clr.w   zsm_loop_count
        clr.w   zsm_loop_defined
        clr.w   zsm_loop_enabled
        clr.w   zsm_fm_mask
        clr.w   zsm_frame_int
        clr.w   zsm_frame_frac
        clr.w   zsm_frame_accum

        movem.l (sp)+,d0-d4/a0
        rts

zsm_pause_music:
        movem.l d0-d4/a0,-(sp)

        move.w  zsm_delay,zsm_saved_delay
        clr.w   zsm_delay

        moveq   #0,d3
        move.b  zsm_fm_mask,d3
        beq.s   .done

        lea     zsm_rlfbcon_shadow,a0
        moveq   #0,d4

.pause_loop:
        btst    d4,d3
        beq.s   .next_voice

        moveq   #8,d0
        move.b  d4,d1
        bsr     zsm_ym_write

        moveq   #$20,d0
        add.b   d4,d0
        moveq   #0,d1
        bsr     zsm_ym_write

.next_voice:
        addq.w  #1,d4
        cmpi.w  #8,d4
        bne.s   .pause_loop

.done:
        movem.l (sp)+,d0-d4/a0
        rts

zsm_resume_music:
        movem.l d0-d4/a0,-(sp)

        tst.w   zsm_saved_delay
        beq.s   .done

        move.w  zsm_saved_delay,zsm_delay
        clr.w   zsm_saved_delay

        moveq   #0,d3
        move.b  zsm_fm_mask,d3
        lea     zsm_rlfbcon_shadow,a0
        moveq   #0,d4

.resume_loop:
        btst    d4,d3
        beq.s   .next_voice

        moveq   #$20,d0
        add.b   d4,d0
        moveq   #0,d1
        move.b  0(a0,d4.w),d1
        bsr     zsm_ym_write

.next_voice:
        addq.w  #1,d4
        cmpi.w  #8,d4
        bne.s   .resume_loop

.done:
        movem.l (sp)+,d0-d4/a0
        rts

zsm_force_loop:
        move.w  d0,zsm_loop_count
        move.w  #1,zsm_loop_enabled
        rts

zsm_set_loop:
        tst.w   zsm_loop_defined
        beq.s   .done
        move.w  d0,zsm_loop_count
.done:
        rts

zsm_disable_loop:
        clr.w   zsm_loop_enabled
        rts

zsm_get_tick_rate:
        moveq   #0,d0
        move.w  zsm_tick_rate,d0
        rts

; ---------------------------------------------------------------------------
; Helpers
; ---------------------------------------------------------------------------

zsm_calc_60hz_rate:
        movem.l d1-d2,-(sp)

        moveq   #0,d1
        move.w  zsm_tick_rate,d1
        move.l  d1,d0
        divu.w  #60,d0
        move.w  d0,zsm_frame_int

        move.l  d0,d2
        clr.w   d2
        divu.w  #60,d2
        move.w  d2,zsm_frame_frac
        clr.w   zsm_frame_accum

        movem.l (sp)+,d1-d2
        rts

zsm_read_le16:
        moveq   #0,d0
        moveq   #0,d1
        move.b  (a1)+,d0
        move.b  (a1)+,d1
        lsl.w   #8,d1
        or.w    d1,d0
        rts

zsm_read_le24:
        moveq   #0,d0
        moveq   #0,d1
        moveq   #0,d2
        move.b  (a1)+,d0
        move.b  (a1)+,d1
        move.b  (a1)+,d2
        lsl.l   #8,d1
        lsl.l   #16,d2
        or.l    d1,d0
        or.l    d2,d0
        rts

zsm_ym_wait:
.wait:
        moveq   #0,d0
        move.b  YM2151_STATUS_PORT,d0
        btst    #7,d0
        bne.s   .wait
        rts

zsm_ym_write:
        movem.l d0-d1,-(sp)
        bsr     zsm_ym_wait
        move.b  d0,YM2151_REG_PORT
        bsr     zsm_ym_wait
        move.b  d1,YM2151_DATA_PORT
        movem.l (sp)+,d0-d1
        rts

        section .bss

zsm_song_base:          ds.l    1
zsm_data_ptr:           ds.l    1
zsm_loop_ptr:           ds.l    1
zsm_tick_rate:          ds.w    1
zsm_delay:              ds.w    1
zsm_saved_delay:        ds.w    1
zsm_loop_count:         ds.w    1
zsm_loop_defined:       ds.w    1
zsm_loop_enabled:       ds.w    1
zsm_fm_mask:            ds.w    1
zsm_frame_int:          ds.w    1
zsm_frame_frac:         ds.w    1
zsm_frame_accum:        ds.w    1
zsm_rlfbcon_shadow:     ds.b    8

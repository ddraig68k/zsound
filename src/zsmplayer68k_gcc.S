/* YM-only ZSM player for 68000 / GCC (GNU as) */

        .equ    ZSM_HDR_SIZE, 16
        .equ    ZSM_EOF,      0x80
        .equ    ZSM_MINVER,   1
        .equ    ZSM_MAXVER,   1

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
        .globl  g_ym2151_base_addr

        .text

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
        bsr     zsm_pcm_init

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
        bne     zsm_start_music_fail
        cmpi.b  #'m',1(a0)
        bne     zsm_start_music_fail

        moveq   #0,d0
        move.b  2(a0),d0
        cmpi.b  #ZSM_MINVER,d0
        blo     zsm_start_music_fail
        cmpi.b  #ZSM_MAXVER,d0
        bhi     zsm_start_music_fail

        moveq   #0,d0
        move.b  9(a0),d0
        move.w  d0,zsm_fm_mask

        lea     3(a0),a1
        bsr     zsm_read_le24
        tst.l   d0
        beq     zsm_start_music_no_loop
        movea.l zsm_song_base,a1
        adda.l  d0,a1
        move.l  a1,zsm_loop_ptr
        move.w  #1,zsm_loop_defined
        move.w  #1,zsm_loop_enabled
        clr.w   zsm_loop_count
        bra     zsm_start_music_loop_done

zsm_start_music_no_loop:
        movea.l zsm_song_base,a1
        adda.w  #ZSM_HDR_SIZE,a1
        move.l  a1,zsm_loop_ptr
        clr.w   zsm_loop_defined
        clr.w   zsm_loop_enabled
        clr.w   zsm_loop_count

zsm_start_music_loop_done:
        lea     12(a0),a1
        bsr     zsm_read_le16
        tst.w   d0
        bne     zsm_start_music_have_rate
        move.w  #60,d0
zsm_start_music_have_rate:
        move.w  d0,zsm_tick_rate
        bsr     zsm_calc_60hz_rate

        movea.l zsm_song_base,a1
        adda.w  #ZSM_HDR_SIZE,a1
        move.l  a1,zsm_data_ptr
        move.w  #1,zsm_delay
        clr.w   zsm_saved_delay
        moveq   #0,d0
        bra     zsm_start_music_done

zsm_start_music_fail:
        bsr     zsm_init_player
        moveq   #-1,d0

zsm_start_music_done:
        movem.l (sp)+,d1-d7/a1-a2
        rts

zsm_step_music:
        movem.l d1-d7/a0-a2,-(sp)
        tst.w   zsm_delay
        beq     zsm_step_music_idle

        subq.w  #1,zsm_delay
        bne     zsm_step_music_active

        movea.l zsm_data_ptr,a0

zsm_step_music_next_cmd:
        moveq   #0,d0
        move.b  (a0)+,d0
        cmpi.b  #ZSM_EOF,d0
        beq     zsm_step_music_handle_eof
        bcc     zsm_step_music_delay_cmd

        btst    #6,d0
        bne     zsm_step_music_ym_or_ext

        addq.l  #1,a0
        bra     zsm_step_music_next_cmd

zsm_step_music_ym_or_ext:
        move.w  d0,d1
        andi.w  #0x003f,d0
        beq     zsm_step_music_handle_extcmd

        move.w  d0,d7
        subq.w  #1,d7

zsm_step_music_ym_loop:
        moveq   #0,d1
        moveq   #0,d2
        move.b  (a0)+,d1
        move.b  (a0)+,d2
        bsr     zsm_ym_write

        cmpi.b  #0x20,d1
        blo     zsm_step_music_next_ym
        cmpi.b  #0x27,d1
        bhi     zsm_step_music_next_ym
        lea     zsm_rlfbcon_shadow,a1
        andi.w  #7,d1
        move.b  d2,0(a1,d1.w)

zsm_step_music_next_ym:
        dbra    d7,zsm_step_music_ym_loop
        bra     zsm_step_music_next_cmd

zsm_step_music_handle_extcmd:
        moveq   #0,d1
        moveq   #0,d2
        move.b  (a0)+,d1
        move.w  d1,d2
        andi.w  #0x003f,d1
        andi.w  #0x00c0,d2
        lsr.w   #6,d2
        cmpi.w  #0,d2
        bne     zsm_step_music_skip_ext_payload
        bsr     zsm_pcm_on_extcmd0
zsm_step_music_skip_ext_payload:
        adda.w  d1,a0
        bra     zsm_step_music_next_cmd

zsm_step_music_delay_cmd:
        andi.w  #0x007f,d0
        move.w  d0,zsm_delay
        move.l  a0,zsm_data_ptr
        bra     zsm_step_music_active

zsm_step_music_handle_eof:
        tst.w   zsm_loop_enabled
        beq     zsm_step_music_stop_now
        tst.l   zsm_loop_ptr
        beq     zsm_step_music_stop_now
        movea.l zsm_loop_ptr,a0
        tst.w   zsm_loop_count
        beq     zsm_step_music_do_loop
        subq.w  #1,zsm_loop_count
        beq     zsm_step_music_stop_after_pass

zsm_step_music_do_loop:
        move.l  a0,zsm_data_ptr
        bra     zsm_step_music_next_cmd

zsm_step_music_stop_after_pass:
        clr.w   zsm_loop_enabled

zsm_step_music_stop_now:
        bsr     zsm_stop_music
zsm_step_music_idle:
        moveq   #1,d0
        bra     zsm_step_music_done

zsm_step_music_active:
        moveq   #0,d0

zsm_step_music_done:
        movem.l (sp)+,d1-d7/a0-a2
        rts

zsm_play_frame_60hz:
        movem.l d1-d3,-(sp)
        moveq   #0,d3
        move.w  zsm_frame_int,d3
        moveq   #0,d0
        move.w  zsm_frame_frac,d0
        add.w   d0,zsm_frame_accum
        bcc     zsm_play_frame_have_steps
        addq.w  #1,d3
zsm_play_frame_have_steps:
        tst.w   d3
        beq     zsm_play_frame_done
        subq.w  #1,d3
zsm_play_frame_step_loop:
        bsr     zsm_step_music
        dbra    d3,zsm_play_frame_step_loop
zsm_play_frame_done:
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
        bsr     zsm_pcm_stop
        movem.l (sp)+,d0-d4/a0
        rts

zsm_pause_music:
        movem.l d0-d4/a0,-(sp)
        move.w  zsm_delay,zsm_saved_delay
        clr.w   zsm_delay
        moveq   #0,d3
        move.b  zsm_fm_mask,d3
        beq     zsm_pause_music_done
        moveq   #0,d4
zsm_pause_music_loop:
        btst    d4,d3
        beq     zsm_pause_music_next_voice
        moveq   #8,d0
        move.b  d4,d1
        bsr     zsm_ym_write
        moveq   #0x20,d0
        add.b   d4,d0
        moveq   #0,d1
        bsr     zsm_ym_write
zsm_pause_music_next_voice:
        addq.w  #1,d4
        cmpi.w  #8,d4
        bne     zsm_pause_music_loop
zsm_pause_music_done:
        movem.l (sp)+,d0-d4/a0
        rts

zsm_resume_music:
        movem.l d0-d4/a0,-(sp)
        tst.w   zsm_saved_delay
        beq     zsm_resume_music_done
        move.w  zsm_saved_delay,zsm_delay
        clr.w   zsm_saved_delay
        moveq   #0,d3
        move.b  zsm_fm_mask,d3
        lea     zsm_rlfbcon_shadow,a0
        moveq   #0,d4
zsm_resume_music_loop:
        btst    d4,d3
        beq     zsm_resume_music_next_voice
        moveq   #0x20,d0
        add.b   d4,d0
        moveq   #0,d1
        move.b  0(a0,d4.w),d1
        bsr     zsm_ym_write
zsm_resume_music_next_voice:
        addq.w  #1,d4
        cmpi.w  #8,d4
        bne     zsm_resume_music_loop
zsm_resume_music_done:
        movem.l (sp)+,d0-d4/a0
        rts

zsm_force_loop:
        move.w  d0,zsm_loop_count
        move.w  #1,zsm_loop_enabled
        rts

zsm_set_loop:
        tst.w   zsm_loop_defined
        beq     zsm_set_loop_done
        move.w  d0,zsm_loop_count
zsm_set_loop_done:
        rts

zsm_disable_loop:
        clr.w   zsm_loop_enabled
        rts

zsm_get_tick_rate:
        moveq   #0,d0
        move.w  zsm_tick_rate,d0
        rts

zsm_patch_ym_voice:
        movem.l d1-d4/a1,-(sp)
        movea.l a0,a1
        move.b  d0,d4

        moveq   #0x20,d1
        add.b   d4,d1
        moveq   #0,d2
        move.b  (a1)+,d2
        move.b  d1,d0
        move.b  d2,d1
        bsr     zsm_ym_write

        moveq   #0x38,d1
        add.b   d4,d1
        moveq   #0,d2
        move.b  (a1)+,d2
        move.b  d1,d0
        move.b  d2,d1
        bsr     zsm_ym_write

        moveq   #0x40,d3
        add.b   d4,d3
        moveq   #23,d4
.patch_loop:
        moveq   #0,d2
        move.b  (a1)+,d2
        move.b  d3,d0
        move.b  d2,d1
        bsr     zsm_ym_write
        addq.b  #8,d3
        dbra    d4,.patch_loop

        movem.l (sp)+,d1-d4/a1
        rts

zsm_pcm_init:
        clr.l   zsm_pcm_ptr
        clr.l   zsm_pcm_length
        clr.w   zsm_pcm_rate
        clr.w   zsm_pcm_format
        clr.w   zsm_pcm_active
        clr.l   zsm_pcm_last_ext_ptr
        clr.w   zsm_pcm_last_ext_len
        clr.w   zsm_pcm_pending
        moveq   #0,d0
        rts

zsm_pcm_play:
        move.l  a0,zsm_pcm_ptr
        move.l  d0,zsm_pcm_length
        move.w  d1,zsm_pcm_rate
        move.w  d2,zsm_pcm_format
        move.w  #1,zsm_pcm_active
        moveq   #0,d0
        rts

zsm_pcm_stop:
        clr.w   zsm_pcm_active
        clr.l   zsm_pcm_ptr
        clr.l   zsm_pcm_length
        clr.w   zsm_pcm_rate
        clr.w   zsm_pcm_format
        moveq   #0,d0
        rts

zsm_pcm_is_active:
        moveq   #0,d0
        move.w  zsm_pcm_active,d0
        rts

zsm_pcm_service:
        rts

zsm_pcm_on_extcmd0:
        move.l  a0,zsm_pcm_last_ext_ptr
        move.w  d1,zsm_pcm_last_ext_len
        move.w  #1,zsm_pcm_pending
        rts

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
        swap    d2
        or.l    d1,d0
        or.l    d2,d0
        rts

zsm_ym_wait:
.wait:
        movea.l g_ym2151_base_addr,a1
        moveq   #0,d0
        move.b  2(a1),d0
        btst    #7,d0
        bne.s   .wait
        rts

zsm_ym_write:
        movem.l d0-d1/a1,-(sp)
        bsr     zsm_ym_wait
        movea.l g_ym2151_base_addr,a1
        move.b  d0,(a1)
        bsr     zsm_ym_wait
        movea.l g_ym2151_base_addr,a1
        move.b  d1,2(a1)
        movem.l (sp)+,d0-d1/a1
        rts

        .bss
        .align  2

zsm_song_base:      .space 4
zsm_data_ptr:       .space 4
zsm_loop_ptr:       .space 4
zsm_tick_rate:      .space 2
zsm_delay:          .space 2
zsm_saved_delay:    .space 2
zsm_loop_count:     .space 2
zsm_loop_defined:   .space 2
zsm_loop_enabled:   .space 2
zsm_fm_mask:        .space 2
zsm_frame_int:      .space 2
zsm_frame_frac:     .space 2
zsm_frame_accum:    .space 2
zsm_rlfbcon_shadow: .space 8
zsm_pcm_ptr:        .space 4
zsm_pcm_length:     .space 4
zsm_pcm_rate:       .space 2
zsm_pcm_format:     .space 2
zsm_pcm_active:     .space 2
zsm_pcm_last_ext_ptr: .space 4
zsm_pcm_last_ext_len: .space 2
zsm_pcm_pending:    .space 2

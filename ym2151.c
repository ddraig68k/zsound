#include <stdio.h>
#include <math.h>
#include "ym2151.h"

uint32_t g_ym2151_base_addr = 0;
YM2151 g_ym2151;

void ym2151_set_base_addr(uint32_t addr)
{
    g_ym2151_base_addr = addr;
}

void __attribute__ ((noinline)) ym2151_writereg(uint8_t reg, uint8_t data)
{
    __asm__ volatile
    (
        "move.l	%0, %%a0\n\t"
        "move.b %1, %%d0\n\t"
        "move.b %2, %%d1\n"
        "or.w #0x0700, %%sr\n\t"
        "move.b %%d0,(%%a0)\n\t"
        "move.b %%d1,2(%%a0)\n\t"
        "and.w	#0xF8FF, %%sr\n\t"
        : /* no outputs */
        : "g" (g_ym2151_base_addr), "g" (reg), "g" (data)
        : "%a0", "%d0", "%d1" /* clobbered registers */
	);
}

void ym2151_set_clock(uint32_t freq)
{
    uint16_t oct = 3.322 * log10(freq / 1039);
    uint16_t dac = round(2048 - ((2078 * pow(2, 10 + oct)) / freq));

    uint8_t CNF = 0;
    uint16_t bitmap = (oct << 12) | (dac << 2) | CNF;

    //printf("Calculated clock values: oct = %d, dac = %d, output = %04X\n", oct, dac, bitmap);
    YM2151_WRITEWORD(YM2151_CLOCK, bitmap);

    // Add a small delay to allow frequency to change
    volatile unsigned long wait = 5000;
    while (wait--);
}

// Initialize LFOs
void ym2151_init_lfo()
{
    ym2151_writereg(0x1, 0x1);
}

/* Read the MDX format timbre data and set it in the register. After setting the timbre data,
   be sure to set the volume and panpot to ensure consistency of related registers.
*/
void ym2151_load_timbre(uint8_t ch, uint8_t *prog_addr)
{
    static uint8_t carrier_slot_tbl[] =
    {
        0x08, 0x08, 0x08, 0x08,
        0x0c, 0x0e, 0x0e, 0x0f,
    };

    uint8_t *taddr = prog_addr;
    //uint8_t no = *taddr++; // unused
    taddr++;

    g_ym2151.RegFLCON[ch] = *taddr++;
    g_ym2151.CarrierSlot[ch] = carrier_slot_tbl[g_ym2151.RegFLCON[ch] & 0x7];
    g_ym2151.RegSLOTMASK[ch] = *taddr++;

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t dt1_mul = *taddr++;
        ym2151_writereg(0x40 + ch + i, dt1_mul);
    }

    for (int i = 0; i < 4; i++)
    {
        g_ym2151.RegTL[ch][i] = *taddr++;
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t ks_ar = *taddr++;
        ym2151_writereg(0x80 + ch + i, ks_ar);
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t ame_d1r = *taddr++;
        ym2151_writereg(0xa0 + ch + i, ame_d1r);
    }
    
    for (int i = 0; i < 32; i += 8)
    {
        uint8_t dt2_d2r = *taddr++;
        ym2151_writereg(0xc0 + ch + i, dt2_d2r);
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t d1l_rr = *taddr++;
        ym2151_writereg(0xe0 + ch + i, d1l_rr);
    }
}

/*
    Read a general FM tone color array and set it in the register, after setting the tone data
    Be sure to set the volume and panpot to ensure consistency of related registers.
*/
void ym2151_load_separation_timbre(uint8_t ch, uint8_t *prog_addr)
{
    // D2R (SR) is 5bit for 2151, and DT2 (additional detune) is ignored, so it is slightly unusable
    // D2R is strange when <<1 is used, so leave it as it is
    static uint8_t carrier_slot_tbl[] = {
        0x08, 0x08, 0x08, 0x08,
        0x0c, 0x0e, 0x0e, 0x0f,
    };

    uint8_t *taddr = prog_addr;
    uint8_t con = *taddr++;
    uint8_t fl = *taddr++;

    g_ym2151.RegFLCON[ch] = (fl << 3) | con;
    g_ym2151.CarrierSlot[ch] = carrier_slot_tbl[con];
    g_ym2151.RegSLOTMASK[ch] = 0xf;

    for (int i = 0; i < 4; i++)
    {
        uint8_t slotindex = i * 8 + ch;
        uint8_t ar = *taddr++;
        uint8_t dr = *taddr++;
        uint8_t sr = *taddr++;
        uint8_t rr = *taddr++;
        uint8_t sl = *taddr++;
        uint8_t ol = *taddr++;
        uint8_t ks = *taddr++;
        uint8_t ml = *taddr++;
        uint8_t dt1 = *taddr++;
        uint8_t ams = *taddr++;

        g_ym2151.RegTL[ch][i] = ol;
        ym2151_writereg(0x40 + slotindex, (dt1 << 4) | ml); // DT1 MUL
        ym2151_writereg(0x80 + slotindex, (ks << 6) | ar);  // KS AR
        ym2151_writereg(0xa0 + slotindex, (ams << 7) | dr); // AMS D1R
        ym2151_writereg(0xc0 + slotindex, 0 | (sr));        // DT2 D2R
        ym2151_writereg(0xe0 + slotindex, (sl << 4) | rr);  // D1L RR
    }
}

// Loads MDX format sound data and dumps it to the console
void ym2151_dump_timbre(uint8_t *prog_addr)
{
    uint8_t *taddr = prog_addr;
    uint8_t no = *taddr++;
    uint8_t flcon = *taddr++;
    uint8_t slotmask = *taddr++;

    printf("No: %d\n", no);
    printf("RegFLCON: %d\n", flcon);
    printf("RegSLOTMASK: %d\n", slotmask);

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t dt1_mul = *taddr++;
        printf("DT1_MUL: %d\n", dt1_mul);
    }

    for (int i = 0; i < 4; i++)
    {
        uint8_t tl = *taddr++;
        printf("TL: %d\n", tl);
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t ks_ar = *taddr++;
        printf("KS_AR: %d\n", ks_ar);
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t ame_d1r = *taddr++;
        printf("AME_D1R: %d\n", ame_d1r);
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t dt2_d2r = *taddr++;
        printf("DT2_D2R: %d\n", dt2_d2r);
    }

    for (int i = 0; i < 32; i += 8)
    {
        uint8_t d1l_rr = *taddr++;
        printf("D1L_RR: %d\n", d1l_rr);
    }
}

/*
    Set the volume, just operate the operator's level
    Depending on the combination of timbre and volume, it may fail.
	param ch				Channel to set
	param volume			Volume, 0 (minimum) to 15 (maximum) If the most significant bit (0x80) is On, 0x80 is masked and added as is to the TL value
	param offset			Add upper 8bit to TL value, for MDX playback
 */
void ym2151_set_volume(uint8_t ch, uint8_t volume, uint16_t offset)
{
    // printf("ch %X volume %X offset %X\n", ch, volume, offset);
    static uint8_t volume_tbl[] = {
        0x2a, 0x28, 0x25, 0x22,
        0x20, 0x1d, 0x1a, 0x18,
        0x15, 0x12, 0x10, 0x0d,
        0x0a, 0x08, 0x05, 0x02,
    };

    int16_t tl, att;
    
    if (volume & (0x80))
    {
        tl = volume & 0x7f;
    }
    else
    {
        if (volume > 15)
            printf("Illegal volume.\n");
        tl = volume_tbl[volume];
    }

    tl += offset >> 8;
    // printf("tl %X \n", tl);
    for (int i = 0; i < 4; i++)
    {
        // printf("regtl %X\n", g_ym2151.RegTL[ch][i]);
        if (g_ym2151.CarrierSlot[ch] & (1 << i))
        {
            // printf("1\n");
            att = g_ym2151.RegTL[ch][i] + tl;
        }
        else
        {
            // printf("2\n");
            att = g_ym2151.RegTL[ch][i];
        }

        if (att > 0x7f || att < 0)
            att = 0x7f;
        // printf("YM2151_setVolume\n");
        ym2151_writereg(0x60 + i * 8 + ch, att);
    }
}

void ym2151_note_on(uint8_t ch)
{
    ym2151_writereg(0x08, (g_ym2151.RegSLOTMASK[ch] << 3) + ch);
}

void ym2151_note_off(uint8_t ch)
{
    ym2151_writereg(0x08, 0x00 + ch);
}

const uint8_t KeyCodeTable[] = {
    0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x09,
    0x0a, 0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14,
    0x15, 0x16, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,
    0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x28, 0x29,
    0x2a, 0x2c, 0x2d, 0x2e, 0x30, 0x31, 0x32, 0x34,
    0x35, 0x36, 0x38, 0x39, 0x3a, 0x3c, 0x3d, 0x3e,
    0x40, 0x41, 0x42, 0x44, 0x45, 0x46, 0x48, 0x49,
    0x4a, 0x4c, 0x4d, 0x4e, 0x50, 0x51, 0x52, 0x54,
    0x55, 0x56, 0x58, 0x59, 0x5a, 0x5c, 0x5d, 0x5e,
    0x60, 0x61, 0x62, 0x64, 0x65, 0x66, 0x68, 0x69,
    0x6a, 0x6c, 0x6d, 0x6e, 0x70, 0x71, 0x72, 0x74,
    0x75, 0x76, 0x78, 0x79, 0x7a, 0x7c, 0x7d, 0x7e,
};

void ym2151_set_tone(uint8_t ch, uint8_t keycode, int16_t kf)
{
    int16_t offset_kf = (kf & 0x3f);
    int16_t offset_note = keycode + (kf >> 6);

    if (offset_note < 0)
        offset_note = 0;
    
    if (offset_note > 0xbf)
        offset_note = 0xbf;

    ym2151_writereg(0x30 + ch, offset_kf << 2);
    ym2151_writereg(0x28 + ch, KeyCodeTable[offset_note]);
}

// Pan setting, 0: no output 1: left 2: right 3: both outputs
void ym2151_set_panpot(uint8_t ch, uint8_t pan)
{
    ym2151_writereg(0x20 + ch, (pan << 6) | (g_ym2151.RegFLCON[ch]));
}

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ddraig.h"
#include "duart.h"
#include "pit.h"
#include "doslib.h"
#include "joystick.h"

#include "vgacard.h"
#include "ym2151.h"
#include "zsmplayer68k.h"
#include "zfxfm68k.h"

uint8_t *g_music_data = NULL;
uint32_t g_music_length = 0;
volatile uint32_t frameCount = 0;
volatile uint8_t vblank = 0;

#define MUSIC_FM_MASK 0x0Fu
#define SFX_FM_MASK   0xF0u
#define FOOTSTEP_VOICE 4u
#define DYNAMIC_SFX_RESERVED_MASK (1u << FOOTSTEP_VOICE)
#define PRIORITY_FOOTSTEP 1u
#define PRIORITY_DING     2u

static void audio_service_tick_60hz(void)
{
    zsm_play_frame_60hz();
    zfx_fm_c_update();
}

static void init_sfx_voices(void)
{
    for (uint8_t voice = 0; voice < 8u; ++voice) {
        if ((SFX_FM_MASK & (1u << voice)) != 0u) {
            zfx_fm_c_bind_patch(voice, ymp_ding);
        }
    }
}

static void trigger_test_ding(void)
{
    int voice = zfx_fm_c_play_any_cached(ymp_ding, zfx_ding, PRIORITY_DING);
    if (voice < 0) {
        printf("\nNo SFX voice available\n");
        return;
    }

    printf("\nTriggered ding on YM voice %d\n", voice);
}

static void trigger_test_footstep(void)
{
    if (zfx_fm_c_play_fixed(FOOTSTEP_VOICE, ymp_ding, zfx_ding, PRIORITY_FOOTSTEP) != 0) {
        printf("\nFootstep voice unavailable\n");
        return;
    }

    printf("\nTriggered fixed-voice SFX on YM voice %d\n", FOOTSTEP_VOICE);
}

void __attribute__((interrupt)) interrupt_player()
{
    // has timer elapsed?
    if ((ym2151_readreg(YM2151_DATA) & 0x2) == 0x2)
    {
        YM2151_WRITE(YM2151_WRITE_REG, 0x14);
        YM2151_WRITE(YM2151_DATA, 0x2A);
    }
}

void __attribute__((interrupt)) interrupt_vblank()
{
    frameCount++;
    vblank = 1;

    // Clear the VGA interrupt flag
    VDP_REG_WRITE(REG_STATUS, 0x04);
}

int check_for_installed_cards()
{
    // Needs to scan expansion slots to check for YM2151
    expansion_dev_t *dev = check_card_installed(DEVICE_YM2151);
    if (dev == NULL)
    {
        printf("The Y2151 Audio card if required for this program\n");
        return -1;
    }
    printf("YM2151 card found in slot %d\n", dev->slot_id + 1);
    ym2151_set_base_addr(dev->exp_base_address);
    uint32_t *int_vec = get_expansion_int_vector(dev->slot_id);
    *int_vec = (uint32_t)&interrupt_player;

        printf("Checking if GfxVGA video card is installed\n");
    dev = check_card_installed(DEVICE_GFXFPGA);
    if (dev == NULL)
    {
        printf("GfxVGA not installed\n");
        return -1;
    }
 
    printf("GfxVGA video card found in slot %d\n", dev->slot_id + 1);
    printf("GfxVGA video memory at address 0x%06lX\n", dev->exp_data_address);
    vdp_init(dev->exp_base_address, dev->exp_data_address);
    // Install vblank interrupt
    int_vec = get_expansion_int_vector(dev->slot_id);
    *int_vec = (uint32_t)&interrupt_vblank;

    return 0;
}

void usage(void)
{
    printf("usage: zsm file\n");
}

int main(int argc, char *argv[])
{
    if (check_for_installed_cards() != 0)
        return -1;

    if (argc < 2)
    {
        usage();
        return -1;
    }

    char *filename = NULL;
    init_joysticks();

    for (int i = 1; i < argc; i++)
    {
        if (filename == NULL)
            filename = argv[i];
        else
            printf("File already set to %s\n", filename);
    }

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Unable to open file %s\n", filename);
        return -1;
    }
    g_music_length = 0;
    fseek(file, 0L, SEEK_END);
    g_music_length = ftell(file);
    fseek(file, 0L, SEEK_SET);

    g_music_data = malloc(g_music_length);
    if (g_music_data == NULL)
    {
        printf("Unable to allocate %ld bytes ram for music\n", g_music_length);
        fclose(file);
        return -1;
    }
    fread(g_music_data, 1, g_music_length, file);
    fclose(file);
    printf("Data loaded: %ld bytes into location %ld, ends at %ld\n", g_music_length, (uint32_t)g_music_data, (uint32_t)g_music_data + g_music_length);

    ym2151_set_clock(4000000);
    printf("Set ym2151 clock to 4mhz\n");

    zsm_init_player();
    zsm_pcm_init();
    zfx_fm_c_init();
    zsm_set_fm_channel_mask(MUSIC_FM_MASK);
    zfx_fm_c_set_allowed_mask(SFX_FM_MASK);
    zfx_fm_c_set_reserved_mask(DYNAMIC_SFX_RESERVED_MASK);
    init_sfx_voices();

    VDP_REG_WRITE(REG_INTERRUPT, 0x0001);
    printf("Music YM mask: 0x%02X, SFX YM mask: 0x%02X, reserved SFX mask: 0x%02X\n", zsm_get_fm_channel_mask(), zfx_fm_c_get_allowed_mask(), zfx_fm_c_get_reserved_mask());
    printf("Press 'f' for fixed-voice SFX, 's' for dynamic SFX, 'm' for music, 'q' to quit\n");
    uint8_t play = 1;
    uint8_t playmusic = 0;

    
    while (play)
    {
        if (playmusic)
        {
            if (vblank)
            {
                vblank = 0;
                audio_service_tick_60hz();
            }
        }
        else if (vblank)
        {
            vblank = 0;
            zfx_fm_c_update();
        }

        if (has_char())
        {
            char key = get_char();
            if (key == 'q')
                play = 0;
            else if (key == 'f')
                trigger_test_footstep();
            else if (key == 's')
                trigger_test_ding();
            else if (key == 'm')
            {
                printf("Playing song %s\n", filename);
                zsm_start_music(g_music_data);
                playmusic = 1;
            }
            printf("%c", key);
        }
    }

    zsm_stop_music();

    VDP_REG_WRITE(REG_INTERRUPT, 0x0000);
    // Disable YM2151 interrupt
    ym2151_writereg(0x14, 0x00);

    if (g_music_data)
        free(g_music_data);

    return 0;
}

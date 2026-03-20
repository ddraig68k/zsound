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

#include "ym2151.h"
#include "zsmplayer68k.h"
#include "zfxfm68k.h"

static void spin_delay(volatile uint32_t ticks)
{
    while (ticks-- != 0u) {
    }
}

static void play_test_ding(void)
{
    zsm_c_patch_ym_voice(0, ymp_ding);
    zfx_fm_c_play(0, zfx_ding);

    while (zfx_fm_c_is_active(0)) {
        zfx_fm_c_update();
        spin_delay(50000u);
    }
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

    return 0;
}

void usage(void)
{
    printf("usage: zsm file\n");
}

#define TEST_READ(x)          (*((volatile uint16_t *) 0xD00000 + x))

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

    ym2151_set_clock(4000000);
    printf("Set ym2151 clock to 4mhz\n");

    zsm_c_init_player();
    zsm_c_pcm_init();
    zfx_fm_c_init();
    play_test_ding();

    printf("Playing song %s\n", filename);
    printf("Press 't' to replay the YM ding test, 'q' to quit\n");
    uint8_t play = 1;
    while (play)
    {
            // volatile uint16_t testread = 0; 
            // testread = read_joystick(MDJOYPAD1);
            // mdx_parser_elapse(g_mdxdata, 1);
            // testread = read_joystick(MDJOYPAD1);
            // if (testread == 0xAA55) return 0;

            if (has_char())
            {
                char key = get_char();
                if (key == 'q')
                    play = 0;
                else if (key == 't')
                    play_test_ding();
                printf("%c", key);
            }
    }

    // Disable YM2151 interrupt
    ym2151_writereg(0x14, 0x00);

    return 0;
}

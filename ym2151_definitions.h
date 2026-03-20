#ifndef _YM2151_DEFINITIONS_H_
#define _YM2151_DEFINITIONS_H_
// YM2151 Definitions file

#include <stdint.h>

// YM2151 Registers
#define YM2151_REG_TEST         0x01        // TEST & LFO Reset
#define YM2151_REG_KEYON        0x08        // Key on (play sounds)
#define YM2151_REG_NOISE        0x0F        // Noise enable & frequency
#define YM2151_REG_CLOCK_A1     0x11        // Clock A Register
#define YM2151_REG_CLOCK_A2     0x12
#define YM2151_REG_CLOCK_B      0x13        // Clock B Register
#define YM2151_REG_CLOCK_FUNC   0x14        // Clock Functions
#define YM2151_REG_LOWFREQ      0x18        // Low Frequency OSC
#define YM2151_REG_PHSAMP       0x19        // Phase and Amplitude modulation
#define YM2151_REG_CTRLOUT      0x1B        // Control output & Wave form select
#define YM2151_REG_CHANCTRL     0x20        // Channel control

// Register = #28 + Channel number (0-7)
// Channel Key code. Note bits 0-3, Octave bits 4-6

#define YM2151_REG_KEYCODE_0    0x28
#define YM2151_REG_KEYCODE_1    0x29
#define YM2151_REG_KEYCODE_2    0x2A
#define YM2151_REG_KEYCODE_3    0x2B
#define YM2151_REG_KEYCODE_4    0x2C
#define YM2151_REG_KEYCODE_5    0x2D
#define YM2151_REG_KEYCODE_6    0x2E
#define YM2151_REG_KEYCODE_7    0x2F

// Register = #38 + Channel number (0-7)
// Channel Phase and Amplitude modulation sensitivity. PMS bits 4-6, AMS bits 0-1
#define YM2151_REG_PHSAMP_0     0x38
#define YM2151_REG_PHSAMP_1     0x39
#define YM2151_REG_PHSAMP_2     0x3A
#define YM2151_REG_PHSAMP_3     0x3B
#define YM2151_REG_PHSAMP_4     0x3C
#define YM2151_REG_PHSAMP_5     0x3D
#define YM2151_REG_PHSAMP_6     0x3E
#define YM2151_REG_PHSAMP_7     0x3F

// If you change value of MODULATOR1 then DEV=0
// If you change value of MODULATOR2 then DEV=1
// If you change value of CARRIER1 then DEV=2
// If you change value of CARRIER2 then DEV=3

// Detune & phase multiply, Detune 1 bits 4-6, Phase multiply bits 0-3
// Register = #40 + 8 * DEV* + Channel number (0-7)
#define YM2151_REG_DETUNE_PHASE 0x40

// Total Level, bits 0-6
// Register = #60 + 8 * DEV* + Channel number (0-7)
#define YM2151_REG_TOTAL_LEVEL  0x60

// EG Attack, KeySCl bits 6-7, Attack rate bits 0-4
// Register = #80 + 8 * DEV* + Channel number (0-7)
#define YM2151_REG_EG_ATTACK    0x80

// EG Decay 1, ASE bit 7, First decay rate bits 0-4
// Register = #A0 + 8 * DEV* + Channel number (0-7)
#define YM2151_REG_EG_DECAY1    0xA0

// EG Decay 2, Detune 2 bits 6-7, Second decay rate bits 0-3
// Register = #A0 + 8 * DEV* + Channel number (0-7)
#define YM2151_REG_EG_DECAY2    0xC0

// EG Decay Release, First decay level bits 4-7, Relese rate bits 0-3
// Register = #A0 + 8 * DEV* + Channel number (0-7)
#define YM2151_REG_EG_DECAYREL  0xE0


// YM2151 Status Register
#define YM2151_STATUS_BUSY      0x80        // Busy flag
#define YM2151_STATUS_BUSY_BIT  0x07        // Busy bit
#define YM2151_STATUS_TIMER1    0x01        // Timer 1 IRQ Overflow flag
#define YM2151_STATUS_TIMER2    0x02        // Timer 2 IRQ Overflow flag

// YM2151 Test Register
#define YM2151_TEST_LFO_RESET   0x02        // LFO Reset

// YM2151 Key on Register
#define YM2151_KEYON_MOD1       0x40        // Modulator 1
#define YM2151_KEYON_MOD2       0x10        // Modulator 2
#define YM2151_KEYON_CAR1       0x20        // Carrier 1
#define YM2151_KEYON_CAR2       0x08        // Carrier 2
#define YM2151_KEYON_CHANNEL    0x03        // Channel number 0-7

// YM2151 Noise register
#define YM2151_NOISE_ENABLE     0x80        // Noise enable
#define YM2151_NOISE_FREQENCY   0x1F        // Noise frequency (5-bit)

// YM2151 Clock functions register
#define YM2151_CLOCK_CSM        0x80        // Enables KEYON on all sound slots when Timer A overflows
#define YM2151_CLOCK_IRQ_A      0x04        // Timer A IRQ enable
#define YM2151_CLOCK_IRQ_B      0x08        // Timer B IRQ enable
#define YM2151_CLOCK_FRESET_A   0x10        // Timer A Overflow flag reset
#define YM2151_CLOCK_FRESET_B   0x20        // Timer B Overflow flag reset
#define YM2151_CLOCK_LOAD_A     0x01        // Timer A Start/Stop
#define YM2151_CLOCK_LOAD_B     0x02        // Timer B Start/Stop

// YM2151 Noise register
#define YM2151_PHSAMP_PHASE     0x80        // Set phase depth, 0 for amplitude

// YM2151 Output and wave register
#define YM2151_CTRLOUT_CT2      0x80        // External control output 2
#define YM2151_CTRLOUT_CT1      0x40        // External control output 1
#define YM2151_CTRLOUT_SAW      0x00        // Saw wave form
#define YM2151_CTRLOUT_SQUARE   0x01        // Square wave form
#define YM2151_CTRLOUT_TRIANGLE 0x02        // Triangle wave form
#define YM2151_CTRLOUT_NOISE    0x03        // Noise wave form

// YM2151 Channel control register
#define YM2151_CHANCTRL_RIGH    0x80        // Right
#define YM2151_CHANCTRL_LEFT    0x40        // Left
#define YM2151_CHANCTRL_FB0     0x00        // Feedback Level 0
#define YM2151_CHANCTRL_FB1     0x08        // Feedback Level 1
#define YM2151_CHANCTRL_FB2     0x10        // Feedback Level 2
#define YM2151_CHANCTRL_FB3     0x18        // Feedback Level 3
#define YM2151_CHANCTRL_FB4     0x20        // Feedback Level 4
#define YM2151_CHANCTRL_FB5     0x28        // Feedback Level 5
#define YM2151_CHANCTRL_FB6     0x30        // Feedback Level 6
#define YM2151_CHANCTRL_FB7     0x38        // Feedback Level 7
#define YM2151_CHANCTRL_CON0    0x00        // Connection 0
#define YM2151_CHANCTRL_CON1    0x01        // Connection 1
#define YM2151_CHANCTRL_CON2    0x02        // Connection 2
#define YM2151_CHANCTRL_CON3    0x03        // Connection 3
#define YM2151_CHANCTRL_CON4    0x04        // Connection 4
#define YM2151_CHANCTRL_CON5    0x05        // Connection 5
#define YM2151_CHANCTRL_CON6    0x06        // Connection 6
#define YM2151_CHANCTRL_CON7    0x07        // Connection 7

#endif

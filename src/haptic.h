#pragma once
// Haptic feedback (fork-only; boards without I2C_ADDR_HAPTIC compile this to stubs).
// DRV2605L driving the board's vibration motor, set up exactly like Waveshare's demo/SensorLib:
// ERM mode with open-loop drive, waveform library 1, internal-trigger mode.
#include "config.h"

#if defined(I2C_ADDR_HAPTIC)
bool haptic_begin(void);   // false if the chip is absent or not a DRV2604/2605 (then haptic_tick() is a no-op)
void haptic_tick(void);    // one short click: library 1 effect HAPTIC_TICK_EFFECT
#else
static inline bool haptic_begin(void) { return false; }
static inline void haptic_tick(void)  {}
#endif

#pragma once
// Rotary knob input (fork-only; boards without PIN_ENC_A/PIN_ENC_B compile this to stubs).
//
// The Waveshare knob is a BIDIRECTIONAL-SWITCH type, NOT quadrature: both lines idle high (internal
// pull-up) and each click pulses exactly ONE line low. Decoded exactly like Waveshare's demo
// (bidi_switch_knob.c): both lines are sampled every 3 ms; a line counts as a click when it was low
// for at least 2 consecutive samples and then rose (shorter pulses are contact chatter: about 8 to
// 27 raw edges per click were measured, down to ~12 us, and are ignored). Line A (PIN_ENC_A) is
// clockwise (+1), line B (PIN_ENC_B) is anticlockwise (-1); measured on the board.
//
// Sampling runs on its own 3 ms esp_timer, NOT in loop(): a full redraw can stall loop() for 150 ms
// or more, and a click pulse is only about 12 ms long, so polling from loop() would drop clicks.
#include "config.h"

#if defined(PIN_ENC_A) && defined(PIN_ENC_B)
void knob_begin(void);   // configure the pins and start the 3 ms sampler
int  knob_take(void);    // clicks since the last call: +n clockwise, -n anticlockwise, 0 = none; clears
#else
static inline void knob_begin(void) {}
static inline int  knob_take(void)  { return 0; }
#endif

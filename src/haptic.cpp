// DRV2605L haptics (see haptic.h). Fork-only: compiles only where the board header defines
// I2C_ADDR_HAPTIC.
//
// The setup is Waveshare's: SensorLib's SensorDRV2605::initImpl() (ERM mode, ERM open loop,
// out of standby, no overdrive/sustain/brake offsets) followed by what the demo does,
// selectLibrary(1) and internal-trigger mode. Register numbers are SensorLib's
// DRV2605Constants.h; the effect numbers are the demo's list (library 1: 1 Strong Click 100%,
// 4/5/6 Sharp Click 100/60/30%, 7/8/9 Soft Bump 100/60/30%, 10 Double Click 100%, ...).
#include "haptic.h"
#if defined(I2C_ADDR_HAPTIC)

#include <Arduino.h>
#include <Wire.h>

#ifndef HAPTIC_TICK_EFFECT
#define HAPTIC_TICK_EFFECT 5          // Sharp Click 60%: a light detent feel; tune by hand
#endif

namespace {

constexpr uint8_t REG_STATUS    = 0x00;
constexpr uint8_t REG_MODE      = 0x01;
constexpr uint8_t REG_RTPIN     = 0x02;
constexpr uint8_t REG_LIBRARY   = 0x03;
constexpr uint8_t REG_WAVESEQ1  = 0x04;
constexpr uint8_t REG_WAVESEQ2  = 0x05;
constexpr uint8_t REG_GO        = 0x0C;
constexpr uint8_t REG_OVERDRIVE = 0x0D;
constexpr uint8_t REG_SUSTAINP  = 0x0E;
constexpr uint8_t REG_SUSTAINN  = 0x0F;
constexpr uint8_t REG_BRAKE     = 0x10;
constexpr uint8_t REG_AUDIOMAX  = 0x13;
constexpr uint8_t REG_FEEDBACK  = 0x1A;
constexpr uint8_t REG_CONTROL3  = 0x1D;

bool g_present = false;

bool rd(uint8_t reg, uint8_t *v) {
    Wire.beginTransmission(I2C_ADDR_HAPTIC);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)I2C_ADDR_HAPTIC, (uint8_t)1) != 1) return false;
    *v = Wire.read();
    return true;
}

bool wr(uint8_t reg, uint8_t v) {
    Wire.beginTransmission(I2C_ADDR_HAPTIC);
    Wire.write(reg);
    Wire.write(v);
    return Wire.endTransmission() == 0;
}

}  // namespace

bool haptic_begin(void) {
    g_present = false;
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);   // idempotent: touch_begin() already started it

    uint8_t status = 0;
    if (!rd(REG_STATUS, &status)) { Serial.println("[haptic] DRV2605 not found"); return false; }
    const uint8_t id = status >> 5;
    if (id != 0x03 && id != 0x04 && id != 0x06 && id != 0x07) {      // SensorLib accepts these four
        Serial.printf("[haptic] unexpected chip id %u\n", id);
        return false;
    }

    // SensorLib initImpl(), in its order
    bool ok = wr(REG_MODE, 0x00);                // out of standby, internal trigger
    ok &= wr(REG_RTPIN, 0x00);                   // no real-time playback
    ok &= wr(REG_WAVESEQ1, 1);                   // strong click (placeholder; haptic_tick() overwrites it)
    ok &= wr(REG_WAVESEQ2, 0);                   // end of sequence
    ok &= wr(REG_OVERDRIVE, 0);
    ok &= wr(REG_SUSTAINP, 0);
    ok &= wr(REG_SUSTAINN, 0);
    ok &= wr(REG_BRAKE, 0);
    ok &= wr(REG_AUDIOMAX, 0x64);
    uint8_t v = 0;
    ok &= rd(REG_FEEDBACK, &v) && wr(REG_FEEDBACK, v & 0x7F);        // N_ERM_LRA off: ERM
    ok &= rd(REG_CONTROL3, &v) && wr(REG_CONTROL3, v | 0x20);        // ERM open loop on
    // what the demo does next
    ok &= wr(REG_LIBRARY, 1);                    // library 1 (ERM)
    ok &= wr(REG_MODE, 0x00);                    // internal trigger

    g_present = ok;
    Serial.printf("[haptic] %s (DRV2604/2605 id %u, ERM open-loop, library 1, tick effect %d)\n",
                  ok ? "ready" : "setup FAILED", id, (int)HAPTIC_TICK_EFFECT);
    return ok;
}

void haptic_tick(void) {
    if (!g_present) return;
    wr(REG_WAVESEQ1, (uint8_t)HAPTIC_TICK_EFFECT);
    wr(REG_WAVESEQ2, 0);
    wr(REG_GO, 0x01);
}

#endif  // I2C_ADDR_HAPTIC

// CST816D capacitive touch over I2C (Arduino). BOARD_KNOB18 only (TOUCH_DRIVER_CST816D).
// Single touch point, polled (the interrupt line is not used; the radar polls like the other
// boards). Uses the read that bringup/main.cpp proved on this exact chip, the same one Waveshare's
// demo makes: 7 bytes from register 0x00 -> [0] unused, [1] gesture, [2] finger count,
// [3] event(bits 7:6)|XH, [4] XL, [5] YH, [6] YL; 12-bit coordinates. (The CST820 driver reads the
// same registers starting at 0x01; this one keeps the demo's form because it is the one measured.)
// Measured on the board: x grows to the right and y grows downward, 1:1 with screen pixels.
// The controller's RST is a real GPIO here (GPIO 10), not a port-expander line.
#include "config.h"
#if defined(TOUCH_DRIVER_CST816D)
#include "touch.h"
#include <Arduino.h>
#include <Wire.h>

#define CST816_REG_DATA        0x00   // 7 bytes, see above
#define CST816_REG_CHIPID      0xA7   // 0xB6 = CST816D (SensorLib's CST816D_CHIP_ID); measured 0xB6
#define CST816_REG_PROJID      0xA8
#define CST816_REG_FWVER       0xA9
#define CST816_REG_DISSLEEP    0xFE   // 1 = disable auto-sleep (keep reporting while polled)

// Write the 8-bit register pointer, repeated-start read.
static bool cst_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(I2C_ADDR_TOUCH);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;   // repeated start
    if (Wire.requestFrom((uint8_t)I2C_ADDR_TOUCH, len) < len) return false;
    for (uint8_t i = 0; i < len; ++i) buf[i] = Wire.read();
    return true;
}

static void cst_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(I2C_ADDR_TOUCH);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

bool touch_begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);   // idempotent (other modules also begin it)
    pinMode(PIN_TP_INT, INPUT);

    // hardware reset (low 10 ms, high 50 ms), the sequence the bring-up used
    pinMode(PIN_TP_RST, OUTPUT);
    digitalWrite(PIN_TP_RST, LOW);
    delay(10);
    digitalWrite(PIN_TP_RST, HIGH);
    delay(50);

    uint8_t id = 0, proj = 0, fw = 0;
    if (cst_read(CST816_REG_CHIPID, &id, 1)) {
        cst_read(CST816_REG_PROJID, &proj, 1);
        cst_read(CST816_REG_FWVER, &fw, 1);
        cst_write(CST816_REG_DISSLEEP, 0x01);       // we poll, so keep the controller awake
        Serial.printf("[touch] CST816D chipID=0x%02X projID=0x%02X fw=0x%02X%s\n", id, proj, fw,
                      id == 0xB6 ? "" : "  (expected chipID 0xB6)");
    } else {
        Serial.println("[touch] CST816D not responding yet (will keep polling)");
    }
    return true;
}

TouchRead touch_read(uint16_t *ox, uint16_t *oy) {
    uint8_t d[7];
    if (!cst_read(CST816_REG_DATA, d, 7)) return TOUCH_NODATA;
    if (d[2] == 0) return TOUCH_UP;                  // finger count

    uint16_t x = (uint16_t)(((d[3] & 0x0F) << 8) | d[4]);
    uint16_t y = (uint16_t)(((d[5] & 0x0F) << 8) | d[6]);

    if (x > SCREEN_W - 1) x = SCREEN_W - 1;
    if (y > SCREEN_H - 1) y = SCREEN_H - 1;
    if (TP_MIRROR_X) x = (SCREEN_W - 1) - x;
    if (TP_MIRROR_Y) y = (SCREEN_H - 1) - y;

    *ox = x;
    *oy = y;
    return TOUCH_DOWN;
}

#endif  // TOUCH_DRIVER_CST816D

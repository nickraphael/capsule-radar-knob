// ============================================================================
// Knob bring-up firmware: a standalone hardware test for the
// Waveshare ESP32-S3-Knob-Touch-LCD-1.8. This is NOT the radar app.
//
// Purpose: check our assumptions one at a time, on the real board, before porting
// the radar UI. It uses only pins traced to Waveshare's demo (see context.txt),
// except the battery ADC pin, which is an INFERENCE (ADC1 channel 0 = GPIO 1).
//
// Talk to it over the USB serial port (115200, any terminal). It also mirrors
// output to UART0 so we learn which port the PC actually sees. Commands end with Enter:
//   ?        help
//   i        chip / flash / PSRAM info + PSRAM write/verify test
//   s        I2C scan (expect 0x15 touch, 0x5A DRV2605)
//   p0       colour bars   (labels must match the colours you see)
//   p1       geometry      (circle, crosshair, corners, TOP/L/R/BOTTOM labels)
//   p2       odd-alignment (odd x/y/w/h rectangles + 1px checker: look for glitches)
//   c        clear screen
//   v        toggle panel inversion (INVON/INVOFF) live
//   b N      backlight 0..255
//   t        toggle touch stream (also draws dots where it thinks you touch)
//   e        toggle encoder CLICK events. This knob is a bidirectional-switch type (NOT quadrature):
//            decoded exactly like Waveshare's demo: 3 ms poll, A (GPIO8) = RIGHT, B (GPIO7) = LEFT
//   E        toggle RAW edge printing (floods the USB output when the knob moves; debugging only)
//   n        print and reset the click counters: RIGHT(A), LEFT(B), net, raw edges (chatter)
//   k        toggle BOOT (GPIO0) button events
//   a        read the battery ADC pin (GPIO1, inferred) once, over serial
//   A        toggle a LIVE ADC readout on the screen (unplug USB and read it off the display)
//   w text   draw words on the screen; '|' starts a new line, "N~" before a line sets its size 1..5
//            e.g.  w TURN NOW|CLOCKWISE|SLOW|12 s left
//   m N      haptic effect N (1..123), ERM library
//   M N      haptic effect N, LRA library
//   x N      set QSPI clock to N MHz (10..80), saved, then reboots
//   r        reboot
// ============================================================================
#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <stdarg.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <Arduino_GFX_Library.h>

#if !__has_include("st77916_knob_init.h")
#error "bringup/st77916_knob_init.h is missing. Generate it: python bringup/gen_init.py <demo>\\Arduino\\examples\\08_LVGL_Test\\lcd_bsp.c"
#endif
#include "st77916_knob_init.h"

// ---- Pins and addresses. Source: Waveshare demo (lcd_config.h, user_config.h) ----
static constexpr int     PIN_LCD_CS = 14, PIN_LCD_SCLK = 13;
static constexpr int     PIN_LCD_D0 = 15, PIN_LCD_D1 = 16, PIN_LCD_D2 = 17, PIN_LCD_D3 = 18;
static constexpr int     PIN_LCD_RST = 21, PIN_LCD_BL = 47;
static constexpr int     PIN_I2C_SDA = 11, PIN_I2C_SCL = 12;
static constexpr int     PIN_TP_RST = 10, PIN_TP_INT = 9;
static constexpr uint8_t TP_ADDR = 0x15, DRV_ADDR = 0x5A;
static constexpr int     PIN_ENC_A = 8, PIN_ENC_B = 7;
static constexpr int     PIN_BOOT = 0;
static constexpr int     PIN_BATT_ADC = 1;     // INFERRED, not seen in any config
static constexpr int     LCD_W = 360, LCD_H = 360;
static constexpr int     BL_FREQ = 50000, BL_BITS = 8;   // demo: LEDC 8-bit, 50 kHz

#define C565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
static constexpr uint16_t K_BLACK = 0x0000, K_WHITE = 0xFFFF;

static Preferences prefs;
static Arduino_DataBus *bus = nullptr;
static Arduino_GFX *gfx = nullptr;
static bool gfxOk = false;
static bool panelInverted = true;   // the demo ends init with INVON
static uint32_t qspiMHz = 40;       // the demo's QSPI clock
static uint8_t blLevel = 0;
static bool touchOn = false, encOn = false, btnOn = false;
static uint16_t dotColor = C565(0, 255, 0);

// ---- output goes to BOTH the USB port and UART0 ----
static void say(const char *fmt, ...) {
  char buf[320];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n <= 0) return;
  size_t len = (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1;
  Serial.write((const uint8_t *)buf, len);
  Serial0.write((const uint8_t *)buf, len);
}

// ---- I2C helpers (8-bit register pointer, repeated start) ----
static bool i2cRead(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, n) < n) return false;
  for (uint8_t i = 0; i < n; ++i) buf[i] = Wire.read();
  return true;
}
static bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
static void i2cScan() {
  say("I2C scan on SDA=%d SCL=%d:\n", PIN_I2C_SDA, PIN_I2C_SCL);
  int found = 0;
  for (uint8_t a = 1; a < 127; ++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      say("  found 0x%02X%s\n", a, a == TP_ADDR ? "  (expected: touch)" : a == DRV_ADDR ? "  (expected: DRV2605)" : "");
      ++found;
    }
  }
  say("  %d device(s)\n", found);
}

// ---- backlight ----
static void blInit() {
  ledcAttach(PIN_LCD_BL, BL_FREQ, BL_BITS);
  ledcWrite(PIN_LCD_BL, 0);
}
static void blSet(uint8_t v) {
  blLevel = v;
  ledcWrite(PIN_LCD_BL, v);
}

// ---- display ----
// Arduino_ST77916::tftInit() ends with invertDisplay(false), which sends INVON only if the
// panel is constructed with ips=true. The demo ends its init with INVON, so ips=true.
// invertDisplay(x) sends INVON when (ips ^ x) is true, so INVON == invertDisplay(false).
static void setPanelInversion(bool on) {
  if (!gfxOk) return;
  gfx->invertDisplay(!on);
  panelInverted = on;
}
static bool displayInit() {
  bus = new Arduino_ESP32QSPI(PIN_LCD_CS, PIN_LCD_SCLK, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);
  gfx = new Arduino_ST77916(bus, PIN_LCD_RST, 0 /*rotation*/, true /*ips: see above*/, LCD_W, LCD_H,
                            0, 0, 0, 0, knob_st77916_init_ops, sizeof(knob_st77916_init_ops));
  return gfx->begin((int32_t)(qspiMHz * 1000000UL));
}
static void textCentered(const char *s, int cx, int y, uint8_t size, uint16_t fg) {
  int w = (int)strlen(s) * 6 * size;
  gfx->setTextSize(size);
  gfx->setTextColor(fg);
  gfx->setCursor(cx - w / 2, y);
  gfx->print(s);
}
static void patBars() {
  if (!gfxOk) return;
  static const char *names[8] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK", "CYAN", "MAGENTA", "YELLOW"};
  const uint16_t cols[8] = {C565(255, 0, 0), C565(0, 255, 0), C565(0, 0, 255), C565(255, 255, 255),
                            C565(0, 0, 0), C565(0, 255, 255), C565(255, 0, 255), C565(255, 255, 0)};
  const int bh = LCD_H / 8;
  for (int i = 0; i < 8; ++i) {
    gfx->fillRect(0, i * bh, LCD_W, bh, cols[i]);
    const bool darkBar = (i == 0 || i == 2 || i == 4 || i == 6);
    textCentered(names[i], LCD_W / 2, i * bh + (bh - 16) / 2, 2, darkBar ? K_WHITE : K_BLACK);
  }
  say("p0: top to bottom you should read RED GREEN BLUE WHITE BLACK CYAN MAGENTA YELLOW, each bar in its own colour.\n");
  say("    Wrong colours = RGB/BGR swapped; negative-looking colours = inversion wrong (try v).\n");
}
static void patGeometry() {
  if (!gfxOk) return;
  const uint16_t g = C565(90, 90, 90);
  gfx->fillScreen(K_BLACK);
  gfx->drawRect(0, 0, LCD_W, LCD_H, C565(255, 0, 255));          // 1px border: first/last row+col must exist
  gfx->drawCircle(LCD_W / 2, LCD_H / 2, 179, K_WHITE);
  gfx->drawCircle(LCD_W / 2, LCD_H / 2, 120, g);
  gfx->drawCircle(LCD_W / 2, LCD_H / 2, 60, g);
  gfx->drawFastHLine(0, LCD_H / 2, LCD_W, g);
  gfx->drawFastVLine(LCD_W / 2, 0, LCD_H, g);
  gfx->fillRect(0, 0, 24, 24, C565(255, 0, 0));                  // corners: red TL, green TR, blue BL, yellow BR
  gfx->fillRect(LCD_W - 24, 0, 24, 24, C565(0, 255, 0));
  gfx->fillRect(0, LCD_H - 24, 24, 24, C565(0, 0, 255));
  gfx->fillRect(LCD_W - 24, LCD_H - 24, 24, 24, C565(255, 255, 0));
  textCentered("TOP", LCD_W / 2, 28, 2, K_WHITE);
  textCentered("BOTTOM", LCD_W / 2, LCD_H - 46, 2, K_WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(K_WHITE);
  gfx->setCursor(30, LCD_H / 2 - 8);
  gfx->print("L");
  gfx->setCursor(LCD_W - 42, LCD_H / 2 - 8);
  gfx->print("R");
  textCentered("360x360", LCD_W / 2, LCD_H / 2 - 30, 2, C565(0, 255, 255));
  say("p1: TOP/L/R/BOTTOM must read correctly (not mirrored or rotated). The magenta border and the\n");
  say("    white circle should be complete; note which corner squares the round bezel hides.\n");
}
static void patOdd() {
  if (!gfxOk) return;
  gfx->fillScreen(K_BLACK);
  // odd origin + odd size rectangles
  const uint16_t cols[5] = {C565(255, 0, 0), C565(0, 255, 0), C565(0, 0, 255), C565(255, 255, 0), C565(255, 0, 255)};
  for (int i = 0; i < 5; ++i) gfx->fillRect(61 + i * 45, 61, 7 + i * 2, 7 + i * 2, cols[i]);
  // 1px vertical lines at odd x
  for (int x = 61; x < 121; x += 2) gfx->drawFastVLine(x, 120, 40, K_WHITE);
  // 60x60 1px checker starting at an odd origin
  for (int y = 0; y < 60; ++y)
    for (int x = 0; x < 60; ++x) gfx->drawPixel(201 + x, 121 + y, ((x ^ y) & 1) ? K_WHITE : K_BLACK);
  // single pixels at odd/odd
  for (int i = 0; i < 10; ++i) gfx->drawPixel(61 + i * 5, 201, K_WHITE);
  say("p2: look for missing, shifted or doubled pixels. The checker should be perfectly even and the\n");
  say("    odd-origin rectangles should be solid. Glitches here mean the panel needs even-aligned flushes.\n");
}

// On-screen prompt: up to 5 lines separated by '|', stacked and centred, so a test can tell you
// in words what to do and when. A line may start with "<digit>~" to choose its text size (1..5),
// e.g. "5~4.76 V". Default size is 4 for the first line and 3 for the rest. Colours by line.
static void drawPrompt(const String &spec) {
  if (!gfxOk) return;
  String lines[5];
  uint8_t sizes[5];
  int n = 0, start = 0;
  while (n < 5) {
    int bar = spec.indexOf('|', start);
    String ln = (bar < 0) ? spec.substring(start) : spec.substring(start, bar);
    ln.trim();
    uint8_t sz = (n == 0) ? 4 : 3;
    if (ln.length() >= 2 && ln[1] == '~' && ln[0] >= '1' && ln[0] <= '5') { sz = (uint8_t)(ln[0] - '0'); ln = ln.substring(2); }
    lines[n] = ln;
    sizes[n] = sz;
    ++n;
    if (bar < 0) break;
    start = bar + 1;
  }
  const uint16_t cols[5] = {C565(255, 255, 0), K_WHITE, C565(0, 255, 0), C565(0, 255, 255), C565(255, 128, 0)};
  int totalH = 0;
  for (int i = 0; i < n; ++i) totalH += sizes[i] * 8 + 10;
  gfx->fillRect(15, 40, 330, 280, K_BLACK);
  int y = LCD_H / 2 - totalH / 2;
  for (int i = 0; i < n; ++i) {
    textCentered(lines[i].c_str(), LCD_W / 2, y, sizes[i], cols[i]);
    y += sizes[i] * 8 + 10;
  }
}

// ---- touch ----
struct TouchSample { uint8_t raw[7]; uint8_t gesture, fingers, evt; uint16_t x, y; };
static void touchReset() {
  pinMode(PIN_TP_RST, OUTPUT);
  digitalWrite(PIN_TP_RST, LOW);
  delay(10);
  digitalWrite(PIN_TP_RST, HIGH);
  delay(50);
}
static bool touchSample(TouchSample &s) {
  if (!i2cRead(TP_ADDR, 0x00, s.raw, 7)) return false;   // same read as the demo: 7 bytes from reg 0
  s.gesture = s.raw[1];
  s.fingers = s.raw[2];
  s.evt = s.raw[3] >> 6;
  s.x = ((uint16_t)(s.raw[3] & 0x0F) << 8) | s.raw[4];
  s.y = ((uint16_t)(s.raw[5] & 0x0F) << 8) | s.raw[6];
  return true;
}
static void touchProbe() {
  uint8_t id = 0, proj = 0, fw = 0;
  bool ok = i2cRead(TP_ADDR, 0xA7, &id, 1);
  if (!ok) { say("touch: no response at 0x%02X\n", TP_ADDR); return; }
  i2cRead(TP_ADDR, 0xA8, &proj, 1);
  i2cRead(TP_ADDR, 0xA9, &fw, 1);
  i2cWrite(TP_ADDR, 0xFE, 0x01);   // disable auto-sleep, as the original's CST820 driver does
  say("touch: chipID(0xA7)=0x%02X projID(0xA8)=0x%02X fw(0xA9)=0x%02X  (firmware strings said CST816S)\n", id, proj, fw);
}
static void touchLoop() {
  static uint32_t last = 0;
  static bool down = false;
  if (!touchOn) return;
  const uint32_t now = millis();
  if (now - last < 40) return;
  last = now;
  TouchSample s;
  if (!touchSample(s)) {
    static uint32_t lastErr = 0;
    if (now - lastErr > 1000) { say("touch: I2C read failed\n"); lastErr = now; }
    return;
  }
  if (s.fingers > 0) {
    say("TOUCH x=%u y=%u fingers=%u gesture=0x%02X evt=%u int=%d raw=%02X %02X %02X %02X %02X %02X %02X\n",
        s.x, s.y, s.fingers, s.gesture, s.evt, digitalRead(PIN_TP_INT),
        s.raw[0], s.raw[1], s.raw[2], s.raw[3], s.raw[4], s.raw[5], s.raw[6]);
    if (gfxOk) gfx->fillCircle(s.x, s.y, 4, dotColor);
    down = true;
  } else if (down) {
    say("TOUCH up\n");
    down = false;
    static const uint16_t pal[4] = {C565(0, 255, 0), C565(255, 128, 0), C565(0, 200, 255), C565(255, 0, 255)};
    static uint8_t pi = 0;
    dotColor = pal[++pi & 3];
  }
}

// ---- encoder ----
// This knob is a BIDIRECTIONAL-SWITCH type, NOT quadrature: both lines idle high (internal pull-up)
// and each click pulses exactly one line low. Waveshare's demo (bidi_switch_knob.c) polls both
// lines every 3 ms; a line counts as a click only if it was low for >= 2 consecutive samples and
// then released, so sub-millisecond contact chatter is ignored. Line A (GPIO 8) = RIGHT / +1,
// line B (GPIO 7) = LEFT / -1. Reproduced here exactly. The ISR below only counts raw edges (to
// show how much chatter there is) and, in raw mode (command E), records them for printing.
struct KnobCh { uint8_t prev; uint8_t deb; uint32_t lowSinceMs; };
static KnobCh chA = {1, 0, 0}, chB = {1, 0, 0};
static int knobCount = 0;
static uint32_t knobClicksA = 0, knobClicksB = 0;
static volatile uint32_t encEdges = 0;          // raw edges seen by the ISR (chatter included)
static volatile bool encRaw = false;

static void knobChannel(KnobCh &c, uint8_t level, int inc, const char *name) {
  if (level == 0) {
    if (c.prev != 0) { c.deb = 0; c.lowSinceMs = millis(); } else c.deb++;      // same as the demo
  } else {
    if (c.prev == 0 && ++c.deb >= 2) {
      c.deb = 0;
      knobCount += inc;
      if (inc > 0) ++knobClicksA; else ++knobClicksB;
      if (encOn) say("CLICK %s count=%+d held low %lu ms, raw edges so far %lu\n", name, knobCount,
                     (unsigned long)(millis() - c.lowSinceMs), (unsigned long)encEdges);
    } else c.deb = 0;
  }
  c.prev = level;
}
static void knobPoll() {
  static uint32_t last = 0;
  const uint32_t now = millis();
  if (now - last < 3) return;
  last = now;
  knobChannel(chA, (uint8_t)digitalRead(PIN_ENC_A), +1, "RIGHT (A)");
  knobChannel(chB, (uint8_t)digitalRead(PIN_ENC_B), -1, "LEFT  (B)");
}
static void knobReport() {
  say("knob since last n: RIGHT(A) clicks=%lu  LEFT(B) clicks=%lu  net=%+d  raw edges=%lu\n",
      (unsigned long)knobClicksA, (unsigned long)knobClicksB, knobCount, (unsigned long)encEdges);
  knobClicksA = knobClicksB = 0;
  knobCount = 0;
  encEdges = 0;
}

struct EncEvt { uint32_t us; uint8_t st; };
static volatile EncEvt encBuf[64];
static volatile uint8_t encHead = 0, encTail = 0;
static void IRAM_ATTR encIsr() {
  encEdges = encEdges + 1;
  if (!encRaw) return;
  const uint8_t h = encHead;
  encBuf[h].us = (uint32_t)esp_timer_get_time();
  encBuf[h].st = (uint8_t)((gpio_get_level((gpio_num_t)PIN_ENC_A) << 1) | gpio_get_level((gpio_num_t)PIN_ENC_B));
  encHead = (uint8_t)((h + 1) & 63);
}
static void encRawLoop() {
  static uint32_t lastUs = 0;
  while (encTail != encHead) {
    const uint8_t t = encTail;
    const uint32_t us = encBuf[t].us;
    const uint8_t st = encBuf[t].st;
    encTail = (uint8_t)((t + 1) & 63);
    say("RAW A=%d B=%d dt=%luus\n", (st >> 1) & 1, st & 1, (unsigned long)(us - lastUs));
    lastUs = us;
  }
}
static void btnLoop() {
  static int last = -1;
  static uint32_t lastChange = 0;
  const int v = digitalRead(PIN_BOOT);
  if (v != last && millis() - lastChange > 30) {
    if (last != -1 && btnOn) say("BOOT button %s\n", v == LOW ? "PRESSED" : "released");
    last = v;
    lastChange = millis();
  }
}

// ---- haptics (DRV2605) ----
static void drvPlay(uint8_t effect, bool lra) {
  uint8_t st = 0;
  if (!i2cRead(DRV_ADDR, 0x00, &st, 1)) { say("DRV2605: no response at 0x%02X\n", DRV_ADDR); return; }
  say("DRV2605 status=0x%02X device-id=%u (3=DRV2605, 7=DRV2605L)\n", st, st >> 5);
  if (effect < 1 || effect > 123) { say("effect must be 1..123\n"); return; }
  i2cWrite(DRV_ADDR, 0x01, 0x00);   // out of standby, internal trigger
  uint8_t fb = 0;
  i2cRead(DRV_ADDR, 0x1A, &fb, 1);
  if (lra) { i2cWrite(DRV_ADDR, 0x1A, fb | 0x80); i2cWrite(DRV_ADDR, 0x03, 6); }
  else     { i2cWrite(DRV_ADDR, 0x1A, fb & 0x7F); i2cWrite(DRV_ADDR, 0x03, 1); }
  i2cWrite(DRV_ADDR, 0x04, effect);
  i2cWrite(DRV_ADDR, 0x05, 0);
  i2cWrite(DRV_ADDR, 0x0C, 0x01);   // GO
  say("played effect %u with the %s library. Did you feel it? Try the other library if it was weak.\n", effect, lra ? "LRA" : "ERM");
}

// ---- info / ADC / PSRAM ----
static bool psramTest() {
  const size_t n = 512 * 1024;
  uint8_t *p = (uint8_t *)ps_malloc(n);
  if (!p) return false;
  for (size_t i = 0; i < n; ++i) p[i] = (uint8_t)((i * 31u) ^ (i >> 8));
  bool ok = true;
  for (size_t i = 0; i < n && ok; ++i) ok = (p[i] == (uint8_t)((i * 31u) ^ (i >> 8)));
  free(p);
  return ok;
}
static const char *resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_DEEPSLEEP: return "deep sleep";
    default: return "other";
  }
}
static void cmdInfo() {
  say("chip: %s rev %d, %d core(s), %d MHz, efuse mac %012llX\n", ESP.getChipModel(), ESP.getChipRevision(),
      ESP.getChipCores(), ESP.getCpuFreqMHz(), (unsigned long long)ESP.getEfuseMac());
  say("flash: %u MB, %u MHz, runtime mode %d (0=QIO 1=QOUT 2=DIO 3=DOUT)\n", (unsigned)(ESP.getFlashChipSize() / 1048576),
      (unsigned)(ESP.getFlashChipSpeed() / 1000000), (int)ESP.getFlashChipMode());
  say("psram: found=%d size=%u KB free=%u KB\n", (int)psramFound(), (unsigned)(ESP.getPsramSize() / 1024), (unsigned)(ESP.getFreePsram() / 1024));
  say("psram write/verify test (512 KB): %s\n", psramTest() ? "PASS" : "FAIL");
  say("heap: free %u KB, min free %u KB\n", (unsigned)(ESP.getFreeHeap() / 1024), (unsigned)(ESP.getMinFreeHeap() / 1024));
  say("arduino core %d.%d.%d, IDF %s\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH, esp_get_idf_version());
  say("build flags: USB_MODE=%d CDC_ON_BOOT=%d\n", (int)ARDUINO_USB_MODE, (int)ARDUINO_USB_CDC_ON_BOOT);
  say("reset reason: %s\n", resetReasonStr(esp_reset_reason()));
  say("display: init %s, QSPI %u MHz, backlight %u/255, panel inversion %s\n", gfxOk ? "OK" : "FAILED", (unsigned)qspiMHz, blLevel,
      panelInverted ? "ON (as demo)" : "OFF");
  say("uptime %lu s\n", (unsigned long)(millis() / 1000));
}
static void cmdAdc() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db);   // the demo uses 12 dB (the core calls it 11db)
  uint32_t raw = 0, mv = 0;
  for (int i = 0; i < 16; ++i) { raw += analogRead(PIN_BATT_ADC); mv += analogReadMilliVolts(PIN_BATT_ADC); delay(2); }
  raw /= 16;
  mv /= 16;
  say("ADC GPIO%d (INFERRED pin): raw=%lu, %lu mV at the pin, x2 divider => %.2f V; demo formula raw*3.3/4096*2 = %.2f V\n",
      PIN_BATT_ADC, (unsigned long)raw, (unsigned long)mv, (mv * 2) / 1000.0f, raw * 3.3f / 4096.0f * 2.0f);
  say("   With no battery fitted this reads whatever the board feeds the divider; compare with and without a battery.\n");
}

// Live ADC readout on the SCREEN (command A): lets you unplug USB and read GPIO1 off the display,
// so we can see whether it tracks the battery. Updates twice a second.
static bool adcLive = false;
static void adcLiveLoop() {
  static uint32_t last = 0;
  if (!adcLive || !gfxOk) return;
  const uint32_t now = millis();
  if (now - last < 500) return;
  last = now;
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db);
  uint32_t raw = 0, mv = 0;
  for (int i = 0; i < 8; ++i) { raw += analogRead(PIN_BATT_ADC); mv += analogReadMilliVolts(PIN_BATT_ADC); }
  raw /= 8;
  mv /= 8;
  char volts[24], rawTxt[24], upTxt[24];
  snprintf(volts, sizeof volts, "5~%.2f V", (mv * 2) / 1000.0f);
  snprintf(rawTxt, sizeof rawTxt, "3~raw %lu", (unsigned long)raw);
  snprintf(upTxt, sizeof upTxt, "2~up %lu s", (unsigned long)(millis() / 1000));
  drawPrompt(String("3~GPIO1 x2|") + volts + "|" + rawTxt + "|" + upTxt);
}

// ---- command handling ----
static void help() {
  say("\nKnob bring-up commands: ? i s p0 p1 p2 c v b<N> t e E n k a A w<text> m<N> M<N> x<N> r\n");
  say("(see the header of bringup/main.cpp for what each one checks)\n");
}
static void handleLine(String s) {
  s.trim();
  if (!s.length()) return;
  const char c = s[0];
  String rest = s.substring(1);
  rest.trim();
  const bool hasArg = rest.length() > 0;
  const long arg = rest.toInt();
  switch (c) {
    case '?': help(); break;
    case 'i': cmdInfo(); break;
    case 's': i2cScan(); break;
    case 'p':
      if (arg == 0) patBars(); else if (arg == 1) patGeometry(); else if (arg == 2) patOdd(); else say("p0, p1 or p2\n");
      break;
    case 'c': if (gfxOk) gfx->fillScreen(K_BLACK); say("cleared\n"); break;
    case 'v': setPanelInversion(!panelInverted); say("panel inversion now %s\n", panelInverted ? "ON (INVON, as the demo)" : "OFF (INVOFF)"); break;
    case 'b':
      if (!hasArg || arg < 0 || arg > 255) { say("usage: b <0..255>\n"); break; }
      blSet((uint8_t)arg);
      say("backlight %ld/255\n", arg);
      break;
    case 't': touchOn = !touchOn; say("touch stream %s\n", touchOn ? "ON (touch the screen; drag along the edge to check mapping)" : "OFF"); break;
    case 'e': encOn = !encOn; say("encoder CLICK events %s (demo-faithful decoder: A=RIGHT, B=LEFT)\n", encOn ? "ON" : "OFF"); break;
    case 'E': encRaw = !encRaw; say("encoder RAW edge printing %s\n", encRaw ? "ON (can flood the USB output)" : "OFF"); break;
    case 'n': knobReport(); break;
    case 'k': btnOn = !btnOn; say("BOOT button events %s\n", btnOn ? "ON" : "OFF"); break;
    case 'w': drawPrompt(rest); break;
    case 'A':
      adcLive = !adcLive;
      say("live ADC on screen %s%s\n", adcLive ? "ON" : "OFF", adcLive ? " (unplug USB and read the display; send A again to stop)" : "");
      if (!adcLive && gfxOk) patGeometry();
      break;
    case 'a': cmdAdc(); break;
    case 'm': drvPlay(hasArg ? (uint8_t)arg : 1, false); break;
    case 'M': drvPlay(hasArg ? (uint8_t)arg : 1, true); break;
    case 'x':
      if (!hasArg || arg < 10 || arg > 80) { say("usage: x <10..80> (MHz)\n"); break; }
      prefs.putUInt("mhz", (uint32_t)arg);
      say("QSPI clock saved as %ld MHz, rebooting...\n", arg);
      delay(300);
      ESP.restart();
      break;
    case 'r': say("rebooting...\n"); delay(300); ESP.restart(); break;
    default: say("unknown command '%c' (? for help)\n", c); break;
  }
}
static String lineA, lineB;
static void pollSerial() {
  while (Serial.available()) {
    const char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') { handleLine(lineA); lineA = ""; } else if (lineA.length() < 120) lineA += ch;
  }
  while (Serial0.available()) {
    const char ch = (char)Serial0.read();
    if (ch == '\n' || ch == '\r') { handleLine(lineB); lineB = ""; } else if (lineB.length() < 120) lineB += ch;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);   // never block when no host is attached
  Serial0.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2500) delay(10);
  delay(200);
  say("\n==== Knob bring-up firmware (built %s %s) ====\n", __DATE__, __TIME__);
  say("This banner is printed on BOTH the USB port and UART0: note which one you are reading.\n");
  say("reset reason: %s\n", resetReasonStr(esp_reset_reason()));

  prefs.begin("bringup", false);
  qspiMHz = prefs.getUInt("mhz", 40);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  touchReset();
  i2cScan();
  touchProbe();

  pinMode(PIN_BOOT, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_TP_INT, INPUT);
  attachInterrupt(PIN_ENC_A, encIsr, CHANGE);
  attachInterrupt(PIN_ENC_B, encIsr, CHANGE);

  blInit();
  gfxOk = displayInit();
  say("display init (QSPI %u MHz): %s\n", (unsigned)qspiMHz, gfxOk ? "OK" : "FAILED");
  if (gfxOk) {
    gfx->fillScreen(K_BLACK);
    blSet(160);
    patGeometry();
  }
  say("psramFound=%d, flash %u MB\n", (int)psramFound(), (unsigned)(ESP.getFlashChipSize() / 1048576));
  help();
}

void loop() {
  pollSerial();
  touchLoop();
  knobPoll();
  encRawLoop();
  btnLoop();
  adcLiveLoop();
  delay(1);
}

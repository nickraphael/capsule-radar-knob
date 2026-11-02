#pragma once
// M0 bring-up: CO5300 AMOLED (Arduino_GFX over QSPI) + LVGL.
// Owns the panel + LVGL display driver so main.cpp stays glue-only.
// Touch (CST9217 indev) and the radar UI come in later milestones.
#include <stdint.h>

namespace display {

// Init the panel and LVGL (draw buffers in PSRAM) and show the M0 hello screen.
// Returns false if the panel failed to initialize.
bool begin();

// Pump LVGL: render dirty areas + run LVGL timers. Call every loop() iteration.
void loop();

// 0..255 panel brightness (CO5300 command 0x51). 0 = dark (face-down sleep): a touch then
// only wakes the screen, and a press that starts while it is dark never reaches LVGL, up to
// its lift (TouchQueue::setWakeOnly).
void setBrightness(uint8_t v);

// ms since the last touch (LVGL inactivity timer) — for idle auto-dim.
uint32_t inactiveMs();

// Count something other than a touch (a knob click) as activity: restarts the inactivity time,
// so main.cpp's idle logic wakes the screen and sends the idle clock away, as it does for a touch.
// Not available on the LCD 2.1 backend; only the knob build calls it.
void noteActivity();

// One serial line with LVGL's own memory pool: bytes used now, the peak, and the biggest free block.
// A diagnostic for the list wheel (the 64 KB pool is what the list's row code is careful about).
// Not available on the LCD 2.1 backend; only the knob build calls it.
void logLvglMem(const char *tag);

// Frame-time diagnostic: perfReset() clears LVGL's per-frame render-time counters, perfLog() prints
// frames, average and worst render time and pixels drawn since. Called around a swipe between pages.
// Not available on the LCD 2.1 backend; only the knob build calls them.
void perfReset();
void perfLog(const char *tag);

// Page slide (fork-only; not available on the LCD 2.1 backend). transitionReady() says whether it can run
// now (the compositor's PSRAM copy of the panel exists and the display is at rotation 0). Then:
//   captureBegin(); ...make LVGL render the whole next page (lv_refr_now)...; captureEnd();
//   slide(dir, ms);
// captureBegin() diverts everything LVGL flushes into an offscreen picture and leaves the panel alone;
// slide() then moves from the panel's current picture to the captured one, over `ms`, with no LVGL work.
// dir > 0 enters the next page from the right, dir < 0 the previous one from the left.
bool transitionReady();
bool captureBegin();
void captureEnd();
void slideReport(int from, int to, uint32_t tileMs, uint32_t buildMs, uint32_t cardMs, uint32_t renderMs);   // diagnostic line per swipe
void slide(int dir, uint32_t ms);

// Rotate the whole UI clockwise by an arbitrary number of degrees (normalized to
// 0..359), e.g. to compensate for any enclosure/mounting angle. Cardinal rotations
// keep their optimized flush paths; other angles use a PSRAM framebuffer. Touch input
// is transformed back into the same logical coordinate space.
void setRotation(uint16_t degrees);
uint16_t rotation();

// Sweep compositor: draws the radar sweep (and the centre ripple) per pixel over the last
// pushed frame, so LVGL no longer repaints the whole layer stack under them on every tick.
// It also draws the idle clock's hands (radar::clockFrame()), which must move at its rate.
// radar_view hands over colours + on/off (main.cpp wires it); the UI is polled for
// visibility and the objects the sweep must not cover.
bool sweepAvailable();          // buffers allocated: the compositor can own the sweep
void sweepConfig(uint16_t ring565, uint16_t lead565, uint16_t ink565, bool sweep, bool pulse);
void sweepTick();               // advance the sweep if a tick is due; no LVGL, no I2C (safe inside the GPS drain)
float sweepAngleDeg();          // where the sweep line is now: degrees, 0 = up (logical), clockwise

} // namespace display

uint32_t display_frames();   // total rendered frames (for FPS measurement)

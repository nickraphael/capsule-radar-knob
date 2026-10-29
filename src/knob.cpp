// Rotary knob decoder (see knob.h). Fork-only: the real code compiles only where the board
// header defines PIN_ENC_A / PIN_ENC_B.
#include "knob.h"
#if defined(PIN_ENC_A) && defined(PIN_ENC_B)

#include <Arduino.h>
#include <atomic>
#include <driver/gpio.h>
#include <esp_timer.h>

namespace {

constexpr int64_t POLL_US        = 3000;   // the demo's TICKS_INTERVAL (3 ms)
constexpr uint8_t DEBOUNCE_TICKS = 2;      // the demo's DEBOUNCE_TICKS: low for >= 2 samples, then rise

struct Channel { uint8_t prev; uint8_t deb; };
Channel g_a = {1, 0};                      // line A: clockwise
Channel g_b = {1, 0};                      // line B: anticlockwise
std::atomic<int> g_pending{0};             // clicks not yet taken: + clockwise, - anticlockwise
esp_timer_handle_t g_timer = nullptr;

// The state machine of bidi_switch_knob.c's process_knob_channel(), with one difference: the
// low-time counter saturates at DEBOUNCE_TICKS instead of wrapping as a uint8_t, so a knob held
// at a detent for a second or two still counts when it is released.
inline void channel(Channel &c, uint8_t level, int inc) {
    if (level == 0) {
        if (c.prev != 0) c.deb = 0;                         // just fell
        else if (c.deb < DEBOUNCE_TICKS) c.deb++;           // still low
    } else {
        if (c.prev == 0 && ++c.deb >= DEBOUNCE_TICKS) {     // rose after being low long enough
            c.deb = 0;
            g_pending.fetch_add(inc, std::memory_order_relaxed);
        } else {
            c.deb = 0;
        }
    }
    c.prev = level;
}

void onTimer(void *) {
    const uint8_t a = (uint8_t)gpio_get_level((gpio_num_t)PIN_ENC_A);
    const uint8_t b = (uint8_t)gpio_get_level((gpio_num_t)PIN_ENC_B);
    channel(g_a, a, +1);
    channel(g_b, b, -1);
}

}  // namespace

void knob_begin(void) {
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);
    g_a = {(uint8_t)digitalRead(PIN_ENC_A), 0};
    g_b = {(uint8_t)digitalRead(PIN_ENC_B), 0};
    g_pending.store(0);

    esp_timer_create_args_t args = {};
    args.callback = onTimer;
    args.arg = nullptr;
    args.dispatch_method = ESP_TIMER_TASK;     // like the demo: runs in the esp_timer task, not an ISR
    args.name = "knob";
    if (esp_timer_create(&args, &g_timer) == ESP_OK && esp_timer_start_periodic(g_timer, POLL_US) == ESP_OK) {
        Serial.printf("[knob] ready: GPIO %d = clockwise, GPIO %d = anticlockwise, 3 ms sampler\n", PIN_ENC_A, PIN_ENC_B);
    } else {
        Serial.println("[knob] FAILED to start the sampler timer");
    }
}

int knob_take(void) {
    return g_pending.exchange(0, std::memory_order_acq_rel);
}

#endif  // PIN_ENC_A && PIN_ENC_B

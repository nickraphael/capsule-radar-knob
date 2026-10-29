#pragma once
// ---------------------------------------------------------------------------
//  Board: Waveshare ESP32-S3-Knob-Touch-LCD-1.8
//  ESP32-S3R8 (8 MB OPI PSRAM, 16 MB flash), 1.8" ROUND LCD 360x360 over QSPI
//  (ST77916 family, see below), CST816D touch, rotary knob (bidirectional-switch type),
//  DRV2605L haptics, battery on an ADC divider. NO PMIC, NO IMU, NO RTC, NO GPS,
//  NO push button: the only inputs are the knob ring and the touch screen.
//
//  Every pin below comes from Waveshare's own demo (lcd_config.h / user_config.h) and was
//  exercised on the real board with bringup/main.cpp. Do not change one without
//  re-measuring (see context.txt).
// ---------------------------------------------------------------------------

#define BOARD_NAME          "ESP32-S3-Knob-Touch-LCD-1.8"
#define BOARD_PIO_ENV       "esp32-s3-knob-18"     // used in the OTA hint printed at boot

// ---------- Screen (round glass, 360x360) ----------
// The visible area is the circle inscribed in the square: radius 179 was measured fully
// visible; the four corners are NOT visible. RADAR_R_OUTER_PX is PROVISIONAL (owner to
// decide); the other boards leave 4..15 px between the outer ring and the glass edge.
#define SCREEN_W            360
#define SCREEN_H            360
#define SCREEN_CX           180
#define SCREEN_CY           180
#define RADAR_R_OUTER_PX    172

// ---------- Panel: QSPI, ST77916 family ----------
// The controller is not named by Waveshare. It is identified by comparison: the demo's init
// table matches Arduino_GFX's st77916 table in structure and in the C0-CB / D0-D2 blocks, and
// the factory firmware contains an ST77916 driver class. The init table itself is Waveshare's
// panel-tuned one, generated into boards/st77916_knob_init.h by bringup/gen_init.py (that file
// is git-ignored: the demo has no license). Run that script before building this board.
#define PIN_LCD_CS          14
#define PIN_LCD_SCLK        13             // QSPI PCLK
#define PIN_LCD_D0          15
#define PIN_LCD_D1          16
#define PIN_LCD_D2          17
#define PIN_LCD_D3          18
#define PIN_LCD_RST         21
#define LCD_COL_OFFSET      0              // no gap on this panel (the demo draws at 0,0)
#define LCD_ROW_OFFSET      0
// The demo clocks QSPI at 40 MHz. 80 MHz also drew static test patterns cleanly, but sustained
// full-screen LVGL redraws at 80 MHz are NOT tested yet, so start at the demo's value.
#define LCD_QSPI_HZ         40000000

// ---------- Backlight: LEDC PWM on a real GPIO (the panel has no brightness command) ----------
// Same values as the demo's lcd_bl_pwm_bsp.c: 8-bit duty, 50 kHz.
#define PIN_LCD_BL          47
#define LCD_BL_PWM_FREQ     50000
#define LCD_BL_PWM_BITS     8

// ---------- Shared I2C bus (touch + DRV2605L haptics; nothing else is fitted) ----------
#define PIN_I2C_SDA         11
#define PIN_I2C_SCL         12

// ---------- Touch: CST816D, polled ----------
#define TOUCH_DRIVER_CST816D 1
#define I2C_ADDR_TOUCH      0x15
#define PIN_TP_RST          10             // a real GPIO on this board (not an expander)
#define PIN_TP_INT          9
#define TP_MIRROR_X         false          // measured: x grows to the right, 1:1 with pixels
#define TP_MIRROR_Y         false          // measured: y grows downward, 1:1 with pixels

// ---------- Peripherals present on this board ----------
#define BOARD_HAS_PMIC      0              // no AXP2101: battery comes from an ADC pin, see below
#define BOARD_HAS_AUDIO     0              // a PCM5100A DAC exists, but nothing uses it yet

// rtc_pcf85063.cpp compiles against this address. There is no RTC on this board: its probe fails
// and the firmware falls back to NTP, as the other boards do without a stored time.
#define I2C_ADDR_RTC        0x51

// ---------- Knob (bidirectional-switch type; NOT quadrature) ----------
// Waveshare's decoder polls both lines every 3 ms; a line counts as a click when it was low for
// at least 2 consecutive samples and then rises. Line A = RIGHT (clockwise, +1), line B = LEFT
// (anticlockwise, -1). Measured: clockwise -> A, anticlockwise -> B. Internal pull-ups.
#define PIN_ENC_A           8
#define PIN_ENC_B           7

// ---------- Haptics: DRV2605L ----------
// Waveshare's setup: ERM mode, ERM open-loop, library 1, internal trigger.
#define I2C_ADDR_HAPTIC     0x5A
// Effect played on each knob click (library 1, the vendor's numbering): 5 = Sharp Click 60%.
// Tune by feel: 4/5/6 = Sharp Click 100/60/30%, 7/8/9 = Soft Bump 100/60/30%, 1/2/3 = Strong Click.
#define HAPTIC_TICK_EFFECT  5

// While the knob is turning, the aircraft detail card stays hidden; it appears once the ring has
// been still for this long (ms). The selected plane stays highlighted on the scope meanwhile.
#define KNOB_CARD_DELAY_MS  1000

// ---------- Aircraft photo size ----------
// The largest photo shown (px); the image proxy resizes to it. The photo sits above the detail card
// with its bottom edge 28 px above the screen centre, so its TOP corners are the points that reach
// the glass: at 176x117 the worst-case corner is about 170 px from the centre (the glass radius is
// 180). The 466 px boards' 232x156 put that corner about 208 px out, which runs off a round 360.
// Smaller also means less to download and decode, and a smaller PSRAM buffer.
#define PH_MAXW             176
#define PH_MAXH             117

// ---------- Battery sense ----------
// GPIO 1 (ADC1 channel 0) behind a divide-by-two divider; 12 dB attenuation;
// V = raw * 3.3 / 4096 * 2. MEASURED: about 4.7 V on USB (the USB-fed rail), 4.16 V on battery.
// It shows the battery only while USB is NOT connected.
#define PIN_BATT_SENSE      1

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
#define KNOB_CARD_DELAY_MS  1500
// The WiFi signal bars live on the Stats page here, not in the radar's top row.
#define WIFI_BARS_ON_STATS  1
// With an aircraft selected, only ITS flight code and altitude are drawn on the scope; every other aircraft
// loses both. With nothing selected, everything is shown as before.
#define LABELS_SELECTED_ONLY 1
// Airports are bright magenta crosses that can be selected like aircraft (tap a cross, or turn the knob: they
// are stops on the same ring), with an info card (code, name, local time, distance, elevation...) and rows in
// the list with a building icon. The details come from src/airports_info.h (generated by tools/gen_airports.py;
// about 210 KB of flash). Needs "Show airports" switched on, as before.
#define AIRPORT_SELECT       1

// ---------- Aircraft photo size ----------
// The largest photo shown (px); the image proxy resizes to it. The photo sits above the detail card
// with its bottom edge 28 px above the screen centre, so its TOP corners are the points that reach
// the glass: at 176x117 the worst-case corner is about 170 px from the centre (the glass radius is
// 180). The 466 px boards' 232x156 put that corner about 208 px out, which runs off a round 360.
// Smaller also means less to download and decode, and a smaller PSRAM buffer.
#define PH_MAXW             176
#define PH_MAXH             117

// ---------- Aircraft list: a wheel for the knob ----------
// The ring scrolls the list one row per click; the row in the middle is the focus (inside a thin
// outline), rows fade with distance from it, and a finger drag snaps to the nearest row.
// LIST_WHEEL switches this on (the other boards keep the plain list). The list box is sized for the
// round 360 glass: a row of about 210 px of text fits the glass for the rows near the middle, and the
// outer rows are faded. ROUND_PANEL_D is the round panel behind the list/stats tiles: the 466 px
// boards use 462, which on this screen would stand 51 px above the glass and put the tile titles
// ("AIRCRAFT", "STATS") off-screen.
// The AC_ prefix is deliberate: a bare LIST_H is FreeRTOS's list.h include guard, and defining it
// here silently skips that header and breaks the build.
#define LIST_WHEEL          1
#define ROUND_PANEL_D       360
#define AC_LIST_W           318     // the glass is ~340 px wide at the middle rows; the worst second line needs ~290 px of text
#define AC_LIST_H           252
#define AC_ROW_H            48      // every row is exactly TWO lines of size-14 text; a fixed height is what makes overlong text end in "..." instead of growing
#define AC_LIST_Y_OFFSET    14      // below the centre, clear of the title
// The aircraft icon spans both text lines at the right of each row (32 px, twice its design size).
// AC_ICON_PAD is the strip reserved at the row's right edge (text is centred in what is left of it);
// AC_ICON_CX is the distance from the row's right edge to the icon's centre. The icon is kept toward
// the middle of the row, not hard against the edge, so it clears the curve of the glass on the rows
// above and below the focus.
#define AC_ICON_PAD         52
#define AC_ICON_CX          34
// Icons on rows other than the outlined (selected) one are fainter: this is the strength, in percent of
// the row's own fade, that an icon reaches one row away from the middle (100 = no extra fade).
#define AC_ICON_FAINT_PCT   35

// ---------- Radar selection ----------
// Tapping the radar's background hides the aircraft card but KEEPS the selection (other boards clear it).
// Tapping the selected aircraft again brings the card back; the knob's "none" stop is how you deselect.
#define KEEP_SELECTION_ON_BG_TAP 1
// The selected aircraft is marked by a small circle that pulses, so it still shows with the card hidden:
// SEL_FLASH_MS per cycle, between SEL_FLASH_MIN_OPA (out of 255) and full strength.
#define SEL_FLASH_MS        1000
#define SEL_FLASH_MIN_OPA   45

// ---------- Stats and weather tiles on the round 360 glass ----------
// Stats: offsets from the panel centre (px). The stats block is 7 lines of size-16 text (~126 px tall),
// the footer is 3 lines of size 14 (~48 px; widest line ~255 px: IP and coordinates) and the version
// line is size 12. The 466 px layout put the footer 77 px below the stats and the version line at +170,
// where a 360 glass is only ~75 px wide, so both ran off the bottom. These group them (14 px between
// stats and footer) and centre the group vertically under the title.
#define STATS_LBL_Y         -43
#define STATS_NET_Y         76
#define STATS_VER_Y         114
// The "Wifi strength" label and its signal bars sit on one line here, above the network text (which is three
// lines, so its top is about 26 px above STATS_NET_Y). Fixed, not derived from STATS_NET_Y, so moving that text
// does not drag the bars with it.
#define STATS_WIFI_Y        36
// Weather: its layout is positioned from the panel's TOP edge, for a 462 px panel. Centred on this
// glass, a 462 px panel puts the 360 px radar circle in the middle (1 px off). Do not shrink it to
// ROUND_PANEL_D: that moved the whole weather page about 51 px down.
#define WEATHER_PANEL_D     462
// The weather page's background is a plain rectangle, not the masked circle the other pages use. LVGL
// pushes every pixel drawn on a clipped circular panel through a mask on every frame, which made swiping
// to this page laggy; here nothing visible changes (the bezel hides the corners, and this panel's rim is
// off-screen).
#define WEATHER_PANEL_PLAIN true
// The title names the weather mode (WX RADAR / SAT CLOUDS / WEATHER). Panel-relative y: the 462 px panel
// stands 51 px above the glass, so 71 puts the title 20 px from the top of the glass.
#define WEATHER_TITLE_Y     71
// The title names the mode and the ring (or a tap anywhere) changes it, so the button at the bottom,
// which only shows the NEXT mode's name, is dropped on this board.
#define WEATHER_NO_MODE_BTN 1
// The "nearest airport to the radar centre" label is positioned above the picture for the bigger
// boards; on this glass it is half above the screen and shows as clipped text at the top.
#define WEATHER_NO_AIRPORT  1
// The radar / cloud picture is a 360 px crop shown 1:1, which fills this 360 px glass edge to edge and
// looks zoomed in. The 466 px boards draw it at 77% of the screen. 280 px is 78% of this glass; the
// picture is scaled about its centre (with its rings and labels), so it shows the same area, smaller.
#define WX_VIEW_D           280
// Forecast page: the bigger boards' layout starts the big temperature 17 px from the top of this glass,
// right where the title now sits, so the two overprinted. FC_HEAD_DY moves the temperature and the
// condition line down clear of the title; FC_METRIC_DY moves the FEELS / HUMIDITY / WIND row down to
// keep its gap from the condition. The day columns and "UPDATED" already fit and stay put.
// 47 / 25 leave room for the location line (WEATHER_LOC_Y) between the title and the temperature:
// title 20-44, location 46-60, temperature 64-96, condition 101-120, metrics 124-163, days from 175.
#define FC_HEAD_DY          47
#define FC_METRIC_DY        25
// Location line on the weather page (panel-relative y; the panel stands 51 px above the glass, so 97 is
// 46 px from the top, just under the title): "33.87S 151.21E | SYD 20 km S". The "N" marker moves down
// to sit just under it.
#define WEATHER_LOC_Y       97
#define WX_NORTH_Y          117
// The distance label ("75 KM") is the radius of the picture, i.e. the range of the OUTER ring, so it
// sits on the outer ring (right-hand side) rather than near the middle ring as on the bigger boards.
#define WX_RANGE_ON_OUTER_RING 1

// ---------- Defaults and identity (knob fork) ----------
// config.h defines these for the upstream project BEFORE this header is included, so they are
// #undef'd and redefined here (a plain #define would only warn, and the earlier value could win).
//
// The idle clock (or dim) starts after 60 s without a touch or knob turn; the shared default is 20 s.
// A value saved from the config page ("Clock or dim after") still wins over this default.
#undef  IDLE_DIM_MS
#define IDLE_DIM_MS         60000
// The board always opens on this page (0 radar, 1 list, 2 stats, 3 weather) instead of restoring the one
// last left, which is what the other boards do.
#define BOOT_VIEW           0
// A swipe between pages becomes a smooth slide of two finished pictures instead of LVGL redrawing the whole
// screen on every frame of a finger drag (about 85 ms a frame here, so about 12 fps). The page does NOT
// follow the finger: the swipe is recognised as soon as it starts and the slide then runs by itself over
// SWIPE_SLIDE_MS. Falls back to LVGL's normal drag if the display is rotated or the PSRAM picture is missing.
// Delete the SWIPE_SLIDE line to go back to the plain drag.
#define SWIPE_SLIDE         1
#define SWIPE_SLIDE_MS      280
// The weather page opens on the 3-day forecast instead of the radar.
#define WEATHER_DEFAULT_FORECAST 1
// This fork's own name and version: the Stats page, the config page footer and the boot log.
#undef  FW_VERSION
#define FW_VERSION          "1.0.0"
#define FW_NAME             "Capsule Radar Knob"

// ---------- Battery sense ----------
// GPIO 1 (ADC1 channel 0) behind a divide-by-two divider; 12 dB attenuation;
// V = raw * 3.3 / 4096 * 2. MEASURED: about 4.7 V on USB (the USB-fed rail), 4.16 V on battery.
// It shows the battery only while USB is NOT connected.
#define PIN_BATT_SENSE      1

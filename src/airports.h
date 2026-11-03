#pragma once
// Airport markers for the radar scope. Projects the embedded OurAirports list
// (airports_data.h) like the coastline: cull to the scope, great-circle project,
// cache screen markers, draw in the static chrome layer. Large airports get a small
// ring + IATA label; medium airports are a faint dot. Projection is done only on a
// home/range change, never per frame.
//
// Fork-only (boards that define AIRPORT_SELECT): the markers are bright crosses, and the airports inside
// the scope can be selected like aircraft (tap, knob ring, list) and described on the info card, from the
// details table in airports_info.h (name, city, country, elevation, ICAO code, time-zone rule).
#include <stdint.h>
#include <lvgl.h>

// The colour of an airport mark on the scope and in the list (aviation charts draw airports in magenta; it
// also keeps clear of the altitude colours the aircraft use).
#define AIRPORT_MARK_HEX 0xFF4FD8

void airports_project(double homeLat, double homeLon, double rangeKm,
                      float cx, float cy, float rOuterPx);

void airports_draw(lv_draw_ctx_t *ctx, lv_color_t color, lv_opa_t opa);

// Find the nearest recognizable airport (one with an IATA code). Used by the
// weather view for aviation context; entirely offline from the embedded data.
bool airports_nearest_iata(double lat, double lon, float maxKm,
                           char iata[4], float *distKm, float *bearingDeg);

// ---- the airports inside the scope (the ones airports_project last kept) ----
struct APInfo {
    int         idx;            // index into the embedded table: stable, and what a selection's key carries
    char        iata[4];
    char        icao[5];        // empty when unknown (or without the details table)
    char        cc[3];          // ISO country code
    char        name[56];       // compact ASCII name, e.g. "Edinburgh", "Sydney Kingsford Smith Intl"
    char        place[40];      // city / municipality (may be empty)
    int         elevFt;         // feet; -9999 when unknown
    float       distKm;         // from the scope's home
    float       bearingDeg;
    bool        large;
    const char *tz;             // POSIX TZ rule for the local time, or nullptr
    lv_point_t  pos;            // where it sits on the scope (screen px)
};

int  airports_visible_count(void);                       // how many are inside the scope
bool airports_visible_info(int k, APInfo *out);          // the k-th of them, nearest first
bool airports_visible_mark(int k, lv_point_t *pt, bool *large);   // just where the k-th sits and its size: cheap, for per-tick use
bool airports_info_by_index(int idx, APInfo *out);       // by table index; false unless it is inside the scope
bool airports_pos(int idx, lv_point_t *pt);              // its screen position, if it is inside the scope
int  airports_hit(int x, int y, int radiusPx);           // table index of the one nearest (x,y) within radiusPx, or -1

// Local time at an airport at the UTC instant `utc` (seconds since the epoch): hours, minutes, and the offset
// from UTC in minutes. False when the airport has no time-zone rule.
bool airports_local_time(const APInfo &a, int64_t utc, int *hh, int *mm, int *offMin);

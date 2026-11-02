#include "airports.h"
#include "config.h"                 // AIRPORT_SELECT comes from the board header
#include "airports_data.h"
#if defined(AIRPORT_SELECT)
#include "airports_info.h"          // names, countries, elevations, time zones (about 210 KB of flash)
#include "posix_tz.h"
#endif
#include "geo.h"
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <string.h>

struct Apt { lv_point_t pos; char iata[4]; uint8_t large; int idx; float distKm, bearingDeg; };
static std::vector<Apt> s_apts;

void airports_project(double homeLat, double homeLon, double rangeKm,
                      float cx, float cy, float rOuterPx) {
    s_apts.clear();
    if (rangeKm <= 0) return;

    const double rangeDeg  = rangeKm / 111.0;
    const double latMargin = rangeDeg * 1.05;
    const double cosLat    = cos(homeLat * M_PI / 180.0);
    const double lonMargin = latMargin / (cosLat < 0.15 ? 0.15 : cosLat);

    for (int i = 0; i < AIRPORT_NUM; ++i) {
        const double lat = AIRPORT_LAT[i] / (double)AIRPORT_SCALE;
        const double lon = AIRPORT_LON[i] / (double)AIRPORT_SCALE;
        const double dlon = lon - homeLon;
        if (fabs(lat - homeLat) > latMargin) continue;                      // cheap bbox reject
        if (fabs(dlon) > lonMargin && fabs(fabs(dlon) - 360.0) > lonMargin) continue;
        const double dist = geo::haversineKm(homeLat, homeLon, lat, lon);
        if (dist > rangeKm) continue;                                       // only inside the scope
        const double brg = geo::bearingDeg(homeLat, homeLon, lat, lon);
        const double rPx = (dist / rangeKm) * rOuterPx;
        const double a   = brg * M_PI / 180.0;
        Apt ap;
        ap.pos.x = (lv_coord_t)lroundf((float)(cx + rPx * sin(a)));
        ap.pos.y = (lv_coord_t)lroundf((float)(cy - rPx * cos(a)));
        memcpy(ap.iata, AIRPORT_IATA[i], 4);
        ap.large = AIRPORT_LARGE[i];
        ap.idx = i;
        ap.distKm = (float)dist;
        ap.bearingDeg = (float)brg;
        s_apts.push_back(ap);
    }
    // Nearest first: the list shows them in this order, and it is cheap (a handful of airports at most).
    std::sort(s_apts.begin(), s_apts.end(), [](const Apt &a, const Apt &b) { return a.distKm < b.distKm; });
}

void airports_draw(lv_draw_ctx_t *ctx, lv_color_t color, lv_opa_t opa) {
    if (s_apts.empty()) return;
#if defined(AIRPORT_SELECT)
    // Bright little crosses, each with a dark halo so it stands out over the coastline, the rings and the
    // aircraft trails. Large airports are a little bigger and keep their IATA label.
    lv_draw_line_dsc_t halo, bar;
    lv_draw_line_dsc_init(&halo);
    halo.color = lv_color_black();
    halo.width = 5;
    halo.opa = (lv_opa_t)(opa / 2);
    halo.round_start = halo.round_end = 0;
    lv_draw_line_dsc_init(&bar);
    bar.color = color;
    bar.width = 2;
    bar.opa = opa;
    bar.round_start = bar.round_end = 0;
    lv_draw_label_dsc_t tag;
    lv_draw_label_dsc_init(&tag);
    tag.color = color;
    tag.opa = opa;
    tag.font = &lv_font_montserrat_12;
    for (const Apt &ap : s_apts) {
        const lv_coord_t r = ap.large ? 7 : 5;
        const lv_point_t h0 = { (lv_coord_t)(ap.pos.x - r), ap.pos.y }, h1 = { (lv_coord_t)(ap.pos.x + r), ap.pos.y };
        const lv_point_t v0 = { ap.pos.x, (lv_coord_t)(ap.pos.y - r) }, v1 = { ap.pos.x, (lv_coord_t)(ap.pos.y + r) };
        lv_draw_line(ctx, &halo, &h0, &h1);
        lv_draw_line(ctx, &halo, &v0, &v1);
        lv_draw_line(ctx, &bar, &h0, &h1);
        lv_draw_line(ctx, &bar, &v0, &v1);
        if (ap.large && ap.iata[0]) {
            lv_area_t la = { (lv_coord_t)(ap.pos.x + r + 3), (lv_coord_t)(ap.pos.y - 7),
                             (lv_coord_t)(ap.pos.x + r + 42), (lv_coord_t)(ap.pos.y + 7) };
            lv_draw_label(ctx, &tag, &la, ap.iata, NULL);
        }
    }
    return;
#endif

    lv_draw_arc_dsc_t ring;
    lv_draw_arc_dsc_init(&ring);
    ring.color = color; ring.width = 2; ring.opa = opa;

    lv_draw_rect_dsc_t dot;
    lv_draw_rect_dsc_init(&dot);
    dot.bg_color = color; dot.bg_opa = opa; dot.radius = LV_RADIUS_CIRCLE;

    lv_draw_label_dsc_t lbl;
    lv_draw_label_dsc_init(&lbl);
    lbl.color = color; lbl.opa = opa; lbl.font = &lv_font_montserrat_12;

    for (const Apt &ap : s_apts) {
        if (ap.large) {
            lv_draw_arc(ctx, &ring, &ap.pos, 3, 0, 360);                    // small hollow ring
            if (ap.iata[0]) {
                lv_area_t la = { (lv_coord_t)(ap.pos.x + 5), (lv_coord_t)(ap.pos.y - 7),
                                 (lv_coord_t)(ap.pos.x + 44), (lv_coord_t)(ap.pos.y + 7) };
                lv_draw_label(ctx, &lbl, &la, ap.iata, NULL);
            }
        } else {
            lv_area_t d = { (lv_coord_t)(ap.pos.x - 1), (lv_coord_t)(ap.pos.y - 1),
                            (lv_coord_t)(ap.pos.x + 1), (lv_coord_t)(ap.pos.y + 1) };
            lv_draw_rect(ctx, &dot, &d);                                    // faint dot
        }
    }
}

// Nearest airport that has an IATA code.
//
// The obvious version ran a double-precision great-circle calculation on all 4,566 airports. The
// ESP32-S3's FPU is single-precision only, so every double sin/cos/asin/sqrt ran in software and one call
// took about 0.7 SECONDS. The weather page called it each time it was built (twice on a radar page), which
// froze the whole UI for that long every time the page came into view.
//
// Now: a cheap single-precision flat-earth distance ranks every airport (about a millisecond for all of
// them), and the exact great-circle distance is computed only for the few within a small margin of the
// best flat distance, so the answer is the same as the exhaustive search. The flat-earth error over the
// distances this is used for (a few hundred km at most) is far below that margin.
bool airports_nearest_iata(double lat, double lon, float maxKm,
                           char iata[4], float *distKm, float *bearingDeg) {
    if (iata) iata[0] = 0;
    const float flat = (float)lat, flon = (float)lon;
    const float kLat = 111.19f;                                     // km per degree of latitude
    const float kLon = kLat * cosf(flat * 0.017453293f);            // km per degree of longitude at this latitude
    const float invScale = 1.0f / (float)AIRPORT_SCALE;

    auto flatKm2 = [&](int i) -> float {                            // squared flat-earth distance (km^2) to airport i
        const float dy = ((float)AIRPORT_LAT[i] * invScale - flat) * kLat;
        float dlon = (float)AIRPORT_LON[i] * invScale - flon;
        if (dlon > 180.0f) dlon -= 360.0f; else if (dlon < -180.0f) dlon += 360.0f;
        const float dx = dlon * kLon;
        return dx * dx + dy * dy;
    };

    float bestFlat2 = 3.0e38f;
    for (int i = 0; i < AIRPORT_NUM; ++i) {
        if (!AIRPORT_IATA[i][0]) continue;
        const float d2 = flatKm2(i);
        if (d2 < bestFlat2) bestFlat2 = d2;
    }
    if (bestFlat2 > 1.0e38f) return false;                          // no airport with an IATA code at all
    const float bestFlat = sqrtf(bestFlat2);
    if (bestFlat > maxKm * 1.05f + 5.0f) return false;              // nothing near enough, whatever the exact figure
    const float limit = bestFlat * 1.03f + 2.0f;                    // exact check only within 3% + 2 km of the flat best
    const float limit2 = limit * limit;

    double best = maxKm;
    int bestIdx = -1;
    for (int i = 0; i < AIRPORT_NUM; ++i) {
        if (!AIRPORT_IATA[i][0]) continue;
        if (flatKm2(i) > limit2) continue;
        const double alat = AIRPORT_LAT[i] / (double)AIRPORT_SCALE;
        const double alon = AIRPORT_LON[i] / (double)AIRPORT_SCALE;
        const double d = geo::haversineKm(lat, lon, alat, alon);
        if (d < best) { best = d; bestIdx = i; }
    }
    if (bestIdx < 0) return false;
    if (iata) memcpy(iata, AIRPORT_IATA[bestIdx], 4);
    if (distKm) *distKm = (float)best;
    if (bearingDeg) {
        const double alat = AIRPORT_LAT[bestIdx] / (double)AIRPORT_SCALE;
        const double alon = AIRPORT_LON[bestIdx] / (double)AIRPORT_SCALE;
        *bearingDeg = (float)geo::bearingDeg(lat, lon, alat, alon);
    }
    return true;
}

// ===================== the airports inside the scope, as things that can be selected =====================

// An APInfo from a kept marker, plus the details table when this build carries it.
static void ap_fill(const Apt &a, APInfo *out) {
    memset(out, 0, sizeof(*out));
    out->idx = a.idx;
    memcpy(out->iata, a.iata, 4);
    out->large = a.large != 0;
    out->pos = a.pos;
    out->distKm = a.distKm;
    out->bearingDeg = a.bearingDeg;
    out->elevFt = -9999;
    out->tz = nullptr;
#if defined(AIRPORT_SELECT)
    snprintf(out->name, sizeof(out->name), "%s", AIRPORT_STR + AIRPORT_STR_OFF[2 * a.idx]);
    snprintf(out->place, sizeof(out->place), "%s", AIRPORT_STR + AIRPORT_STR_OFF[2 * a.idx + 1]);
    memcpy(out->icao, AIRPORT_ICAO[a.idx], 5);
    memcpy(out->cc, AIRPORT_CC[a.idx], 3);
    out->elevFt = AIRPORT_ELEV[a.idx];
    out->tz = AIRPORT_TZ[AIRPORT_TZIDX[a.idx]];
#endif
}

int airports_visible_count(void) { return (int)s_apts.size(); }

bool airports_visible_info(int k, APInfo *out) {
    if (k < 0 || k >= (int)s_apts.size()) return false;
    ap_fill(s_apts[k], out);
    return true;
}

bool airports_info_by_index(int idx, APInfo *out) {
    for (const Apt &a : s_apts)
        if (a.idx == idx) { ap_fill(a, out); return true; }
    return false;
}

bool airports_pos(int idx, lv_point_t *pt) {
    for (const Apt &a : s_apts)
        if (a.idx == idx) { *pt = a.pos; return true; }
    return false;
}

int airports_hit(int x, int y, int radiusPx) {
    int best = -1;
    long bestD = (long)radiusPx * radiusPx;
    for (const Apt &a : s_apts) {
        const long dx = (long)a.pos.x - x, dy = (long)a.pos.y - y;
        const long dd = dx * dx + dy * dy;
        if (dd <= bestD) { bestD = dd; best = a.idx; }
    }
    return best;
}

bool airports_local_time(const APInfo &a, int64_t utc, int *hh, int *mm, int *offMin) {
#if defined(AIRPORT_SELECT)
    int off = 0;
    if (!a.tz || !ptz::offset_at(a.tz, utc, &off)) return false;
    const int64_t local = utc + off;
    const int sod = (int)(((local % 86400) + 86400) % 86400);          // seconds into the local day
    *hh = sod / 3600;
    *mm = (sod % 3600) / 60;
    *offMin = off / 60;
    return true;
#else
    (void)a; (void)utc; (void)hh; (void)mm; (void)offMin;
    return false;
#endif
}

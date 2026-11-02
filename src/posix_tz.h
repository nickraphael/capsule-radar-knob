#pragma once
// Local time for an arbitrary POSIX TZ rule string, e.g. "GMT0BST,M3.5.0/1,M10.5.0", "<+0530>-5:30" or
// "AEST-10AEDT,M10.1.0,M4.1.0/3", WITHOUT touching the process-wide TZ (setenv/tzset): the device's own
// clock keeps its zone, and this can be asked about any other zone, any number of times, from any task.
// Pure integer arithmetic. Supports the forms the IANA database writes in its zone files' footers:
// Mm.w.d, Jn and n rules, a /time that may be negative or over 24 h, and "negative DST" (Europe/Dublin).
//
// Tested against Python's zoneinfo for every zone in the IANA database, at random moments and at the exact
// second of every offset change from 2026 to 2034 (157,836 cases): identical, except Morocco and Western
// Sahara, whose Ramadan-dependent schedule is stored as explicit dates and has no POSIX rule.
#include <stdint.h>

namespace ptz {

// Days since 1970-01-01 of a civil (proleptic Gregorian) date.
static inline int64_t days_from_civil(int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

static inline int64_t year_of_days(int64_t z) {                       // civil year containing day number z
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int64_t y = (int64_t)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned m = mp < 10 ? mp + 3 : mp - 9;
    return y + (m <= 2);
}

static inline bool is_leap(int64_t y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

struct Rule { int kind; int m, w, d; int64_t timeSec; };              // kind: 1 = Mm.w.d, 2 = Jn, 3 = n (zero based)

struct Zone {
    int  stdEast = 0;        // seconds east of UTC, standard time
    int  dstEast = 0;        // seconds east of UTC, daylight time
    bool hasDst  = false;
    Rule start{}, end{};
};

static inline bool parse_name(const char *&p) {
    if (*p == '<') {
        while (*p && *p != '>') ++p;
        if (*p != '>') return false;
        ++p;
        return true;
    }
    int n = 0;
    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) { ++p; ++n; }
    return n >= 3;
}

static inline bool parse_uint(const char *&p, int *out) {
    if (*p < '0' || *p > '9') return false;
    int v = 0;
    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); ++p; }
    *out = v;
    return true;
}

// [+|-]hh[:mm[:ss]]  ->  seconds, the sign as written.
static inline bool parse_hms(const char *&p, int64_t *out) {
    int sign = 1;
    if (*p == '+') ++p;
    else if (*p == '-') { sign = -1; ++p; }
    int h, m = 0, s = 0;
    if (!parse_uint(p, &h)) return false;
    if (*p == ':') { ++p; if (!parse_uint(p, &m)) return false; if (*p == ':') { ++p; if (!parse_uint(p, &s)) return false; } }
    *out = sign * ((int64_t)h * 3600 + m * 60 + s);
    return true;
}

static inline bool parse_rule(const char *&p, Rule *r) {
    r->timeSec = 7200;                                                // default 02:00:00
    if (*p == 'M') {
        ++p;
        r->kind = 1;
        if (!parse_uint(p, &r->m) || *p != '.') return false;
        ++p;
        if (!parse_uint(p, &r->w) || *p != '.') return false;
        ++p;
        if (!parse_uint(p, &r->d)) return false;
    } else if (*p == 'J') {
        ++p;
        r->kind = 2;
        if (!parse_uint(p, &r->d)) return false;
    } else {
        r->kind = 3;
        if (!parse_uint(p, &r->d)) return false;
    }
    if (*p == '/') { ++p; if (!parse_hms(p, &r->timeSec)) return false; }
    return true;
}

static inline bool parse(const char *tz, Zone *z) {
    if (!tz || !*tz) return false;
    const char *p = tz;
    if (!parse_name(p)) return false;
    int64_t off;
    if (!parse_hms(p, &off)) return false;
    z->stdEast = (int)(-off);                                         // POSIX offsets are positive WEST of UTC
    z->hasDst = false;
    z->dstEast = z->stdEast;
    if (!*p) return true;
    if (!parse_name(p)) return false;
    z->hasDst = true;
    z->dstEast = z->stdEast + 3600;
    if (*p && *p != ',') {
        if (!parse_hms(p, &off)) return false;
        z->dstEast = (int)(-off);
    }
    if (*p == ',') {
        ++p;
        if (!parse_rule(p, &z->start)) return false;
        if (*p != ',') return false;
        ++p;
        if (!parse_rule(p, &z->end)) return false;
    } else {                                                          // POSIX default (the US rule)
        z->start = { 1, 3, 2, 0, 7200 };
        z->end   = { 1, 11, 1, 0, 7200 };
    }
    return *p == '\0';
}

// The local calendar day (days since the epoch) a rule falls on in `year`, plus its time of day.
static inline int64_t rule_local_seconds(const Rule &r, int64_t year) {
    int64_t day;
    if (r.kind == 1) {
        const int64_t first = days_from_civil(year, (unsigned)r.m, 1);
        const int wd = (int)(((first % 7) + 7 + 4) % 7);              // 0 = Sunday (1970-01-01 was a Thursday)
        day = first + ((r.d - wd + 7) % 7) + (int64_t)(r.w - 1) * 7;
        const int64_t next = days_from_civil(year + (r.m == 12), (unsigned)(r.m % 12 + 1), 1);
        if (day >= next) day -= 7;                                    // week 5 means "the last one"
    } else if (r.kind == 2) {                                         // Jn: 1..365, February 29 never counted
        day = days_from_civil(year, 1, 1) + (r.d - 1) + ((is_leap(year) && r.d >= 60) ? 1 : 0);
    } else {                                                          // n: 0..365, February 29 counted
        day = days_from_civil(year, 1, 1) + r.d;
    }
    return day * 86400 + r.timeSec;
}

// Seconds east of UTC at the instant `utc` (seconds since the epoch). False if the rule cannot be parsed.
static inline bool offset_at(const char *tz, int64_t utc, int *offEast, bool *isDst = nullptr) {
    Zone z;
    if (!parse(tz, &z)) return false;
    bool dst = false;
    if (z.hasDst) {
        const int64_t year = year_of_days((utc + z.stdEast) / 86400 - ((utc + z.stdEast) % 86400 < 0 ? 1 : 0));
        const int64_t startUtc = rule_local_seconds(z.start, year) - z.stdEast;   // DST begins at local STANDARD time
        const int64_t endUtc   = rule_local_seconds(z.end,   year) - z.dstEast;   // ...and ends at local DAYLIGHT time
        dst = (startUtc < endUtc) ? (utc >= startUtc && utc < endUtc)
                                  : (utc >= startUtc || utc < endUtc);            // southern hemisphere: DST spans New Year
    }
    *offEast = dst ? z.dstEast : z.stdEast;
    if (isDst) *isDst = dst;
    return true;
}

} // namespace ptz

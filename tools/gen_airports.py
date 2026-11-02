#!/usr/bin/env python3
"""Generate src/airports_data.h (and optionally src/airports_info.h) from the OurAirports airports.csv.

airports_data.h keeps large airports (always) and medium airports that have an IATA code, so the
map shows the meaningful/scheduled fields without the tens of thousands of tiny strips. It emits int16
lat/lon (degrees * 100), a 3-letter IATA code, and a 'large' flag.

airports_info.h (the knob fork's airport info card and list) carries the DETAILS of the very same airports,
in the very same order, so index i means the same airport in both files:
  name, city, country code, elevation, ICAO code, and a time-zone rule (a POSIX TZ string such as
  "GMT0BST,M3.5.0/1,M10.5.0") so the device can show the airport's local time, daylight saving included.
Names are folded to plain ASCII (the display fonts have no accents) and compacted ("... International
Airport" -> "... Intl"). Time zones come from the airport's coordinates (timezonefinder), and the rule
text from the IANA database's own zone files (tzdata). Both are build-time only: nothing of either is on
the device except the resulting tables.

Usage:
    python3 tools/gen_airports.py airports.csv src/airports_data.h [src/airports_info.h]

The third argument is optional. Without it only airports_data.h is written (no extra packages needed).
For it:   pip install "timezonefinder<6.2" "h3<4" numpy tzdata
(Newer timezonefinder releases need a C compiler to install; this older one is plain Python.)

Source data: OurAirports (public domain). https://ourairports.com/data/
"""
import csv
import sys
import unicodedata

SCALE = 100.0


# ----------------------------------------------------------------------------- text helpers
def ascii_fold(s):
    """Plain ASCII: drop accents, map the common typographic marks, drop what is left."""
    s = (s or "").replace("\u2019", "'").replace("\u2018", "'").replace("\u2013", "-").replace("\u2014", "-")
    s = unicodedata.normalize("NFKD", s)
    s = "".join(c for c in s if not unicodedata.combining(c))
    s = s.encode("ascii", "ignore").decode("ascii")
    return " ".join(s.split())          # collapse runs of whitespace


def compact_name(name, maxlen=47):
    s = ascii_fold(name)
    for old, new in ((" International Airport", " Intl"), (" Int'l Airport", " Intl"), (" International", " Intl"),
                     (" Airport", ""), (" Airfield", ""), (" Aerodrome", "")):
        if s.endswith(old):
            s = s[: -len(old)] + new
            break
    s = s.strip() or ascii_fold(name)
    return s[:maxlen].rstrip()


def c_escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


# ----------------------------------------------------------------------------- time zones
def load_tz_tools():
    try:
        from timezonefinder import TimezoneFinder
        from importlib.resources import files
    except Exception as e:                          # noqa: BLE001
        sys.exit("airports_info.h needs timezonefinder and tzdata: pip install \"timezonefinder<6.2\" \"h3<4\" numpy tzdata\n(%s)" % e)
    try:
        import tzdata  # noqa: F401
    except Exception as e:                          # noqa: BLE001
        sys.exit("airports_info.h needs the tzdata package: pip install tzdata (%s)" % e)
    return TimezoneFinder(), files("tzdata.zoneinfo")


def posix_rule(zone_root, zone):
    """The POSIX TZ string from the footer of an IANA zone file ('...\\n<rule>\\n'), or None."""
    node = zone_root
    for part in zone.split("/"):
        node = node / part
    try:
        data = node.read_bytes()
    except Exception:                               # noqa: BLE001
        return None
    if data[:4] != b"TZif" or data[4:5] < b"2":
        return None
    rule = data.rstrip(b"\n").rsplit(b"\n", 1)[-1].decode("ascii", "replace")
    return rule or None


# ----------------------------------------------------------------------------- main
def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "/tmp/airports.csv"
    dst = sys.argv[2] if len(sys.argv) > 2 else "src/airports_data.h"
    info_dst = sys.argv[3] if len(sys.argv) > 3 else None

    rows = []          # (lat_s, lon_s, iata, large, lat, lon, details)
    with open(src, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            t = r["type"]
            iata = (r.get("iata_code") or "").strip().upper()[:3]
            if t == "large_airport":
                large = 1
            elif t == "medium_airport" and iata:
                large = 0
            else:
                continue
            try:
                lat = float(r["latitude_deg"])
                lon = float(r["longitude_deg"])
            except (ValueError, KeyError):
                continue
            lat_s = max(-9000, min(9000, round(lat * SCALE)))
            lon_s = max(-18000, min(18000, round(lon * SCALE)))
            icao = (r.get("icao_code") or "").strip().upper()
            if not icao:
                ident = (r.get("ident") or "").strip().upper()
                icao = ident if len(ident) == 4 and ident.isalnum() else ""
            try:
                elev = int(float(r.get("elevation_ft") or ""))
                elev = max(-1500, min(32000, elev))
            except ValueError:
                elev = -9999
            details = dict(name=compact_name(r.get("name", "")), place=ascii_fold(r.get("municipality", ""))[:31],
                           cc=(r.get("iso_country") or "").strip().upper()[:2], elev=elev, icao=icao[:4])
            rows.append((lat_s, lon_s, iata, large, lat, lon, details))

    n_large = sum(1 for r in rows if r[3])
    with open(dst, "w", encoding="ascii") as f:
        f.write("// Auto-generated by tools/gen_airports.py - DO NOT EDIT.\n")
        f.write("// Source: OurAirports (public domain).\n")
        f.write("// %d airports (%d large w/ label, %d medium). lat,lon are int16 deg*%d.\n"
                % (len(rows), n_large, len(rows) - n_large, int(SCALE)))
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write("#define AIRPORT_SCALE %d\n" % int(SCALE))
        f.write("#define AIRPORT_NUM %d\n\n" % len(rows))

        f.write("static const int16_t AIRPORT_LAT[AIRPORT_NUM] = {\n")
        for i in range(0, len(rows), 18):
            f.write("  " + ",".join(str(r[0]) for r in rows[i:i + 18]) + ",\n")
        f.write("};\n\n")

        f.write("static const int16_t AIRPORT_LON[AIRPORT_NUM] = {\n")
        for i in range(0, len(rows), 18):
            f.write("  " + ",".join(str(r[1]) for r in rows[i:i + 18]) + ",\n")
        f.write("};\n\n")

        f.write("// 1 = large airport (gets an IATA label), 0 = medium (marker only).\n")
        f.write("static const uint8_t AIRPORT_LARGE[AIRPORT_NUM] = {\n")
        for i in range(0, len(rows), 32):
            f.write("  " + ",".join(str(r[3]) for r in rows[i:i + 32]) + ",\n")
        f.write("};\n\n")

        f.write("// 3-letter IATA code (may be empty for a large airport without one).\n")
        f.write("static const char AIRPORT_IATA[AIRPORT_NUM][4] = {\n")
        for i in range(0, len(rows), 12):
            f.write("  " + ",".join('"%s"' % r[2] for r in rows[i:i + 12]) + ",\n")
        f.write("};\n")

    bytes_total = len(rows) * (2 + 2 + 1 + 4)
    print("airports: %d (%d large, %d medium)" % (len(rows), n_large, len(rows) - n_large))
    print("flash:    ~%.1f KB" % (bytes_total / 1024.0))
    print("written:  %s" % dst)

    if not info_dst:
        return

    # ---- details, with a time zone for every airport
    tf, zone_root = load_tz_tools()
    zone_of = []
    missing_zone = 0
    for (lat_s, lon_s, iata, large, lat, lon, d) in rows:
        z = tf.timezone_at(lat=lat, lng=lon)
        if z is None or z.startswith("Etc/"):                       # offshore or boundary: take the nearest real zone
            z2 = tf.closest_timezone_at(lat=lat, lng=lon, delta_degree=3)
            if z2:
                z = z2
        zone_of.append(z)
        if z is None:
            missing_zone += 1
    rules, rule_idx, zone_rule, bad_zones = [], {}, {}, set()
    tz_index = []
    for z in zone_of:
        if z not in zone_rule:
            rule = posix_rule(zone_root, z) if z else None
            if rule is None:
                bad_zones.add(z)
                rule = "UTC0"
            zone_rule[z] = rule
        rule = zone_rule[z]
        if rule not in rule_idx:
            rule_idx[rule] = len(rules)
            rules.append(rule)
        tz_index.append(rule_idx[rule])
    if len(rules) > 255:
        sys.exit("more than 255 distinct time-zone rules (%d): AIRPORT_TZIDX needs a wider type" % len(rules))

    pool, offs = [], []
    pos = 0
    for r in rows:
        for text in (r[6]["name"], r[6]["place"]):
            offs.append(pos)
            pool.append(text)
            pos += len(text) + 1
    offs.append(pos)                                                # end marker

    with open(info_dst, "w", encoding="ascii") as f:
        f.write("// Auto-generated by tools/gen_airports.py - DO NOT EDIT.\n")
        f.write("// Details of the airports in airports_data.h: same order, same AIRPORT_NUM, so index i is one airport.\n")
        f.write("// Source: OurAirports (public domain); time-zone rules from the IANA database.\n")
        f.write("#pragma once\n#include <stdint.h>\n#include \"airports_data.h\"\n\n")
        f.write("// Local-time rules (POSIX TZ strings), one per distinct rule; AIRPORT_TZIDX picks one for each airport.\n")
        f.write("#define AIRPORT_TZ_NUM %d\n" % len(rules))
        f.write("static const char *const AIRPORT_TZ[AIRPORT_TZ_NUM] = {\n")
        for rule in rules:
            f.write('  "%s",\n' % c_escape(rule))
        f.write("};\n\n")
        f.write("static const uint8_t AIRPORT_TZIDX[AIRPORT_NUM] = {\n")
        for i in range(0, len(rows), 32):
            f.write("  " + ",".join(str(x) for x in tz_index[i:i + 32]) + ",\n")
        f.write("};\n\n")
        f.write("// Elevation in feet; -9999 when unknown.\n")
        f.write("static const int16_t AIRPORT_ELEV[AIRPORT_NUM] = {\n")
        for i in range(0, len(rows), 18):
            f.write("  " + ",".join(str(r[6]["elev"]) for r in rows[i:i + 18]) + ",\n")
        f.write("};\n\n")
        f.write("static const char AIRPORT_ICAO[AIRPORT_NUM][5] = {\n")
        for i in range(0, len(rows), 10):
            f.write("  " + ",".join('"%s"' % r[6]["icao"] for r in rows[i:i + 10]) + ",\n")
        f.write("};\n\n")
        f.write("static const char AIRPORT_CC[AIRPORT_NUM][3] = {\n")
        for i in range(0, len(rows), 16):
            f.write("  " + ",".join('"%s"' % r[6]["cc"] for r in rows[i:i + 16]) + ",\n")
        f.write("};\n\n")
        f.write("// Name and city of airport i: AIRPORT_STR + AIRPORT_STR_OFF[2*i] and ... [2*i + 1] (NUL-terminated).\n")
        f.write("static const uint32_t AIRPORT_STR_OFF[2 * AIRPORT_NUM + 1] = {\n")
        for i in range(0, len(offs), 16):
            f.write("  " + ",".join(str(x) for x in offs[i:i + 16]) + ",\n")
        f.write("};\n\n")
        f.write("static const char AIRPORT_STR[] =\n")
        for text in pool:
            f.write('  "%s\\0"\n' % c_escape(text))
        f.write(";\n")

    print("details:  %d airports, %d distinct time-zone rules, strings %.1f KB" % (len(rows), len(rules), pos / 1024.0))
    print("          airports with no time zone found: %d; zone files missing in tzdata: %s" % (missing_zone, sorted(x for x in bad_zones if x)))
    print("written:  %s" % info_dst)


if __name__ == "__main__":
    main()

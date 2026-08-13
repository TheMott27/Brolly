/**
 * Brolly Settings — Defaults & Types
 * All keys must match package.json messageKeys exactly.
 * Key numbers are shown in comments for reference.
 * Weather source: Open-Meteo only (no API key required).
 * Defaults are kept in sync with C code (main.c prv_load_default_settings).
 */

export interface BrollySettings {
  // ── Display ──────────────────────────────────────────────────────────────
  KEY_NUMBER_FONT: number          // 121 — 0=Digital 1=Standard 2=Traditional 3=Thin 4=Oversize
  KEY_NUMBER_SIZE: number          // 150 — 1–5
  KEY_ICON_SIZE: number            // 151 — 1–5
  KEY_ICON_COLOR_MODE: number      // 153 — 0=single colour 1=weather-based
  KEY_DISPLAY_MODE: number         // 158 — 0=both 1=temp 2=date 3=none
  KEY_SHAKE_MODE: number           // 107 — 0=show icons on shake 1=always show 2=always hide 3=side-by-side

  // ── Background ───────────────────────────────────────────────────────────
  KEY_BACKGROUND_COLOR: number     // 126

  // ── Watch Hands ──────────────────────────────────────────────────────────
  KEY_HOUR_HAND_OUTER: number      // 114
  KEY_HOUR_HAND_INNER: number      // 115
  KEY_MIN_HAND_OUTER: number       // 116
  KEY_MIN_HAND_INNER: number       // 117
  KEY_SECONDS_HAND_COLOR: number   // 141
  KEY_SECONDS_HAND_MODE: number    // 142 — 0=never 1=always 2=shake only
  KEY_SECONDS_SHAKE_DUR: number    // 143 — seconds to show after shake (1–30)

  // ── Markers, Numbers & Icons ─────────────────────────────────────────────
  KEY_HOUR_MARKER_COLOR: number    // 129
  KEY_MINUTE_MARKER_COLOR: number  // 130
  KEY_NUMBER_COLOR: number         // 127
  KEY_ICON_COLOR: number           // 128

  // ── Date & Temperature ───────────────────────────────────────────────────
  KEY_DATE_COLOR: number           // 134
  KEY_TEMP_COLOR: number           // 135
  KEY_DATE_VISIBLE: number         // 118 — 0=always 1=off 2=shake
  KEY_TEMP_VISIBLE: number         // 119 — 0=always 1=off 2=shake
  KEY_TEMP_UNIT: number            // 110 — 0=celsius 1=fahrenheit

  // ── Weather / Location ───────────────────────────────────────────────────
  KEY_CUSTOM_LOCATION: string      // 113 — city name string; empty = use GPS
  KEY_CITY_DISPLAY_MODE: number    // 160 — 0=off 1=shake 2=always
  KEY_CITY_COLOR: number           // 161
  KEY_COMPLICATION_LAYER: number   // 162 — 0=behind hands 1=on top of hands
  KEY_WEATHER_INTERVAL: number     // 163 — 30/60/120 minutes

  // ── Sunrise / Sunset ─────────────────────────────────────────────────────
  KEY_SUNRISE_MARKER_VISIBLE: number // 147 — 0=always 1=with icons only 2=off
  KEY_SUNRISE_MARKER_COLOR: number   // 148
  KEY_SUNSET_MARKER_COLOR: number    // 149

  // ── Bluetooth / Alerts ───────────────────────────────────────────────────
  KEY_VIBRATE_BT_DISCONNECT: number       // 54
  KEY_VIBRATE_BT_RECONNECT: number        // 55
  KEY_BT_DISCONNECT_MIN_INNER_RED: number // 53
  KEY_BT_DISCONNECT_OUTER_COLOR: number   // 136
  KEY_BT_DISCONNECT_INNER_COLOR: number   // 137

  // ── Battery Alerts ───────────────────────────────────────────────────────
  KEY_BATTERY_RING_THRESHOLD: number   // 138
  KEY_BATTERY_CENTER_THRESHOLD: number // 139
}

export const DEFAULTS: BrollySettings = {
  KEY_NUMBER_FONT: 0,
  KEY_NUMBER_SIZE: 3,
  KEY_ICON_SIZE: 3,
  KEY_ICON_COLOR_MODE: 0,
  KEY_DISPLAY_MODE: 0,
  KEY_SHAKE_MODE: 0,

  KEY_BACKGROUND_COLOR: 0x000000,

  KEY_HOUR_HAND_OUTER:  0xffffff,   // white  (matches C GColorWhite)
  KEY_HOUR_HAND_INNER:  -1,          // transparent/clear (matches C GColorClear, rgb_to_gcolor(-1) = GColorClear)
  KEY_MIN_HAND_OUTER:   0x000000,   // black  (matches C GColorBlack)
  KEY_MIN_HAND_INNER:   0x0055ff,   // blue   (nearest Pebble colour to C 0x0061fe)
  KEY_SECONDS_HAND_COLOR: 0xffffff, // white  (matches C GColorWhite)
  KEY_SECONDS_HAND_MODE: 2,         // shake to show
  KEY_SECONDS_SHAKE_DUR: 10,

  KEY_HOUR_MARKER_COLOR:   0xffffff, // white
  KEY_MINUTE_MARKER_COLOR: 0x555555, // mid-grey
  KEY_NUMBER_COLOR:        0xffffff, // white
  KEY_ICON_COLOR:          0xffffff, // white

  KEY_DATE_COLOR: 0x555555,  // mid-grey (user request)
  KEY_TEMP_COLOR: 0x555555,  // mid-grey (user request)
  KEY_DATE_VISIBLE: 0,
  KEY_TEMP_VISIBLE: 0,
  KEY_TEMP_UNIT: 0,

  KEY_CUSTOM_LOCATION: '',
  KEY_CITY_DISPLAY_MODE: 1,        // shake (same as date/temp default)
  KEY_CITY_COLOR: 0x0000aa,        // navy blue
  KEY_COMPLICATION_LAYER: 0,
  KEY_WEATHER_INTERVAL: 60,

  KEY_SUNRISE_MARKER_VISIBLE: 0,
  KEY_SUNRISE_MARKER_COLOR: 0xff5500,  // orange-red (user request)
  KEY_SUNSET_MARKER_COLOR:  0x0055ff,  // blue       (user request)

  KEY_VIBRATE_BT_DISCONNECT: 1,
  KEY_VIBRATE_BT_RECONNECT: 0,
  KEY_BT_DISCONNECT_MIN_INNER_RED: 1,
  KEY_BT_DISCONNECT_OUTER_COLOR: 0xff0000,
  KEY_BT_DISCONNECT_INNER_COLOR: 0xff0000,

  KEY_BATTERY_RING_THRESHOLD: 20,
  KEY_BATTERY_CENTER_THRESHOLD: 10,
}

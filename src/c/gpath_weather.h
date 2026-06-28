#pragma once
#include <pebble.h>

// ─────────────────────────────────────────────────────────────────────────────
// Weather icon GPath data for Brolly v2.0.0
// All coordinates are on a 0–23 grid (24 lines × 24 lines).
// native_w and native_h are both 23 for all new icons.
// ─────────────────────────────────────────────────────────────────────────────

// ── Icon IDs ─────────────────────────────────────────────────────────────────
typedef enum {
  GPATH_ID_CLOUDY_DAY = 0,
  GPATH_ID_THUNDERSTORM,
  GPATH_ID_HEAVY_RAIN,
  GPATH_ID_HEAVY_SNOW,
  GPATH_ID_LIGHT_RAIN,
  GPATH_ID_LIGHT_SNOW,
  GPATH_ID_RAINING_AND_SNOWING,
  GPATH_ID_PARTLY_CLOUDY,
  GPATH_ID_TIMELINE_SUN,
  GPATH_ID_TIMELINE_MOON,
  GPATH_ID_PARTLY_CLOUDY_NIGHT,
  GPATH_ID_UNKNOWN,
  GPATH_ID_COUNT
} GPathIconID;

// ── Struct to hold a multi-path icon ─────────────────────────────────────────
typedef struct {
  int native_w;
  int native_h;
  int num_paths;
  const GPathInfo *paths;
} WeatherIconDef;

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_CLOUDY_DAY — two overlapping scaled cloud sprites, native 23×23
// Back cloud (top-left): x=0..19, y=0..13
// Front cloud (bottom-right): x=4..23, y=10..23
// Back cloud bottom line clipped where it falls behind front cloud.
// ─────────────────────────────────────────────────────────────────────────────
// Back cloud outline (visible portion — right side clipped behind front cloud)
static const GPoint s_cloudy_day_0_pts[] = {
  {0,13},{0,9},{0,7},{2,4},{6,4},{8,0},{12,0},{14,4},{17,4},{19,7},{19,9},{19,13}
};
// Back cloud bottom line (left portion only, before front cloud starts)
static const GPoint s_cloudy_day_1_pts[] = {
  {0,13},{4,13}
};
// Front cloud outline
static const GPoint s_cloudy_day_2_pts[] = {
  {4,23},{4,19},{4,17},{6,14},{10,14},{12,10},{16,10},{18,14},{21,14},{23,17},{23,19},{23,23}
};
// Front cloud bottom line
static const GPoint s_cloudy_day_3_pts[] = {
  {4,23},{23,23}
};
static const GPathInfo s_cloudy_day_paths[] = {
  { ARRAY_LENGTH(s_cloudy_day_0_pts), (GPoint *)s_cloudy_day_0_pts },
  { ARRAY_LENGTH(s_cloudy_day_1_pts), (GPoint *)s_cloudy_day_1_pts },
  { ARRAY_LENGTH(s_cloudy_day_2_pts), (GPoint *)s_cloudy_day_2_pts },
  { ARRAY_LENGTH(s_cloudy_day_3_pts), (GPoint *)s_cloudy_day_3_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_THUNDERSTORM — native 23×23
// Cloud upper arc split into left and right halves with a gap where the bolt
// protrudes. A short stub (10,5)→(11,5) marks the cloud top above the bolt.
// Bolt: (14,0)→(14,10)→(18,10)→(10,23)→(10,14)→(6,14)→(14,0)
// ─────────────────────────────────────────────────────────────────────────────
// Left arc: left edge up to the gap at (10,5)
static const GPoint s_thunderstorm_0_pts[] = {
  {0,16},{0,14},{0,11},{3,8},{7,8},{10,5}
};
// Right arc: from gap at (14,5) to right edge
static const GPoint s_thunderstorm_5_pts[] = {
  {14,5},{17,8},{21,8},{23,11},{23,14},{23,16}
};
// Stub: short segment at cloud top above bolt gap
static const GPoint s_thunderstorm_stub_pts[] = {
  {10,5},{11,5}
};
// Left bottom segment (0,16)→(9,16)
static const GPoint s_thunderstorm_1_pts[] = {
  {0,16},{9,16}
};
// Right bottom segment (15,16)→(23,16)
static const GPoint s_thunderstorm_2_pts[] = {
  {15,16},{23,16}
};
// Right close line (23,11)→(23,16)
static const GPoint s_thunderstorm_3_pts[] = {
  {23,11},{23,16}
};
// Bolt
static const GPoint s_thunderstorm_4_pts[] = {
  {14,0},{14,10},{18,10},{10,23},{10,14},{6,14},{14,0}
};
static const GPathInfo s_thunderstorm_paths[] = {
  { ARRAY_LENGTH(s_thunderstorm_0_pts),    (GPoint *)s_thunderstorm_0_pts },
  { ARRAY_LENGTH(s_thunderstorm_5_pts),    (GPoint *)s_thunderstorm_5_pts },
  { ARRAY_LENGTH(s_thunderstorm_stub_pts), (GPoint *)s_thunderstorm_stub_pts },
  { ARRAY_LENGTH(s_thunderstorm_1_pts),    (GPoint *)s_thunderstorm_1_pts },
  { ARRAY_LENGTH(s_thunderstorm_2_pts),    (GPoint *)s_thunderstorm_2_pts },
  { ARRAY_LENGTH(s_thunderstorm_3_pts),    (GPoint *)s_thunderstorm_3_pts },
  { ARRAY_LENGTH(s_thunderstorm_4_pts),    (GPoint *)s_thunderstorm_4_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_HEAVY_RAIN — native 23×23
// Standard cloud (top at y=0, bottom at y=11), 4 diagonal rain lines y=11..23
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_heavy_rain_0_pts[] = {
  {0,11},{0,8},{0,6},{3,3},{7,3},{10,0},{14,0},{17,3},{21,3},{23,6},{23,8},{23,11}
};
static const GPoint s_heavy_rain_1_pts[] = { {0,11},{23,11} };  // cloud bottom
static const GPoint s_heavy_rain_2_pts[] = { {0,11},{5,23} };
static const GPoint s_heavy_rain_3_pts[] = { {6,11},{11,23} };
static const GPoint s_heavy_rain_4_pts[] = { {12,11},{17,23} };
static const GPoint s_heavy_rain_5_pts[] = { {18,11},{23,23} };
static const GPathInfo s_heavy_rain_paths[] = {
  { ARRAY_LENGTH(s_heavy_rain_0_pts), (GPoint *)s_heavy_rain_0_pts },
  { ARRAY_LENGTH(s_heavy_rain_1_pts), (GPoint *)s_heavy_rain_1_pts },
  { ARRAY_LENGTH(s_heavy_rain_2_pts), (GPoint *)s_heavy_rain_2_pts },
  { ARRAY_LENGTH(s_heavy_rain_3_pts), (GPoint *)s_heavy_rain_3_pts },
  { ARRAY_LENGTH(s_heavy_rain_4_pts), (GPoint *)s_heavy_rain_4_pts },
  { ARRAY_LENGTH(s_heavy_rain_5_pts), (GPoint *)s_heavy_rain_5_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_HEAVY_SNOW — native 23×23
// Standard cloud (top at y=0, bottom at y=11), 3 snowflakes
// Left (3,20), Middle (11,16), Right (20,20), half_len=3
// Outer arms extended to x=0 and x=23
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_heavy_snow_0_pts[] = {
  {0,11},{0,8},{0,6},{3,3},{7,3},{10,0},{14,0},{17,3},{21,3},{23,6},{23,8},{23,11}
};
static const GPoint s_heavy_snow_1_pts[] = { {0,11},{23,11} };  // cloud bottom
// Left flake (3,20) half=3
static const GPoint s_heavy_snow_2_pts[] = { {3,17},{3,23} };
static const GPoint s_heavy_snow_3_pts[] = { {0,20},{6,20} };
// Middle flake (11,16) half=3
static const GPoint s_heavy_snow_4_pts[] = { {11,13},{11,19} };
static const GPoint s_heavy_snow_5_pts[] = { {8,16},{14,16} };
// Right flake (20,20) half=3
static const GPoint s_heavy_snow_6_pts[] = { {20,17},{20,23} };
static const GPoint s_heavy_snow_7_pts[] = { {17,20},{23,20} };
static const GPathInfo s_heavy_snow_paths[] = {
  { ARRAY_LENGTH(s_heavy_snow_0_pts), (GPoint *)s_heavy_snow_0_pts },
  { ARRAY_LENGTH(s_heavy_snow_1_pts), (GPoint *)s_heavy_snow_1_pts },
  { ARRAY_LENGTH(s_heavy_snow_2_pts), (GPoint *)s_heavy_snow_2_pts },
  { ARRAY_LENGTH(s_heavy_snow_3_pts), (GPoint *)s_heavy_snow_3_pts },
  { ARRAY_LENGTH(s_heavy_snow_4_pts), (GPoint *)s_heavy_snow_4_pts },
  { ARRAY_LENGTH(s_heavy_snow_5_pts), (GPoint *)s_heavy_snow_5_pts },
  { ARRAY_LENGTH(s_heavy_snow_6_pts), (GPoint *)s_heavy_snow_6_pts },
  { ARRAY_LENGTH(s_heavy_snow_7_pts), (GPoint *)s_heavy_snow_7_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_LIGHT_RAIN — native 23×23
// Standard cloud (top at y=0, bottom at y=11), 3 diagonal rain lines y=11..23
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_light_rain_0_pts[] = {
  {0,11},{0,8},{0,6},{3,3},{7,3},{10,0},{14,0},{17,3},{21,3},{23,6},{23,8},{23,11}
};
static const GPoint s_light_rain_1_pts[] = { {0,11},{23,11} };  // cloud bottom
static const GPoint s_light_rain_2_pts[] = { {0,11},{5,23} };
static const GPoint s_light_rain_3_pts[] = { {9,11},{14,23} };
static const GPoint s_light_rain_4_pts[] = { {18,11},{23,23} };
static const GPathInfo s_light_rain_paths[] = {
  { ARRAY_LENGTH(s_light_rain_0_pts), (GPoint *)s_light_rain_0_pts },
  { ARRAY_LENGTH(s_light_rain_1_pts), (GPoint *)s_light_rain_1_pts },
  { ARRAY_LENGTH(s_light_rain_2_pts), (GPoint *)s_light_rain_2_pts },
  { ARRAY_LENGTH(s_light_rain_3_pts), (GPoint *)s_light_rain_3_pts },
  { ARRAY_LENGTH(s_light_rain_4_pts), (GPoint *)s_light_rain_4_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_LIGHT_SNOW — native 23×23
// Standard cloud (top at y=0, bottom at y=11), 2 snowflakes
// Left flake centre (5,15) half=4; Right flake centre (18,19) half=4 (bottom at y=23)
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_light_snow_0_pts[] = {
  {0,11},{0,8},{0,6},{3,3},{7,3},{10,0},{14,0},{17,3},{21,3},{23,6},{23,8},{23,11}
};
static const GPoint s_light_snow_1_pts[] = { {0,11},{23,11} };  // cloud bottom
// Left flake (5,15) half=4
static const GPoint s_light_snow_2_pts[] = { {5,11},{5,19} };
static const GPoint s_light_snow_3_pts[] = { {1,15},{9,15} };
// Right flake (18,19) half=4, bottom at y=23
static const GPoint s_light_snow_4_pts[] = { {18,15},{18,23} };
static const GPoint s_light_snow_5_pts[] = { {14,19},{22,19} };
static const GPathInfo s_light_snow_paths[] = {
  { ARRAY_LENGTH(s_light_snow_0_pts), (GPoint *)s_light_snow_0_pts },
  { ARRAY_LENGTH(s_light_snow_1_pts), (GPoint *)s_light_snow_1_pts },
  { ARRAY_LENGTH(s_light_snow_2_pts), (GPoint *)s_light_snow_2_pts },
  { ARRAY_LENGTH(s_light_snow_3_pts), (GPoint *)s_light_snow_3_pts },
  { ARRAY_LENGTH(s_light_snow_4_pts), (GPoint *)s_light_snow_4_pts },
  { ARRAY_LENGTH(s_light_snow_5_pts), (GPoint *)s_light_snow_5_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_RAINING_AND_SNOWING — native 23×23
// Standard cloud (top at y=0, bottom at y=11)
// 2 rain lines (left half), 1 snowflake centre (19,17) half=4
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_rain_snow_0_pts[] = {
  {0,11},{0,8},{0,6},{3,3},{7,3},{10,0},{14,0},{17,3},{21,3},{23,6},{23,8},{23,11}
};
static const GPoint s_rain_snow_1_pts[] = { {0,11},{23,11} };  // cloud bottom
static const GPoint s_rain_snow_2_pts[] = { {0,11},{5,23} };
static const GPoint s_rain_snow_3_pts[] = { {8,11},{13,23} };
// Snowflake (19,17) half=4
static const GPoint s_rain_snow_4_pts[] = { {19,13},{19,21} };
static const GPoint s_rain_snow_5_pts[] = { {15,17},{23,17} };
static const GPathInfo s_rain_snow_paths[] = {
  { ARRAY_LENGTH(s_rain_snow_0_pts), (GPoint *)s_rain_snow_0_pts },
  { ARRAY_LENGTH(s_rain_snow_1_pts), (GPoint *)s_rain_snow_1_pts },
  { ARRAY_LENGTH(s_rain_snow_2_pts), (GPoint *)s_rain_snow_2_pts },
  { ARRAY_LENGTH(s_rain_snow_3_pts), (GPoint *)s_rain_snow_3_pts },
  { ARRAY_LENGTH(s_rain_snow_4_pts), (GPoint *)s_rain_snow_4_pts },
  { ARRAY_LENGTH(s_rain_snow_5_pts), (GPoint *)s_rain_snow_5_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_PARTLY_CLOUDY — native 23×23
// Small sun octagon (centre 5,5 radius 4) with rays, large cloud bottom (y=12..23)
// ─────────────────────────────────────────────────────────────────────────────
// Small sun octagon
static const GPoint s_partly_cloudy_0_pts[] = {
  {3,1},{1,3},{1,7},{3,9},{7,9},{9,7},{9,3},{7,1},{3,1}
};
// Cardinal rays
static const GPoint s_partly_cloudy_1_pts[] = { {5,0},{5,1} };   // N
static const GPoint s_partly_cloudy_2_pts[] = { {0,5},{1,5} };   // W
static const GPoint s_partly_cloudy_3_pts[] = { {10,5},{9,5} };  // E
static const GPoint s_partly_cloudy_4_pts[] = { {5,10},{5,9} };  // S
// Diagonal rays
static const GPoint s_partly_cloudy_5_pts[] = { {9,1},{10,0} };  // NE
static const GPoint s_partly_cloudy_6_pts[] = { {9,9},{10,10} }; // SE
static const GPoint s_partly_cloudy_7_pts[] = { {0,0},{1,1} };   // NW
static const GPoint s_partly_cloudy_8_pts[] = { {0,10},{1,9} };  // SW
// Large cloud (top at y=12, bottom at y=23)
static const GPoint s_partly_cloudy_9_pts[] = {
  {0,23},{0,20},{0,18},{3,15},{7,15},{10,12},{14,12},{17,15},{21,15},{23,18},{23,20},{23,23}
};
static const GPoint s_partly_cloudy_10_pts[] = { {0,23},{23,23} }; // cloud bottom
static const GPathInfo s_partly_cloudy_paths[] = {
  { ARRAY_LENGTH(s_partly_cloudy_0_pts),  (GPoint *)s_partly_cloudy_0_pts },
  { ARRAY_LENGTH(s_partly_cloudy_1_pts),  (GPoint *)s_partly_cloudy_1_pts },
  { ARRAY_LENGTH(s_partly_cloudy_2_pts),  (GPoint *)s_partly_cloudy_2_pts },
  { ARRAY_LENGTH(s_partly_cloudy_3_pts),  (GPoint *)s_partly_cloudy_3_pts },
  { ARRAY_LENGTH(s_partly_cloudy_4_pts),  (GPoint *)s_partly_cloudy_4_pts },
  { ARRAY_LENGTH(s_partly_cloudy_5_pts),  (GPoint *)s_partly_cloudy_5_pts },
  { ARRAY_LENGTH(s_partly_cloudy_6_pts),  (GPoint *)s_partly_cloudy_6_pts },
  { ARRAY_LENGTH(s_partly_cloudy_7_pts),  (GPoint *)s_partly_cloudy_7_pts },
  { ARRAY_LENGTH(s_partly_cloudy_8_pts),  (GPoint *)s_partly_cloudy_8_pts },
  { ARRAY_LENGTH(s_partly_cloudy_9_pts),  (GPoint *)s_partly_cloudy_9_pts },
  { ARRAY_LENGTH(s_partly_cloudy_10_pts), (GPoint *)s_partly_cloudy_10_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_TIMELINE_SUN — native 23×23
// Octagon centre (12,12) radius 7, 8 rays to grid edges
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_timeline_sun_0_pts[] = {
  {9,5},{15,5},{19,9},{19,15},{15,19},{9,19},{5,15},{5,9},{9,5}
};
// N ray
static const GPoint s_timeline_sun_1_pts[] = { {12,0},{12,5} };
// S ray
static const GPoint s_timeline_sun_2_pts[] = { {12,19},{12,23} };
// W ray
static const GPoint s_timeline_sun_3_pts[] = { {0,12},{5,12} };
// E ray
static const GPoint s_timeline_sun_4_pts[] = { {19,12},{23,12} };
// NW ray
static const GPoint s_timeline_sun_5_pts[] = { {0,0},{3,3} };
// NE ray
static const GPoint s_timeline_sun_6_pts[] = { {23,0},{20,3} };
// SW ray
static const GPoint s_timeline_sun_7_pts[] = { {0,23},{3,20} };
// SE ray
static const GPoint s_timeline_sun_8_pts[] = { {23,23},{20,20} };
static const GPathInfo s_timeline_sun_paths[] = {
  { ARRAY_LENGTH(s_timeline_sun_0_pts), (GPoint *)s_timeline_sun_0_pts },
  { ARRAY_LENGTH(s_timeline_sun_1_pts), (GPoint *)s_timeline_sun_1_pts },
  { ARRAY_LENGTH(s_timeline_sun_2_pts), (GPoint *)s_timeline_sun_2_pts },
  { ARRAY_LENGTH(s_timeline_sun_3_pts), (GPoint *)s_timeline_sun_3_pts },
  { ARRAY_LENGTH(s_timeline_sun_4_pts), (GPoint *)s_timeline_sun_4_pts },
  { ARRAY_LENGTH(s_timeline_sun_5_pts), (GPoint *)s_timeline_sun_5_pts },
  { ARRAY_LENGTH(s_timeline_sun_6_pts), (GPoint *)s_timeline_sun_6_pts },
  { ARRAY_LENGTH(s_timeline_sun_7_pts), (GPoint *)s_timeline_sun_7_pts },
  { ARRAY_LENGTH(s_timeline_sun_8_pts), (GPoint *)s_timeline_sun_8_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_TIMELINE_MOON — native 23×23
// Outer arc: (16,0)→(23,7)→(23,16)→(16,23)→(7,23)→(0,16)
// Inner arc: (16,0)→(16,9)→(9,16)→(0,16)
// Star cross: H=(2,6)→(10,6), V=(6,2)→(6,10)  [shifted +2x +2y from (0,4)→(8,4),(4,0)→(4,8)]
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_timeline_moon_0_pts[] = {
  {16,0},{23,7},{23,16},{16,23},{7,23},{0,16}
};
static const GPoint s_timeline_moon_1_pts[] = {
  {16,0},{16,9},{9,16},{0,16}
};
static const GPoint s_timeline_moon_2_pts[] = { {2,6},{10,6} };  // star H
static const GPoint s_timeline_moon_3_pts[] = { {6,2},{6,10} };  // star V
static const GPathInfo s_timeline_moon_paths[] = {
  { ARRAY_LENGTH(s_timeline_moon_0_pts), (GPoint *)s_timeline_moon_0_pts },
  { ARRAY_LENGTH(s_timeline_moon_1_pts), (GPoint *)s_timeline_moon_1_pts },
  { ARRAY_LENGTH(s_timeline_moon_2_pts), (GPoint *)s_timeline_moon_2_pts },
  { ARRAY_LENGTH(s_timeline_moon_3_pts), (GPoint *)s_timeline_moon_3_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_PARTLY_CLOUDY_NIGHT — native 23×23
// Small moon (no star) top-right, scaled 0.48 offset (12,0)
// Large cloud bottom (y=12..23)
// ─────────────────────────────────────────────────────────────────────────────
// Small moon outer arc (scaled 0.48, offset 12,0)
static const GPoint s_partly_cloudy_night_0_pts[] = {
  {20,0},{23,3},{23,8},{20,11},{15,11},{12,8}
};
// Small moon inner arc
static const GPoint s_partly_cloudy_night_1_pts[] = {
  {20,0},{20,4},{16,8},{12,8}
};
// Large cloud
static const GPoint s_partly_cloudy_night_2_pts[] = {
  {0,23},{0,20},{0,18},{3,15},{7,15},{10,12},{14,12},{17,15},{21,15},{23,18},{23,20},{23,23}
};
static const GPoint s_partly_cloudy_night_3_pts[] = { {0,23},{23,23} }; // cloud bottom
static const GPathInfo s_partly_cloudy_night_paths[] = {
  { ARRAY_LENGTH(s_partly_cloudy_night_0_pts), (GPoint *)s_partly_cloudy_night_0_pts },
  { ARRAY_LENGTH(s_partly_cloudy_night_1_pts), (GPoint *)s_partly_cloudy_night_1_pts },
  { ARRAY_LENGTH(s_partly_cloudy_night_2_pts), (GPoint *)s_partly_cloudy_night_2_pts },
  { ARRAY_LENGTH(s_partly_cloudy_night_3_pts), (GPoint *)s_partly_cloudy_night_3_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// GPATH_UNKNOWN — native 23×23
// Diamond (12,0)→(23,12)→(12,23)→(0,12)→(12,0)
// Question mark (shifted right 1px, down 1px)
// ─────────────────────────────────────────────────────────────────────────────
static const GPoint s_unknown_0_pts[] = {
  {12,0},{23,12},{12,23},{0,12},{12,0}
};
static const GPoint s_unknown_1_pts[] = {
  {9,9},{9,7},{11,5},{13,5},{15,7},{15,10},{13,13},{12,15}
};
static const GPoint s_unknown_2_pts[] = { {12,18},{12,19} };
static const GPathInfo s_unknown_paths[] = {
  { ARRAY_LENGTH(s_unknown_0_pts), (GPoint *)s_unknown_0_pts },
  { ARRAY_LENGTH(s_unknown_1_pts), (GPoint *)s_unknown_1_pts },
  { ARRAY_LENGTH(s_unknown_2_pts), (GPoint *)s_unknown_2_pts }
};

// ─────────────────────────────────────────────────────────────────────────────
// Master icon table
// ─────────────────────────────────────────────────────────────────────────────
static const WeatherIconDef s_weather_icons[GPATH_ID_COUNT] = {
  [GPATH_ID_CLOUDY_DAY]          = { 23, 23, 4,  s_cloudy_day_paths },
  [GPATH_ID_THUNDERSTORM]        = { 23, 23, 7,  s_thunderstorm_paths },
  [GPATH_ID_HEAVY_RAIN]          = { 23, 23, 6,  s_heavy_rain_paths },
  [GPATH_ID_HEAVY_SNOW]          = { 23, 23, 8,  s_heavy_snow_paths },
  [GPATH_ID_LIGHT_RAIN]          = { 23, 23, 5,  s_light_rain_paths },
  [GPATH_ID_LIGHT_SNOW]          = { 23, 23, 6,  s_light_snow_paths },
  [GPATH_ID_RAINING_AND_SNOWING] = { 23, 23, 6,  s_rain_snow_paths },
  [GPATH_ID_PARTLY_CLOUDY]       = { 23, 23, 11, s_partly_cloudy_paths },
  [GPATH_ID_TIMELINE_SUN]        = { 23, 23, 9,  s_timeline_sun_paths },
  [GPATH_ID_TIMELINE_MOON]       = { 23, 23, 4,  s_timeline_moon_paths },
  [GPATH_ID_PARTLY_CLOUDY_NIGHT] = { 23, 23, 4,  s_partly_cloudy_night_paths },
  [GPATH_ID_UNKNOWN]             = { 23, 23, 3,  s_unknown_paths }
};

// ─────────────────────────────────────────────────────────────────────────────
// Weather colour table (used when icon_color_mode == 1)
// One GColor8 per path, indexed [icon_id][path_index].
// Approved colour scheme:
//   clouds/arcs = grey   #aaaaaa  → 0xEA (11 10 10 10)
//   sun/rays    = amber  #ffaa00  → 0xFC (11 11 10 00)
//   lightning   = yellow #ffff00  → 0xFF (11 11 11 00) — closest Pebble yellow
//   heavy rain  = bright blue #0055ff → 0xC3 (11 00 01 11)
//   light rain  = soft blue   #5555ff → 0xC7 (11 01 01 11)
//   snow        = cyan   #aaffff  → 0xCF (11 00 11 11)
//   heavy snow  = white  #ffffff  → 0xFF
//   moon        = pale blue #aaaaff → 0xCB (11 00 10 11)
//   unknown     = dark grey  #555555 → 0xD5 (11 01 01 01)
// GColor8 argb byte: 0b11rrggbb, r/g/b are 2-bit (0-3).
// ─────────────────────────────────────────────────────────────────────────────
// Maximum paths across all icons = 11 (PARTLY_CLOUDY)
#define WEATHER_COLOR_MAX_PATHS 11

// Plain uint8_t argb constants — true C99 integer constants, safe in static arrays.
// GColor8 argb format: 0b11rrggbb (top 2 bits always 1).
#define WC_GREY   0xEA  // #aaaaaa cloud grey
#define WC_AMBER  0xFC  // #ffaa00 sun amber
#define WC_YELLOW 0xFF  // #ffff00 lightning yellow
#define WC_BBLUE  0xC3  // #0055ff heavy rain
#define WC_SBLUE  0xC7  // #5555ff light rain
#define WC_CYAN   0xCF  // #aaffff snow cyan
#define WC_WHITE  0xFF  // #ffffff heavy snow
#define WC_PALE   0xCB  // #aaaaff moon pale blue
#define WC_DGREY  0xD5  // #555555 unknown dark grey

// Per-path colour tables indexed by [icon_id][path_index]
// Stored as raw argb bytes; cast to GColor8 at point of use.
static const uint8_t s_weather_colors[GPATH_ID_COUNT][WEATHER_COLOR_MAX_PATHS] = {
  // CLOUDY_DAY: 4 paths — back cloud arc, back cloud bottom, front cloud arc, front cloud bottom
  [GPATH_ID_CLOUDY_DAY] = { WC_GREY, WC_GREY, WC_GREY, WC_GREY },

  // THUNDERSTORM: 7 paths — left arc, right arc, stub, left bottom, right bottom, right close, bolt
  [GPATH_ID_THUNDERSTORM] = { WC_GREY, WC_GREY, WC_GREY, WC_GREY, WC_GREY, WC_GREY, WC_YELLOW },

  // HEAVY_RAIN: 6 paths — cloud arc, cloud bottom, rain×4
  [GPATH_ID_HEAVY_RAIN] = { WC_GREY, WC_GREY, WC_BBLUE, WC_BBLUE, WC_BBLUE, WC_BBLUE },

  // HEAVY_SNOW: 8 paths — cloud arc, cloud bottom, snowflake×3 (V+H each)
  [GPATH_ID_HEAVY_SNOW] = { WC_GREY, WC_GREY, WC_WHITE, WC_WHITE, WC_WHITE, WC_WHITE, WC_WHITE, WC_WHITE },

  // LIGHT_RAIN: 5 paths — cloud arc, cloud bottom, rain×3
  [GPATH_ID_LIGHT_RAIN] = { WC_GREY, WC_GREY, WC_SBLUE, WC_SBLUE, WC_SBLUE },

  // LIGHT_SNOW: 6 paths — cloud arc, cloud bottom, snowflake×2 (V+H each)
  [GPATH_ID_LIGHT_SNOW] = { WC_GREY, WC_GREY, WC_CYAN, WC_CYAN, WC_CYAN, WC_CYAN },

  // RAINING_AND_SNOWING: 6 paths — cloud arc, cloud bottom, rain×2, snowflake V, snowflake H
  [GPATH_ID_RAINING_AND_SNOWING] = { WC_GREY, WC_GREY, WC_SBLUE, WC_SBLUE, WC_CYAN, WC_CYAN },

  // PARTLY_CLOUDY: 11 paths — sun octagon, N/W/E/S rays, NE/SE/NW/SW rays, cloud arc, cloud bottom
  [GPATH_ID_PARTLY_CLOUDY] = { WC_AMBER, WC_AMBER, WC_AMBER, WC_AMBER, WC_AMBER,
                                WC_AMBER, WC_AMBER, WC_AMBER, WC_AMBER,
                                WC_GREY, WC_GREY },

  // TIMELINE_SUN: 9 paths — octagon, N/S/W/E rays, NW/NE/SW/SE rays
  [GPATH_ID_TIMELINE_SUN] = { WC_AMBER, WC_AMBER, WC_AMBER, WC_AMBER, WC_AMBER,
                               WC_AMBER, WC_AMBER, WC_AMBER, WC_AMBER },

  // TIMELINE_MOON: 4 paths — outer arc, inner arc, star H, star V
  [GPATH_ID_TIMELINE_MOON] = { WC_PALE, WC_PALE, WC_WHITE, WC_WHITE },

  // PARTLY_CLOUDY_NIGHT: 4 paths — moon outer arc, moon inner arc, cloud arc, cloud bottom
  [GPATH_ID_PARTLY_CLOUDY_NIGHT] = { WC_PALE, WC_PALE, WC_GREY, WC_GREY },

  // UNKNOWN: 3 paths — diamond, question mark, dot
  [GPATH_ID_UNKNOWN] = { WC_DGREY, WC_DGREY, WC_DGREY }
};

// ─────────────────────────────────────────────────────────────────────────────
// Icon code → GPath ID mapping
// ─────────────────────────────────────────────────────────────────────────────
#define ICON_UNKNOWN          0
#define ICON_UNDEFINED        1
#define ICON_CLEAR            2
#define ICON_CLEAR_N          3
#define ICON_PARTLY_CLOUDY    4
#define ICON_PARTLY_CLOUDY_N  5
#define ICON_MOSTLY_CLOUDY    6
#define ICON_MOSTLY_CLOUDY_N  7
#define ICON_CLOUDY           8
#define ICON_CLOUDY_N         9
#define ICON_CHANCE_FLURRIES  10
#define ICON_FLURRIES         11
#define ICON_CHANCE_FLURRIES_N 12
#define ICON_FLURRIES_N       13
#define ICON_CHANCE_RAIN      14
#define ICON_RAIN             15
#define ICON_CHANCE_RAIN_N    16
#define ICON_RAIN_N           17
#define ICON_CHANCE_SLEET     18
#define ICON_SLEET            19
#define ICON_CHANCE_SLEET_N   20
#define ICON_SLEET_N          21
#define ICON_CHANCE_SNOW      22
#define ICON_SNOW             23
#define ICON_CHANCE_SNOW_N    24
#define ICON_SNOW_N           25
#define ICON_CHANCE_TSTORMS   26
#define ICON_TSTORMS          27
#define ICON_CHANCE_TSTORMS_N 28
#define ICON_TSTORMS_N        29
#define ICON_FOG              30
#define ICON_HAZE             31
#define ICON_FOG_N            32
#define ICON_HAZE_N           33

static inline GPathIconID icon_code_to_gpath(int icon_code) {
  switch (icon_code) {
    case ICON_CLEAR:
    case ICON_PARTLY_CLOUDY:
      return GPATH_ID_TIMELINE_SUN;

    case ICON_CLEAR_N:
    case ICON_PARTLY_CLOUDY_N:
    case ICON_MOSTLY_CLOUDY_N:
    case ICON_CLOUDY_N:
      return GPATH_ID_TIMELINE_MOON;

    case ICON_MOSTLY_CLOUDY:
      return GPATH_ID_PARTLY_CLOUDY;

    case ICON_CLOUDY:
    case ICON_FOG:
    case ICON_FOG_N:
    case ICON_HAZE:
    case ICON_HAZE_N:
      return GPATH_ID_CLOUDY_DAY;

    case ICON_CHANCE_RAIN:
    case ICON_CHANCE_RAIN_N:
      return GPATH_ID_LIGHT_RAIN;

    case ICON_RAIN:
    case ICON_RAIN_N:
      return GPATH_ID_HEAVY_RAIN;

    case ICON_CHANCE_SNOW:
    case ICON_SNOW:
    case ICON_CHANCE_SNOW_N:
    case ICON_SNOW_N:
    case ICON_CHANCE_FLURRIES:
    case ICON_FLURRIES:
    case ICON_CHANCE_FLURRIES_N:
    case ICON_FLURRIES_N:
    case ICON_CHANCE_SLEET:
    case ICON_SLEET:
    case ICON_CHANCE_SLEET_N:
    case ICON_SLEET_N:
      return GPATH_ID_HEAVY_SNOW;

    case ICON_CHANCE_TSTORMS:
    case ICON_TSTORMS:
    case ICON_CHANCE_TSTORMS_N:
    case ICON_TSTORMS_N:
      return GPATH_ID_THUNDERSTORM;

    default:
      return GPATH_ID_UNKNOWN;
  }
}

// Draw a weather icon at (ox, oy) scaled to sz×sz.
// color      — base colour used when use_weather_colors is false.
// use_weather_colors — when true, each path is drawn in its approved weather
//                      colour from s_weather_colors instead of `color`.
static void draw_weather_icon(GContext *ctx, GPathIconID icon_id, int ox, int oy,
                              int sz, GColor color, bool use_weather_colors) {
  if (icon_id >= GPATH_ID_COUNT) return;
  const WeatherIconDef *def = &s_weather_icons[icon_id];
  int native_max = def->native_w > def->native_h ? def->native_w : def->native_h;
  if (native_max == 0) return;
  int scale256 = (sz * 256) / native_max;

  graphics_context_set_stroke_width(ctx, 1);

  for (int p = 0; p < def->num_paths; p++) {
    GColor path_color = color;
    if (use_weather_colors) {
      // Apply the weather colour directly (MONO_COLOR is in main.c scope;
      // on aplite the caller passes a pre-mono'd colour anyway).
      GColor8 wc; wc.argb = s_weather_colors[icon_id][p];
      path_color = (GColor){ .argb = wc.argb };
    }
    graphics_context_set_stroke_color(ctx, path_color);

    const GPathInfo *pi = &def->paths[p];
    for (int i = 0; i < (int)pi->num_points - 1; i++) {
      GPoint a = GPoint(ox + (pi->points[i].x   * scale256) / 256,
                        oy + (pi->points[i].y   * scale256) / 256);
      GPoint b = GPoint(ox + (pi->points[i+1].x * scale256) / 256,
                        oy + (pi->points[i+1].y * scale256) / 256);
      graphics_draw_line(ctx, a, b);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// LINE SHADING (icon_color_mode == 3)
// 45° diagonal hatching clipped to closed fill polygons.
// Clouds/rain/snow: NE→SW lines (x+y = const, gap 12px in scaled space).
// Sun/moon/bolt:    NW→SE lines (x-y = const, gap 12px in scaled space).
// Unknown: outline only, no shading.
// ─────────────────────────────────────────────────────────────────────────────

// Shading direction flags
#define SHADE_NE_SW 0  // x+y = const  (cloud direction)
#define SHADE_NW_SE 1  // x-y = const  (sun/moon/bolt direction)

// Shading gap in native-grid units (will be scaled with the icon).
// 12px gap at native 23 → scales proportionally.
#define SHADE_GAP_NATIVE 4

// Descriptor for one fill region used in shading.
typedef struct {
  const GPoint *pts;    // polygon vertices (native coords, closed not required)
  int           n;      // number of vertices
  uint8_t       color;  // WC_* argb byte
  uint8_t       dir;    // SHADE_NE_SW or SHADE_NW_SE
  bool          cutout; // true → erase (black fill) rather than shade
} ShadeFill;

// ── Shading polygon tables per icon ──────────────────────────────────────────

// Standard cloud body polygon (arc + flat bottom), native 23×23, top at y=0
static const GPoint s_shade_cloud_std[] = {
  {0,11},{0,6},{3,3},{7,3},{10,0},{14,0},{17,3},{21,3},{23,6},{23,11}
};
// Cloudy-day back cloud polygon
static const GPoint s_shade_cloud_back[] = {
  {0,13},{0,7},{2,4},{6,4},{8,0},{12,0},{14,4},{17,4},{19,7},{19,13}
};
// Cloudy-day front cloud polygon
static const GPoint s_shade_cloud_front[] = {
  {4,23},{4,17},{6,14},{10,14},{12,10},{16,10},{18,14},{21,14},{23,17},{23,23}
};
// Thunderstorm cloud body polygon
static const GPoint s_shade_thunder_cloud[] = {
  {0,16},{0,11},{3,8},{7,8},{10,5},{14,5},{17,8},{21,8},{23,11},{23,16}
};
// Lightning bolt polygon
static const GPoint s_shade_bolt[] = {
  {14,0},{14,10},{18,10},{10,23},{10,14},{6,14}
};
// Sun octagon (TIMELINE_SUN)
static const GPoint s_shade_sun_oct[] = {
  {9,5},{15,5},{19,9},{19,15},{15,19},{9,19},{5,15},{5,9}
};
// Small sun octagon (PARTLY_CLOUDY)
static const GPoint s_shade_sun_small[] = {
  {3,1},{1,3},{1,7},{3,9},{7,9},{9,7},{9,3},{7,1}
};
// Partly-cloudy cloud body polygon
static const GPoint s_shade_cloud_partly[] = {
  {0,23},{0,18},{3,15},{7,15},{10,12},{14,12},{17,15},{21,15},{23,18},{23,23}
};
// Moon crescent polygon (TIMELINE_MOON)
// Outer arc forward: (16,0)→(23,7)→(23,16)→(16,23)→(7,23)→(0,16)
// Inner arc backward: (0,16)→(9,16)→(16,9)→(16,0)
// Combined into one closed crescent shape — no cutout needed.
static const GPoint s_shade_moon_crescent[] = {
  {16,0},{23,7},{23,16},{16,23},{7,23},{0,16},{9,16},{16,9}
};
// Small moon crescent polygon (PARTLY_CLOUDY_NIGHT)
// Outer arc forward: (20,0)→(23,3)→(23,8)→(20,11)→(15,11)→(12,8)
// Inner arc backward: (12,8)→(16,8)→(20,4)→(20,0)
static const GPoint s_shade_moon_sm_crescent[] = {
  {20,0},{23,3},{23,8},{20,11},{15,11},{12,8},{16,8},{20,4}
};

// Per-icon fill descriptor arrays
static const ShadeFill s_shade_cloudy_day[] = {
  { s_shade_cloud_back,  10, WC_GREY, SHADE_NE_SW, false },
  { s_shade_cloud_front, 10, WC_GREY, SHADE_NE_SW, false },
};
static const ShadeFill s_shade_thunderstorm[] = {
  { s_shade_thunder_cloud, 10, WC_GREY,   SHADE_NE_SW, false },
  { s_shade_bolt,           6, WC_YELLOW, SHADE_NW_SE, false },
};
static const ShadeFill s_shade_heavy_rain[] = {
  { s_shade_cloud_std, 10, WC_GREY, SHADE_NE_SW, false },
};
static const ShadeFill s_shade_heavy_snow[] = {
  { s_shade_cloud_std, 10, WC_GREY, SHADE_NE_SW, false },
};
static const ShadeFill s_shade_light_rain[] = {
  { s_shade_cloud_std, 10, WC_GREY, SHADE_NE_SW, false },
};
static const ShadeFill s_shade_light_snow[] = {
  { s_shade_cloud_std, 10, WC_GREY, SHADE_NE_SW, false },
};
static const ShadeFill s_shade_rain_snow[] = {
  { s_shade_cloud_std, 10, WC_GREY, SHADE_NE_SW, false },
};
static const ShadeFill s_shade_partly_cloudy[] = {
  { s_shade_sun_small,   8, WC_AMBER, SHADE_NW_SE, false },
  { s_shade_cloud_partly,10, WC_GREY,  SHADE_NE_SW, false },
};
static const ShadeFill s_shade_timeline_sun[] = {
  { s_shade_sun_oct, 8, WC_AMBER, SHADE_NW_SE, false },
};
static const ShadeFill s_shade_timeline_moon[] = {
  { s_shade_moon_crescent, 8, WC_PALE, SHADE_NW_SE, false },
};
static const ShadeFill s_shade_partly_cloudy_night[] = {
  { s_shade_moon_sm_crescent, 8, WC_PALE, SHADE_NW_SE, false },
  { s_shade_cloud_partly,    10, WC_GREY, SHADE_NE_SW, false },
};
// Unknown: no fill entries (outline only)

typedef struct {
  const ShadeFill *fills;
  int              n_fills;
} ShadeIconDef;

static const ShadeIconDef s_shade_icons[GPATH_ID_COUNT] = {
  [GPATH_ID_CLOUDY_DAY]          = { s_shade_cloudy_day,          2 },
  [GPATH_ID_THUNDERSTORM]        = { s_shade_thunderstorm,         2 },
  [GPATH_ID_HEAVY_RAIN]          = { s_shade_heavy_rain,           1 },
  [GPATH_ID_HEAVY_SNOW]          = { s_shade_heavy_snow,           1 },
  [GPATH_ID_LIGHT_RAIN]          = { s_shade_light_rain,           1 },
  [GPATH_ID_LIGHT_SNOW]          = { s_shade_light_snow,           1 },
  [GPATH_ID_RAINING_AND_SNOWING] = { s_shade_rain_snow,            1 },
  [GPATH_ID_PARTLY_CLOUDY]       = { s_shade_partly_cloudy,        2 },
  [GPATH_ID_TIMELINE_SUN]        = { s_shade_timeline_sun,         1 },
  [GPATH_ID_TIMELINE_MOON]       = { s_shade_timeline_moon,        2 },
  [GPATH_ID_PARTLY_CLOUDY_NIGHT] = { s_shade_partly_cloudy_night,  3 },
  [GPATH_ID_UNKNOWN]             = { NULL,                         0 },
};

// ── Scanline polygon clipper for diagonal lines ───────────────────────────────
// Clips 45° diagonal lines (NE→SW or NW→SE) to a convex-or-concave polygon
// using a simple even-odd edge-intersection scan.
// Works in scaled pixel space.

// Compute x-intersection of horizontal scanline y=scan_y with edge (x0,y0)→(x1,y1).
// Returns false if edge doesn't cross scan_y.
static bool shade_edge_x(int x0, int y0, int x1, int y1, int scan_y, int *out_x) {
  if (y0 == y1) return false;
  if (scan_y < (y0 < y1 ? y0 : y1)) return false;
  if (scan_y >= (y0 > y1 ? y0 : y1)) return false;
  // x = x0 + (scan_y - y0) * (x1 - x0) / (y1 - y0)
  *out_x = x0 + (scan_y - y0) * (x1 - x0) / (y1 - y0);
  return true;
}

// Sort two ints ascending.
static void shade_sort2(int *a, int *b) {
  if (*a > *b) { int t = *a; *a = *b; *b = t; }
}

// Draw hatching lines for one fill polygon.
// poly_scaled: polygon vertices already in screen pixel coords.
// n: number of vertices.
// gap: spacing between lines in pixels.
// dir: SHADE_NE_SW or SHADE_NW_SE.
static void shade_draw_fill(GContext *ctx, const GPoint *poly_scaled, int n,
                            int gap, int dir) {
  if (n < 3) return;

  // Bounding box
  int min_x = poly_scaled[0].x, max_x = poly_scaled[0].x;
  int min_y = poly_scaled[0].y, max_y = poly_scaled[0].y;
  for (int i = 1; i < n; i++) {
    if (poly_scaled[i].x < min_x) min_x = poly_scaled[i].x;
    if (poly_scaled[i].x > max_x) max_x = poly_scaled[i].x;
    if (poly_scaled[i].y < min_y) min_y = poly_scaled[i].y;
    if (poly_scaled[i].y > max_y) max_y = poly_scaled[i].y;
  }

  // For NE→SW: lines where x+y = c.  Scan c from min_x+min_y to max_x+max_y.
  // For NW→SE: lines where x-y = c.  Scan c from min_x-max_y to max_x-min_y.
  int c_start, c_end;
  if (dir == SHADE_NE_SW) {
    c_start = min_x + min_y;
    c_end   = max_x + max_y;
  } else {
    c_start = min_x - max_y;
    c_end   = max_x - min_y;
  }

  // Align c_start to the nearest multiple of gap
  if (c_start < 0) c_start = c_start - (gap - 1 + ((-c_start) % gap)) % gap;
  else             c_start = (c_start / gap) * gap;

  // Intersection buffer (max edges = n)
  int xs[16];  // up to 16 intersections per scanline

  for (int c = c_start; c <= c_end; c += gap) {
    // Find all intersections of the diagonal line with polygon edges.
    // NE→SW: y = c - x  →  substitute into edge equations.
    // NW→SE: y = x - c  →  substitute into edge equations.
    // We convert to a horizontal scanline problem by a coordinate transform:
    //   NE→SW: u = x+y, v = x-y  →  scan u=c, find v intersections.
    //   NW→SE: u = x-y, v = x+y  →  scan u=c, find v intersections.
    // Then convert back to (x,y) to draw.
    int n_xs = 0;
    for (int i = 0; i < n; i++) {
      int j = (i + 1) % n;
      int u0, v0, u1, v1;
      if (dir == SHADE_NE_SW) {
        u0 = poly_scaled[i].x + poly_scaled[i].y;
        v0 = poly_scaled[i].x - poly_scaled[i].y;
        u1 = poly_scaled[j].x + poly_scaled[j].y;
        v1 = poly_scaled[j].x - poly_scaled[j].y;
      } else {
        u0 = poly_scaled[i].x - poly_scaled[i].y;
        v0 = poly_scaled[i].x + poly_scaled[i].y;
        u1 = poly_scaled[j].x - poly_scaled[j].y;
        v1 = poly_scaled[j].x + poly_scaled[j].y;
      }
      int vx;
      if (shade_edge_x(v0, u0, v1, u1, c, &vx)) {
        if (n_xs < 16) xs[n_xs++] = vx;
      }
    }
    if (n_xs < 2) continue;
    // Sort intersections
    for (int a = 0; a < n_xs - 1; a++)
      for (int b = a + 1; b < n_xs; b++)
        shade_sort2(&xs[a], &xs[b]);
    // Draw interior segments (pairs)
    for (int k = 0; k + 1 < n_xs; k += 2) {
      int va = xs[k], vb = xs[k+1];
      GPoint pa, pb;
      if (dir == SHADE_NE_SW) {
        // u=x+y=c, v=x-y  →  x=(u+v)/2, y=(u-v)/2
        pa = GPoint((c + va) / 2, (c - va) / 2);
        pb = GPoint((c + vb) / 2, (c - vb) / 2);
      } else {
        // u=x-y=c, v=x+y  →  x=(u+v)/2, y=(v-u)/2
        pa = GPoint((c + va) / 2, (va - c) / 2);
        pb = GPoint((c + vb) / 2, (vb - c) / 2);
      }
      graphics_draw_line(ctx, pa, pb);
    }
  }
}

// Draw a weather icon with line shading (icon_color_mode == 3).
// Uses weather colours for both the fill hatching and the outline strokes.
static void draw_weather_icon_shaded(GContext *ctx, GPathIconID icon_id,
                                     int ox, int oy, int sz) {
  if (icon_id >= GPATH_ID_COUNT) return;
  const WeatherIconDef *def = &s_weather_icons[icon_id];
  int native_max = def->native_w > def->native_h ? def->native_w : def->native_h;
  if (native_max == 0) return;
  int scale256 = (sz * 256) / native_max;

  // Scaled gap
  int gap = (SHADE_GAP_NATIVE * scale256) / 256;
  if (gap < 2) gap = 2;

  const ShadeIconDef *sdef = &s_shade_icons[icon_id];

  // ── Step 1: Draw shaded fill regions ─────────────────────────────────────
  for (int f = 0; f < sdef->n_fills; f++) {
    const ShadeFill *sf = &sdef->fills[f];
    // Scale polygon to screen coords
    GPoint scaled[16];
    int n = sf->n;
    if (n > 16) n = 16;
    for (int i = 0; i < n; i++) {
      scaled[i] = GPoint(ox + (sf->pts[i].x * scale256) / 256,
                         oy + (sf->pts[i].y * scale256) / 256);
    }

    GColor8 wc; wc.argb = sf->color;
    GColor fill_color = (GColor){ .argb = wc.argb };

    if (sf->cutout) {
      // Erase interior with black hatching (effectively a cutout)
      graphics_context_set_stroke_color(ctx, GColorBlack);
      shade_draw_fill(ctx, scaled, n, gap, sf->dir);
    } else {
      graphics_context_set_stroke_color(ctx, fill_color);
      shade_draw_fill(ctx, scaled, n, gap, sf->dir);
    }
  }

  // ── Step 2: Draw all original gpath outlines on top ───────────────────────
  graphics_context_set_stroke_width(ctx, 1);
  for (int p = 0; p < def->num_paths; p++) {
    GColor8 wc; wc.argb = s_weather_colors[icon_id][p];
    graphics_context_set_stroke_color(ctx, (GColor){ .argb = wc.argb });
    const GPathInfo *pi = &def->paths[p];
    for (int i = 0; i < (int)pi->num_points - 1; i++) {
      GPoint a = GPoint(ox + (pi->points[i].x   * scale256) / 256,
                        oy + (pi->points[i].y   * scale256) / 256);
      GPoint b = GPoint(ox + (pi->points[i+1].x * scale256) / 256,
                        oy + (pi->points[i+1].y * scale256) / 256);
      graphics_draw_line(ctx, a, b);
    }
  }
}

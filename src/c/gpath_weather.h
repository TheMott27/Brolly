#pragma once
#include <pebble.h>

// Weather icon GPath IDs
enum WEATHER_GPATH_ID {
  GPATH_CLOUDY_DAY = 0,
  GPATH_HEAVY_RAIN,
  GPATH_HEAVY_SNOW,
  GPATH_LIGHT_RAIN,
  GPATH_LIGHT_SNOW,
  GPATH_PARTLY_CLOUDY,
  GPATH_RAINING_AND_SNOWING,
  GPATH_TIMELINE_SUN,
  GPATH_TIMELINE_MOON,
  GPATH_PARTLY_CLOUDY_NIGHT,
  GPATH_THUNDERSTORM,
  GPATH_UNKNOWN,
  GPATH_TIMELINE_WEATHER,
  GPATH_WEATHER_UNKNOWN,
};

// Native drawn dimensions for each icon (after cropping to zero blank space).
// Used for scaling: icon fills sz box with no padding.
typedef struct { int16_t w; int16_t h; } GPathBounds;
static const GPathBounds GPATH_BOUNDS[] = {
  [GPATH_CLOUDY_DAY]           = { 23, 14 },
  [GPATH_HEAVY_RAIN]           = { 23, 22 },
  [GPATH_HEAVY_SNOW]           = { 23, 23 },
  [GPATH_LIGHT_RAIN]           = { 23, 23 },
  [GPATH_LIGHT_SNOW]           = { 23, 23 },
  [GPATH_PARTLY_CLOUDY]        = { 23, 24 },
  [GPATH_RAINING_AND_SNOWING]  = { 23, 23 },
  [GPATH_TIMELINE_SUN]         = { 22, 23 },
  [GPATH_TIMELINE_MOON]        = { 15, 22 },
  [GPATH_PARTLY_CLOUDY_NIGHT]  = { 23, 23 },
  [GPATH_THUNDERSTORM]         = { 23, 18 },
  [GPATH_UNKNOWN]              = { 22, 22 },
  [GPATH_TIMELINE_WEATHER]     = { 23, 23 },
  [GPATH_WEATHER_UNKNOWN]      = { 22, 22 },
};

// CLOUDY_DAY — cropped: min_x=1 min_y=6 → subtract (1,6)
static const int CLOUDY_DAY_PATH_COUNT = 2;
static const struct GPathInfo CLOUDY_DAY_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {21,14},{23,12},{23,8},{20,4},{16,4},{13,0},{8,0},
    {4,4},{2,4},{0,6},{0,12},{2,14},{21,14}
  }},
  { .num_points = 2, .points = (GPoint []) { {4,4},{7,7} }}
};

// THUNDERSTORM — cropped: min_x=1 min_y=6 → subtract (1,6)
static const int THUNDERSTORM_PATH_COUNT = 4;
static const struct GPathInfo THUNDERSTORM_PATHS[] = {
  { .num_points = 7, .points = (GPoint []) {
    {8,0},{4,4},{2,4},{0,6},{0,12},{2,14},{7,14}
  }},
  { .num_points = 6, .points = (GPoint []) {
    {18,14},{21,14},{23,12},{23,8},{20,4},{17,4}
  }},
  { .num_points = 3, .points = (GPoint []) {
    {17,4},{13,0},{8,0}
  }},
  { .num_points = 7, .points = (GPoint []) {
    {12,2},{6,10},{10,10},{7,18},{18,8},{13,8},{12,2}
  }}
};

// HEAVY_RAIN — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int HEAVY_RAIN_PATH_COUNT = 5;
static const struct GPathInfo HEAVY_RAIN_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {21,11},{23,9},{23,6},{20,3},{16,3},{13,0},{8,0},
    {5,3},{3,3},{0,6},{0,9},{2,11},{21,11}
  }},
  { .num_points = 2, .points = (GPoint []) { {1,22},{3,14} }},
  { .num_points = 2, .points = (GPoint []) { {6,22},{8,14} }},
  { .num_points = 2, .points = (GPoint []) { {12,18},{13,14} }},
  { .num_points = 2, .points = (GPoint []) { {17,18},{18,14} }}
};

// HEAVY_SNOW — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int HEAVY_SNOW_PATH_COUNT = 8;
static const struct GPathInfo HEAVY_SNOW_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {21,11},{23,9},{23,5},{21,3},{16,3},{13,0},{7,0},
    {4,3},{2,3},{0,5},{0,9},{2,11},{21,11}
  }},
  { .num_points = 2, .points = (GPoint []) { {4,3},{7,6} }},
  { .num_points = 2, .points = (GPoint []) { {4,18},{4,14} }},
  { .num_points = 2, .points = (GPoint []) { {2,16},{6,16} }},
  { .num_points = 2, .points = (GPoint []) { {10,23},{10,19} }},
  { .num_points = 2, .points = (GPoint []) { {8,21},{12,21} }},
  { .num_points = 2, .points = (GPoint []) { {15,18},{15,14} }},
  { .num_points = 2, .points = (GPoint []) { {13,16},{17,16} }}
};

// LIGHT_RAIN — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int LIGHT_RAIN_PATH_COUNT = 6;
static const struct GPathInfo LIGHT_RAIN_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {21,11},{23,9},{23,5},{21,3},{16,3},{13,0},{7,0},
    {4,3},{2,3},{0,5},{0,9},{2,11},{21,11}
  }},
  { .num_points = 2, .points = (GPoint []) { {4,3},{7,6} }},
  { .num_points = 2, .points = (GPoint []) { {13,17},{13,14} }},
  { .num_points = 2, .points = (GPoint []) { {3,17},{3,14} }},
  { .num_points = 2, .points = (GPoint []) { {8,20},{8,17} }},
  { .num_points = 2, .points = (GPoint []) { {3,23},{3,20} }}
};

// LIGHT_SNOW — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int LIGHT_SNOW_PATH_COUNT = 6;
static const struct GPathInfo LIGHT_SNOW_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {21,11},{23,9},{23,5},{21,3},{16,3},{13,0},{7,0},
    {4,3},{2,3},{0,5},{0,9},{2,11},{21,11}
  }},
  { .num_points = 2, .points = (GPoint []) { {4,3},{7,6} }},
  { .num_points = 2, .points = (GPoint []) { {4,18},{4,14} }},
  { .num_points = 2, .points = (GPoint []) { {2,16},{6,16} }},
  { .num_points = 2, .points = (GPoint []) { {10,23},{10,19} }},
  { .num_points = 2, .points = (GPoint []) { {8,21},{12,21} }}
};

// PARTLY_CLOUDY — cropped: min_x=1 min_y=0 → subtract (1,0)
static const int PARTLY_CLOUDY_PATH_COUNT = 8;
static const struct GPathInfo PARTLY_CLOUDY_PATHS[] = {
  { .num_points = 9, .points = (GPoint []) {
    {12,4},{10,6},{10,9},{12,11},{15,11},{17,9},{17,6},{15,4},{12,4}
  }},
  { .num_points = 2, .points = (GPoint []) { {6,1},{8,3} }},
  { .num_points = 2, .points = (GPoint []) { {21,1},{19,3} }},
  { .num_points = 13, .points = (GPoint []) {
    {21,24},{23,22},{23,18},{21,16},{15,16},{12,13},{8,13},
    {5,16},{2,16},{0,18},{0,22},{2,24},{21,24}
  }},
  { .num_points = 2, .points = (GPoint []) { {5,16},{8,19} }},
  { .num_points = 2, .points = (GPoint []) { {20,8},{23,8} }},
  { .num_points = 2, .points = (GPoint []) { {4,8},{7,8} }},
  { .num_points = 2, .points = (GPoint []) { {13,0},{13,3} }}
};

// RAINING_AND_SNOWING — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int RAINING_AND_SNOWING_PATH_COUNT = 6;
static const struct GPathInfo RAINING_AND_SNOWING_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {21,11},{23,9},{23,5},{21,3},{16,3},{13,0},{7,0},
    {4,3},{2,3},{0,5},{0,9},{2,11},{21,11}
  }},
  { .num_points = 2, .points = (GPoint []) { {4,3},{7,6} }},
  { .num_points = 2, .points = (GPoint []) { {4,18},{4,14} }},
  { .num_points = 2, .points = (GPoint []) { {10,23},{10,19} }},
  { .num_points = 2, .points = (GPoint []) { {15,18},{15,14} }},
  { .num_points = 2, .points = (GPoint []) { {13,16},{17,16} }}
};

// TIMELINE_SUN — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int TIMELINE_SUN_PATH_COUNT = 9;
static const struct GPathInfo TIMELINE_SUN_PATHS[] = {
  { .num_points = 9, .points = (GPoint []) {
    {9,5},{5,9},{5,13},{9,17},{13,17},{17,13},{17,9},{13,5},{9,5}
  }},
  { .num_points = 2, .points = (GPoint []) { {11,0},{11,2} }},
  { .num_points = 2, .points = (GPoint []) { {11,23},{11,20} }},
  { .num_points = 2, .points = (GPoint []) { {0,22},{4,18} }},
  { .num_points = 2, .points = (GPoint []) { {2,2},{4,4} }},
  { .num_points = 2, .points = (GPoint []) { {22,22},{18,18} }},
  { .num_points = 2, .points = (GPoint []) { {20,2},{18,4} }},
  { .num_points = 2, .points = (GPoint []) { {2,11},{0,11} }},
  { .num_points = 2, .points = (GPoint []) { {22,11},{20,11} }}
};

// PARTLY_CLOUDY_NIGHT — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int PARTLY_CLOUDY_NIGHT_PATH_COUNT = 3;
static const struct GPathInfo PARTLY_CLOUDY_NIGHT_PATHS[] = {
  { .num_points = 33, .points = (GPoint []) {
    {12,0},{13,0},{14,0},{15,0},{16,0},{17,1},{18,2},
    {19,3},{19,4},{19,5},{19,6},{19,7},{18,8},{17,9},
    {16,10},{15,10},{14,10},{13,10},{12,10},{13,10},
    {13,9},{14,9},{15,8},{15,7},{15,6},{15,5},
    {15,4},{15,3},{15,2},{14,1},{13,1},{13,0},{12,0}
  }},
  { .num_points = 13, .points = (GPoint []) {
    {21,23},{23,21},{23,17},{21,15},{15,15},{12,12},{8,12},
    {5,15},{2,15},{0,17},{0,21},{2,23},{21,23}
  }},
  { .num_points = 2, .points = (GPoint []) { {5,15},{8,18} }}
};

// TIMELINE_MOON — cropped: min_x=3 min_y=1 → subtract (3,1)
static const int TIMELINE_MOON_PATH_COUNT = 1;
static const struct GPathInfo TIMELINE_MOON_PATHS[] = {
  { .num_points = 36, .points = (GPoint []) {
    {0,1},{3,0},{5,0},{7,0},{9,1},{10,2},{12,3},
    {13,5},{14,7},{15,9},{15,11},{15,13},{14,15},
    {13,17},{12,19},{10,20},{9,21},{7,22},{5,22},
    {3,22},{1,21},{2,21},{4,20},{5,18},{6,17},
    {7,15},{8,13},{8,11},{8,9},{7,7},{6,5},
    {5,4},{4,2},{2,1},{1,1},{0,1}
  }}
};

// UNKNOWN — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int UNKNOWN_PATH_COUNT = 3;
static const struct GPathInfo UNKNOWN_PATHS[] = {
  { .num_points = 5, .points = (GPoint []) {
    {11,0},{22,11},{11,22},{0,11},{11,0}
  }},
  { .num_points = 7, .points = (GPoint []) {
    {8,7},{9,5},{11,4},{13,5},{13,8},{11,10},{11,13}
  }},
  { .num_points = 2, .points = (GPoint []) { {11,16},{11,17} }}
};

// TIMELINE_WEATHER — cropped: min_x=1 min_y=1 → subtract (1,1)
static const int TIMELINE_WEATHER_PATH_COUNT = 8;
static const struct GPathInfo TIMELINE_WEATHER_PATHS[] = {
  { .num_points = 9, .points = (GPoint []) {
    {17,5},{15,7},{15,11},{17,13},{21,13},{23,11},{23,7},{21,5},{17,5}
  }},
  { .num_points = 2, .points = (GPoint []) { {19,0},{19,2} }},
  { .num_points = 2, .points = (GPoint []) { {12,2},{14,4} }},
  { .num_points = 13, .points = (GPoint []) {
    {21,19},{23,17},{23,13},{21,11},{14,11},{11,8},{8,8},
    {5,11},{2,11},{0,13},{0,17},{2,19},{21,19}
  }},
  { .num_points = 2, .points = (GPoint []) { {5,11},{7,13} }},
  { .num_points = 2, .points = (GPoint []) { {3,23},{3,22} }},
  { .num_points = 2, .points = (GPoint []) { {7,23},{7,22} }},
  { .num_points = 2, .points = (GPoint []) { {11,23},{11,22} }}
};

// WEATHER_UNKNOWN — same as UNKNOWN (cropped)
static const int WEATHER_UNKNOWN_PATH_COUNT = 3;
static const struct GPathInfo WEATHER_UNKNOWN_PATHS[] = {
  { .num_points = 5, .points = (GPoint []) {
    {11,0},{22,11},{11,22},{0,11},{11,0}
  }},
  { .num_points = 7, .points = (GPoint []) {
    {8,7},{9,5},{11,4},{13,5},{13,8},{11,10},{11,13}
  }},
  { .num_points = 2, .points = (GPoint []) { {11,16},{11,17} }}
};

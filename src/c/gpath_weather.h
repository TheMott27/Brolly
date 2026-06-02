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

// CLOUDY_DAY (single cloud, resized to fill canvas)
static const int CLOUDY_DAY_PATH_COUNT = 2;
static const struct GPathInfo CLOUDY_DAY_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {22, 20}, {24, 18}, {24, 14}, {21, 10}, {17, 10}, {14, 6}, {9, 6},
    {5, 10}, {3, 10}, {1, 12}, {1, 18}, {3, 20}, {22, 20}
  }},
  { .num_points = 2, .points = (GPoint []) { {5, 10}, {8, 13} }}
};

// THUNDERSTORM (Cloudy Day cloud split around bolt, no cloud lines inside bolt)
static const int THUNDERSTORM_PATH_COUNT = 4;
static const struct GPathInfo THUNDERSTORM_PATHS[] = {
  // Cloud left segment (stops at bolt left edge)
  { .num_points = 7, .points = (GPoint []) {
    {9, 6}, {5, 10}, {3, 10}, {1, 12}, {1, 18}, {3, 20}, {8, 20}
  }},
  // Cloud right segment (starts at bolt right edge)
  { .num_points = 6, .points = (GPoint []) {
    {19, 20}, {22, 20}, {24, 18}, {24, 14}, {21, 10}, {18, 10}
  }},
  // Cloud top segment (above bolt)
  { .num_points = 3, .points = (GPoint []) {
    {18, 10}, {14, 6}, {9, 6}
  }},
  // Lightning bolt (closed shape)
  { .num_points = 7, .points = (GPoint []) {
    {13, 8}, {7, 16}, {11, 16}, {8, 24}, {19, 14}, {14, 14}, {13, 8}
  }}
};

// HEAVY_RAIN
static const int HEAVY_RAIN_PATH_COUNT = 5;
static const struct GPathInfo HEAVY_RAIN_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {22, 12}, {24, 10}, {24, 7}, {21, 4}, {17, 4}, {14, 1}, {9, 1},
    {6, 4}, {4, 4}, {1, 7}, {1, 10}, {3, 12}, {22, 12}
  }},
  { .num_points = 2, .points = (GPoint []) { {2, 23}, {4, 15} }},
  { .num_points = 2, .points = (GPoint []) { {7, 23}, {9, 15} }},
  { .num_points = 2, .points = (GPoint []) { {13, 19}, {14, 15} }},
  { .num_points = 2, .points = (GPoint []) { {18, 19}, {19, 15} }}
};

// HEAVY_SNOW
static const int HEAVY_SNOW_PATH_COUNT = 8;
static const struct GPathInfo HEAVY_SNOW_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {22, 12}, {24, 10}, {24, 6}, {22, 4}, {17, 4}, {14, 1}, {8, 1},
    {5, 4}, {3, 4}, {1, 6}, {1, 10}, {3, 12}, {22, 12}
  }},
  { .num_points = 2, .points = (GPoint []) { {5, 4}, {8, 7} }},
  { .num_points = 2, .points = (GPoint []) { {5, 19}, {5, 15} }},
  { .num_points = 2, .points = (GPoint []) { {3, 17}, {7, 17} }},
  { .num_points = 2, .points = (GPoint []) { {11, 24}, {11, 20} }},
  { .num_points = 2, .points = (GPoint []) { {9, 22}, {13, 22} }},
  { .num_points = 2, .points = (GPoint []) { {16, 19}, {16, 15} }},
  { .num_points = 2, .points = (GPoint []) { {14, 17}, {18, 17} }}
};

// LIGHT_RAIN
static const int LIGHT_RAIN_PATH_COUNT = 6;
static const struct GPathInfo LIGHT_RAIN_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {22, 12}, {24, 10}, {24, 6}, {22, 4}, {17, 4}, {14, 1}, {8, 1},
    {5, 4}, {3, 4}, {1, 6}, {1, 10}, {3, 12}, {22, 12}
  }},
  { .num_points = 2, .points = (GPoint []) { {5, 4}, {8, 7} }},
  { .num_points = 2, .points = (GPoint []) { {14, 18}, {14, 15} }},
  { .num_points = 2, .points = (GPoint []) { {4, 18}, {4, 15} }},
  { .num_points = 2, .points = (GPoint []) { {9, 21}, {9, 18} }},
  { .num_points = 2, .points = (GPoint []) { {4, 24}, {4, 21} }}
};

// LIGHT_SNOW
static const int LIGHT_SNOW_PATH_COUNT = 6;
static const struct GPathInfo LIGHT_SNOW_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {22, 12}, {24, 10}, {24, 6}, {22, 4}, {17, 4}, {14, 1}, {8, 1},
    {5, 4}, {3, 4}, {1, 6}, {1, 10}, {3, 12}, {22, 12}
  }},
  { .num_points = 2, .points = (GPoint []) { {5, 4}, {8, 7} }},
  { .num_points = 2, .points = (GPoint []) { {5, 19}, {5, 15} }},
  { .num_points = 2, .points = (GPoint []) { {3, 17}, {7, 17} }},
  { .num_points = 2, .points = (GPoint []) { {11, 24}, {11, 20} }},
  { .num_points = 2, .points = (GPoint []) { {9, 22}, {13, 22} }}
};

// PARTLY_CLOUDY (sun with 7 rays including top vertical, lower cloud)
static const int PARTLY_CLOUDY_PATH_COUNT = 8;
static const struct GPathInfo PARTLY_CLOUDY_PATHS[] = {
  { .num_points = 9, .points = (GPoint []) {
    {13, 4}, {11, 6}, {11, 9}, {13, 11}, {16, 11}, {18, 9}, {18, 6}, {16, 4}, {13, 4}
  }},
  { .num_points = 2, .points = (GPoint []) { {7, 1}, {9, 3} }},
  { .num_points = 2, .points = (GPoint []) { {22, 1}, {20, 3} }},
  { .num_points = 13, .points = (GPoint []) {
    {22, 24}, {24, 22}, {24, 18}, {22, 16}, {16, 16}, {13, 13}, {9, 13},
    {6, 16}, {3, 16}, {1, 18}, {1, 22}, {3, 24}, {22, 24}
  }},
  { .num_points = 2, .points = (GPoint []) { {6, 16}, {9, 19} }},
  { .num_points = 2, .points = (GPoint []) { {21, 8}, {24, 8} }},
  { .num_points = 2, .points = (GPoint []) { {5, 8}, {8, 8} }},
  { .num_points = 2, .points = (GPoint []) { {14, 0}, {14, 3} }}  // top vertical ray (longer)
};

// RAINING_AND_SNOWING
static const int RAINING_AND_SNOWING_PATH_COUNT = 6;
static const struct GPathInfo RAINING_AND_SNOWING_PATHS[] = {
  { .num_points = 13, .points = (GPoint []) {
    {22, 12}, {24, 10}, {24, 6}, {22, 4}, {17, 4}, {14, 1}, {8, 1},
    {5, 4}, {3, 4}, {1, 6}, {1, 10}, {3, 12}, {22, 12}
  }},
  { .num_points = 2, .points = (GPoint []) { {5, 4}, {8, 7} }},
  { .num_points = 2, .points = (GPoint []) { {5, 19}, {5, 15} }},
  { .num_points = 2, .points = (GPoint []) { {11, 24}, {11, 20} }},
  { .num_points = 2, .points = (GPoint []) { {16, 19}, {16, 15} }},
  { .num_points = 2, .points = (GPoint []) { {14, 17}, {18, 17} }}
};

// TIMELINE_SUN
static const int TIMELINE_SUN_PATH_COUNT = 9;
static const struct GPathInfo TIMELINE_SUN_PATHS[] = {
  { .num_points = 9, .points = (GPoint []) {
    {10, 6}, {6, 10}, {6, 14}, {10, 18}, {14, 18}, {18, 14}, {18, 10}, {14, 6}, {10, 6}
  }},
  { .num_points = 2, .points = (GPoint []) { {12, 1}, {12, 3} }},
  { .num_points = 2, .points = (GPoint []) { {12, 24}, {12, 21} }},
  { .num_points = 2, .points = (GPoint []) { {1, 23}, {5, 19} }},
  { .num_points = 2, .points = (GPoint []) { {3, 3}, {5, 5} }},
  { .num_points = 2, .points = (GPoint []) { {23, 23}, {19, 19} }},
  { .num_points = 2, .points = (GPoint []) { {21, 3}, {19, 5} }},
  { .num_points = 2, .points = (GPoint []) { {3, 12}, {1, 12} }},
  { .num_points = 2, .points = (GPoint []) { {23, 12}, {21, 12} }}
};

// PARTLY_CLOUDY_NIGHT (crescent moon upper-right + lower cloud, smooth)
static const int PARTLY_CLOUDY_NIGHT_PATH_COUNT = 3;
static const struct GPathInfo PARTLY_CLOUDY_NIGHT_PATHS[] = {
  // Crescent moon (scaled 0.5x, shifted right+8, smooth)
  { .num_points = 33, .points = (GPoint []) {
    {13, 1}, {14, 1}, {15, 1}, {16, 1}, {17, 1}, {18, 2}, {19, 3},
    {20, 4}, {20, 5}, {20, 6}, {20, 7}, {20, 8}, {19, 9}, {18, 10},
    {17, 11}, {16, 11}, {15, 11}, {14, 11}, {13, 11}, {14, 11},
    {14, 10}, {15, 10}, {16, 9}, {16, 8}, {16, 7}, {16, 6}, {16, 5},
    {16, 4}, {16, 3}, {15, 2}, {14, 2}, {14, 1}, {13, 1}
  }},
  // Cloud
  { .num_points = 13, .points = (GPoint []) {
    {22, 24}, {24, 22}, {24, 18}, {22, 16}, {16, 16}, {13, 13}, {9, 13},
    {6, 16}, {3, 16}, {1, 18}, {1, 22}, {3, 24}, {22, 24}
  }},
  { .num_points = 2, .points = (GPoint []) { {6, 16}, {9, 19} }}
};

// TIMELINE_MOON (crescent moon - single closed outline, smooth, shifted left 6px)
static const int TIMELINE_MOON_PATH_COUNT = 1;
static const struct GPathInfo TIMELINE_MOON_PATHS[] = {
  { .num_points = 36, .points = (GPoint []) {
    {3, 2}, {6, 1}, {8, 1}, {10, 1}, {12, 2}, {13, 3}, {15, 4},
    {16, 6}, {17, 8}, {18, 10}, {18, 12}, {18, 14}, {17, 16},
    {16, 18}, {15, 20}, {13, 21}, {12, 22}, {10, 23}, {8, 23},
    {6, 23}, {4, 22}, {5, 22}, {7, 21}, {8, 19}, {9, 18},
    {10, 16}, {11, 14}, {11, 12}, {11, 10}, {10, 8}, {9, 6},
    {8, 5}, {7, 3}, {5, 2}, {4, 2}, {3, 2}
  }}
};

// UNKNOWN (question mark in a diamond)
static const int UNKNOWN_PATH_COUNT = 3;
static const struct GPathInfo UNKNOWN_PATHS[] = {
  // Diamond outline
  { .num_points = 5, .points = (GPoint []) {
    {12, 1}, {23, 12}, {12, 23}, {1, 12}, {12, 1}
  }},
  // Question mark curve
  { .num_points = 7, .points = (GPoint []) {
    {9, 8}, {10, 6}, {12, 5}, {14, 6}, {14, 9}, {12, 11}, {12, 14}
  }},
  // Question mark dot
  { .num_points = 2, .points = (GPoint []) { {12, 17}, {12, 18} }}
};

// TIMELINE_WEATHER
static const int TIMELINE_WEATHER_PATH_COUNT = 8;
static const struct GPathInfo TIMELINE_WEATHER_PATHS[] = {
  { .num_points = 9, .points = (GPoint []) {
    {18, 6}, {16, 8}, {16, 12}, {18, 14}, {22, 14}, {24, 12}, {24, 8}, {22, 6}, {18, 6}
  }},
  { .num_points = 2, .points = (GPoint []) { {20, 1}, {20, 3} }},
  { .num_points = 2, .points = (GPoint []) { {13, 3}, {15, 5} }},
  { .num_points = 13, .points = (GPoint []) {
    {22, 20}, {24, 18}, {24, 14}, {22, 12}, {15, 12}, {12, 9}, {9, 9},
    {6, 12}, {3, 12}, {1, 14}, {1, 18}, {3, 20}, {22, 20}
  }},
  { .num_points = 2, .points = (GPoint []) { {6, 12}, {8, 14} }},
  { .num_points = 2, .points = (GPoint []) { {4, 24}, {4, 23} }},
  { .num_points = 2, .points = (GPoint []) { {8, 24}, {8, 23} }},
  { .num_points = 2, .points = (GPoint []) { {12, 24}, {12, 23} }}
};

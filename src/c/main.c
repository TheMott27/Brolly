/*
 * AWW2 — Analogue Weather Watchface
 * Pebble watchface
 *
 * Features:
 * - Analogue clock with square perimeter layout
 * - Hour numbers or weather icons at hour positions (toggle via shake)
 * - 10-condition vector weather icon set (GPath, white outline)
 * - Temperature + date complication (moves to avoid minute hand)
 * - Battery indicator in centre cap (yellow <50%, red <20%)
 * - User-configurable hand colours, font, and complication visibility
 *
 * Battery optimisations:
 * - Dedicated seconds layer: only a 1px line redraws per second (not the full bg)
 * - Dynamic tick subscription: MINUTE_UNIT when seconds hand hidden, SECOND_UNIT only when needed
 * - Hour hand angle tracking: skip redraw when angle hasn't changed (moves every 2 min)
 * - Pre-computed marker positions: 60 minute + 12 hour marker points cached in static arrays
 * - Pre-measured number sizes: text dimensions cached, invalidated only on font change
 * - Pre-formatted number strings: static array avoids snprintf in draw loop
 * - Selective layer dirtying in inbox handler: only affected layers redraw on settings change
 * - Conditional accel subscription: unsubscribed when shake is irrelevant
 * - Pre-computed sunrise/sunset marker angles: recalculated only on data arrival
 * - bg_layer only redrawn on hour boundary, show_icons toggle, or new data
 * - Complication layer skipped entirely when both complications are off
 * - Battery handler only redraws on visual threshold crossings (50%, 20%)
 * - time()/localtime() called once per tick; shared via s_tick_tm
 * - App message inbox sized to actual usage, not the SDK maximum
 * - Marker inward direction uses integer alpha-max+beta-min (no sqrt/float)
 * - Zero heap allocation in icon drawing: manual point translation on stack
 * - Font pointer cached; resolved once, invalidated only on settings change
 * - Single icon_code_to_gpath() replaces two-stage slot indirection
 */

#include <pebble.h>

// ============================================================
// CONSTANTS
// ============================================================

// Screen dimensions - set at runtime in window_load
static int s_screen_w = 144;
static int s_screen_h = 168;

// Basalt reference dimensions (144x168) - used for scaling calculations
#define DESIGN_W 144
#define DESIGN_H 168

// Runtime scaling functions that use actual screen dimensions
static inline int POS_X(int px) { return (px * s_screen_w) / DESIGN_W; }
static inline int POS_Y(int py) { return (py * s_screen_h) / DESIGN_H; }

// Weather condition codes (match JS side)
#define ICON_UNKNOWN            0
#define ICON_UNDEFINED          1
#define ICON_CLEAR              2
#define ICON_CLEAR_N            3
#define ICON_PARTLY_CLOUDY      4
#define ICON_PARTLY_CLOUDY_N    5
#define ICON_MOSTLY_CLOUDY      6
#define ICON_MOSTLY_CLOUDY_N    7
#define ICON_CLOUDY             8
#define ICON_CLOUDY_N           9
#define ICON_CHANCE_FLURRIES    10
#define ICON_FLURRIES           11
#define ICON_CHANCE_FLURRIES_N  12
#define ICON_FLURRIES_N         13
#define ICON_CHANCE_RAIN        14
#define ICON_RAIN               15
#define ICON_CHANCE_RAIN_N      16
#define ICON_RAIN_N             17
#define ICON_CHANCE_SLEET       18
#define ICON_SLEET              19
#define ICON_CHANCE_SLEET_N     20
#define ICON_SLEET_N            21
#define ICON_CHANCE_SNOW        22
#define ICON_SNOW               23
#define ICON_CHANCE_SNOW_N      24
#define ICON_SNOW_N             25
#define ICON_CHANCE_TSTORMS     26
#define ICON_TSTORMS            27
#define ICON_CHANCE_TSTORMS_N   28
#define ICON_TSTORMS_N          29
#define ICON_FOG                30
#define ICON_HAZE               31
#define ICON_FOG_N              32
#define ICON_HAZE_N             33

// Message keys — weather data
#define KEY_ICON_0                  0
#define KEY_TEMP_C                 58
#define KEY_TEMP_F                 59

// Message keys — boolean/enum settings
#define KEY_DISPLAY_HOUR_MARKERS   40
#define KEY_DISPLAY_MINOR_MARKERS  41
#define KEY_VIBRATE_BT_DISCONNECT  54
#define KEY_VIBRATE_BT_RECONNECT   55
#define KEY_SHAKE_MODE            107
#define KEY_TEMP_UNIT             110
#define KEY_DATE_VISIBLE          118
#define KEY_TEMP_VISIBLE          119
#define KEY_NUMBER_FONT           121
#define KEY_NUMBER_COLOR_MODE     152
#define KEY_ICON_COLOR_MODE       153
#define KEY_BATTERY_INDICATOR_ENABLED 138
#define KEY_SECONDS_HAND_MODE     142
#define KEY_SECONDS_SHAKE_DUR     143
#define KEY_TEST_BATTERY_ALERT    144
#define KEY_TEST_BT_DISCONNECT    145
#define KEY_TEST_BATTERY_50       146

// Message keys — colour settings (sent as 0xRRGGBB)
#define KEY_HOUR_HAND_OUTER       114
#define KEY_HOUR_HAND_INNER       115
#define KEY_MIN_HAND_OUTER        116
#define KEY_MIN_HAND_INNER        117
#define KEY_BACKGROUND_COLOR      126
#define KEY_NUMBER_COLOR          127
#define KEY_ICON_COLOR            128
#define KEY_HOUR_MARKER_COLOR     129
#define KEY_MINUTE_MARKER_COLOR   130
#define KEY_CENTER_DOT_50_COLOR   131
#define KEY_CENTER_DOT_20_COLOR   132
#define KEY_MIDDLE_RING_20_COLOR  133
#define KEY_DATE_COLOR            134
#define KEY_TEMP_COLOR            135
#define KEY_BT_DISCONNECT_OUTER_COLOR 136
#define KEY_BT_DISCONNECT_INNER_COLOR 137
#define KEY_SUNRISE_HOUR          25
#define KEY_SUNRISE_MINUTE        26
#define KEY_SUNSET_HOUR           27
#define KEY_SUNSET_MINUTE         28
#define KEY_SECONDS_HAND_COLOR    141
#define KEY_SUNRISE_MARKER_VISIBLE 147
#define KEY_SUNRISE_MARKER_COLOR   148
#define KEY_SUNSET_MARKER_COLOR    149

// Sunrise/sunset marker visibility modes
#define SUNRISE_MARKER_ALWAYS       0
#define SUNRISE_MARKER_WITH_WEATHER 1
#define SUNRISE_MARKER_OFF          2

// Seconds hand visibility modes
#define SECONDS_MODE_ALWAYS      1
#define SECONDS_MODE_NEVER       0
#define SECONDS_MODE_SHAKE       2

// Shake mode
#define SHAKE_MODE_ON_SHAKE     0
#define SHAKE_MODE_NUMBERS_ONLY 1
#define SHAKE_MODE_ICONS_ONLY   2

// Complication visibility
#define COMPLICATION_ALWAYS     0
#define COMPLICATION_OFF        1
#define COMPLICATION_SHAKE      2

// Temperature unit
#define TEMP_UNIT_CELSIUS       0
#define TEMP_UNIT_FAHRENHEIT    1

// Persistent storage keys
#define PERSIST_ICONS           0
#define PERSIST_TEMP_C          24
#define PERSIST_TEMP_F          25
#define PERSIST_SETTINGS        30

#define SHAKE_DISPLAY_MS        5000
#define APP_MSG_INBOX_SIZE      512
#define APP_MSG_OUTBOX_SIZE     64

#define FIXED_BATT_PCT_MID       50
#define FIXED_BATT_PCT_LOW       20

// Hand geometry
#define FIXED_HAND_BASE_WIDTH    3
#define FIXED_HAND_OUTER_WIDTH   6
#define FIXED_HAND_INNER_WIDTH   2
#define FIXED_HAND_BASE_PX      20
#if defined(PBL_PLATFORM_EMERY)
  #define FIXED_ICON_SIZE 30
#else
  #define FIXED_ICON_SIZE 24
#endif

// Helper: true on Chalk (round screen)
static inline bool is_round_screen(void) {
#if defined(PBL_PLATFORM_CHALK)
  return true;
#else
  return false;
#endif
}
#define FIXED_HOUR_MARKER_LENGTH 1
// Gap between icon edge and the innermost marker tick (increased to match top icon distance)
#define ICON_MARKER_GAP 12

// Uncomment to enable debug overlays (white bbox, red centre line, green/blue screen lines)
#define DEBUG_ICON_OVERLAY


// ============================================================
// SETTINGS STRUCTURE
// ============================================================

typedef struct {
  bool display_hour_markers;
  bool display_minor_markers;
  int8_t shake_mode;
  int8_t date_visible;
  int8_t temp_visible;
  int8_t temp_unit;
  bool bt_disconnect_min_inner_red;
  GColor bt_disconnect_outer_color;
  GColor bt_disconnect_inner_color;
  bool vibrate_bt_disconnect;
  bool vibrate_bt_reconnect;
  GColor hour_hand_outer;
  GColor hour_hand_inner;
  GColor min_hand_outer;
  GColor min_hand_inner;
  int8_t number_font;
  int8_t number_color_mode;  // 0=single color, 1=rainbow
  int8_t icon_color_mode;    // 0=single color, 1=rainbow
  GColor background_color;
  GColor number_color;
  GColor icon_color;
  GColor hour_marker_color;
  GColor minute_marker_color;
  GColor center_dot_50_color;
  GColor center_dot_20_color;
  GColor middle_ring_20_color;
  GColor date_color;
  GColor temp_color;
  bool battery_indicator_enabled;
  GColor seconds_hand_color;
  int8_t seconds_hand_mode;
  int8_t seconds_shake_dur;
  int8_t sunrise_marker_visible;
  GColor sunrise_marker_color;
  GColor sunset_marker_color;
} Settings;

// ============================================================
// GLOBAL STATE
// ============================================================

static Window *s_window;
static Layer *s_bg_layer;
static Layer *s_seconds_layer;  // Dedicated layer for seconds hand (lightweight redraw)
static Layer *s_hour_layer;
static Layer *s_minute_layer;
static Layer *s_complication_layer;

static Settings s_settings;
static int8_t s_icons[24];
static int16_t s_temp_c = 0;
static int16_t s_temp_f = 32;
static bool s_bt_connected = true;
static uint8_t s_battery_pct = 100;
static bool s_showing_icons = false;
static AppTimer *s_shake_timer = NULL;
static AppTimer *s_seconds_timer = NULL;
static AppTimer *s_numbers_timer = NULL;
static bool s_showing_seconds = false;

// Test mode: temporarily override battery/BT state for 5 seconds
static AppTimer *s_test_timer = NULL;
static uint8_t  s_test_saved_battery_pct = 0;
static bool     s_test_saved_bt_connected = true;
static bool     s_test_active = false;

static bool s_battery_handler_initialized = false;

// Sunrise/sunset times (defaults: 6:00 AM and 6:00 PM)
static int8_t s_sunrise_hour = 6;
static int8_t s_sunrise_min  = 0;
static int8_t s_sunset_hour  = 18;
static int8_t s_sunset_min   = 0;

// Forward declaration
static void cache_sunrise_sunset_markers(void);

// Shared time snapshot
static struct tm s_tick_tm;

// Last hour at which bg_layer was drawn
static int8_t s_bg_last_hour = -1;

// Hour hand angle tracking — skip redraw if unchanged
static int32_t s_last_hour_angle = -1;

// ============================================================
// CACHED MARKER POSITIONS (computed once, reused every redraw)
// ============================================================

// Minute markers: outer and inner points for all 60 markers
static GPoint s_min_marker_outer[60];
static GPoint s_min_marker_inner[60];
static bool s_markers_cached = false;

// Hour tick marks: outer and inner points for all 12
static GPoint s_hour_marker_outer[12];
static GPoint s_hour_marker_inner[12];

// Pre-formatted number strings
static const char *s_num_strings[12] = {
  "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"
};

// Cached number text sizes (invalidated on font change)
static GSize s_num_sizes[12];
static bool s_num_sizes_cached = false;

// Pre-computed sunrise/sunset marker data (recalculated on data arrival)
static bool s_sr_marker_valid = false;
static GPoint s_sr_marker_outer;
static GPoint s_sr_marker_inner;
static bool s_ss_marker_valid = false;
static GPoint s_ss_marker_outer;
static GPoint s_ss_marker_inner;

// ============================================================
// HELPERS
// ============================================================

static GColor rgb_to_gcolor(int32_t rgb) {
  if (rgb == -1) return GColorClear;
  return GColorFromRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// Generate rainbow color based on hour position (0-11)
// Hue cycles through the spectrum: Red -> Orange -> Yellow -> Green -> Cyan -> Blue -> Magenta -> Red
static GColor hour_to_rainbow_color(int h) {
  // Map hour (0-11) to hue (0-360 degrees)
  int hue = (h * 30) % 360;  // 12 hours * 30 = 360 degrees
  
  // Convert HSV to RGB with S=100%, V=100% for vibrant colors
  int r, g, b;
  int h_sector = hue / 60;
  int remainder = (hue % 60) * 255 / 60;
  
  switch (h_sector) {
    case 0:  r = 255; g = remainder; b = 0; break;      // Red -> Yellow
    case 1:  r = 255 - remainder; g = 255; b = 0; break; // Yellow -> Green
    case 2:  r = 0; g = 255; b = remainder; break;       // Green -> Cyan
    case 3:  r = 0; g = 255 - remainder; b = 255; break; // Cyan -> Blue
    case 4:  r = remainder; g = 0; b = 255; break;       // Blue -> Magenta
    case 5:  r = 255; g = 0; b = 255 - remainder; break; // Magenta -> Red
    default: r = 255; g = 0; b = 0; break;              // Fallback to Red
  }
  
  return GColorFromRGB(r, g, b);
}

#if defined(PBL_PLATFORM_APLITE) || defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT)
  #define MONO_COLOR(c) (gcolor_equal((c), GColorBlack) ? GColorBlack : GColorWhite)
#else
  #define MONO_COLOR(c) (c)
#endif

static void settings_set_defaults(Settings *s) {
  s->display_hour_markers        = true;
  s->display_minor_markers       = true;
  s->shake_mode                  = SHAKE_MODE_ON_SHAKE;
  s->date_visible                = COMPLICATION_ALWAYS;
  s->temp_visible                = COMPLICATION_ALWAYS;
  s->temp_unit                   = TEMP_UNIT_CELSIUS;
  s->bt_disconnect_min_inner_red = true;
  s->bt_disconnect_outer_color   = GColorRed;
  s->bt_disconnect_inner_color   = GColorRed;
  s->vibrate_bt_disconnect       = true;
  s->vibrate_bt_reconnect        = false;
  s->hour_hand_outer             = GColorWhite;
  s->hour_hand_inner             = GColorBlack;
  s->min_hand_outer              = GColorBlack;
  s->min_hand_inner              = GColorFromRGB(0, 97, 254);
  s->number_font                 = 0;  // Digital (LECO 28 Light)
  s->number_color_mode           = 0;  // single color
  s->icon_color_mode             = 0;  // single color
  s->background_color            = GColorBlack;
  s->number_color                = GColorWhite;
  s->icon_color                  = GColorWhite;
  s->hour_marker_color           = GColorWhite;
  s->minute_marker_color         = GColorFromRGB(0x6b, 0x7f, 0x99);
  s->center_dot_50_color         = GColorBlack;
  s->center_dot_20_color         = GColorBlack;
  s->middle_ring_20_color        = GColorBlack;
  s->date_color                  = GColorFromRGB(0x4a, 0x5f, 0x7f);
  s->temp_color                  = GColorFromRGB(0x4a, 0x5f, 0x7f);
  s->battery_indicator_enabled   = true;
  s->seconds_hand_color          = GColorWhite;
  s->seconds_hand_mode           = SECONDS_MODE_SHAKE;
  s->seconds_shake_dur           = 10;
  s->sunrise_marker_visible      = SUNRISE_MARKER_ALWAYS;
  s->sunrise_marker_color          = GColorOrange;
  s->sunset_marker_color           = GColorOxfordBlue;
}

static GPoint polar_to_point(GPoint center, int32_t angle, int radius) {
  return GPoint(
    center.x + (int)(radius * sin_lookup(angle) / TRIG_MAX_RATIO),
    center.y - (int)(radius * cos_lookup(angle) / TRIG_MAX_RATIO)
  );
}

static GPoint square_perimeter_point(GPoint center, int32_t angle,
                                     int margin_x, int margin_y) {
  int32_t sin_a = sin_lookup(angle);
  int32_t cos_a = cos_lookup(angle);
  int32_t abs_sin = sin_a < 0 ? -sin_a : sin_a;
  int32_t abs_cos = cos_a < 0 ? -cos_a : cos_a;
  int left   = center.x - margin_x;
  int right  = (s_screen_w - 1 - margin_x) - center.x;
  int top    = center.y - margin_y;
  int bottom = (s_screen_h - 1 - margin_y) - center.y;
  int hw = (sin_a > 0) ? right : left;
  int hh = (cos_a > 0) ? top   : bottom;
  if (hw < 0) hw = 0;
  if (hh < 0) hh = 0;
  int32_t t;
  if (abs_sin == 0 && abs_cos == 0) {
    t = TRIG_MAX_RATIO;
  } else if (abs_sin == 0) {
    t = (int32_t)((int64_t)hh * TRIG_MAX_RATIO / abs_cos);
  } else if (abs_cos == 0) {
    t = (int32_t)((int64_t)hw * TRIG_MAX_RATIO / abs_sin);
  } else {
    int32_t t_w = (int32_t)((int64_t)hw * TRIG_MAX_RATIO / abs_sin);
    int32_t t_h = (int32_t)((int64_t)hh * TRIG_MAX_RATIO / abs_cos);
    t = t_w < t_h ? t_w : t_h;
  }
  GPoint pt = GPoint(
    center.x + (int)((int64_t)sin_a * t / TRIG_MAX_RATIO),
    center.y - (int)((int64_t)cos_a * t / TRIG_MAX_RATIO)
  );
  // Snap edge coordinate to exact boundary pixel
  if (abs_sin >= abs_cos) {
    pt.x = (sin_a > 0) ? (s_screen_w - 1 - margin_x) : margin_x;
  } else {
    pt.y = (cos_a > 0) ? margin_y : (s_screen_h - 1 - margin_y);
  }
  return pt;
}

// Compute and cache all marker positions
static void cache_marker_positions(void) {
  GPoint center = GPoint((s_screen_w - 1) / 2, (s_screen_h - 1) / 2);
  bool round = is_round_screen();
  // Chalk: radius to the inner edge of the bezel (90px for 180px screen)
  int circle_r = (s_screen_w < s_screen_h ? s_screen_w : s_screen_h) / 2;

  // Minute markers
  for (int i = 0; i < 60; i++) {
    int32_t angle = DEG_TO_TRIGANGLE(i * 6);
    GPoint outer_pt;
    int marker_len;
#if defined(PBL_PLATFORM_BASALT)
    marker_len = 2;  // shorter on Basalt
    if (i == 7 || i == 23 || i == 37 || i == 53) marker_len = 4;
#elif defined(PBL_PLATFORM_EMERY)
    marker_len = 4;
    if (i == 7 || i == 23 || i == 37 || i == 53) marker_len = 10;
#elif defined(PBL_PLATFORM_CHALK)
    marker_len = 4;
#else
    marker_len = 4;
#endif
    if (round) {
      outer_pt = polar_to_point(center, angle, circle_r - 1);
      GPoint inner_pt = polar_to_point(center, angle, circle_r - 1 - marker_len);
      s_min_marker_outer[i] = outer_pt;
      s_min_marker_inner[i] = inner_pt;
    } else {
      outer_pt = square_perimeter_point(center, angle, 0, 0);
      int dx = center.x - outer_pt.x;
      int dy = center.y - outer_pt.y;
      int adx = dx < 0 ? -dx : dx;
      int ady = dy < 0 ? -dy : dy;
      int dist = (adx > ady ? adx : ady) + ((adx < ady ? adx : ady) * 3 / 8);
      s_min_marker_outer[i] = outer_pt;
      if (dist > 0) {
        s_min_marker_inner[i] = GPoint(outer_pt.x + dx * marker_len / dist,
                                       outer_pt.y + dy * marker_len / dist);
      } else {
        s_min_marker_inner[i] = outer_pt;
      }
    }
  }

  // Hour tick marks
  for (int h = 0; h < 12; h++) {
    int32_t angle = DEG_TO_TRIGANGLE(h * 30);
    GPoint outer_pt;
    if (round) {
      outer_pt = polar_to_point(center, angle, circle_r - 1);
      GPoint inner_pt = polar_to_point(center, angle, circle_r - 1 - 8);  // 8px hour tick
      s_hour_marker_outer[h] = outer_pt;
      s_hour_marker_inner[h] = inner_pt;
    } else {
      outer_pt = square_perimeter_point(center, angle, 0, 0);
      int dx = center.x - outer_pt.x;
      int dy = center.y - outer_pt.y;
      int adx = dx < 0 ? -dx : dx;
      int ady = dy < 0 ? -dy : dy;
      int dist = (adx > ady ? adx : ady) + ((adx < ady ? adx : ady) * 3 / 8);
      s_hour_marker_outer[h] = outer_pt;
      if (dist > 0) {
        s_hour_marker_inner[h] = GPoint(outer_pt.x + dx * 2 / dist, outer_pt.y + dy * 2 / dist);
      } else {
        s_hour_marker_inner[h] = outer_pt;
      }
    }
  }

  s_markers_cached = true;
}

// Pre-compute sunrise/sunset marker geometry from current data
static void cache_sunrise_sunset_markers(void) {
  GPoint center = GPoint((s_screen_w - 1) / 2, (s_screen_h - 1) / 2);
  bool round = is_round_screen();
  int circle_r = (s_screen_w < s_screen_h ? s_screen_w : s_screen_h) / 2;

  for (int evt = 0; evt < 2; evt++) {
    int8_t eh = (evt == 0) ? s_sunrise_hour : s_sunset_hour;
    int8_t em = (evt == 0) ? s_sunrise_min  : s_sunset_min;
    bool *valid = (evt == 0) ? &s_sr_marker_valid : &s_ss_marker_valid;
    GPoint *outer = (evt == 0) ? &s_sr_marker_outer : &s_ss_marker_outer;
    GPoint *inner = (evt == 0) ? &s_sr_marker_inner : &s_ss_marker_inner;

    if (eh < 0) { *valid = false; continue; }

    int hour12 = (int)eh % 12;
    int rounded_min = (((int)em + 6) / 12) * 12;
    if (rounded_min >= 60) { hour12 = (hour12 + 1) % 12; rounded_min = 0; }
    int marker = hour12 * 5 + rounded_min / 12;
    int32_t angle = DEG_TO_TRIGANGLE(marker * 6);

    if (round) {
      *outer = polar_to_point(center, angle, circle_r - 1);
      *inner = polar_to_point(center, angle, circle_r - 1 - 5);
    } else {
      GPoint opt = square_perimeter_point(center, angle, 0, 0);
      int dx = center.x - opt.x;
      int dy = center.y - opt.y;
      int adx = dx < 0 ? -dx : dx;
      int ady = dy < 0 ? -dy : dy;
      int dist = (adx > ady ? adx : ady) + ((adx < ady ? adx : ady) * 3 / 8);
      *outer = opt;
      if (dist > 0) {
        *inner = GPoint(opt.x + dx * (s_screen_w >= 200 ? 8 : 5) / dist, opt.y + dy * (s_screen_w >= 200 ? 8 : 5) / dist);
      } else {
        *inner = opt;
      }
    }
    *valid = true;
  }
}

// ============================================================
// WEATHER ICON DRAWING
// ============================================================

#include "gpath_weather.h"

static int icon_code_to_gpath(int icon) {
  switch (icon) {
    case ICON_CLEAR:
    case ICON_PARTLY_CLOUDY:     return GPATH_TIMELINE_SUN;
    case ICON_CLEAR_N:
    case ICON_PARTLY_CLOUDY_N:
    case ICON_MOSTLY_CLOUDY_N:
    case ICON_CLOUDY_N:          return GPATH_TIMELINE_MOON;
    case ICON_MOSTLY_CLOUDY:     return GPATH_PARTLY_CLOUDY;
    case ICON_CLOUDY:
    case ICON_FOG:
    case ICON_FOG_N:
    case ICON_HAZE:
    case ICON_HAZE_N:            return GPATH_CLOUDY_DAY;
    case ICON_CHANCE_RAIN:
    case ICON_CHANCE_RAIN_N:     return GPATH_LIGHT_RAIN;
    case ICON_RAIN:
    case ICON_RAIN_N:            return GPATH_HEAVY_RAIN;
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
    case ICON_SLEET_N:           return GPATH_HEAVY_SNOW;
    case ICON_CHANCE_TSTORMS:
    case ICON_TSTORMS:
    case ICON_CHANCE_TSTORMS_N:
    case ICON_TSTORMS_N:         return GPATH_THUNDERSTORM;
    default:                     return GPATH_UNKNOWN;
  }
}

// Edge alignment modes for draw_weather_icon
typedef enum {
  ICON_ALIGN_CENTER = 0,
  ICON_ALIGN_RIGHT,   // right edge of drawn icon aligns to center.x
  ICON_ALIGN_LEFT,    // left edge of drawn icon aligns to center.x
  ICON_ALIGN_TOP,     // top edge of drawn icon aligns to center.y
  ICON_ALIGN_BOTTOM,  // bottom edge of drawn icon aligns to center.y
} IconAlign;

static void draw_weather_icon_aligned(GContext *ctx, int8_t icon, GPoint center, int sz, int h, IconAlign align) {
  int gpath_id = icon_code_to_gpath(icon);
  int path_count = 0;
  const GPathInfo *paths = NULL;

  switch (gpath_id) {
    case GPATH_CLOUDY_DAY:
      path_count = CLOUDY_DAY_PATH_COUNT;         paths = CLOUDY_DAY_PATHS;         break;
    case GPATH_HEAVY_RAIN:
      path_count = HEAVY_RAIN_PATH_COUNT;         paths = HEAVY_RAIN_PATHS;         break;
    case GPATH_HEAVY_SNOW:
      path_count = HEAVY_SNOW_PATH_COUNT;         paths = HEAVY_SNOW_PATHS;         break;
    case GPATH_LIGHT_RAIN:
      path_count = LIGHT_RAIN_PATH_COUNT;         paths = LIGHT_RAIN_PATHS;         break;
    case GPATH_PARTLY_CLOUDY:
      path_count = PARTLY_CLOUDY_PATH_COUNT;      paths = PARTLY_CLOUDY_PATHS;      break;
    case GPATH_TIMELINE_SUN:
      path_count = TIMELINE_SUN_PATH_COUNT;       paths = TIMELINE_SUN_PATHS;       break;
    case GPATH_TIMELINE_MOON:
      path_count = TIMELINE_MOON_PATH_COUNT;      paths = TIMELINE_MOON_PATHS;      break;
    case GPATH_PARTLY_CLOUDY_NIGHT:
      path_count = PARTLY_CLOUDY_NIGHT_PATH_COUNT; paths = PARTLY_CLOUDY_NIGHT_PATHS; break;
    case GPATH_THUNDERSTORM:
      path_count = THUNDERSTORM_PATH_COUNT;       paths = THUNDERSTORM_PATHS;       break;
    default:
      path_count = UNKNOWN_PATH_COUNT;            paths = UNKNOWN_PATHS;            break;
  }

  // Icons are now cropped: each icon's coordinate space starts at (0,0).
  // GPATH_BOUNDS[].w and .h are the actual drawn dimensions in native coords.
  // Scale independently on x and y so the icon fills sz x sz exactly.
  int native_w = 24, native_h = 24;
  if (gpath_id >= 0 && gpath_id < (int)(sizeof(GPATH_BOUNDS)/sizeof(GPATH_BOUNDS[0]))) {
    native_w = GPATH_BOUNDS[gpath_id].w;
    native_h = GPATH_BOUNDS[gpath_id].h;
    if (native_w < 1) native_w = 24;
    if (native_h < 1) native_h = 24;
  }
  // Use the smaller dimension to preserve aspect ratio (icon fits within sz box)
  int native_max = (native_w > native_h) ? native_w : native_h;
  int scale256 = (sz * 256) / native_max;  // uniform scale to fit within sz
  int scaled_w = (native_w * scale256) / 256;
  int scaled_h = (native_h * scale256) / 256;
  int half = sz / 2;
  // Default: centre the icon within the sz box
  int ox = center.x - scaled_w / 2;
  int oy = center.y - scaled_h / 2;

  // For edge-aligned modes, align the drawn edge to center.x or center.y
  if (align == ICON_ALIGN_RIGHT) {
    ox = center.x - scaled_w;  // right edge at center.x
    oy = center.y - scaled_h / 2;
  } else if (align == ICON_ALIGN_LEFT) {
    ox = center.x;             // left edge at center.x
    oy = center.y - scaled_h / 2;
  } else if (align == ICON_ALIGN_TOP) {
    ox = center.x - scaled_w / 2;
    oy = center.y;             // top edge at center.y
  } else if (align == ICON_ALIGN_BOTTOM) {
    ox = center.x - scaled_w / 2;
    oy = center.y - scaled_h;  // bottom edge at center.y
  }
  (void)half;

  // Choose color based on mode: rainbow or single
  GColor icon_color = (s_settings.icon_color_mode == 1) 
    ? hour_to_rainbow_color(h) 
    : s_settings.icon_color;

  graphics_context_set_stroke_color(ctx, MONO_COLOR(icon_color));
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < path_count; i++) {
    int npts = paths[i].num_points;
    if (npts < 2) continue;
    for (int j = 0; j < npts; j++) {
      GPoint a = GPoint(ox + (paths[i].points[j].x * scale256) / 256,
                        oy + (paths[i].points[j].y * scale256) / 256);
      GPoint b = GPoint(ox + (paths[i].points[(j+1)%npts].x * scale256) / 256,
                        oy + (paths[i].points[(j+1)%npts].y * scale256) / 256);
      graphics_draw_line(ctx, a, b);
    }
  }

#ifdef DEBUG_ICON_OVERLAY
  // White bounding box around rendered icon
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(ox, oy, scaled_w, scaled_h));
  // Red horizontal line through rendered centre
  int icon_centre_y = oy + scaled_h / 2;
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_draw_line(ctx, GPoint(ox, icon_centre_y), GPoint(ox + scaled_w, icon_centre_y));
#endif
}

static void draw_weather_icon(GContext *ctx, int8_t icon, GPoint center, int sz, int h) {
  draw_weather_icon_aligned(ctx, icon, center, sz, h, ICON_ALIGN_CENTER);
}

// ============================================================
// HOUR NUMBER DRAWING
// ============================================================

// Icon size is fixed per platform, no user configuration
static int get_icon_size(void) {
  return FIXED_ICON_SIZE;
}

// Select font based on screen dimensions
static GFont get_complication_font(void) {
  // Scale font selection based on screen width
  if (s_screen_w >= 200) {
    return fonts_get_system_font(FONT_KEY_GOTHIC_24);  // Emery: slightly larger
  } else if (s_screen_w >= 180) {
    return fonts_get_system_font(FONT_KEY_GOTHIC_18);  // Chalk: same as Basalt
  } else {
    return fonts_get_system_font(FONT_KEY_GOTHIC_14);  // Basalt, Diorite: smaller
  }
}

static GFont s_cached_number_font = NULL;

static GFont resolve_number_font(int8_t id) {
  // Font sizes scale as percentages of screen width:
  // Index 0, 2, 3: ~10% of screen width
  // Index 1: ~13% of screen width
  // Index 4: ~20% of screen width
  // Select fonts based on actual screen size to maintain these ratios
  
  int screen_w = s_screen_w > 0 ? s_screen_w : 144;  // Default to Basalt if not yet set
  
  // Supported: Basalt (144px) and Emery (200px)
  
  switch (id) {
    case 0:  // 10% screen width (Digital)
      if (screen_w >= 200) return fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS);  // Emery: 28px light
      else return fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS);  // Basalt: 28px light
    case 1:  // 13% screen width
      if (screen_w >= 200) return fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);  // Emery: 42px ~21%
      else return fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);  // Basalt: 34px ~23.6%
    case 2:  // 10% screen width
      return fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD);  // Both: 28px
    case 3:  // 10% screen width
      if (screen_w >= 200) return fonts_get_system_font(FONT_KEY_GOTHIC_28);  // Emery: 28px ~14%
      else return fonts_get_system_font(FONT_KEY_GOTHIC_24);  // Basalt: 24px ~16.7%
    case 4:  // 20% screen width
      return fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);  // Both: 42px
    default: return fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS);
  }
}

static GFont get_number_font(void) {
  if (!s_cached_number_font) {
    s_cached_number_font = resolve_number_font(s_settings.number_font);
  }
  return s_cached_number_font;
}

// Cache text sizes for all 12 numbers
static void cache_number_sizes(void) {
  GFont font = get_number_font();
  for (int h = 0; h < 12; h++) {
    int measure_box = (s_screen_w * 50) / 100;  // TEST: 50% of screen width for measurement
    GSize sz = graphics_text_layout_get_content_size(s_num_strings[h], font,
      GRect(0, 0, measure_box, measure_box), GTextOverflowModeWordWrap, GTextAlignmentCenter);
    s_num_sizes[h] = GSize(sz.w + 4, sz.h);  // Remove vertical padding
  }
  s_num_sizes_cached = true;
}

static void draw_hour_number(GContext *ctx, int h, GPoint center, GFont font) {
  const char *buf = s_num_strings[h];
  if (!s_num_sizes_cached) cache_number_sizes();
  int tw = s_num_sizes[h].w;
  int th = s_num_sizes[h].h;
  // Actual rendered text offsets within GRect: top 7px, bottom 0px, left 3px, right 3px
  // Shift GRect up by 7px so actual text top aligns with center.y - actual_h/2
  int actual_h = th - 7;
  int tx = center.x - tw / 2;
  int ty = center.y - actual_h / 2 - 7;  // Shift up 7px so text renders at actual_h center
  
  // Choose color based on mode: rainbow or single
  GColor text_color = (s_settings.number_color_mode == 1) 
    ? hour_to_rainbow_color(h) 
    : s_settings.number_color;
  
  graphics_context_set_text_color(ctx, MONO_COLOR(text_color));
  
  graphics_draw_text(ctx, buf, font, GRect(tx, ty, tw, th),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  

}

// ============================================================
// SECONDS LAYER — lightweight per-second redraw
// ============================================================

static void seconds_layer_update(Layer *layer, GContext *ctx) {
  if (s_settings.seconds_hand_mode == SECONDS_MODE_NEVER) return;
  if (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE && !s_showing_seconds) return;

  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint((bounds.size.w - 1) / 2, (bounds.size.h - 1) / 2);
  int32_t sec_angle = DEG_TO_TRIGANGLE(s_tick_tm.tm_sec * 6);
  GPoint sec_tip;
  if (is_round_screen()) {
    int circle_r = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2;
    sec_tip = polar_to_point(center, sec_angle, circle_r - 2);
  } else {
    sec_tip = square_perimeter_point(center, sec_angle, 0, 0);
  }
  GPoint sec_tail = polar_to_point(center, sec_angle + DEG_TO_TRIGANGLE(180), POS_Y(18));
  graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.seconds_hand_color));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, sec_tail, sec_tip);
}

// ============================================================
// BACKGROUND LAYER — markers, numbers, weather icons
// Redrawn only on hour boundary, show_icons toggle, or new data.
// ============================================================

static void bg_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  s_screen_w = bounds.size.w;
  s_screen_h = bounds.size.h;
  GPoint center = GPoint((s_screen_w - 1) / 2, (s_screen_h - 1) / 2);

  // Ensure marker positions are cached
  if (!s_markers_cached) {
    cache_marker_positions();
    cache_sunrise_sunset_markers(); // initialise with default/current times
  }

  graphics_context_set_fill_color(ctx, MONO_COLOR(s_settings.background_color));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);



  bool show_icons = false;
  switch (s_settings.shake_mode) {
    case SHAKE_MODE_ON_SHAKE:     show_icons = s_showing_icons; break;
    case SHAKE_MODE_NUMBERS_ONLY: show_icons = false;           break;
    case SHAKE_MODE_ICONS_ONLY:   show_icons = true;            break;
  }

  // ---- Minute markers (60 × 1px dot) ----
  if (s_settings.display_minor_markers) {
    graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.minute_marker_color));
    graphics_context_set_stroke_width(ctx, 1);
    for (int i = 0; i < 60; i++) {
      graphics_draw_line(ctx, s_min_marker_inner[i], s_min_marker_outer[i]);
    }
  }

  // ---- Hour tick marks (12 × 3px wide, 1px deep) ----
  if (s_settings.display_hour_markers) {
    graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.hour_marker_color));
    graphics_context_set_stroke_width(ctx, 3);
    for (int h = 0; h < 12; h++) {
      graphics_draw_line(ctx, s_hour_marker_inner[h], s_hour_marker_outer[h]);
    }
  }

  // ---- Sunrise / sunset markers ----
  bool show_sr_ss = false;
  switch (s_settings.sunrise_marker_visible) {
    case SUNRISE_MARKER_ALWAYS:       show_sr_ss = true;            break;
    case SUNRISE_MARKER_WITH_WEATHER: show_sr_ss = show_icons;      break;
    case SUNRISE_MARKER_OFF:          show_sr_ss = false;           break;
  }
  if (show_sr_ss) {
    int now_min = s_tick_tm.tm_hour * 60 + s_tick_tm.tm_min;
    if (s_sr_marker_valid) {
      int event_min = (int)s_sunrise_hour * 60 + (int)s_sunrise_min;
      int delta = event_min - now_min;
      if (delta < 0) delta += 1440;
      if (delta <= 720) {
        // Draw 2px coloured marker
        graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.sunrise_marker_color));
        graphics_context_set_stroke_width(ctx, 2);
        graphics_draw_line(ctx, s_sr_marker_inner, s_sr_marker_outer);
      }
    }
    if (s_ss_marker_valid) {
      int event_min = (int)s_sunset_hour * 60 + (int)s_sunset_min;
      int delta = event_min - now_min;
      if (delta < 0) delta += 1440;
      if (delta <= 720) {
        // Draw 2px coloured marker
        graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.sunset_marker_color));
        graphics_context_set_stroke_width(ctx, 2);
        graphics_draw_line(ctx, s_ss_marker_inner, s_ss_marker_outer);
      }
    }
  }

  // ---- Hour numbers / icons ----
  GFont num_font = get_number_font();
  if (!s_num_sizes_cached) cache_number_sizes();
  int cur_hour = s_tick_tm.tm_hour;
  int cur_min  = s_tick_tm.tm_min;
  bool round_screen = is_round_screen();
  // Chalk: place numbers/icons on a circle inset from the edge
  int circle_r = (s_screen_w < s_screen_h ? s_screen_w : s_screen_h) / 2;
  // Inset radius for icon/number centres: leave room for the icon half-size + marker gap
  int chalk_icon_r  = circle_r - FIXED_ICON_SIZE / 2 - ICON_MARKER_GAP;
  int chalk_num_r   = circle_r - 22;  // ~22px from edge for numbers

  for (int h = 0; h < 12; h++) {
    int32_t angle = DEG_TO_TRIGANGLE(h * 30);
    bool is_top_bottom = (h == 0 || h == 1 || h == 5 || h == 6 || h == 7 || h == 11);
    GPoint pos = round_screen
      ? polar_to_point(center, angle, chalk_num_r)
      : square_perimeter_point(center, angle, 0, 0);

    if (show_icons) {
      int sz = get_icon_size();
      int half = sz / 2;
      GPoint icon_center;
      if (round_screen) {
        // Chalk: place icons on a circle
        icon_center = polar_to_point(center, angle, chalk_icon_r);
      } else if (s_screen_w >= 200) {
        // Emery: icon_center carries the margin boundary for the aligned axis;
        // draw_weather_icon_aligned will shift ox/oy so the icon edge lands there.
        icon_center = pos;
        if (is_top_bottom) {
          // top group: icon_center.y = top margin boundary
          if (h == 0 || h == 1 || h == 11) {
            icon_center.y = ICON_MARKER_GAP;
          } else {
            // bottom group: icon_center.y = bottom margin boundary
            icon_center.y = (s_screen_h - 1) - ICON_MARKER_GAP;
          }
          // Align h=1,5,7,11 x to angular ray intersection, shifted 7px toward centre
          if (h == 1 || h == 5 || h == 7 || h == 11) {
            int ray_dx = sin_lookup(angle);
            int ray_dy = -cos_lookup(angle);
            int raw_x;
            if (ray_dy < 0) {
              raw_x = center.x + (int32_t)ray_dx * center.y / (-ray_dy);
            } else {
              raw_x = center.x + (int32_t)ray_dx * (s_screen_h - 1 - center.y) / ray_dy;
            }
            icon_center.x = raw_x + (raw_x > center.x ? -7 : 7);
          }
        } else {
          int y_mid = (s_screen_h - 1) / 2;  // 113 — h=3,9
          int y_bot = (s_screen_h - 1) - ICON_MARKER_GAP;  // 215 — h=5,6,7 boundary
          // h=1,11 are top-aligned: top edge at ICON_MARKER_GAP.
          // Rendered centre of top icons (ALIGN_TOP): icon_center.y + scaled_h/2 ≈ gap + 14 = 26
          // Rendered centre of bottom icons (ALIGN_BOTTOM): icon_center.y - scaled_h/2 ≈ bot - 14 = 201
          // h=2,10 y = midpoint(26, 113) = 69
          // h=4,8  y = midpoint(113, 201) = 157
          int y_2_10 = (ICON_MARKER_GAP + 14 + y_mid) / 2;  // 69
          int y_4_8  = (y_mid + y_bot - 14) / 2;            // 157
          // right group: icon_center.x = right margin boundary
          if (h == 8 || h == 9 || h == 10) {
            // left group: icon_center.x = left margin boundary
            icon_center.x = ICON_MARKER_GAP;
          } else {
            icon_center.x = (s_screen_w - 1) - ICON_MARKER_GAP;
          }
          if (h == 2 || h == 10) {
            icon_center.y = y_2_10;
          } else if (h == 3 || h == 9) {
            icon_center.y = y_mid;
          } else if (h == 4 || h == 8) {
            icon_center.y = y_4_8;
          }
        }
      } else {
        // Basalt/Diorite/Flint: edge-aligned for top/bottom, centred for sides
        int bas_margin = 6;  // inset from screen edge
        icon_center = square_perimeter_point(center, angle, half + bas_margin, half + bas_margin);
        if (is_top_bottom) {
          // Pass margin boundary; alignment mode will place the icon edge there
          if (h == 0 || h == 1 || h == 11) {
            icon_center.y = bas_margin;  // top margin boundary
          } else {
            icon_center.y = (s_screen_h - 1) - bas_margin;  // bottom margin boundary
          }
          // Align h=1,5,7,11 x to angular ray + 2px toward centre
          if (h == 1 || h == 5 || h == 7 || h == 11) {
            int ray_dx = sin_lookup(angle);
            int ray_dy = -cos_lookup(angle);
            int raw_x;
            if (ray_dy < 0) {
              raw_x = center.x + (int32_t)ray_dx * center.y / (-ray_dy);
            } else {
              raw_x = center.x + (int32_t)ray_dx * (s_screen_h - 1 - center.y) / ray_dy;
            }
            icon_center.x = raw_x + (raw_x > center.x ? -2 : 2);
          }
        } else {
          int y_mid = (s_screen_h - 1) / 2;
          int y_top = bas_margin;
          int y_bot = (s_screen_h - 1) - bas_margin;
          // Centre icon on x axis so it stays on screen
          if (h == 8 || h == 9 || h == 10) {
            icon_center.x = half + bas_margin;
          } else {
            icon_center.x = (s_screen_w - 1) - (half + bas_margin);
          }
          if (h == 2 || h == 10) {
            icon_center.y = (y_top + y_mid) / 2;
          } else if (h == 3 || h == 9) {
            icon_center.y = y_mid;
          } else if (h == 4 || h == 8) {
            icon_center.y = (y_mid + y_bot) / 2;
          }
        }
      }
      int clock_num = (h == 0) ? 12 : h;
      int am_hour   = (clock_num == 12) ? 0  : clock_num;
      int pm_hour   = (clock_num == 12) ? 12 : clock_num + 12;
      bool am_passed = (am_hour < cur_hour) || (am_hour == cur_hour && cur_min > 0);
      bool pm_passed = (pm_hour < cur_hour) || (pm_hour == cur_hour && cur_min > 0);
      int icon_hour  = (!am_passed) ? am_hour : (!pm_passed) ? pm_hour : am_hour;
      int8_t icon = s_icons[icon_hour];
      if (icon < 0) icon = ICON_UNKNOWN;
      // Align each icon's outer edge to the margin boundary on Emery and Basalt
      IconAlign align = ICON_ALIGN_CENTER;
      if (!round_screen) {
        if (h == 0 || h == 1 || h == 11) {
          align = ICON_ALIGN_TOP;     // top edge to margin
        } else if (h == 5 || h == 6 || h == 7) {
          align = ICON_ALIGN_BOTTOM;  // bottom edge to margin
        } else if (s_screen_w >= 200) {
          // side alignment only on Emery (Basalt sides remain centred)
          if (h == 2 || h == 3 || h == 4) {
            align = ICON_ALIGN_RIGHT;
          } else if (h == 8 || h == 9 || h == 10) {
            align = ICON_ALIGN_LEFT;
          }
        }
      }
      draw_weather_icon_aligned(ctx, icon, icon_center, sz, h, align);

    } else if (s_settings.display_hour_markers) {
      if (round_screen) {
        // Chalk: pos is already on the circle (chalk_num_r), draw directly
        draw_hour_number(ctx, h, pos, num_font);
      } else {
        int ntw = s_num_sizes[h].w;
        int nth = s_num_sizes[h].h;
        // Actual rendered size offsets from GRect: top 7, bottom 0, left 3, right 3
        // Position so actual text edge is 6px from screen edge
        // actual_h = nth - 7, actual_w = ntw - 6
        // draw_hour_number centers on actual_h, so pos.y = actual_top + actual_h/2
        int actual_h = nth - 7;
        int actual_w = ntw - 6;
        int margin = (s_screen_w >= 200) ? 12 : 6;
        if (is_top_bottom) {
          if (h == 0 || h == 1 || h == 11) {
            pos.y = margin + actual_h / 2;
          } else {
            pos.y = (s_screen_h - 1 - margin) - actual_h / 2;
          }
          // Align h=1,5,7,11 x to angular ray intersection, shifted toward centre
          // Basalt: 2px, Emery: 4px
          if (h == 1 || h == 5 || h == 7 || h == 11) {
            int32_t ray_angle = DEG_TO_TRIGANGLE(h * 30);
            int ray_dx = sin_lookup(ray_angle);
            int ray_dy = -cos_lookup(ray_angle);
            int raw_x;
            if (ray_dy < 0) {
              raw_x = center.x + (int32_t)ray_dx * center.y / (-ray_dy);
            } else {
              raw_x = center.x + (int32_t)ray_dx * (s_screen_h - 1 - center.y) / ray_dy;
            }
            int shift = (s_screen_w >= 200) ? 7 : 2;
            pos.x = raw_x + (raw_x > center.x ? -shift : shift);
          }
        } else {
          int y_top = margin + actual_h / 2;
          int y_mid = (s_screen_h - 1) / 2;
          int y_bot = (s_screen_h - 1 - margin) - actual_h / 2;
          int y2 = (y_top + y_mid) / 2;
          int y3 = y_mid;
          int y4 = (y_mid + y_bot) / 2;
          if (h == 8 || h == 9 || h == 10) {
            pos.x = margin + actual_w / 2;
          } else {
            pos.x = (s_screen_w - 1 - margin) - actual_w / 2;
          }
          if (h == 2 || h == 10) {
            pos.y = y2;
          } else if (h == 3 || h == 9) {
            pos.y = y3;
          } else if (h == 4 || h == 8) {
            pos.y = y4;
          }
        }
        draw_hour_number(ctx, h, pos, num_font);
      }
    }
  }

#ifdef DEBUG_ICON_OVERLAY
  // Green lines at 25%, 50%, 75% of screen height
  graphics_context_set_stroke_color(ctx, GColorGreen);
  graphics_context_set_stroke_width(ctx, 1);
  int g25 = (s_screen_h - 1) / 4;
  int g50 = (s_screen_h - 1) / 2;
  int g75 = (s_screen_h - 1) * 3 / 4;
  graphics_draw_line(ctx, GPoint(0, g25), GPoint(s_screen_w - 1, g25));
  graphics_draw_line(ctx, GPoint(0, g50), GPoint(s_screen_w - 1, g50));
  graphics_draw_line(ctx, GPoint(0, g75), GPoint(s_screen_w - 1, g75));
  // Blue lines at midpoint between rendered centres of icons 1&3 and 3&5
  // Rendered centre of h=1 (top-aligned, top edge at ICON_MARKER_GAP): gap + scaled_h/2 ~ gap+14
  // Rendered centre of h=3 (side, centre at y_mid)
  // Rendered centre of h=5 (bottom-aligned, bottom edge at bot): bot - scaled_h/2 ~ bot-14
  int emery = (s_screen_w >= 200);
  int gap2 = emery ? ICON_MARKER_GAP : 6;
  int bot2 = (s_screen_h - 1) - gap2;
  int mid2 = (s_screen_h - 1) / 2;
  int h1_cy = gap2 + 14;
  int h5_cy = bot2 - 14;
  int blue13 = (h1_cy + mid2) / 2;
  int blue35 = (mid2 + h5_cy) / 2;
  graphics_context_set_stroke_color(ctx, GColorBlue);
  graphics_draw_line(ctx, GPoint(0, blue13), GPoint(s_screen_w - 1, blue13));
  graphics_draw_line(ctx, GPoint(0, blue35), GPoint(s_screen_w - 1, blue35));
#endif

  s_bg_last_hour = (int8_t)cur_hour;
}

// ============================================================
// HAND DRAWING
// ============================================================

static void draw_inittick_hand(GContext *ctx, GPoint center, GPoint tip,
                               GColor outer_color, GColor inner_color) {
  int dx = tip.x - center.x;
  int dy = tip.y - center.y;
  int adx = dx < 0 ? -dx : dx;
  int ady = dy < 0 ? -dy : dy;
  int dist = (adx > ady ? adx : ady) + ((adx < ady ? adx : ady) * 3 / 8);
  if (dist == 0) return;
  int hand_base_pos = POS_Y(FIXED_HAND_BASE_PX);
  GPoint base_pt = {
    .x = center.x + dx * hand_base_pos / dist,
    .y = center.y + dy * hand_base_pos / dist,
  };
  graphics_context_set_stroke_color(ctx, outer_color);
  graphics_context_set_stroke_width(ctx, FIXED_HAND_BASE_WIDTH);
  graphics_draw_line(ctx, center, base_pt);
  graphics_context_set_stroke_width(ctx, FIXED_HAND_OUTER_WIDTH);
  graphics_draw_line(ctx, base_pt, tip);
  graphics_context_set_fill_color(ctx, outer_color);
  graphics_fill_circle(ctx, base_pt, FIXED_HAND_OUTER_WIDTH / 2);
  graphics_fill_circle(ctx, tip,     FIXED_HAND_OUTER_WIDTH / 2);
  graphics_context_set_stroke_color(ctx, inner_color);
  graphics_context_set_stroke_width(ctx, FIXED_HAND_INNER_WIDTH);
  graphics_draw_line(ctx, base_pt, tip);
  graphics_context_set_fill_color(ctx, inner_color);
  graphics_fill_circle(ctx, base_pt, FIXED_HAND_INNER_WIDTH / 2);
  graphics_fill_circle(ctx, tip,     FIXED_HAND_INNER_WIDTH / 2);
}

static void hour_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint((bounds.size.w - 1) / 2, (bounds.size.h - 1) / 2);
  int radius = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2;
  int32_t angle = DEG_TO_TRIGANGLE(
    (s_tick_tm.tm_hour % 12) * 30 + s_tick_tm.tm_min / 2);
  draw_inittick_hand(ctx, center, polar_to_point(center, angle, radius * 60 / 100),
                     MONO_COLOR(s_settings.hour_hand_outer), MONO_COLOR(s_settings.hour_hand_inner));
}

static void minute_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint((bounds.size.w - 1) / 2, (bounds.size.h - 1) / 2);
  int radius = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2;
  int32_t angle = DEG_TO_TRIGANGLE(s_tick_tm.tm_min * 6);
  GColor outer = (!s_bt_connected && s_settings.bt_disconnect_min_inner_red)
                 ? s_settings.bt_disconnect_outer_color : s_settings.min_hand_outer;
  GColor inner = (!s_bt_connected && s_settings.bt_disconnect_min_inner_red)
                 ? s_settings.bt_disconnect_inner_color : s_settings.min_hand_inner;
  outer = MONO_COLOR(outer);
  inner = MONO_COLOR(inner);

  // Draw minute hand beneath cap
  draw_inittick_hand(ctx, center, polar_to_point(center, angle, radius * 95 / 100),
                     outer, inner);

  // Draw cap on top of hand — all circles at same centre as hand pivot
  GColor battery_ring, inner_ring, dot;
  if (s_settings.battery_indicator_enabled && s_battery_pct <= FIXED_BATT_PCT_LOW) {
    GColor alert = MONO_COLOR(s_settings.center_dot_20_color);
    battery_ring = alert;
    inner_ring   = alert;
    dot          = alert;
  } else if (s_settings.battery_indicator_enabled && s_battery_pct <= FIXED_BATT_PCT_MID) {
    battery_ring = MONO_COLOR(s_settings.center_dot_50_color);
    inner_ring   = MONO_COLOR(s_settings.min_hand_inner);
    dot          = MONO_COLOR(s_settings.hour_hand_outer);
  } else {
    battery_ring = GColorWhite;
    inner_ring   = MONO_COLOR(s_settings.min_hand_inner);
    dot          = MONO_COLOR(s_settings.hour_hand_outer);
  }
  int r5 = POS_X(7);
  int r4 = POS_X(5);
  int r3 = POS_X(4);
  int r2 = POS_X(2);
  int r1 = POS_X(1);
  graphics_context_set_fill_color(ctx, battery_ring);
  graphics_fill_circle(ctx, center, r5);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, r4);
  graphics_context_set_fill_color(ctx, inner_ring);
  graphics_fill_circle(ctx, center, r3);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, r2);
  graphics_context_set_fill_color(ctx, dot);
  graphics_fill_circle(ctx, center, r1);
}

// ============================================================
// COMPLICATION LAYER — temperature + date
// ============================================================

static void draw_centred_text(GContext *ctx, const char *str, GFont font,
                              int cx, int y, int max_w, GColor color) {
  GSize ts = graphics_text_layout_get_content_size(
    str, font, GRect(0, 0, max_w, 50),
    GTextOverflowModeWordWrap, GTextAlignmentCenter);
  int rw = ts.w + 6;
  int rh = ts.h + 6;
  int rx = cx - rw / 2;
  int ry = y  - rh / 2;
  if (rx < 0) rx = 0;
  if (ry < 0) ry = 0;
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, str, font, GRect(rx, ry, rw, rh),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void complication_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  bool show_temp = false;
  switch (s_settings.temp_visible) {
    case COMPLICATION_ALWAYS: show_temp = true;            break;
    case COMPLICATION_OFF:    show_temp = false;           break;
    case COMPLICATION_SHAKE:  show_temp = s_showing_icons; break;
  }
  bool show_date = false;
  switch (s_settings.date_visible) {
    case COMPLICATION_ALWAYS: show_date = true;            break;
    case COMPLICATION_OFF:    show_date = false;           break;
    case COMPLICATION_SHAKE:  show_date = s_showing_icons; break;
  }
  if (!show_temp && !show_date) return;

  int cx = (bounds.size.w - 1) / 2;
  int comp_y = (s_tick_tm.tm_min >= 20 && s_tick_tm.tm_min <= 40) ? POS_Y(45) : POS_Y(105);
  GFont font = get_complication_font();

  if (show_temp) {
    char temp_str[12];
    if (s_settings.temp_unit == TEMP_UNIT_FAHRENHEIT) {
      snprintf(temp_str, sizeof(temp_str), "%d\xc2\xb0" "F", (int)s_temp_f);
    } else {
      snprintf(temp_str, sizeof(temp_str), "%d\xc2\xb0" "C", (int)s_temp_c);
    }
    draw_centred_text(ctx, temp_str, font, cx, comp_y, bounds.size.w, MONO_COLOR(s_settings.temp_color));
  }

  if (show_date) {
    static const char *day_names[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char date_str[12];
    snprintf(date_str, sizeof(date_str), "%s %d",
             day_names[s_tick_tm.tm_wday], s_tick_tm.tm_mday);
    draw_centred_text(ctx, date_str, font, cx,
                      show_temp ? comp_y + POS_Y(18) : comp_y,
                      bounds.size.w, MONO_COLOR(s_settings.date_color));
  }
}

// ============================================================
// EVENT HANDLERS
// ============================================================

static void shake_timer_callback(void *data);

static void numbers_timer_callback(void *data) {
  s_numbers_timer = NULL;
  s_showing_icons = true;
  s_bg_last_hour = -1;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_complication_layer);
  if (s_shake_timer) app_timer_cancel(s_shake_timer);
  s_shake_timer = app_timer_register(SHAKE_DISPLAY_MS, shake_timer_callback, NULL);
}

static void shake_timer_callback(void *data) {
  s_shake_timer = NULL;
  s_showing_icons = false;
  s_bg_last_hour = -1;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_complication_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

static bool needs_second_ticks(void) {
  return (s_settings.seconds_hand_mode == SECONDS_MODE_ALWAYS) ||
         (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE && s_showing_seconds);
}

static bool s_subscribed_seconds = false;
static void update_tick_subscription(void) {
  bool want_seconds = needs_second_ticks();
  if (want_seconds && !s_subscribed_seconds) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    s_subscribed_seconds = true;
  } else if (!want_seconds && s_subscribed_seconds) {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    s_subscribed_seconds = false;
  }
}

// Forward declaration for accel_tap_handler (used by update_accel_subscription)
static void accel_tap_handler(AccelAxisType axis, int32_t direction);

// Manage accel subscription based on whether shake does anything
static bool s_accel_subscribed = false;
static void update_accel_subscription(void) {
  bool need_accel = (s_settings.shake_mode == SHAKE_MODE_ON_SHAKE) ||
                    (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE);
  if (need_accel && !s_accel_subscribed) {
    accel_tap_service_subscribe(accel_tap_handler);
    s_accel_subscribed = true;
  } else if (!need_accel && s_accel_subscribed) {
    accel_tap_service_unsubscribe();
    s_accel_subscribed = false;
  }
}

static void seconds_timer_callback(void *data) {
  s_seconds_timer = NULL;
  s_showing_seconds = false;
  layer_mark_dirty(s_seconds_layer);
  update_tick_subscription();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_tick_tm = *tick_time;

  // Seconds hand: only dirty the lightweight seconds layer
  if (needs_second_ticks()) {
    layer_mark_dirty(s_seconds_layer);
  }

  // Minute hand and complication: only on minute boundary
  if (units_changed & MINUTE_UNIT) {
    layer_mark_dirty(s_minute_layer);

    // Hour hand: only dirty if angle actually changed (moves every 2 min)
    int32_t hour_angle = DEG_TO_TRIGANGLE(
      (s_tick_tm.tm_hour % 12) * 30 + s_tick_tm.tm_min / 2);
    if (hour_angle != s_last_hour_angle) {
      s_last_hour_angle = hour_angle;
      layer_mark_dirty(s_hour_layer);
    }

    // Complication: only dirty when position changes (minute crosses 20 or 40 boundary)
    // or at midnight (date changes). Temp changes are handled by inbox_received.
    if (s_settings.date_visible != COMPLICATION_OFF ||
        s_settings.temp_visible != COMPLICATION_OFF) {
      int prev_min = (s_tick_tm.tm_min == 0) ? 59 : s_tick_tm.tm_min - 1;
      bool pos_changed = (prev_min < 20 && s_tick_tm.tm_min >= 20) ||
                         (prev_min < 40 && s_tick_tm.tm_min >= 40) ||
                         (prev_min >= 40 && s_tick_tm.tm_min < 20) ||
                         (prev_min >= 20 && prev_min < 40 && s_tick_tm.tm_min >= 40);
      bool date_changed = (s_tick_tm.tm_hour == 0 && s_tick_tm.tm_min == 0);
      if (pos_changed || date_changed) {
        layer_mark_dirty(s_complication_layer);
      }
    }
  }

  // Background: only on hour boundary
  if ((int8_t)tick_time->tm_hour != s_bg_last_hour) {
    layer_mark_dirty(s_bg_layer);
  }
}

static void test_timer_callback(void *context) {
  s_test_timer = NULL;
  s_test_active = false;
  s_battery_pct = s_test_saved_battery_pct;
  s_bt_connected = s_test_saved_bt_connected;
  layer_mark_dirty(s_minute_layer);
}

static void start_test(uint8_t fake_battery_pct, bool fake_bt_connected) {
  if (s_test_timer) app_timer_cancel(s_test_timer);
  if (!s_test_active) {
    s_test_saved_battery_pct  = s_battery_pct;
    s_test_saved_bt_connected = s_bt_connected;
    s_test_active = true;
  }
  s_battery_pct  = fake_battery_pct;
  s_bt_connected = fake_bt_connected;
  layer_mark_dirty(s_minute_layer);
  s_test_timer = app_timer_register(5000, test_timer_callback, NULL);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_settings.shake_mode == SHAKE_MODE_ON_SHAKE) {
    // Cancel any in-flight timers
    if (s_numbers_timer) app_timer_cancel(s_numbers_timer);
    if (s_shake_timer) app_timer_cancel(s_shake_timer);

    if (s_showing_icons) {
      s_shake_timer = app_timer_register(SHAKE_DISPLAY_MS, shake_timer_callback, NULL);
      // Fall through to seconds hand check below
    } else {
      s_showing_icons = false;
      s_bg_last_hour = -1;
      layer_mark_dirty(s_bg_layer);
      layer_mark_dirty(s_complication_layer);
      s_numbers_timer = app_timer_register(500, numbers_timer_callback, NULL);
    }
  }

  // Show seconds hand on shake if in shake mode
  if (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE) {
    // Cancel any existing timer first
    if (s_seconds_timer) app_timer_cancel(s_seconds_timer);
    // Show seconds immediately
    time_t now = time(NULL);
    s_tick_tm = *localtime(&now);
    s_showing_seconds = true;
    layer_mark_dirty(s_seconds_layer);
    // Set timer to hide after duration
    s_seconds_timer = app_timer_register((uint32_t)s_settings.seconds_shake_dur * 1000, seconds_timer_callback, NULL);
    update_tick_subscription();
  }
}

static void battery_handler(BatteryChargeState charge) {
  if (!s_settings.battery_indicator_enabled) return;
  uint8_t old_pct = s_battery_pct;
  s_battery_pct = charge.charge_percent;
  if (!s_battery_handler_initialized) {
    s_battery_handler_initialized = true;
    return;
  }
  bool crossed = (old_pct > FIXED_BATT_PCT_MID) != (s_battery_pct > FIXED_BATT_PCT_MID) ||
                 (old_pct > FIXED_BATT_PCT_LOW) != (s_battery_pct > FIXED_BATT_PCT_LOW);
  if (crossed) layer_mark_dirty(s_minute_layer);
}

static void bt_handler(bool connected) {
  bool was_connected = s_bt_connected;
  s_bt_connected = connected;
  if (!connected && was_connected && s_settings.vibrate_bt_disconnect) {
    vibes_double_pulse();
  } else if (connected && !was_connected && s_settings.vibrate_bt_reconnect) {
    vibes_short_pulse();
  }
  layer_mark_dirty(s_hour_layer);
  layer_mark_dirty(s_minute_layer);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Track which layers need redrawing
  bool dirty_bg = false;
  bool dirty_hands = false;
  bool dirty_complication = false;

  for (int i = 0; i < 24; i++) {
    Tuple *t = dict_find(iter, KEY_ICON_0 + i);
    if (t) { s_icons[i] = (int8_t)t->value->int32; dirty_bg = true; }
  }

  Tuple *tc = dict_find(iter, KEY_TEMP_C);
  if (tc) { s_temp_c = (int16_t)tc->value->int32; dirty_complication = true; }
  Tuple *tf = dict_find(iter, KEY_TEMP_F);
  if (tf) { s_temp_f = (int16_t)tf->value->int32; dirty_complication = true; }

  Tuple *dhm = dict_find(iter, KEY_DISPLAY_HOUR_MARKERS);
  if (dhm) { s_settings.display_hour_markers = dhm->value->int32 != 0; dirty_bg = true; }
  Tuple *dmm = dict_find(iter, KEY_DISPLAY_MINOR_MARKERS);
  if (dmm) { s_settings.display_minor_markers = dmm->value->int32 != 0; dirty_bg = true; }
  Tuple *sm = dict_find(iter, KEY_SHAKE_MODE);
  if (sm) { s_settings.shake_mode = (int8_t)sm->value->int32; dirty_bg = true; }
  Tuple *dv = dict_find(iter, KEY_DATE_VISIBLE);
  if (dv) { s_settings.date_visible = (int8_t)dv->value->int32; dirty_complication = true; }
  Tuple *tv = dict_find(iter, KEY_TEMP_VISIBLE);
  if (tv) { s_settings.temp_visible = (int8_t)tv->value->int32; dirty_complication = true; }
  Tuple *tu = dict_find(iter, KEY_TEMP_UNIT);
  if (tu) { s_settings.temp_unit = (int8_t)tu->value->int32; dirty_complication = true; }
  Tuple *btmr = dict_find(iter, MESSAGE_KEY_KEY_BT_DISCONNECT_MIN_INNER_RED);
  if (btmr) { s_settings.bt_disconnect_min_inner_red = btmr->value->int32 != 0; dirty_hands = true; }
  Tuple *btoc = dict_find(iter, KEY_BT_DISCONNECT_OUTER_COLOR);
  if (btoc) { s_settings.bt_disconnect_outer_color = rgb_to_gcolor(btoc->value->int32); dirty_hands = true; }
  Tuple *btic = dict_find(iter, KEY_BT_DISCONNECT_INNER_COLOR);
  if (btic) { s_settings.bt_disconnect_inner_color = rgb_to_gcolor(btic->value->int32); dirty_hands = true; }
  Tuple *vbt = dict_find(iter, KEY_VIBRATE_BT_DISCONNECT);
  if (vbt) s_settings.vibrate_bt_disconnect = vbt->value->int32 != 0;
  Tuple *vbtr = dict_find(iter, KEY_VIBRATE_BT_RECONNECT);
  if (vbtr) s_settings.vibrate_bt_reconnect = vbtr->value->int32 != 0;
  Tuple *hho = dict_find(iter, KEY_HOUR_HAND_OUTER);
  if (hho) { s_settings.hour_hand_outer = rgb_to_gcolor(hho->value->int32); dirty_hands = true; }
  Tuple *hhi = dict_find(iter, KEY_HOUR_HAND_INNER);
  if (hhi) { s_settings.hour_hand_inner = rgb_to_gcolor(hhi->value->int32); dirty_hands = true; }
  Tuple *mho = dict_find(iter, KEY_MIN_HAND_OUTER);
  if (mho) { s_settings.min_hand_outer = rgb_to_gcolor(mho->value->int32); dirty_hands = true; }
  Tuple *mhi = dict_find(iter, KEY_MIN_HAND_INNER);
  if (mhi) { s_settings.min_hand_inner = rgb_to_gcolor(mhi->value->int32); dirty_hands = true; }
  Tuple *nf = dict_find(iter, KEY_NUMBER_FONT);
  if (nf) {
    s_settings.number_font = (int8_t)nf->value->int32;
    s_cached_number_font = NULL;
    s_num_sizes_cached = false;
    dirty_bg = true;
  }
  Tuple *ncm = dict_find(iter, KEY_NUMBER_COLOR_MODE);
  if (ncm) { s_settings.number_color_mode = (int8_t)ncm->value->int32; dirty_bg = true; }
  Tuple *icm = dict_find(iter, KEY_ICON_COLOR_MODE);
  if (icm) { s_settings.icon_color_mode = (int8_t)icm->value->int32; dirty_bg = true; }
  Tuple *bgc = dict_find(iter, KEY_BACKGROUND_COLOR);
  if (bgc) { s_settings.background_color = rgb_to_gcolor(bgc->value->int32); dirty_bg = true; }
  Tuple *nc = dict_find(iter, KEY_NUMBER_COLOR);
  if (nc) { s_settings.number_color = rgb_to_gcolor(nc->value->int32); dirty_bg = true; }
  Tuple *ic = dict_find(iter, KEY_ICON_COLOR);
  if (ic) { s_settings.icon_color = rgb_to_gcolor(ic->value->int32); dirty_bg = true; }
  Tuple *hmc = dict_find(iter, KEY_HOUR_MARKER_COLOR);
  if (hmc) { s_settings.hour_marker_color = rgb_to_gcolor(hmc->value->int32); dirty_bg = true; }
  Tuple *mmc = dict_find(iter, KEY_MINUTE_MARKER_COLOR);
  if (mmc) { s_settings.minute_marker_color = rgb_to_gcolor(mmc->value->int32); dirty_bg = true; }
  Tuple *cd50 = dict_find(iter, KEY_CENTER_DOT_50_COLOR);
  if (cd50) { s_settings.center_dot_50_color = rgb_to_gcolor(cd50->value->int32); dirty_hands = true; }
  Tuple *cd20 = dict_find(iter, KEY_CENTER_DOT_20_COLOR);
  if (cd20) { s_settings.center_dot_20_color = rgb_to_gcolor(cd20->value->int32); dirty_hands = true; }
  Tuple *mr20 = dict_find(iter, KEY_MIDDLE_RING_20_COLOR);
  if (mr20) { s_settings.middle_ring_20_color = rgb_to_gcolor(mr20->value->int32); dirty_hands = true; }
  Tuple *dc = dict_find(iter, KEY_DATE_COLOR);
  if (dc) { s_settings.date_color = rgb_to_gcolor(dc->value->int32); dirty_complication = true; }
  Tuple *tpc = dict_find(iter, KEY_TEMP_COLOR);
  if (tpc) { s_settings.temp_color = rgb_to_gcolor(tpc->value->int32); dirty_complication = true; }
  Tuple *bie = dict_find(iter, KEY_BATTERY_INDICATOR_ENABLED);
  if (bie) { s_settings.battery_indicator_enabled = (bool)bie->value->int32; dirty_hands = true; }

  // Sunrise / sunset
  bool sr_ss_changed = false;
  Tuple *srh = dict_find(iter, KEY_SUNRISE_HOUR);
  if (srh) { s_sunrise_hour = (int8_t)srh->value->int32; sr_ss_changed = true; }
  Tuple *srm = dict_find(iter, KEY_SUNRISE_MINUTE);
  if (srm) { s_sunrise_min = (int8_t)srm->value->int32; sr_ss_changed = true; }
  Tuple *ssh = dict_find(iter, KEY_SUNSET_HOUR);
  if (ssh) { s_sunset_hour = (int8_t)ssh->value->int32; sr_ss_changed = true; }
  Tuple *ssm = dict_find(iter, KEY_SUNSET_MINUTE);
  if (ssm) { s_sunset_min = (int8_t)ssm->value->int32; sr_ss_changed = true; }
  if (sr_ss_changed) {
    cache_sunrise_sunset_markers();
    dirty_bg = true;
  }

  Tuple *shc = dict_find(iter, KEY_SECONDS_HAND_COLOR);
  if (shc) { s_settings.seconds_hand_color = rgb_to_gcolor(shc->value->int32); layer_mark_dirty(s_seconds_layer); }
  Tuple *shm = dict_find(iter, KEY_SECONDS_HAND_MODE);
  if (shm) { s_settings.seconds_hand_mode = (int8_t)shm->value->int32; layer_mark_dirty(s_seconds_layer); }
  Tuple *ssd = dict_find(iter, KEY_SECONDS_SHAKE_DUR);
  if (ssd) s_settings.seconds_shake_dur = (int8_t)ssd->value->int32;
  Tuple *smv = dict_find(iter, KEY_SUNRISE_MARKER_VISIBLE);
  if (smv) { s_settings.sunrise_marker_visible = (int8_t)smv->value->int32; dirty_bg = true; }
  Tuple *smc = dict_find(iter, KEY_SUNRISE_MARKER_COLOR);
  if (smc) { s_settings.sunrise_marker_color = rgb_to_gcolor(smc->value->int32); dirty_bg = true; }
  Tuple *ssmc = dict_find(iter, KEY_SUNSET_MARKER_COLOR);
  if (ssmc) { s_settings.sunset_marker_color = rgb_to_gcolor(ssmc->value->int32); dirty_bg = true; }

  // Test mode: temporarily override battery/BT state
  Tuple *tba = dict_find(iter, KEY_TEST_BATTERY_ALERT);
  if (tba && tba->value->int32) start_test(10, s_bt_connected);  // <20% battery
  Tuple *tb50 = dict_find(iter, KEY_TEST_BATTERY_50);
  if (tb50 && tb50->value->int32) start_test(35, s_bt_connected); // 50%–20% battery
  Tuple *tbt = dict_find(iter, KEY_TEST_BT_DISCONNECT);
  if (tbt && tbt->value->int32) start_test(s_battery_pct, false); // BT disconnect

  // Only persist data that actually changed (flash writes are expensive)
  persist_write_data(PERSIST_SETTINGS, &s_settings, sizeof(Settings));
  if (dirty_bg) {
    persist_write_data(PERSIST_ICONS, s_icons, sizeof(s_icons));
  }
  if (dirty_complication) {
    persist_write_int(PERSIST_TEMP_C, s_temp_c);
    persist_write_int(PERSIST_TEMP_F, s_temp_f);
  }

  // Re-evaluate subscriptions after settings change
  update_tick_subscription();
  update_accel_subscription();

  // Only dirty layers that actually changed
  if (dirty_bg) { s_bg_last_hour = -1; layer_mark_dirty(s_bg_layer); }
  if (dirty_hands) { layer_mark_dirty(s_hour_layer); layer_mark_dirty(s_minute_layer); }
  if (dirty_complication) layer_mark_dirty(s_complication_layer);
}

// ============================================================
// WINDOW SETUP
// ============================================================

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  // Initialize screen dimensions early so all positioning calculations use correct values
  s_screen_w = bounds.size.w;
  s_screen_h = bounds.size.h;

  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_layer_update);
  layer_add_child(root, s_bg_layer);

  s_seconds_layer = layer_create(bounds);
  layer_set_update_proc(s_seconds_layer, seconds_layer_update);
  layer_add_child(root, s_seconds_layer);

  s_hour_layer = layer_create(bounds);
  layer_set_update_proc(s_hour_layer, hour_layer_update);
  layer_add_child(root, s_hour_layer);

  s_complication_layer = layer_create(bounds);
  layer_set_update_proc(s_complication_layer, complication_layer_update);
  layer_add_child(root, s_complication_layer);

  s_minute_layer = layer_create(bounds);
  layer_set_update_proc(s_minute_layer, minute_layer_update);
  layer_add_child(root, s_minute_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_bg_layer);
  layer_destroy(s_seconds_layer);
  layer_destroy(s_hour_layer);
  layer_destroy(s_complication_layer);
  layer_destroy(s_minute_layer);
}

// ============================================================
// MAIN
// ============================================================

static void init(void) {
  settings_set_defaults(&s_settings);
  if (persist_exists(PERSIST_SETTINGS)) {
    persist_read_data(PERSIST_SETTINGS, &s_settings, sizeof(Settings));
  }

  for (int i = 0; i < 24; i++) s_icons[i] = ICON_UNKNOWN;
  if (persist_exists(PERSIST_ICONS)) {
    persist_read_data(PERSIST_ICONS, s_icons, sizeof(s_icons));
  }
  if (persist_exists(PERSIST_TEMP_C)) s_temp_c = (int16_t)persist_read_int(PERSIST_TEMP_C);
  if (persist_exists(PERSIST_TEMP_F)) s_temp_f = (int16_t)persist_read_int(PERSIST_TEMP_F);

  s_battery_pct = battery_state_service_peek().charge_percent;
  s_battery_handler_initialized = true;

  time_t now = time(NULL);
  s_tick_tm = *localtime(&now);

  // Don't cache with default times - only show markers when actual data is received

  s_window = window_create();
  window_set_background_color(s_window, MONO_COLOR(s_settings.background_color));
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // Subscribe based on current seconds hand mode
  if (needs_second_ticks()) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    s_subscribed_seconds = true;
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    s_subscribed_seconds = false;
  }

  // Conditionally subscribe to accelerometer
  bool need_accel = (s_settings.shake_mode == SHAKE_MODE_ON_SHAKE) ||
                    (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE);
  if (need_accel) {
    accel_tap_service_subscribe(accel_tap_handler);
    s_accel_subscribed = true;
  }

  battery_state_service_subscribe(battery_handler);

  s_bt_connected = connection_service_peek_pebble_app_connection();
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler
  });

  app_message_open(APP_MSG_INBOX_SIZE, APP_MSG_OUTBOX_SIZE);
  app_message_register_inbox_received(inbox_received_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  if (s_accel_subscribed) accel_tap_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  if (s_shake_timer) app_timer_cancel(s_shake_timer);
  if (s_seconds_timer) app_timer_cancel(s_seconds_timer);
  if (s_numbers_timer) app_timer_cancel(s_numbers_timer);
  if (s_test_timer) app_timer_cancel(s_test_timer);
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

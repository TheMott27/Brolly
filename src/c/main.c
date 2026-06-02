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
 * - Dynamic tick subscription: MINUTE_UNIT when seconds hand hidden, SECOND_UNIT only when needed
 * - Conditional layer redraws: hour/minute/complication only on minute boundary
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

#define SCREEN_W 144
#define SCREEN_H 168

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
#define KEY_ICON_0                  0  // icons 0–23 occupy keys 0–23
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
#define KEY_BATTERY_INDICATOR_ENABLED 138
#define KEY_SECONDS_HAND_MODE     142
#define KEY_SECONDS_SHAKE_DUR     143

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
#define KEY_CENTER_DOT_COLOR      139
#define KEY_MIDDLE_RING_COLOR     140
#define KEY_SECONDS_HAND_COLOR    141

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
#define PERSIST_SETTINGS        29

#define SHAKE_DISPLAY_MS        30000
#define APP_MSG_INBOX_SIZE      512
#define APP_MSG_OUTBOX_SIZE     64

#define FIXED_BATT_PCT_MID       50
#define FIXED_BATT_PCT_LOW       20

// Hand geometry
#define FIXED_HAND_BASE_WIDTH    3
#define FIXED_HAND_OUTER_WIDTH   6
#define FIXED_HAND_INNER_WIDTH   2
#define FIXED_HAND_BASE_PX      20
#define FIXED_ICON_SIZE          24

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
  int8_t number_font;  // 0=LECO28, 1=Bitham30Black, 2=Gothic24Bold, 3=RobotoCondensed21, 4=DroidSerif28Bold, 5=Bitham42Light
  // Colour settings
  GColor background_color;
  GColor number_color;
  GColor icon_color;
  GColor hour_marker_color;
  GColor minute_marker_color;
  GColor center_dot_50_color;
  GColor center_dot_20_color;
  GColor middle_ring_20_color;
  GColor center_dot_color;
  GColor middle_ring_color;
  GColor date_color;
  GColor temp_color;
  bool battery_indicator_enabled;
  GColor seconds_hand_color;
  int8_t seconds_hand_mode; // 0=never, 1=always, 2=shake only
  int8_t seconds_shake_dur; // seconds to show on shake: 5, 10, 20, 30
} Settings;

// ============================================================
// GLOBAL STATE
// ============================================================

static Window *s_window;
static Layer *s_bg_layer;
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
static bool s_showing_seconds = false;

// Shared time snapshot — set once per tick, read by all layer callbacks
static struct tm s_tick_tm;

// Last hour at which bg_layer was drawn (avoids redundant redraws)
static int8_t s_bg_last_hour = -1;

// ============================================================
// HELPERS
// ============================================================

static GColor rgb_to_gcolor(int32_t rgb) {
  if (rgb == -1) return GColorClear;
  return GColorFromRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

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
  s->number_font                 = 3;  // Roboto Condensed 21
  s->background_color            = GColorBlack;
  s->number_color                = GColorWhite;
  s->icon_color                  = GColorWhite;
  s->hour_marker_color           = GColorWhite;
  s->minute_marker_color         = GColorWhite;
  s->center_dot_50_color         = GColorRed;
  s->center_dot_20_color         = GColorRed;
  s->middle_ring_20_color        = GColorRed;
  s->center_dot_color            = GColorWhite;
  s->middle_ring_color           = GColorWhite;
  s->date_color                  = GColorFromRGB(0x85, 0x85, 0x85); // #858585
  s->temp_color                  = GColorFromRGB(0x85, 0x85, 0x85); // #858585
  s->battery_indicator_enabled   = true;
  s->seconds_hand_color          = GColorWhite;
  s->seconds_hand_mode           = SECONDS_MODE_SHAKE;
  s->seconds_shake_dur           = 10;
}

static GPoint polar_to_point(GPoint center, int32_t angle, int radius) {
  return GPoint(
    center.x + (int)(radius * sin_lookup(angle) / TRIG_MAX_RATIO),
    center.y - (int)(radius * cos_lookup(angle) / TRIG_MAX_RATIO)
  );
}

// Maps a clock angle to a point on the rectangular screen perimeter.
static GPoint square_perimeter_point(GPoint center, int32_t angle,
                                     int margin_x, int margin_y) {
  int32_t sin_a = sin_lookup(angle);
  int32_t cos_a = cos_lookup(angle);
  int hw = center.x - margin_x;
  int hh = center.y - margin_y;
  if (hw < 1) hw = 1;
  if (hh < 1) hh = 1;
  int32_t abs_sin = sin_a < 0 ? -sin_a : sin_a;
  int32_t abs_cos = cos_a < 0 ? -cos_a : cos_a;
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
  return GPoint(
    center.x + (int)((int64_t)sin_a * t / TRIG_MAX_RATIO),
    center.y - (int)((int64_t)cos_a * t / TRIG_MAX_RATIO)
  );
}

// ============================================================
// WEATHER ICON DRAWING
// ============================================================

#include "gpath_weather.h"

// Maps icon condition code directly to gpath ID — no intermediate slot integer.
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

static void draw_weather_icon(GContext *ctx, int8_t icon, GPoint center, int sz) {
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

  // Draw icon paths without heap allocation: translate points on the stack.
  int half = sz / 2;
  int ox = center.x - half;
  int oy = center.y - half;

  graphics_context_set_stroke_color(ctx, s_settings.icon_color);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < path_count; i++) {
    int npts = paths[i].num_points;
    if (npts < 2) continue;
    for (int j = 0; j < npts; j++) {
      GPoint a = GPoint(paths[i].points[j].x + ox, paths[i].points[j].y + oy);
      GPoint b = GPoint(paths[i].points[(j + 1) % npts].x + ox,
                        paths[i].points[(j + 1) % npts].y + oy);
      graphics_draw_line(ctx, a, b);
    }
  }
}

// ============================================================
// HOUR NUMBER DRAWING
// ============================================================

// Cached font pointer — updated when settings change.
static GFont s_cached_number_font = NULL;

static GFont resolve_number_font(int8_t id) {
  switch (id) {
    case 1:  return fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
    case 2:  return fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    case 3:  return fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
    case 4:  return fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD);
    case 5:  return fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
    default: return fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS);
  }
}

static GFont get_number_font(void) {
  if (!s_cached_number_font) {
    s_cached_number_font = resolve_number_font(s_settings.number_font);
  }
  return s_cached_number_font;
}

static void draw_hour_number(GContext *ctx, int hour, GPoint center) {
  if (hour == 0) hour = 12;
  char buf[3];
  snprintf(buf, sizeof(buf), "%d", hour);
  GFont font = get_number_font();
  GSize ts = graphics_text_layout_get_content_size(buf, font,
    GRect(0, 0, 40, 40), GTextOverflowModeWordWrap, GTextAlignmentCenter);
  int tw = ts.w + 4;
  int th = ts.h + 4;
  int tx = center.x - tw / 2;
  int ty = center.y - th / 2;
  if (tx < 2) tx = 2;
  if (ty < 2) ty = 2;
  if (tx + tw > SCREEN_W - 2) tx = SCREEN_W - 2 - tw;
  if (ty + th > SCREEN_H - 2) ty = SCREEN_H - 2 - th;
  graphics_context_set_text_color(ctx, s_settings.number_color);
  graphics_draw_text(ctx, buf, font, GRect(tx, ty, tw, th),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

// ============================================================
// BACKGROUND LAYER — markers, numbers, weather icons
// Redrawn only on hour boundary, show_icons toggle, or new data.
// ============================================================

static void bg_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

  graphics_context_set_fill_color(ctx, s_settings.background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // ---- Seconds hand (drawn after fill, before markers/numbers so it sits behind them) ----
  if (s_settings.seconds_hand_mode != SECONDS_MODE_NEVER &&
      !(s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE && !s_showing_seconds)) {
    int32_t sec_angle = DEG_TO_TRIGANGLE(s_tick_tm.tm_sec * 6);
    GPoint sec_tip  = square_perimeter_point(center, sec_angle, 0, 0);
    GPoint sec_tail = polar_to_point(center, sec_angle + DEG_TO_TRIGANGLE(180), 8);
    graphics_context_set_stroke_color(ctx, s_settings.seconds_hand_color);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, sec_tail, sec_tip);
  }

  bool show_icons = false;
  switch (s_settings.shake_mode) {
    case SHAKE_MODE_ON_SHAKE:     show_icons = s_showing_icons; break;
    case SHAKE_MODE_NUMBERS_ONLY: show_icons = false;           break;
    case SHAKE_MODE_ICONS_ONLY:   show_icons = true;            break;
  }

  // ---- Minute markers (60 × 1px dot) ----
  if (s_settings.display_minor_markers) {
    graphics_context_set_stroke_color(ctx, s_settings.minute_marker_color);
    graphics_context_set_stroke_width(ctx, 1);
    for (int i = 0; i < 60; i++) {
      int32_t angle = DEG_TO_TRIGANGLE(i * 6);
      GPoint outer_pt = square_perimeter_point(center, angle, 1, 0);
      int dx = center.x - outer_pt.x;
      int dy = center.y - outer_pt.y;
      int adx = dx < 0 ? -dx : dx;
      int ady = dy < 0 ? -dy : dy;
      int dist = (adx > ady ? adx : ady) + ((adx < ady ? adx : ady) * 3 / 8);
      if (dist == 0) continue;
      GPoint inner_pt = GPoint(outer_pt.x + dx / dist, outer_pt.y + dy / dist);
      graphics_draw_line(ctx, inner_pt, outer_pt);
    }
  }

  // ---- Hour tick marks (12 × 3px wide, 1px deep) ----
  graphics_context_set_stroke_color(ctx, s_settings.hour_marker_color);
  graphics_context_set_stroke_width(ctx, 3);
  for (int h = 0; h < 12; h++) {
    int32_t angle = DEG_TO_TRIGANGLE(h * 30);
    GPoint outer_pt = square_perimeter_point(center, angle, 0, 0);
    int dx = center.x - outer_pt.x;
    int dy = center.y - outer_pt.y;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int dist = (adx > ady ? adx : ady) + ((adx < ady ? adx : ady) * 3 / 8);
    if (dist == 0) continue;
    GPoint inner_pt = GPoint(outer_pt.x + dx / dist, outer_pt.y + dy / dist);
    graphics_draw_line(ctx, inner_pt, outer_pt);
  }

  // ---- Hour numbers / icons ----
  const int icon_half = FIXED_ICON_SIZE / 2;
  const int num_half  = 14;
  int cur_hour = s_tick_tm.tm_hour;
  int cur_min  = s_tick_tm.tm_min;

  for (int h = 0; h < 12; h++) {
    int32_t angle = DEG_TO_TRIGANGLE(h * 30);
    bool is_top_bottom = (h == 0 || h == 1 || h == 5 || h == 6 || h == 7 || h == 11);
    GPoint pos = square_perimeter_point(center, angle, 0, 0);

    if (!is_top_bottom) {
      pos.x = (h == 2 || h == 3 || h == 4) ? 119 : 24;
      if      (h == 2 || h == 4)  pos.x += 4;
      else if (h == 3)            pos.x += 8;
      else if (h == 10 || h == 8) pos.x -= 4;
      else if (h == 9)            pos.x -= 8;
      if      (h == 2 || h == 10) pos.y += 5;
      else if (h == 4 || h == 8)  pos.y -= 5;
    }

    if (show_icons) {
      if (is_top_bottom) {
        if (h == 0 || h == 1 || h == 11) pos.y += icon_half + 5;
        else                              pos.y -= icon_half + 5;
      }
      int clock_num = (h == 0) ? 12 : h;
      int am_hour   = (clock_num == 12) ? 0  : clock_num;
      int pm_hour   = (clock_num == 12) ? 12 : clock_num + 12;
      bool am_passed = (am_hour < cur_hour) || (am_hour == cur_hour && cur_min > 0);
      bool pm_passed = (pm_hour < cur_hour) || (pm_hour == cur_hour && cur_min > 0);
      int icon_hour  = (!am_passed) ? am_hour : (!pm_passed) ? pm_hour : am_hour;
      int8_t icon = s_icons[icon_hour];
      if (icon < 0) icon = ICON_UNKNOWN;
      draw_weather_icon(ctx, icon, pos, FIXED_ICON_SIZE);

    } else if (s_settings.display_hour_markers) {
      if (is_top_bottom) {
        if (h == 0 || h == 1 || h == 11) pos.y += num_half;
        else                              pos.y -= num_half;
      }
      draw_hour_number(ctx, h, pos);
    }
  }

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
  GPoint base_pt = {
    .x = center.x + dx * FIXED_HAND_BASE_PX / dist,
    .y = center.y + dy * FIXED_HAND_BASE_PX / dist,
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
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  int radius = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2;
  int32_t angle = DEG_TO_TRIGANGLE(
    (s_tick_tm.tm_hour % 12) * 30 + s_tick_tm.tm_min / 2);
  draw_inittick_hand(ctx, center, polar_to_point(center, angle, radius * 60 / 100),
                     s_settings.hour_hand_outer, s_settings.hour_hand_inner);
}

static void minute_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  int radius = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2;
  int32_t angle = DEG_TO_TRIGANGLE(s_tick_tm.tm_min * 6);
  GColor outer = (!s_bt_connected && s_settings.bt_disconnect_min_inner_red)
                 ? s_settings.bt_disconnect_outer_color : s_settings.min_hand_outer;
  GColor inner = (!s_bt_connected && s_settings.bt_disconnect_min_inner_red)
                 ? s_settings.bt_disconnect_inner_color : s_settings.min_hand_inner;
  draw_inittick_hand(ctx, center, polar_to_point(center, angle, radius * 95 / 100),
                     outer, inner);

  // Centre cap: white outer → black border → battery ring → black outline → dot
  // Outer ring (always middle_ring_color from appearance) and inner ring (changes with battery)
  GColor outer_ring = s_settings.middle_ring_color;  // Never changes
  GColor inner_ring, dot;
  
  if (s_settings.battery_indicator_enabled) {
    // >50%: both ring and dot use normal appearance colours
    if (s_battery_pct > FIXED_BATT_PCT_MID) {
      inner_ring = s_settings.middle_ring_color;
      dot        = s_settings.center_dot_color;
    }
    // 50%–20%: ring stays normal, dot turns alert colour
    else if (s_battery_pct > FIXED_BATT_PCT_LOW) {
      inner_ring = s_settings.middle_ring_color;
      dot        = s_settings.center_dot_50_color;
    }
    // <20%: both ring and dot turn alert colour
    else {
      inner_ring = s_settings.middle_ring_20_color;
      dot        = s_settings.center_dot_20_color;
    }
  } else {
    // Battery indicator off: both use normal appearance colours
    inner_ring = s_settings.middle_ring_color;
    dot        = s_settings.center_dot_color;
  }
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 7);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 6);
  graphics_context_set_fill_color(ctx, outer_ring);
  graphics_fill_circle(ctx, center, 5);
  graphics_context_set_fill_color(ctx, inner_ring);
  graphics_fill_circle(ctx, center, 4);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 2);
  graphics_context_set_fill_color(ctx, dot);
  graphics_fill_circle(ctx, center, 1);
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
  if (rx < 2) rx = 2;
  if (ry < 2) ry = 2;
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

  int cx = bounds.size.w / 2;
  int comp_y = (s_tick_tm.tm_min >= 20 && s_tick_tm.tm_min <= 40) ? 45 : 105;
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);

  if (show_temp) {
    char temp_str[12];
    if (s_settings.temp_unit == TEMP_UNIT_FAHRENHEIT) {
      snprintf(temp_str, sizeof(temp_str), "%d\xc2\xb0" "F", (int)s_temp_f);
    } else {
      snprintf(temp_str, sizeof(temp_str), "%d\xc2\xb0" "C", (int)s_temp_c);
    }
    draw_centred_text(ctx, temp_str, font, cx, comp_y, bounds.size.w, s_settings.temp_color);
  }

  if (show_date) {
    static const char *day_names[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char date_str[12];
    snprintf(date_str, sizeof(date_str), "%s %d",
             day_names[s_tick_tm.tm_wday], s_tick_tm.tm_mday);
    draw_centred_text(ctx, date_str, font, cx,
                      show_temp ? comp_y + 18 : comp_y,
                      bounds.size.w, s_settings.date_color);
  }
}

// ============================================================
// EVENT HANDLERS
// ============================================================

static void shake_timer_callback(void *data) {
  s_shake_timer = NULL;
  s_showing_icons = false;
  s_bg_last_hour = -1;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_complication_layer);
}

// Forward declaration for tick_handler (used by update_tick_subscription)
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

// Determines whether we need per-second ticks right now.
static bool needs_second_ticks(void) {
  return (s_settings.seconds_hand_mode == SECONDS_MODE_ALWAYS) ||
         (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE && s_showing_seconds);
}

// Switches between SECOND_UNIT and MINUTE_UNIT based on current state.
static bool s_subscribed_seconds = true;
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

static void seconds_timer_callback(void *data) {
  s_seconds_timer = NULL;
  s_showing_seconds = false;
  layer_mark_dirty(s_bg_layer);
  update_tick_subscription();  // Drop back to MINUTE_UNIT
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_tick_tm = *tick_time;

  // Seconds layer: only redraw when seconds are actually visible
  if (needs_second_ticks()) {
    layer_mark_dirty(s_bg_layer);
  }

  // Minute hand and complication: only on minute boundary
  if (units_changed & MINUTE_UNIT) {
    layer_mark_dirty(s_minute_layer);
    layer_mark_dirty(s_hour_layer);
    // Only dirty complication layer if at least one complication is visible
    if (s_settings.date_visible != COMPLICATION_OFF ||
        s_settings.temp_visible != COMPLICATION_OFF) {
      layer_mark_dirty(s_complication_layer);
    }
  }

  // Background: only on hour boundary
  if ((int8_t)tick_time->tm_hour != s_bg_last_hour) {
    layer_mark_dirty(s_bg_layer);
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_settings.shake_mode != SHAKE_MODE_ON_SHAKE) return;
  s_showing_icons = true;
  s_bg_last_hour = -1;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_complication_layer);

  if (s_shake_timer) app_timer_cancel(s_shake_timer);
  s_shake_timer = app_timer_register(5000, shake_timer_callback, NULL);

  // Also show seconds hand on shake if in shake mode
  if (s_settings.seconds_hand_mode == SECONDS_MODE_SHAKE) {
    s_showing_seconds = true;
    layer_mark_dirty(s_bg_layer);
    if (s_seconds_timer) app_timer_cancel(s_seconds_timer);
    s_seconds_timer = app_timer_register((uint32_t)s_settings.seconds_shake_dur * 1000, seconds_timer_callback, NULL);
    update_tick_subscription();  // Switch to SECOND_UNIT while showing
  }
}

static void battery_handler(BatteryChargeState charge) {
  uint8_t old_pct = s_battery_pct;
  s_battery_pct = charge.charge_percent;
  // Only redraw if battery crossed a visual threshold (50% or 20%)
  if (!s_settings.battery_indicator_enabled) return;
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
  for (int i = 0; i < 24; i++) {
    Tuple *t = dict_find(iter, KEY_ICON_0 + i);
    if (t) s_icons[i] = (int8_t)t->value->int32;
  }

  Tuple *tc = dict_find(iter, KEY_TEMP_C);
  if (tc) s_temp_c = (int16_t)tc->value->int32;
  Tuple *tf = dict_find(iter, KEY_TEMP_F);
  if (tf) s_temp_f = (int16_t)tf->value->int32;

  Tuple *dhm = dict_find(iter, KEY_DISPLAY_HOUR_MARKERS);
  if (dhm) s_settings.display_hour_markers = dhm->value->int32 != 0;
  Tuple *dmm = dict_find(iter, KEY_DISPLAY_MINOR_MARKERS);
  if (dmm) s_settings.display_minor_markers = dmm->value->int32 != 0;
  Tuple *sm = dict_find(iter, KEY_SHAKE_MODE);
  if (sm) s_settings.shake_mode = (int8_t)sm->value->int32;
  Tuple *dv = dict_find(iter, KEY_DATE_VISIBLE);
  if (dv) s_settings.date_visible = (int8_t)dv->value->int32;
  Tuple *tv = dict_find(iter, KEY_TEMP_VISIBLE);
  if (tv) s_settings.temp_visible = (int8_t)tv->value->int32;
  Tuple *tu = dict_find(iter, KEY_TEMP_UNIT);
  if (tu) s_settings.temp_unit = (int8_t)tu->value->int32;
  Tuple *btmr = dict_find(iter, MESSAGE_KEY_KEY_BT_DISCONNECT_MIN_INNER_RED);
  if (btmr) s_settings.bt_disconnect_min_inner_red = btmr->value->int32 != 0;
  Tuple *btoc = dict_find(iter, KEY_BT_DISCONNECT_OUTER_COLOR);
  if (btoc) s_settings.bt_disconnect_outer_color = rgb_to_gcolor(btoc->value->int32);
  Tuple *btic = dict_find(iter, KEY_BT_DISCONNECT_INNER_COLOR);
  if (btic) s_settings.bt_disconnect_inner_color = rgb_to_gcolor(btic->value->int32);
  Tuple *vbt = dict_find(iter, KEY_VIBRATE_BT_DISCONNECT);
  if (vbt) s_settings.vibrate_bt_disconnect = vbt->value->int32 != 0;
  Tuple *vbtr = dict_find(iter, KEY_VIBRATE_BT_RECONNECT);
  if (vbtr) s_settings.vibrate_bt_reconnect = vbtr->value->int32 != 0;
  Tuple *hho = dict_find(iter, KEY_HOUR_HAND_OUTER);
  if (hho) s_settings.hour_hand_outer = rgb_to_gcolor(hho->value->int32);
  Tuple *hhi = dict_find(iter, KEY_HOUR_HAND_INNER);
  if (hhi) s_settings.hour_hand_inner = rgb_to_gcolor(hhi->value->int32);
  Tuple *mho = dict_find(iter, KEY_MIN_HAND_OUTER);
  if (mho) s_settings.min_hand_outer  = rgb_to_gcolor(mho->value->int32);
  Tuple *mhi = dict_find(iter, KEY_MIN_HAND_INNER);
  if (mhi) s_settings.min_hand_inner  = rgb_to_gcolor(mhi->value->int32);
  Tuple *nf = dict_find(iter, KEY_NUMBER_FONT);
  if (nf) {
    s_settings.number_font = (int8_t)nf->value->int32;
    s_cached_number_font = NULL;  // Invalidate font cache
  }
  Tuple *bgc = dict_find(iter, KEY_BACKGROUND_COLOR);
  if (bgc) s_settings.background_color = rgb_to_gcolor(bgc->value->int32);
  Tuple *nc = dict_find(iter, KEY_NUMBER_COLOR);
  if (nc) s_settings.number_color = rgb_to_gcolor(nc->value->int32);
  Tuple *ic = dict_find(iter, KEY_ICON_COLOR);
  if (ic) s_settings.icon_color = rgb_to_gcolor(ic->value->int32);
  Tuple *hmc = dict_find(iter, KEY_HOUR_MARKER_COLOR);
  if (hmc) s_settings.hour_marker_color = rgb_to_gcolor(hmc->value->int32);
  Tuple *mmc = dict_find(iter, KEY_MINUTE_MARKER_COLOR);
  if (mmc) s_settings.minute_marker_color = rgb_to_gcolor(mmc->value->int32);
  Tuple *cd50 = dict_find(iter, KEY_CENTER_DOT_50_COLOR);
  if (cd50) s_settings.center_dot_50_color = rgb_to_gcolor(cd50->value->int32);
  Tuple *cd20 = dict_find(iter, KEY_CENTER_DOT_20_COLOR);
  if (cd20) s_settings.center_dot_20_color = rgb_to_gcolor(cd20->value->int32);
  Tuple *mr20 = dict_find(iter, KEY_MIDDLE_RING_20_COLOR);
  if (mr20) s_settings.middle_ring_20_color = rgb_to_gcolor(mr20->value->int32);
  Tuple *cdc = dict_find(iter, KEY_CENTER_DOT_COLOR);
  if (cdc) s_settings.center_dot_color = rgb_to_gcolor(cdc->value->int32);
  Tuple *mrc = dict_find(iter, KEY_MIDDLE_RING_COLOR);
  if (mrc) s_settings.middle_ring_color = rgb_to_gcolor(mrc->value->int32);
  Tuple *dc = dict_find(iter, KEY_DATE_COLOR);
  if (dc) s_settings.date_color = rgb_to_gcolor(dc->value->int32);
  Tuple *tpc = dict_find(iter, KEY_TEMP_COLOR);
  if (tpc) s_settings.temp_color = rgb_to_gcolor(tpc->value->int32);
  Tuple *bie = dict_find(iter, KEY_BATTERY_INDICATOR_ENABLED);
  if (bie) s_settings.battery_indicator_enabled = (bool)bie->value->int32;
  Tuple *shc = dict_find(iter, KEY_SECONDS_HAND_COLOR);
  if (shc) s_settings.seconds_hand_color = rgb_to_gcolor(shc->value->int32);
  Tuple *shm = dict_find(iter, KEY_SECONDS_HAND_MODE);
  if (shm) s_settings.seconds_hand_mode = (int8_t)shm->value->int32;
  Tuple *ssd = dict_find(iter, KEY_SECONDS_SHAKE_DUR);
  if (ssd) s_settings.seconds_shake_dur = (int8_t)ssd->value->int32;

  persist_write_data(PERSIST_SETTINGS, &s_settings, sizeof(Settings));
  persist_write_data(PERSIST_ICONS, s_icons, sizeof(s_icons));
  persist_write_int(PERSIST_TEMP_C, s_temp_c);
  persist_write_int(PERSIST_TEMP_F, s_temp_f);

  // Re-evaluate tick frequency after settings change
  update_tick_subscription();

  s_bg_last_hour = -1;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_hour_layer);
  layer_mark_dirty(s_minute_layer);
  layer_mark_dirty(s_complication_layer);
}

// ============================================================
// WINDOW SETUP
// ============================================================

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_layer_update);
  layer_add_child(root, s_bg_layer);

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

  time_t now = time(NULL);
  s_tick_tm = *localtime(&now);

  s_window = window_create();
  window_set_background_color(s_window, s_settings.background_color);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // Subscribe based on current seconds hand mode
  if (needs_second_ticks()) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
  accel_tap_service_subscribe(accel_tap_handler);

  s_battery_pct = battery_state_service_peek().charge_percent;
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
  accel_tap_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  if (s_shake_timer) app_timer_cancel(s_shake_timer);
  if (s_seconds_timer) app_timer_cancel(s_seconds_timer);
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

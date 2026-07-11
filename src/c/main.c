// Brolly v2.0.0 — main.c
// Complete watchface implementation for Pebble (Aplite/Basalt/Emery)

#include <pebble.h>
#include "gpath_weather.h"
#include <stdlib.h>

// Integer square root (avoids float sqrt and math.h dependency)
static int isqrt_int(int n) {
  if (n <= 0) return 0;
  int x = n;
  int y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}

// Forward declaration
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

// ─────────────────────────────────────────────────────────────────────────────
// Design constants & scaling
// ─────────────────────────────────────────────────────────────────────────────
#define DESIGN_W 144
#define DESIGN_H 168

static int s_screen_w = 144;
static int s_screen_h = 168;

static inline int POS_X(int px) { return (px * s_screen_w) / DESIGN_W; }
static inline int POS_Y(int py) { return (py * s_screen_h) / DESIGN_H; }

#ifdef PBL_COLOR
  #define MONO_COLOR(c) (c)
#else
  #define MONO_COLOR(c) (gcolor_equal((c), GColorBlack) ? GColorBlack : GColorWhite)
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Persist keys
// ─────────────────────────────────────────────────────────────────────────────
#define PERSIST_ICONS    0
#define PERSIST_SETTINGS 1
#define PERSIST_CITY     2

// ─────────────────────────────────────────────────────────────────────────────
// AppMessage inbox/outbox sizes
// ─────────────────────────────────────────────────────────────────────────────
#define APP_MSG_INBOX_SIZE  512
#define APP_MSG_OUTBOX_SIZE  64

// ─────────────────────────────────────────────────────────────────────────────
// Shake / seconds timings
// ─────────────────────────────────────────────────────────────────────────────
#define SHAKE_DISPLAY_MS  5000
#define SHAKE_DELAY_MS     500

// ─────────────────────────────────────────────────────────────────────────────
// Hand geometry constants
// ─────────────────────────────────────────────────────────────────────────────
#define FIXED_HAND_BASE_WIDTH   3
#define FIXED_HAND_OUTER_WIDTH  10
#define FIXED_HAND_INNER_WIDTH  5
#define FIXED_HAND_BASE_PX     20

// ─────────────────────────────────────────────────────────────────────────────
// Settings structure
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
  // Background
  GColor background_color;
  // Hour markers
  bool   display_hour_markers;
  GColor hour_marker_color;
  // Minute markers
  bool   display_minor_markers;
  GColor minute_marker_color;
  // Numbers
  uint8_t number_font;    // 0=Digital 1=Standard 2=Traditional 3=Thin 4=Oversize
  uint8_t number_size;    // 1–5 → {18,22,26,30,36}
  GColor  number_color;
  // Icons
  uint8_t icon_size;      // 1–5 → {18,22,26,30,36} (matches font sizes)
  GColor  icon_color;
  // Watch hands
  GColor hour_hand_outer;
  GColor hour_hand_inner;
  GColor min_hand_outer;
  GColor min_hand_inner;
  // Seconds hand
  GColor  seconds_hand_color;
  uint8_t seconds_hand_mode;   // 0=Never 1=Always 2=Shake
  uint8_t seconds_shake_dur;   // seconds (default 10)
  // Date / temperature
  uint8_t date_visible;   // 0=Always 1=Off 2=Shake
  uint8_t temp_visible;   // 0=Always 1=Off 2=Shake
  uint8_t display_mode;   // 0=Both 1=Temp 2=Date 3=None
  uint8_t temp_unit;      // 0=C 1=F
  GColor  date_color;
  GColor  temp_color;
  // Battery ring
  uint8_t battery_ring_threshold;    // 0=off,50,40,30,20,10
  uint8_t battery_center_threshold;  // 0=off,20,10,5
  // Sunrise/sunset markers
  uint8_t sunrise_marker_visible;    // 0=Always 1=Only with icons 2=Off
  GColor  sunrise_marker_color;
  GColor  sunset_marker_color;
  // Bluetooth
  bool   vibrate_bt_disconnect;
  bool   vibrate_bt_reconnect;
  bool   bt_disconnect_min_inner_red;
  GColor bt_disconnect_outer_color;
  GColor bt_disconnect_inner_color;
  // Shake mode (for icons)
  uint8_t shake_mode;   // 0=Show on shake 1=Always show 2=Always hide
  // City name display
  uint8_t city_display_mode; // 0=Off 1=Shake 2=Always
  GColor  city_color;
  // Icon/number colour mode
  uint8_t icon_color_mode;   // 0=Single colour 1=Weather based 2=Rainbow
  uint8_t complication_layer; // 0=Behind hands 1=On top of hands
} Settings;

// ─────────────────────────────────────────────────────────────────────────────
// Global state
// ─────────────────────────────────────────────────────────────────────────────
static Window   *s_window;
static Layer    *s_bg_layer;
static Layer    *s_complication_layer;
static Layer    *s_hour_layer;
static Layer    *s_minute_layer;
static Layer    *s_seconds_layer;

static Settings  s_settings;
static int8_t    s_icons[24];
static int8_t    s_temp_c = 0;
static int8_t    s_temp_f = 32;
static char      s_city_name[32] = "";
static int8_t    s_sunrise_hour = 6,  s_sunrise_min = 0;
static int8_t    s_sunset_hour  = 18, s_sunset_min  = 0;

static struct tm s_last_time;
static bool      s_bt_connected = true;
static uint8_t   s_battery_pct  = 100;

// Marker cache
static GPoint s_min_marker_outer[60];
static GPoint s_min_marker_inner[60];
static GPoint s_hour_marker_outer[12];
static GPoint s_hour_marker_inner[12];

// Font cache
static GFont s_cached_number_font = NULL;
static uint8_t s_cached_font_id   = 255;
static uint8_t s_cached_font_size  = 255;

// Ink-bounds cache for the current number font (measured once per font change).
// These describe, for a given digit string, how much empty padding sits inside
// the text box on each side: the visible ink rectangle relative to the box.
// Index 0..11 matches s_num_strings ("12","1",.."11").
typedef struct {
  int8_t left;   // px from box left  to first ink column
  int8_t top;    // px from box top   to first ink row
  int8_t right;  // px from last ink column to box right
  int8_t bottom; // px from last ink row    to box bottom
  uint8_t box_w; // measured text box width
  uint8_t box_h; // measured text box height
  bool valid;
} InkBounds;
static InkBounds s_ink[12];
static bool s_ink_valid = false;
// Per-group minimum edge padding: the smallest empty-space value on the
// screen-facing side across all digits in that group. Used so all numbers
// in a group share the same perpendicular baseline regardless of glyph shape.
// top_min = min(ib.top)  for h=11,0,1
// bot_min = min(ib.bottom) for h=5,6,7
// lft_min = min(ib.left)  for h=8,9,10
// rgt_min = min(ib.right) for h=2,3,4
static int8_t s_ink_top_min = 0;
static int8_t s_ink_bot_min = 0;
static int8_t s_ink_lft_min = 0;
static int8_t s_ink_rgt_min = 0;
// Per-group max box dimension on the perpendicular axis. Ensures all
// digits in a group use the same box size for the edge-anchor formula,
// so varying SDK box heights/widths do not shift individual digits.
static uint8_t s_ink_top_box_h = 0; // max box_h for top group (h=11,0,1)
static uint8_t s_ink_bot_box_h = 0; // max box_h for bottom group (h=5,6,7)
static uint8_t s_ink_lft_box_w = 0; // max box_w for left group (h=8,9,10)
static uint8_t s_ink_rgt_box_w = 0; // max box_w for right group (h=2,3,4)

// Shake / icon display state
static bool     s_showing_icons   = false;
static AppTimer *s_shake_timer     = NULL;
static AppTimer *s_shake_delay_timer = NULL;

// Seconds visibility
static bool     s_seconds_visible = false;
static AppTimer *s_seconds_timer   = NULL;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Convert 0xRRGGBB integer to GColor
static GColor rgb_to_gcolor(int32_t rgb) {
  if (rgb == -1) return GColorClear;
  return GColorFromRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// Polar to point (clock-hand angle, 12 o'clock = 0)
static GPoint polar_to_point(GPoint center, int32_t angle, int radius) {
  return GPoint(
    center.x + (int)(sin_lookup(angle) * radius / TRIG_MAX_RATIO),
    center.y - (int)(cos_lookup(angle) * radius / TRIG_MAX_RATIO)
  );
}

// Square perimeter point: where a ray from center at angle hits the rect boundary
// margin_x / margin_y inset the boundary
static GPoint square_perimeter_point(GPoint center, int32_t angle, int margin_x, int margin_y) {
  int hw = s_screen_w / 2 - margin_x;
  int hh = s_screen_h / 2 - margin_y;

  int32_t sin_a = sin_lookup(angle);
  int32_t cos_a = cos_lookup(angle);

  // Avoid division by zero
  if (sin_a == 0 && cos_a == 0) return center;

  // Scale to find intersection with rectangle
  int32_t t_x = (sin_a != 0) ? (hw * TRIG_MAX_RATIO / abs(sin_a)) : INT32_MAX;
  int32_t t_y = (cos_a != 0) ? (hh * TRIG_MAX_RATIO / abs(cos_a)) : INT32_MAX;
  int32_t t   = (t_x < t_y) ? t_x : t_y;

  return GPoint(
    center.x + (int)(sin_a * t / TRIG_MAX_RATIO),
    center.y - (int)(cos_a * t / TRIG_MAX_RATIO)
  );
}

// Angle for a clock position (minutes 0–59 or hours 0–11 scaled)
static int32_t angle_for_minute(int minute) {
  return TRIG_MAX_ANGLE * minute / 60;
}

static int32_t angle_for_hour(int hour, int minute) {
  return TRIG_MAX_ANGLE * (hour * 60 + minute) / 720;
}

// ─────────────────────────────────────────────────────────────────────────────
// Marker cache computation
// ─────────────────────────────────────────────────────────────────────────────
static void compute_markers(void) {
  GPoint center = GPoint(s_screen_w / 2, s_screen_h / 2);
  bool is_emery = (s_screen_w >= 200);

  for (int i = 0; i < 60; i++) {
    int32_t angle = angle_for_minute(i);
    bool is_quarter = (i == 7 || i == 23 || i == 37 || i == 53);
    int marker_len;
    if (is_emery) {
      marker_len = is_quarter ? 10 : 4;
    } else {
      marker_len = is_quarter ? 4 : 2;
    }
    GPoint outer = square_perimeter_point(center, angle, 0, 0);
    // Inner: move marker_len pixels toward center
    int dx = center.x - outer.x;
    int dy = center.y - outer.y;
    int dist = isqrt_int(dx*dx + dy*dy);
    GPoint inner;
    if (dist > 0) {
      inner = GPoint(outer.x + dx * marker_len / dist,
                     outer.y + dy * marker_len / dist);
    } else {
      inner = outer;
    }
    s_min_marker_outer[i] = outer;
    s_min_marker_inner[i] = inner;
  }

  for (int i = 0; i < 12; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    GPoint outer = square_perimeter_point(center, angle, 0, 0);
    int dx = center.x - outer.x;
    int dy = center.y - outer.y;
    int dist = isqrt_int(dx*dx + dy*dy);
    GPoint inner;
    if (dist > 0) {
      inner = GPoint(outer.x + dx * 2 / dist,
                     outer.y + dy * 2 / dist);
    } else {
      inner = outer;
    }
    s_hour_marker_outer[i] = outer;
    s_hour_marker_inner[i] = inner;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Font loading
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t get_font_resource_id(uint8_t font_id, uint8_t size_idx) {
#ifdef PBL_PLATFORM_APLITE
  return 0; // Use system font on Aplite
#else
  static const uint32_t font_resources[5][5] = {
    // Digital
    { RESOURCE_ID_FONT_DIGITAL_18, RESOURCE_ID_FONT_DIGITAL_22,
      RESOURCE_ID_FONT_DIGITAL_26, RESOURCE_ID_FONT_DIGITAL_30,
      RESOURCE_ID_FONT_DIGITAL_36 },
    // Standard
    { RESOURCE_ID_FONT_STANDARD_18, RESOURCE_ID_FONT_STANDARD_22,
      RESOURCE_ID_FONT_STANDARD_26, RESOURCE_ID_FONT_STANDARD_30,
      RESOURCE_ID_FONT_STANDARD_36 },
    // Traditional
    { RESOURCE_ID_FONT_TRADITIONAL_18, RESOURCE_ID_FONT_TRADITIONAL_22,
      RESOURCE_ID_FONT_TRADITIONAL_26, RESOURCE_ID_FONT_TRADITIONAL_30,
      RESOURCE_ID_FONT_TRADITIONAL_36 },
    // Thin
    { RESOURCE_ID_FONT_THIN_18, RESOURCE_ID_FONT_THIN_22,
      RESOURCE_ID_FONT_THIN_26, RESOURCE_ID_FONT_THIN_30,
      RESOURCE_ID_FONT_THIN_36 },
    // Oversize
    { RESOURCE_ID_FONT_OVERSIZE_18, RESOURCE_ID_FONT_OVERSIZE_22,
      RESOURCE_ID_FONT_OVERSIZE_26, RESOURCE_ID_FONT_OVERSIZE_30,
      RESOURCE_ID_FONT_OVERSIZE_36 }
  };
  if (font_id > 4 || size_idx > 4) return 0;
  return font_resources[font_id][size_idx];
#endif
}

static GFont get_number_font(void) {
  uint8_t fid  = s_settings.number_font;
  uint8_t sidx = (s_settings.number_size >= 1 && s_settings.number_size <= 5)
                   ? s_settings.number_size - 1 : 2;

  if (fid == s_cached_font_id && sidx == s_cached_font_size && s_cached_number_font) {
    return s_cached_number_font;
  }

  // Unload old
  if (s_cached_number_font) {
    fonts_unload_custom_font(s_cached_number_font);
    s_cached_number_font = NULL;
  }

#ifdef PBL_PLATFORM_APLITE
  // System font fallbacks for Aplite
  static const char *aplite_fonts[5] = {
    FONT_KEY_LECO_28_LIGHT_NUMBERS,
    FONT_KEY_BITHAM_42_MEDIUM_NUMBERS,
    FONT_KEY_DROID_SERIF_28_BOLD,
    FONT_KEY_GOTHIC_28,
    FONT_KEY_BITHAM_42_BOLD
  };
  s_cached_number_font = fonts_get_system_font(aplite_fonts[fid < 5 ? fid : 0]);
#else
  uint32_t res_id = get_font_resource_id(fid, sidx);
  if (res_id) {
    s_cached_number_font = fonts_load_custom_font(resource_get_handle(res_id));
  } else {
    s_cached_number_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  }
#endif

  s_cached_font_id   = fid;
  s_cached_font_size = sidx;
  s_ink_valid = false;  // force re-measure of ink bounds for the new font
  return s_cached_number_font;
}

// ─────────────────────────────────────────────────────────────────────────────
// Number positioning helpers
// ─────────────────────────────────────────────────────────────────────────────
static const char *s_num_strings[12] = {
  "12","1","2","3","4","5","6","7","8","9","10","11"
};

static GPoint number_position(int h, GFont font) {
  bool is_emery = (s_screen_w >= 200);
  int margin = is_emery ? 12 : 6;
  int sw = s_screen_w;
  int sh = s_screen_h;

  // Measure text size
  GRect measure_box = GRect(0, 0, 50, 50);
  GSize sz = graphics_text_layout_get_content_size(
    s_num_strings[h], font, measure_box,
    GTextOverflowModeWordWrap, GTextAlignmentCenter);
  int actual_w = sz.w;
  int actual_h = sz.h;

  int extra_top = (s_settings.number_font == 0) ? 7 : 0;
  int y_top = margin + actual_h / 2;
  int y_mid = (sh - 1) / 2;
  int y_bot = (sh - 1 - margin) - actual_h / 2;
  // Actual rendered centre of 12 and 6 (used for 25%/75% positioning of 2/4/8/10)
  int y_12 = y_top + extra_top;  // centre of number 12
  int y_6  = y_bot;              // centre of number 6
  int y_25 = y_12 + (y_6 - y_12) * 25 / 100;  // 25% between 12 and 6
  int y_75 = y_12 + (y_6 - y_12) * 75 / 100;  // 75% between 12 and 6

  GPoint pos = GPoint(sw / 2, sh / 2);

  if (h == 0 || h == 1 || h == 11) {
    // Top edge
    pos.y = margin + actual_h / 2 + extra_top;
    if (h == 0) {
      pos.x = sw / 2;
    } else {
      // Diagonal: project angle to top/bottom edge
      int32_t angle = TRIG_MAX_ANGLE * h / 12;
      GPoint edge = square_perimeter_point(GPoint(sw/2, sh/2), angle, 0, 0);
      int shift = is_emery ? 7 : 2;
      pos.x = edge.x + (sw/2 - edge.x) * shift / actual_w;
    }
  } else if (h == 5 || h == 6 || h == 7) {
    // Bottom edge
    pos.y = sh - 1 - margin - actual_h / 2;
    if (h == 6) {
      pos.x = sw / 2;
    } else {
      int32_t angle = TRIG_MAX_ANGLE * h / 12;
      GPoint edge = square_perimeter_point(GPoint(sw/2, sh/2), angle, 0, 0);
      int shift = is_emery ? 7 : 2;
      pos.x = edge.x + (sw/2 - edge.x) * shift / actual_w;
    }
  } else if (h == 8 || h == 9 || h == 10) {
    // Left edge
    pos.x = margin;
    if (h == 9)       pos.y = y_mid;
    else if (h == 10) pos.y = y_25;  // 25% between 12 and 6
    else              pos.y = y_75;  // 75% between 12 and 6 (h==8)
  } else {
    // Right edge (h == 2, 3, 4)
    pos.x = sw - 1 - margin;
    if (h == 3)      pos.y = y_mid;
    else if (h == 2) pos.y = y_25;  // 25% between 12 and 6
    else             pos.y = y_75;  // 75% between 12 and 6 (h==4)
  }

  return pos;
}

// ─────────────────────────────────────────────────────────────────────────────
// Hand drawing
// The hand is drawn as a single continuous GPath outline:
//   centre → stem → base cap (inward semicircle) → left side →
//   tip cap (outward semicircle) → right side → base cap → centre
// This produces one seamless cohesive shape with no joints.
// The interior gap is never drawn — true hole showing layers below.
// ─────────────────────────────────────────────────────────────────────────────
//
// Pill dimensions (all in pixels, fixed regardless of hand length):
//   2px wall | 5px gap | 2px wall  =  9px total outer width
//   Wall centre = 3.5px from axis  -> use PILL_LINE_OFF = 4 (rounds outward)
//   Cap radius  = PILL_LINE_OFF so the semicircle exactly spans the wall centres
//   Stem width  = 2px, centred on axis, connects at the inner wall edge (2.5px)
#define PILL_LINE_OFF   4    // px from axis to centre of each wall line
#define PILL_CAP_R      4    // semicircle radius = PILL_LINE_OFF
#define PILL_STROKE     2    // wall line and cap stroke width
#define STEM_STROKE     2    // centre stem stroke width
#define PILL_CAP_SEGS   12   // segments per semicircle (12 = smoother at 4px radius)

// Path points: base right + base arc interior + base left + left wall +
//   tip left (dup) + tip arc interior + tip right + right wall = 2*SEGS+6
#define HAND_PATH_PTS  (2 * PILL_CAP_SEGS + 6)

static void draw_inittick_hand(GContext *ctx, GPoint center, GPoint tip,
                                GColor hand_color, GColor inner_color) {
  if (gcolor_equal(hand_color, GColorClear) && gcolor_equal(inner_color, GColorClear)) return;

  int dx = tip.x - center.x;
  int dy = tip.y - center.y;
  int dist = isqrt_int(dx * dx + dy * dy);
  if (dist == 0) return;

  GColor hc = MONO_COLOR(hand_color);

  // Hand axis angle and perpendicular angle
  int32_t ha = atan2_lookup(dx, -dy);
  int32_t pa = ha + (TRIG_MAX_ANGLE / 4);

  // Perpendicular offset to wall line centres
  int ox = (int32_t)PILL_LINE_OFF * sin_lookup(pa) / TRIG_MAX_RATIO;
  int oy = -(int32_t)PILL_LINE_OFF * cos_lookup(pa) / TRIG_MAX_RATIO;
  if (ox == 0 && oy == 0) ox = 1;

  // base_pt: centre of the base cap (POS_Y(FIXED_HAND_BASE_PX) from centre)
  int base_px = POS_Y(FIXED_HAND_BASE_PX);
  GPoint base_pt = GPoint(center.x + dx * base_px / dist,
                          center.y + dy * base_px / dist);

  // ── Centre stem: 2px line from pivot to the inward edge of the base cap ────
  // Stop at base_pt - PILL_CAP_R so the stem does not intrude into the gap
  GPoint stem_end = GPoint(
    base_pt.x - PILL_CAP_R * dx / dist,
    base_pt.y - PILL_CAP_R * dy / dist
  );
  if (!gcolor_equal(hand_color, GColorClear)) {
    graphics_context_set_stroke_color(ctx, hc);
    graphics_context_set_stroke_width(ctx, STEM_STROKE);
    graphics_draw_line(ctx, center, stem_end);
  }

  // ── Pill outline: single continuous polyline ─────────────────────────────────────────────
  // Sequence: base cap (inward) → left side → tip cap (outward) → right side
  // The path is open (not closed back to centre) — stem handles that visually.
  GPoint pts[HAND_PATH_PTS];
  int n = 0;

  // Base cap: inward semicircle, arc from (ha+90°) to (ha+270°).
  // Force first/last points to exact wall endpoints to eliminate integer-rounding gaps.
  pts[n++] = GPoint(base_pt.x + ox, base_pt.y + oy);  // exact right wall start
  for (int i = 1; i < PILL_CAP_SEGS; i++) {
    int32_t a = (ha + (TRIG_MAX_ANGLE / 4))
                + (TRIG_MAX_ANGLE / 2) * i / PILL_CAP_SEGS;
    pts[n++] = GPoint(
      base_pt.x + PILL_CAP_R * sin_lookup(a) / TRIG_MAX_RATIO,
      base_pt.y - PILL_CAP_R * cos_lookup(a) / TRIG_MAX_RATIO
    );
  }
  pts[n++] = GPoint(base_pt.x - ox, base_pt.y - oy);  // exact left wall start

  // Left wall to tip left
  pts[n++] = GPoint(tip.x - ox, tip.y - oy);

  // Tip cap: outward semicircle, arc from (ha-90°) to (ha+90°).
  // Force first/last points to exact wall endpoints.
  pts[n++] = GPoint(tip.x - ox, tip.y - oy);  // exact left wall end (duplicate for join)
  for (int i = 1; i < PILL_CAP_SEGS; i++) {
    int32_t a = (ha - (TRIG_MAX_ANGLE / 4))
                + (TRIG_MAX_ANGLE / 2) * i / PILL_CAP_SEGS;
    pts[n++] = GPoint(
      tip.x + PILL_CAP_R * sin_lookup(a) / TRIG_MAX_RATIO,
      tip.y - PILL_CAP_R * cos_lookup(a) / TRIG_MAX_RATIO
    );
  }
  pts[n++] = GPoint(tip.x + ox, tip.y + oy);  // exact right wall end

  // Right wall back to base right
  pts[n++] = GPoint(base_pt.x + ox, base_pt.y + oy);

  // Draw the pill outline as one connected polyline
  graphics_context_set_stroke_color(ctx, hc);
  graphics_context_set_stroke_width(ctx, PILL_STROKE);
  for (int i = 0; i < n - 1; i++) {
    graphics_draw_line(ctx, pts[i], pts[i + 1]);
  }
  // ── Inner colour line + cap sealing ──────────────────────────────────────────
  // Cap fill circles are only drawn when inner_color is set, so that a
  // transparent inner colour does not leave visible dots at the cap ends.
#define INNER_LINE_WIDTH  4  // 4px — centred in the 6px gap
#define INNER_END_R       2  // 2px end cap radius
  if (!gcolor_equal(inner_color, GColorClear)) {
    GColor ic = MONO_COLOR(inner_color);
    // Seal cap pixel gaps in hand colour first, then paint inner colour on top
    graphics_context_set_fill_color(ctx, hc);
    graphics_fill_circle(ctx, base_pt, PILL_CAP_R);
    graphics_fill_circle(ctx, tip,     PILL_CAP_R);
    // Inner colour line
    graphics_context_set_stroke_color(ctx, ic);
    graphics_context_set_stroke_width(ctx, INNER_LINE_WIDTH);
    graphics_draw_line(ctx, base_pt, tip);
    // Rounded ends on top of cap fills
    graphics_context_set_fill_color(ctx, ic);
    graphics_fill_circle(ctx, base_pt, INNER_END_R);
    graphics_fill_circle(ctx, tip,     INNER_END_R);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Layer update callbacks
// ─────────────────────────────────────────────────────────────────────────────

// Measure the true visible-ink rectangle of each digit string for the current
// number font. We draw each string into a scratch area, capture the frame
// buffer, and scan for lit pixels. Run once per font change inside the paint
// callback (the only place text rendering + framebuffer capture is valid).
// While measuring, the scratch draws are overwritten by the normal paint that
// follows, so nothing is visible to the user.
static void measure_ink_bounds(GContext *ctx, GFont font, int sw, int sh) {
  (void)sw; (void)sh;
  // Scratch box near top-left. Must be large enough for any digit at any size.
  // We use 80x80 to match the content-size query rect exactly, so that
  // max_y is always relative to the same origin as box.h.
  const int SX = 0, SY = 0, SW = 80, SH = 80;

  for (int h = 0; h < 12; h++) {
    InkBounds *ib = &s_ink[h];
    ib->valid = false;

    // Use the full SW×SH scratch area as both the draw rect and the scan
    // region. The SDK's graphics_text_layout_get_content_size can return
    // a box.h smaller than the actual rendered pixels for some glyphs
    // (e.g. Raleway-Light "5"/"7"), causing those digits to be placed
    // lower than "6". By scanning the full area and using SW/SH as the
    // reference box, all digits share the same fixed frame and the
    // padding values are directly comparable across glyphs.
    ib->box_w = (uint8_t)SW;
    ib->box_h = (uint8_t)SH;

    // Clear scratch area, draw digit in white, top-left aligned.
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(SX, SY, SW, SH), 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, s_num_strings[h], font,
                       GRect(SX, SY, SW, SH),
                       GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    // Capture and scan the full SW×SH area.
    GBitmap *fb = graphics_capture_frame_buffer(ctx);
    if (!fb) continue;

    int min_x = SW, min_y = SH, max_x = -1, max_y = -1;

    for (int y = 0; y < SH; y++) {
      GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, SY + y);
      for (int x = 0; x < SW; x++) {
        int px = SX + x;
        if (px < row.min_x || px > row.max_x) continue;
        bool lit;
#if defined(PBL_COLOR)
        GColor8 c = (GColor8){ .argb = row.data[px] };
        lit = (c.r || c.g || c.b);
#else
        uint8_t byte = row.data[px >> 3];
        lit = (byte >> (px & 7)) & 1;
#endif
        if (lit) {
          if (x < min_x) min_x = x;
          if (x > max_x) max_x = x;
          if (y < min_y) min_y = y;
          if (y > max_y) max_y = y;
        }
      }
    }
    graphics_release_frame_buffer(ctx, fb);

    if (max_x < 0) {
      // No ink found â fall back to zero padding.
      ib->left = ib->top = ib->right = ib->bottom = 0;
      ib->valid = true;
      continue;
    }

    // Padding from each side of the SWÃSH scratch box to the ink edge.
    ib->left   = (int8_t)min_x;
    ib->top    = (int8_t)min_y;
    ib->right  = (int8_t)(SW - 1 - max_x);
    ib->bottom = (int8_t)(SH - 1 - max_y);
    if (ib->right  < 0) ib->right  = 0;
    if (ib->bottom < 0) ib->bottom = 0;
    ib->valid = true;
  }

  // Compute per-group minimum edge padding. All digits in a group share the
  // same perpendicular baseline: the one set by whichever digit has the LEAST
  // empty space on the screen-facing side (i.e. whose ink extends furthest
  // toward the edge). This prevents glyphs with more internal whitespace from
  // appearing to "float" away from the edge relative to their neighbours.
  s_ink_top_min = 127;  s_ink_top_box_h = 0;
  s_ink_bot_min = 127;  s_ink_bot_box_h = 0;
  s_ink_lft_min = 127;  s_ink_lft_box_w = 0;
  s_ink_rgt_min = 127;  s_ink_rgt_box_w = 0;
  for (int h = 0; h < 12; h++) {
    if (!s_ink[h].valid) continue;
    if (h == 11 || h == 0 || h == 1) {
      if (s_ink[h].top    < s_ink_top_min) s_ink_top_min = s_ink[h].top;
      if (s_ink[h].box_h  > s_ink_top_box_h) s_ink_top_box_h = s_ink[h].box_h;
    } else if (h == 5 || h == 6 || h == 7) {
      if (s_ink[h].bottom < s_ink_bot_min) s_ink_bot_min = s_ink[h].bottom;
      if (s_ink[h].box_h  > s_ink_bot_box_h) s_ink_bot_box_h = s_ink[h].box_h;
    } else if (h == 8 || h == 9 || h == 10) {
      if (s_ink[h].left   < s_ink_lft_min) s_ink_lft_min = s_ink[h].left;
      if (s_ink[h].box_w  > s_ink_lft_box_w) s_ink_lft_box_w = s_ink[h].box_w;
    } else {
      if (s_ink[h].right  < s_ink_rgt_min) s_ink_rgt_min = s_ink[h].right;
      if (s_ink[h].box_w  > s_ink_rgt_box_w) s_ink_rgt_box_w = s_ink[h].box_w;
    }
  }
  if (s_ink_top_min == 127) s_ink_top_min = 0;
  if (s_ink_bot_min == 127) s_ink_bot_min = 0;
  if (s_ink_lft_min == 127) s_ink_lft_min = 0;
  if (s_ink_rgt_min == 127) s_ink_rgt_min = 0;

  // Clear the scratch area so the subsequent normal paint starts clean.
  graphics_context_set_fill_color(ctx, MONO_COLOR(s_settings.background_color));
  graphics_fill_rect(ctx, GRect(SX, SY, SW, SH), 0, GCornerNone);

  s_ink_valid = true;
}

// BG layer: background fill, markers, numbers/icons
static void bg_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int sw = bounds.size.w;
  int sh = bounds.size.h;
  GPoint center = GPoint(sw / 2, sh / 2);

  // Fill background
  graphics_context_set_fill_color(ctx, MONO_COLOR(s_settings.background_color));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Minute markers
  if (s_settings.display_minor_markers) {
    graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.minute_marker_color));
    graphics_context_set_stroke_width(ctx, 1);
    for (int i = 0; i < 60; i++) {
      graphics_draw_line(ctx, s_min_marker_outer[i], s_min_marker_inner[i]);
    }
  }

  // Hour markers
  if (s_settings.display_hour_markers) {
    graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.hour_marker_color));
    graphics_context_set_stroke_width(ctx, 3);
    for (int i = 0; i < 12; i++) {
      graphics_draw_line(ctx, s_hour_marker_outer[i], s_hour_marker_inner[i]);
    }
  }

  // Sunrise / sunset markers
  int sun_inset = (sw >= 200) ? 8 : 5;
  bool show_sun_markers = false;
  if (s_settings.sunrise_marker_visible == 0) {
    show_sun_markers = true;
  } else if (s_settings.sunrise_marker_visible == 1) {
    show_sun_markers = s_showing_icons;
  }
  // else 2 = Off

  if (show_sun_markers) {

    // Sunrise marker
    int sr_total_min = s_sunrise_hour * 60 + s_sunrise_min;
    int cur_total_min = s_last_time.tm_hour * 60 + s_last_time.tm_min;
    int sr_delta = sr_total_min - cur_total_min;
    if (sr_delta < 0) sr_delta += 720;
    if (sr_delta <= 720) {
      int32_t sr_angle = angle_for_minute(s_sunrise_hour * 5 + s_sunrise_min / 12);
      GPoint sr_outer = square_perimeter_point(center, sr_angle, 0, 0);
      int dx = center.x - sr_outer.x;
      int dy = center.y - sr_outer.y;
      int dist = isqrt_int(dx*dx + dy*dy);
      GPoint sr_inner = (dist > 0)
        ? GPoint(sr_outer.x + dx * sun_inset / dist, sr_outer.y + dy * sun_inset / dist)
        : sr_outer;
      graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.sunrise_marker_color));
      graphics_context_set_stroke_width(ctx, 2);
      graphics_draw_line(ctx, sr_outer, sr_inner);
    }

    // Sunset marker
    int ss_total_min = s_sunset_hour * 60 + s_sunset_min;
    int ss_delta = ss_total_min - cur_total_min;
    if (ss_delta < 0) ss_delta += 720;
    if (ss_delta <= 720) {
      int32_t ss_angle = angle_for_minute(s_sunset_hour * 5 + s_sunset_min / 12);
      GPoint ss_outer = square_perimeter_point(center, ss_angle, 0, 0);
      int dx = center.x - ss_outer.x;
      int dy = center.y - ss_outer.y;
      int dist = isqrt_int(dx*dx + dy*dy);
      GPoint ss_inner = (dist > 0)
        ? GPoint(ss_outer.x + dx * sun_inset / dist, ss_outer.y + dy * sun_inset / dist)
        : ss_outer;
      graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.sunset_marker_color));
      graphics_context_set_stroke_width(ctx, 2);
      graphics_draw_line(ctx, ss_outer, ss_inner);
    }
  }

  // Numbers or icons
  GFont num_font = get_number_font();
  int cur_hour = s_last_time.tm_hour;
  int cur_min  = s_last_time.tm_min;

  // Measure true ink bounds once per font change. Must happen here (paint
  // callback) where text rendering and framebuffer capture are valid.
  if (!s_ink_valid) {
    measure_ink_bounds(ctx, num_font, sw, sh);
  }

  // Determine icon size in pixels
  static const int s_icon_sizes[5] = {18, 22, 26, 30, 36};
  int icon_sz_idx = (s_settings.icon_size >= 1 && s_settings.icon_size <= 5)
                      ? s_settings.icon_size - 1 : 2;
  int icon_sz = s_icon_sizes[icon_sz_idx];

  bool draw_icons = false;
  if (s_settings.shake_mode == 1) {
    draw_icons = true;
  } else if (s_settings.shake_mode == 0) {
    draw_icons = s_showing_icons;
  }
  // shake_mode == 2 = always hide icons

  for (int h = 0; h < 12; h++) {
    if (draw_icons) {
      // Determine which forecast hour to show
      int clock_num = (h == 0) ? 12 : h;
      int am_hour = (clock_num == 12) ? 0 : clock_num;
      int pm_hour = (clock_num == 12) ? 12 : clock_num + 12;
      bool am_passed = (am_hour < cur_hour) || (am_hour == cur_hour && cur_min > 0);
      bool pm_passed = (pm_hour < cur_hour) || (pm_hour == cur_hour && cur_min > 0);
      int icon_hour = (!am_passed) ? am_hour : (!pm_passed) ? pm_hour : am_hour;
      int8_t icon_code = s_icons[icon_hour];
      GPathIconID gpath_id = icon_code_to_gpath(icon_code);

      // Position icon flush to the nearest screen edge, with the cross-axis
      // following the hour-angle perimeter ray (same alignment as numbers).
      int32_t i_angle = TRIG_MAX_ANGLE * h / 12;
      GPoint i_edge = square_perimeter_point(GPoint(sw/2, sh/2), i_angle, 0, 0);

      // Gap from screen edge to icon edge. Matches number gap to clear markers.
      int icon_gap = sun_inset * 3 / 2;
      int ox, oy;

      if (h == 11 || h == 0 || h == 1) {
        // Top group: top edge of icon `icon_gap` from top edge.
        oy = icon_gap;
        ox = i_edge.x - icon_sz / 2;
        if (h == 11) ox += icon_sz / 4;   // shift 11 right by 25% of width
        if (h == 1)  ox -= icon_sz / 4;   // shift 1 left by 25% of width
      } else if (h == 5 || h == 6 || h == 7) {
        // Bottom group: bottom edge `icon_gap` from bottom edge.
        oy = sh - icon_gap - icon_sz;
        ox = i_edge.x - icon_sz / 2;
        if (h == 7) ox += icon_sz / 4;   // shift 7 right by 25% of width
        if (h == 5) ox -= icon_sz / 4;   // shift 5 left by 25% of width
      } else if (h == 8 || h == 9 || h == 10) {
        // Left group: left edge `icon_gap` from left edge.
        ox = icon_gap;
        {
          // 25%/75% between rendered centres of 12 and 6 icons
          int y_12c = icon_gap + icon_sz / 2;
          int y_6c  = sh - icon_gap - icon_sz / 2;
          int cross_y = i_edge.y;
          if (h == 10)      cross_y = y_12c + (y_6c - y_12c) * 25 / 100;
          else if (h == 8)  cross_y = y_12c + (y_6c - y_12c) * 75 / 100;
          oy = cross_y - icon_sz / 2;
        }
      } else {
        // Right group (h==2,3,4): right edge `icon_gap` from right edge.
        ox = sw - icon_gap - icon_sz;
        {
          // 25%/75% between rendered centres of 12 and 6 icons
          int y_12c = icon_gap + icon_sz / 2;
          int y_6c  = sh - icon_gap - icon_sz / 2;
          int cross_y = i_edge.y;
          if (h == 2)      cross_y = y_12c + (y_6c - y_12c) * 25 / 100;
          else if (h == 4) cross_y = y_12c + (y_6c - y_12c) * 75 / 100;
          oy = cross_y - icon_sz / 2;
        }
      }

      // Determine icon colour based on mode
      GColor icon_draw_color;
      if (s_settings.icon_color_mode == 2) {
        // Rainbow: colour based on hour position around the dial
        // 12 positions → hues 0..330 in steps of 30
        static const GColor8 s_rainbow_colors[12] = {
          { .argb = 0xF0 }, // h=0  red        (255,0,0)
          { .argb = 0xF8 }, // h=1  orange     (255,170,0)
          { .argb = 0xFC }, // h=2  yellow     (255,255,0)
          { .argb = 0xEC }, // h=3  chartreuse (170,255,0)
          { .argb = 0xCC }, // h=4  green      (0,255,0)
          { .argb = 0xCE }, // h=5  spring     (0,255,170)
          { .argb = 0xCF }, // h=6  cyan       (0,255,255)
          { .argb = 0xCB }, // h=7  sky blue   (0,170,255)
          { .argb = 0xC3 }, // h=8  blue       (0,0,255)
          { .argb = 0xE3 }, // h=9  violet     (170,0,255)
          { .argb = 0xF3 }, // h=10 magenta    (255,0,255)
          { .argb = 0xF2 }, // h=11 rose       (255,0,170)
        };
        icon_draw_color = MONO_COLOR(s_rainbow_colors[h]);
      } else {
        icon_draw_color = MONO_COLOR(s_settings.icon_color);
      }
      if (s_settings.icon_color_mode == 3) {
        // Line shading mode: hatched fill with weather colours
        draw_weather_icon_shaded(ctx, gpath_id, ox, oy, icon_sz);
      } else {
        draw_weather_icon(ctx, gpath_id, ox, oy, icon_sz, icon_draw_color,
                          s_settings.icon_color_mode == 1);
      }
    } else {
      // Draw number anchored by its TRUE VISIBLE INK edge a constant gap from
      // the nearest screen edge. The cross-axis position comes from the
      // hour-angle perimeter ray so each number lines up under its clock
      // position. The text box itself is offset so that, after the font's
      // internal padding (s_ink[h]), the lit pixels land exactly where we want.
      int32_t angle = TRIG_MAX_ANGLE * h / 12;
      GPoint edge = square_perimeter_point(GPoint(sw/2, sh/2), angle, 0, 0);

      InkBounds ib = s_ink[h];
      if (!ib.valid) {
        // Defensive fallback: treat the whole box as ink.
        GSize sz = graphics_text_layout_get_content_size(
          s_num_strings[h], num_font, GRect(0,0,80,80),
          GTextOverflowModeWordWrap, GTextAlignmentLeft);
        ib.box_w = (uint8_t)sz.w; ib.box_h = (uint8_t)sz.h;
        ib.left = ib.top = ib.right = ib.bottom = 0;
        ib.valid = true;
      }

      // Dimensions of the visible ink itself.
      int ink_w = ib.box_w - ib.left - ib.right;
      int ink_h = ib.box_h - ib.top  - ib.bottom;
      if (ink_w < 1) ink_w = 1;
      if (ink_h < 1) ink_h = 1;

      // Gap from screen edge to visible ink. Must clear the longest marker
      // (4px on Basalt quarter-hour, 10px on Emery) plus a small margin.
      int gap = sun_inset * 3 / 2;

      // Top-left corner of the (untrimmed) text box. We position so that the
      // visible ink edge sits `gap` from the screen edge, and the ink centre
      // sits on the perimeter ray on the cross axis.
      int rx, ry;

      if (h == 11 || h == 0 || h == 1) {
        // Top group: shared baseline = digit with smallest ib.top.
        // All digits use the group max box_h so the box origin is consistent.
        ry = gap - ib.top;
        rx = edge.x - ink_w / 2 - ib.left;
        if (h == 11) rx += ink_w / 4;   // shift 11 right by 25% of its own ink width
        if (h == 1) {
          // shift 1 left by 25% of 11's ink width
          InkBounds ib11 = s_ink[11];
          int ink_w_11 = ib11.box_w - ib11.left - ib11.right;
          if (ink_w_11 < 1) ink_w_11 = 1;
          rx -= ink_w_11 / 4;
        }
      } else if (h == 5 || h == 6 || h == 7) {
        // Bottom group: shared baseline = digit with smallest ib.bottom.
        // Use group max box_h so all digits share the same ry regardless of
        // per-digit SDK box height variation (fixes Thin 5/7 too low).
        ry = sh - gap - 80 + ib.bottom;
        rx = edge.x - ink_w / 2 - ib.left;
        if (h == 7) rx += ink_w / 4;   // shift 7 right by 25% of ink width
        if (h == 5) rx -= ink_w / 4;   // shift 5 left by 25% of ink width
      } else if (h == 8 || h == 9 || h == 10) {
        // Left group: shared baseline = digit with smallest ib.left.
        // Use group max box_w so all digits share the same rx regardless of
        // per-digit SDK box width variation (fixes Digital 10 not at edge).
        rx = gap - ib.left;
        {
          // 25%/75% between rendered ink centres of 12 and 6
          InkBounds ib12 = s_ink[0];  // h==0 is "12"
          InkBounds ib6  = s_ink[6];  // h==6 is "6"
          int y_12c = gap + (ib12.box_h - ib12.top - ib12.bottom) / 2;
          int y_6c  = sh - gap - (ib6.box_h - ib6.top - ib6.bottom) / 2;
          int cross_y = edge.y;
          if (h == 10)      cross_y = y_12c + (y_6c - y_12c) * 25 / 100;
          else if (h == 8)  cross_y = y_12c + (y_6c - y_12c) * 75 / 100;
          ry = cross_y - ink_h / 2 - ib.top;
        }
      } else {
        // Right group: shared baseline = digit with smallest ib.right.
        // Use group max box_w for consistent rx.
        rx = sw - gap - 80 + ib.right;
        {
          // 25%/75% between rendered ink centres of 12 and 6
          InkBounds ib12 = s_ink[0];  // h==0 is "12"
          InkBounds ib6  = s_ink[6];  // h==6 is "6"
          int y_12c = gap + (ib12.box_h - ib12.top - ib12.bottom) / 2;
          int y_6c  = sh - gap - (ib6.box_h - ib6.top - ib6.bottom) / 2;
          int cross_y = edge.y;
          if (h == 2)      cross_y = y_12c + (y_6c - y_12c) * 25 / 100;
          else if (h == 4) cross_y = y_12c + (y_6c - y_12c) * 75 / 100;
          ry = cross_y - ink_h / 2 - ib.top;
        }
      }

      GRect text_rect = GRect(rx, ry, 80, 80);
      // Determine number colour based on mode
      GColor num_draw_color;
      if (s_settings.icon_color_mode == 2) {
        // Rainbow: same colour wheel as icons
        static const GColor8 s_rainbow_num_colors[12] = {
          { .argb = 0xF0 }, // h=0  red        (255,0,0)
          { .argb = 0xF8 }, // h=1  orange     (255,170,0)
          { .argb = 0xFC }, // h=2  yellow     (255,255,0)
          { .argb = 0xEC }, // h=3  chartreuse (170,255,0)
          { .argb = 0xCC }, // h=4  green      (0,255,0)
          { .argb = 0xCE }, // h=5  spring     (0,255,170)
          { .argb = 0xCF }, // h=6  cyan       (0,255,255)
          { .argb = 0xCB }, // h=7  sky blue   (0,170,255)
          { .argb = 0xC3 }, // h=8  blue       (0,0,255)
          { .argb = 0xE3 }, // h=9  violet     (170,0,255)
          { .argb = 0xF3 }, // h=10 magenta    (255,0,255)
          { .argb = 0xF2 }, // h=11 rose       (255,0,170)
        };
        num_draw_color = MONO_COLOR(s_rainbow_num_colors[h]);
      } else {
        num_draw_color = MONO_COLOR(s_settings.number_color);
      }
      graphics_context_set_text_color(ctx, num_draw_color);
      graphics_draw_text(ctx, s_num_strings[h], num_font, text_rect,
                         GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  }
}

// Complication layer: date + temperature
static void complication_layer_update(Layer *layer, GContext *ctx) {
  int sw = s_screen_w;
  (void)sw;
  int cur_min = s_last_time.tm_min;

  // Determine y position (avoid minute hand)
  int comp_y;
  if (cur_min >= 20 && cur_min <= 40) {
    comp_y = POS_Y(45);
  } else {
    comp_y = POS_Y(105);
  }

  bool is_emery = (sw >= 200);
  GFont comp_font = fonts_get_system_font(is_emery ? FONT_KEY_GOTHIC_24 : FONT_KEY_GOTHIC_14);

  // Determine what to show
  bool show_date = false;
  bool show_temp = false;

  // display_mode: 0=Both 1=Temp 2=Date 3=None
  bool mode_date = (s_settings.display_mode == 0 || s_settings.display_mode == 2);
  bool mode_temp = (s_settings.display_mode == 0 || s_settings.display_mode == 1);

  // date_visible / temp_visible: 0=Always 1=Off 2=Shake
  if (mode_date) {
    if (s_settings.date_visible == 0) show_date = true;
    else if (s_settings.date_visible == 2) show_date = s_showing_icons;
  }
  if (mode_temp) {
    if (s_settings.temp_visible == 0) show_temp = true;
    else if (s_settings.temp_visible == 2) show_temp = s_showing_icons;
  }

  if (!show_date && !show_temp) return;

  // Date string
  char date_buf[16] = "";
  if (show_date) {
    static const char *day_names[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    snprintf(date_buf, sizeof(date_buf), "%s %d",
             day_names[s_last_time.tm_wday], s_last_time.tm_mday);
  }

  // Temperature string
  char temp_buf[16] = "";
  if (show_temp) {
    if (s_settings.temp_unit == 0) {
      snprintf(temp_buf, sizeof(temp_buf), "%d\xc2\xb0""C", (int)s_temp_c);
    } else {
      snprintf(temp_buf, sizeof(temp_buf), "%d\xc2\xb0""F", (int)s_temp_f);
    }
  }

  int line_gap = 18;
  int x = sw / 2;

  if (show_date) {
    GRect dr = GRect(0, comp_y - 10, sw, 20);
    graphics_context_set_text_color(ctx, MONO_COLOR(s_settings.date_color));
    graphics_draw_text(ctx, date_buf, comp_font, dr,
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    (void)x;
  }

  if (show_temp) {
    int ty = show_date ? comp_y + line_gap - 10 : comp_y - 10;
    GRect tr = GRect(0, ty, sw, 20);
    graphics_context_set_text_color(ctx, MONO_COLOR(s_settings.temp_color));
    graphics_draw_text(ctx, temp_buf, comp_font, tr,
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }

  // City name: show on the opposite side from date/temp
  if (s_settings.city_display_mode != 0 && s_city_name[0] != '\0') {
    bool show_city = false;
    if (s_settings.city_display_mode == 2) {
      show_city = true;
    } else if (s_settings.city_display_mode == 1) {
      show_city = s_showing_icons; // same shake gate as icons
    }
    if (show_city) {
      // comp_y is POS_Y(45) or POS_Y(105). Opposite side:
      int city_y;
      if (comp_y < s_screen_h / 2) {
        // date/temp is near top → city near bottom
        city_y = POS_Y(105);
      } else {
        // date/temp is near bottom → city near top
        city_y = POS_Y(45);
      }
      GRect cr = GRect(0, city_y - 10, sw, 20);
      graphics_context_set_text_color(ctx, MONO_COLOR(s_settings.city_color));
      graphics_draw_text(ctx, s_city_name, comp_font, cr,
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
  }
}

// Hour hand layer
static void hour_layer_update(Layer *layer, GContext *ctx) {
  int sw = s_screen_w;
  int sh = s_screen_h;
  GPoint center = GPoint(sw / 2, sh / 2);
  int radius = (sw < sh ? sw : sh) / 2;

  int32_t angle = angle_for_hour(s_last_time.tm_hour % 12, s_last_time.tm_min);
  GPoint tip = polar_to_point(center, angle, radius * 60 / 100);

  draw_inittick_hand(ctx, center, tip,
                     s_settings.hour_hand_outer,
                     s_settings.hour_hand_inner);
}

// Minute hand + seconds hand (below centre rings) + centre cap layer
static void minute_layer_update(Layer *layer, GContext *ctx) {
  int sw = s_screen_w;
  int sh = s_screen_h;
  GPoint center = GPoint(sw / 2, sh / 2);
  int radius = (sw < sh ? sw : sh) / 2;

  int32_t angle = angle_for_minute(s_last_time.tm_min);
  GPoint tip = polar_to_point(center, angle, radius * 95 / 100);

  // BT disconnect override
  GColor min_outer = s_settings.min_hand_outer;
  GColor min_inner = s_settings.min_hand_inner;
  if (!s_bt_connected && s_settings.bt_disconnect_min_inner_red) {
    min_outer = s_settings.bt_disconnect_outer_color;
    min_inner = s_settings.bt_disconnect_inner_color;
  }

  draw_inittick_hand(ctx, center, tip, min_outer, min_inner);

  // ── Seconds hand — drawn here so it appears BELOW the centre rings ──────────
  if (s_seconds_visible) {
    int32_t sec_angle = angle_for_minute(s_last_time.tm_sec);
    GPoint sec_tip  = square_perimeter_point(center, sec_angle, 0, 0);
    int32_t tail_angle = sec_angle + (TRIG_MAX_ANGLE / 2);
    GPoint sec_tail = polar_to_point(center, tail_angle, POS_Y(18));
    graphics_context_set_stroke_color(ctx, MONO_COLOR(s_settings.seconds_hand_color));
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, sec_tail, sec_tip);
  }

  // Centre cap — five concentric circles
  // Battery alert colours
  GColor battery_ring = GColorWhite;
  GColor inner_ring   = s_settings.min_hand_inner;
  GColor dot          = s_settings.hour_hand_outer;

  bool low_battery      = (s_battery_pct <= s_settings.battery_ring_threshold &&
                            s_settings.battery_ring_threshold > 0);
  bool critical_battery = (s_battery_pct <= s_settings.battery_center_threshold &&
                            s_settings.battery_center_threshold > 0);

  if (low_battery)      battery_ring = GColorRed;
  if (critical_battery) { battery_ring = GColorRed; inner_ring = GColorRed; dot = GColorRed; }

  // r5 outer ring
  graphics_context_set_fill_color(ctx, MONO_COLOR(battery_ring));
  graphics_fill_circle(ctx, center, POS_X(7));
  // r4 black gap
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, POS_X(5));
  // r3 inner ring
  graphics_context_set_fill_color(ctx, MONO_COLOR(inner_ring));
  graphics_fill_circle(ctx, center, POS_X(4));
  // r2 black gap
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, POS_X(2));
  // r1 centre dot
  graphics_context_set_fill_color(ctx, MONO_COLOR(dot));
  graphics_fill_circle(ctx, center, POS_X(1));
}

// Seconds hand layer — intentionally empty.
// The seconds hand is drawn inside minute_layer_update (before the centre rings)
// so it always appears below the centre ring/dot.
static void seconds_layer_update(Layer *layer, GContext *ctx) {
  (void)layer; (void)ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick subscription management
// ─────────────────────────────────────────────────────────────────────────────
static void update_tick_subscription(void) {
  bool need_seconds = false;
  if (s_settings.seconds_hand_mode == 1) {
    need_seconds = true;
    s_seconds_visible = true;
  } else if (s_settings.seconds_hand_mode == 2) {
    need_seconds = s_seconds_visible;
  } else {
    s_seconds_visible = false;
  }

  tick_timer_service_unsubscribe();
  if (need_seconds) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shake / icon display timers
// ─────────────────────────────────────────────────────────────────────────────
static void hide_icons_callback(void *data) {
  s_shake_timer = NULL;
  s_showing_icons = false;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_complication_layer);
}

static void show_icons_callback(void *data) {
  s_shake_delay_timer = NULL;
  s_showing_icons = true;
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_complication_layer);

  if (s_shake_timer) {
    app_timer_reschedule(s_shake_timer, SHAKE_DISPLAY_MS);
  } else {
    s_shake_timer = app_timer_register(SHAKE_DISPLAY_MS, hide_icons_callback, NULL);
  }
}

static void hide_seconds_callback(void *data) {
  s_seconds_timer = NULL;
  s_seconds_visible = false;
  update_tick_subscription();
  layer_mark_dirty(s_seconds_layer);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick handler
// ─────────────────────────────────────────────────────────────────────────────
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  bool hour_changed = (tick_time->tm_hour != s_last_time.tm_hour);
  s_last_time = *tick_time;

  if (units_changed & SECOND_UNIT) {
    layer_mark_dirty(s_minute_layer);  // seconds drawn inside minute layer
  }
  if (units_changed & MINUTE_UNIT) {
    layer_mark_dirty(s_minute_layer);
    layer_mark_dirty(s_hour_layer);
    layer_mark_dirty(s_complication_layer);
  }
  if (hour_changed) {
    layer_mark_dirty(s_bg_layer);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Accelerometer tap (shake) handler
// ─────────────────────────────────────────────────────────────────────────────
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // Seconds hand on shake
  if (s_settings.seconds_hand_mode == 2) {
    // Refresh s_last_time from the system clock so the first draw shows the
    // correct second position, not the stale value from the last minute tick.
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) s_last_time = *t;

    s_seconds_visible = true;
    update_tick_subscription();
    if (s_seconds_timer) {
      app_timer_reschedule(s_seconds_timer, s_settings.seconds_shake_dur * 1000);
    } else {
      s_seconds_timer = app_timer_register(
        s_settings.seconds_shake_dur * 1000, hide_seconds_callback, NULL);
    }
    layer_mark_dirty(s_minute_layer);  // seconds drawn inside minute layer
  }

  // Icons on shake (shake_mode == 0).
  // Every shake restarts the full show cycle: cancel any in-flight timers,
  // hide icons immediately, then start the 500ms delay before re-showing.
  // This eliminates any lockout window regardless of seconds-hand state.
  if (s_settings.shake_mode == 0) {
    // Cancel hide timer if running
    if (s_shake_timer) {
      app_timer_cancel(s_shake_timer);
      s_shake_timer = NULL;
    }
    // Hide icons immediately so the delay gives a clear "flash off then on" feel
    s_showing_icons = false;
    // Start (or restart) the 500ms delay before showing icons
    if (s_shake_delay_timer) {
      app_timer_reschedule(s_shake_delay_timer, SHAKE_DELAY_MS);
    } else {
      s_shake_delay_timer = app_timer_register(SHAKE_DELAY_MS, show_icons_callback, NULL);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Battery handler
// ─────────────────────────────────────────────────────────────────────────────
static void battery_handler(BatteryChargeState charge) {
  s_battery_pct = charge.charge_percent;
  layer_mark_dirty(s_minute_layer);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bluetooth handler
// ─────────────────────────────────────────────────────────────────────────────
static void bt_handler(bool connected) {
  bool was_connected = s_bt_connected;
  s_bt_connected = connected;

  if (!connected && was_connected && s_settings.vibrate_bt_disconnect) {
    vibes_long_pulse();
  }
  if (connected && !was_connected && s_settings.vibrate_bt_reconnect) {
    vibes_short_pulse();
  }

  layer_mark_dirty(s_minute_layer);
}

// ─────────────────────────────────────────────────────────────────────────────
// AppMessage handler
// ─────────────────────────────────────────────────────────────────────────────
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  bool weather_changed  = false;
  bool settings_changed = false;
  bool bg_dirty         = false;

  Tuple *t;

  // Icons
  for (int i = 0; i < 24; i++) {
    t = dict_find(iter, i);  // KEY_ICON_0 = 0 ... KEY_ICON_23 = 23
    if (t) {
      s_icons[i] = (int8_t)t->value->int32;
      weather_changed = true;
    }
  }

  // Temperature
  t = dict_find(iter, 58); // KEY_TEMP_C
  if (t) { s_temp_c = (int8_t)t->value->int32; weather_changed = true; }
  t = dict_find(iter, 59); // KEY_TEMP_F
  if (t) { s_temp_f = (int8_t)t->value->int32; weather_changed = true; }

  // Sunrise / sunset
  t = dict_find(iter, 25); if (t) { s_sunrise_hour = (int8_t)t->value->int32; weather_changed = true; }
  t = dict_find(iter, 26); if (t) { s_sunrise_min  = (int8_t)t->value->int32; weather_changed = true; }
  t = dict_find(iter, 27); if (t) { s_sunset_hour  = (int8_t)t->value->int32; weather_changed = true; }
  t = dict_find(iter, 28); if (t) { s_sunset_min   = (int8_t)t->value->int32; weather_changed = true; }

  if (weather_changed) {
    persist_write_data(PERSIST_ICONS, s_icons, sizeof(s_icons));
  }

  // Settings
  t = dict_find(iter, 40); // KEY_DISPLAY_HOUR_MARKERS
  if (t) { s_settings.display_hour_markers = (bool)t->value->int32; settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 41); // KEY_DISPLAY_MINOR_MARKERS
  if (t) { s_settings.display_minor_markers = (bool)t->value->int32; settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 53); // KEY_BT_DISCONNECT_MIN_INNER_RED
  if (t) { s_settings.bt_disconnect_min_inner_red = (bool)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 54); // KEY_VIBRATE_BT_DISCONNECT
  if (t) { s_settings.vibrate_bt_disconnect = (bool)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 55); // KEY_VIBRATE_BT_RECONNECT
  if (t) { s_settings.vibrate_bt_reconnect = (bool)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 107); // KEY_SHAKE_MODE
  if (t) { s_settings.shake_mode = (uint8_t)t->value->int32; settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 110); // KEY_TEMP_UNIT
  if (t) { s_settings.temp_unit = (uint8_t)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 113); // KEY_CUSTOM_LOCATION (string, handled by JS)
  // No C-side action needed

  t = dict_find(iter, 114); // KEY_HOUR_HAND_OUTER
  if (t) { s_settings.hour_hand_outer = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 115); // KEY_HOUR_HAND_INNER
  if (t) { s_settings.hour_hand_inner = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 116); // KEY_MIN_HAND_OUTER
  if (t) { s_settings.min_hand_outer = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 117); // KEY_MIN_HAND_INNER
  if (t) { s_settings.min_hand_inner = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 118); // KEY_DATE_VISIBLE
  if (t) { s_settings.date_visible = (uint8_t)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 119); // KEY_TEMP_VISIBLE
  if (t) { s_settings.temp_visible = (uint8_t)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 121); // KEY_NUMBER_FONT
  if (t) {
    uint8_t new_font = (uint8_t)t->value->int32;
    if (new_font != s_settings.number_font) {
      s_settings.number_font = new_font;
      s_cached_font_id = 255; // Invalidate cache
      bg_dirty = true;
    }
    settings_changed = true;
  }

  t = dict_find(iter, 126); // KEY_BACKGROUND_COLOR
  if (t) { s_settings.background_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 127); // KEY_NUMBER_COLOR
  if (t) { s_settings.number_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 128); // KEY_ICON_COLOR
  if (t) { s_settings.icon_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 129); // KEY_HOUR_MARKER_COLOR
  if (t) { s_settings.hour_marker_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 130); // KEY_MINUTE_MARKER_COLOR
  if (t) { s_settings.minute_marker_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 134); // KEY_DATE_COLOR
  if (t) { s_settings.date_color = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 135); // KEY_TEMP_COLOR
  if (t) { s_settings.temp_color = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 136); // KEY_BT_DISCONNECT_OUTER_COLOR
  if (t) { s_settings.bt_disconnect_outer_color = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 137); // KEY_BT_DISCONNECT_INNER_COLOR
  if (t) { s_settings.bt_disconnect_inner_color = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 138); // KEY_BATTERY_RING_THRESHOLD
  if (t) { s_settings.battery_ring_threshold = (uint8_t)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 139); // KEY_BATTERY_CENTER_THRESHOLD
  if (t) { s_settings.battery_center_threshold = (uint8_t)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 141); // KEY_SECONDS_HAND_COLOR
  if (t) { s_settings.seconds_hand_color = rgb_to_gcolor(t->value->int32); settings_changed = true; }

  t = dict_find(iter, 142); // KEY_SECONDS_HAND_MODE
  if (t) {
    s_settings.seconds_hand_mode = (uint8_t)t->value->int32;
    update_tick_subscription();
    settings_changed = true;
  }

  t = dict_find(iter, 143); // KEY_SECONDS_SHAKE_DUR
  if (t) { s_settings.seconds_shake_dur = (uint8_t)t->value->int32; settings_changed = true; }

  t = dict_find(iter, 144); // KEY_TEST_BATTERY_ALERT
  if (t && t->value->int32) {
    // Temporarily show battery alert
    uint8_t saved = s_battery_pct;
    s_battery_pct = 5;
    layer_mark_dirty(s_minute_layer);
    s_battery_pct = saved;
  }

  t = dict_find(iter, 145); // KEY_TEST_BT_DISCONNECT
  if (t && t->value->int32) {
    bool saved = s_bt_connected;
    s_bt_connected = false;
    layer_mark_dirty(s_minute_layer);
    s_bt_connected = saved;
  }

  t = dict_find(iter, 146); // KEY_TEST_CRITICAL_BATTERY_ALERT
  if (t && t->value->int32) {
    // Temporarily show critical battery alert (below center threshold)
    uint8_t saved = s_battery_pct;
    s_battery_pct = 1;
    layer_mark_dirty(s_minute_layer);
    s_battery_pct = saved;
  }

  t = dict_find(iter, 147); // KEY_SUNRISE_MARKER_VISIBLE
  if (t) { s_settings.sunrise_marker_visible = (uint8_t)t->value->int32; settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 148); // KEY_SUNRISE_MARKER_COLOR
  if (t) { s_settings.sunrise_marker_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 149); // KEY_SUNSET_MARKER_COLOR
  if (t) { s_settings.sunset_marker_color = rgb_to_gcolor(t->value->int32); settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 150); // KEY_NUMBER_SIZE
  if (t) {
    uint8_t new_size = (uint8_t)t->value->int32;
    if (new_size != s_settings.number_size) {
      s_settings.number_size = new_size;
      s_cached_font_size = 255; // Invalidate cache
      bg_dirty = true;
    }
    settings_changed = true;
  }

  t = dict_find(iter, 151); // KEY_ICON_SIZE
  if (t) { s_settings.icon_size = (uint8_t)t->value->int32; settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 158); // KEY_DISPLAY_MODE
  if (t) { s_settings.display_mode = (uint8_t)t->value->int32; settings_changed = true; }
  t = dict_find(iter, 159); // KEY_CITY_NAME (string)
  if (t && t->type == TUPLE_CSTRING) {
    strncpy(s_city_name, t->value->cstring, sizeof(s_city_name) - 1);
    s_city_name[sizeof(s_city_name) - 1] = '\0';
    persist_write_string(PERSIST_CITY, s_city_name);
  }
  t = dict_find(iter, 160); // KEY_CITY_DISPLAY_MODE
  if (t) { s_settings.city_display_mode = (uint8_t)t->value->int32; settings_changed = true; }
  t = dict_find(iter, 161); // KEY_CITY_COLOR
  if (t) { s_settings.city_color = rgb_to_gcolor(t->value->int32); settings_changed = true; }
  t = dict_find(iter, 153); // KEY_ICON_COLOR_MODE
  if (t) { s_settings.icon_color_mode = (uint8_t)t->value->int32; settings_changed = true; bg_dirty = true; }

  t = dict_find(iter, 162); // KEY_COMPLICATION_LAYER
  if (t) {
    uint8_t new_layer = (uint8_t)t->value->int32;
    if (new_layer != s_settings.complication_layer) {
      s_settings.complication_layer = new_layer;
      // Re-order layers: remove then re-insert at correct position
      layer_remove_from_parent(s_complication_layer);
      if (s_settings.complication_layer == 0) {
        // Behind hands: insert below hour layer (above bg)
        layer_insert_below_sibling(s_complication_layer, s_hour_layer);
      } else {
        // On top of hands: insert below seconds_layer (which is topmost)
        layer_insert_below_sibling(s_complication_layer, s_seconds_layer);
      }
    }
    settings_changed = true;
  }

  // Shake mode also controls accel subscription
  if (settings_changed) {
    persist_write_data(PERSIST_SETTINGS, &s_settings, sizeof(Settings));
    bool need_accel = (s_settings.shake_mode == 0 || s_settings.seconds_hand_mode == 2);
    if (need_accel) {
      accel_tap_service_subscribe(accel_tap_handler);
    } else {
      accel_tap_service_unsubscribe();
    }
  }

  if (bg_dirty || weather_changed) {
    layer_mark_dirty(s_bg_layer);
  }
  layer_mark_dirty(s_complication_layer);
  layer_mark_dirty(s_hour_layer);
  layer_mark_dirty(s_minute_layer);
  layer_mark_dirty(s_seconds_layer);
}

// ─────────────────────────────────────────────────────────────────────────────
// Default settings
// ─────────────────────────────────────────────────────────────────────────────
static void load_default_settings(void) {
  bool is_emery = (s_screen_w >= 200);

  s_settings.background_color       = GColorBlack;
  s_settings.display_hour_markers   = true;
  s_settings.hour_marker_color      = GColorWhite;
  s_settings.display_minor_markers  = true;
  s_settings.minute_marker_color    = GColorFromRGB(0x6b, 0x7f, 0x99);
  s_settings.number_font            = 0;   // Digital
  s_settings.number_size            = 3;
  s_settings.number_color           = GColorWhite;
  s_settings.icon_size              = 3;
  s_settings.icon_color             = GColorWhite;
  s_settings.hour_hand_outer        = GColorWhite;
  s_settings.hour_hand_inner        = GColorClear;
  s_settings.min_hand_outer         = GColorBlack;
  s_settings.min_hand_inner         = GColorFromRGB(0, 97, 254);
  s_settings.seconds_hand_color     = GColorWhite;
  s_settings.seconds_hand_mode      = 2;   // Shake to show
  s_settings.seconds_shake_dur      = 10;
  s_settings.date_visible           = 0;   // Always
  s_settings.temp_visible           = 0;   // Always
  s_settings.display_mode           = 0;   // Both
  s_settings.temp_unit              = 0;   // °C
  s_settings.date_color             = GColorFromRGB(0x4a, 0x5f, 0x7f);
  s_settings.temp_color             = GColorFromRGB(0x4a, 0x5f, 0x7f);
  s_settings.battery_ring_threshold   = is_emery ? 30 : 50;
  s_settings.battery_center_threshold = is_emery ? 10 : 20;
  s_settings.sunrise_marker_visible = 0;   // Always
  s_settings.sunrise_marker_color   = GColorFromRGB(0xff, 0x95, 0x00);
  s_settings.sunset_marker_color    = GColorFromRGB(0x00, 0x61, 0xfe);
  s_settings.vibrate_bt_disconnect  = true;
  s_settings.vibrate_bt_reconnect   = false;
  s_settings.bt_disconnect_min_inner_red = true;
  s_settings.bt_disconnect_outer_color   = GColorRed;
  s_settings.bt_disconnect_inner_color   = GColorRed;
  s_settings.shake_mode             = 0;   // Show on shake
  s_settings.city_display_mode      = 1;   // Shake
  s_settings.city_color             = GColorFromRGB(0x00, 0x00, 0xaa);
  s_settings.icon_color_mode        = 0;   // Single colour
  s_settings.complication_layer     = 0;   // Behind hands
}

// ─────────────────────────────────────────────────────────────────────────────
// Window load / unload
// ─────────────────────────────────────────────────────────────────────────────
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_screen_w = bounds.size.w;
  s_screen_h = bounds.size.h;

  // Load persisted settings or defaults
  if (persist_exists(PERSIST_SETTINGS)) {
    persist_read_data(PERSIST_SETTINGS, &s_settings, sizeof(Settings));
  } else {
    load_default_settings();
  }

  // Load persisted icons
  if (persist_exists(PERSIST_ICONS)) {
    persist_read_data(PERSIST_ICONS, s_icons, sizeof(s_icons));
  } else {
    memset(s_icons, ICON_UNKNOWN, sizeof(s_icons));
  }
  // Load persisted city name
  if (persist_exists(PERSIST_CITY)) {
    persist_read_string(PERSIST_CITY, s_city_name, sizeof(s_city_name));
  }

  // Compute marker caches
  compute_markers();

  // Create layers (bottom to top)
  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_layer_update);
  layer_add_child(root, s_bg_layer);

  s_hour_layer = layer_create(bounds);
  layer_set_update_proc(s_hour_layer, hour_layer_update);
  layer_add_child(root, s_hour_layer);

  s_minute_layer = layer_create(bounds);
  layer_set_update_proc(s_minute_layer, minute_layer_update);
  layer_add_child(root, s_minute_layer);

  s_complication_layer = layer_create(bounds);
  layer_set_update_proc(s_complication_layer, complication_layer_update);
  // Always add to root first (will be reordered below)
  layer_add_child(root, s_complication_layer);
  if (s_settings.complication_layer == 0) {
    // Behind hands: insert below hour layer
    layer_remove_from_parent(s_complication_layer);
    layer_insert_below_sibling(s_complication_layer, s_hour_layer);
  }
  // complication_layer == 1: already on top (added last before seconds)

  s_seconds_layer = layer_create(bounds);
  layer_set_update_proc(s_seconds_layer, seconds_layer_update);
  // seconds_layer is kept for timer/dirty tracking but draws nothing itself
  layer_add_child(root, s_seconds_layer);

  // Get initial time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  if (t) s_last_time = *t;

  // Initial seconds visibility
  s_seconds_visible = (s_settings.seconds_hand_mode == 1);

  // Subscribe to tick timer
  update_tick_subscription();

  // Subscribe to battery and BT
  BatteryChargeState bcs = battery_state_service_peek();
  s_battery_pct = bcs.charge_percent;
  battery_state_service_subscribe(battery_handler);

  s_bt_connected = connection_service_peek_pebble_app_connection();
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = bt_handler
  });

  // Subscribe to accel tap if needed
  bool need_accel = (s_settings.shake_mode == 0 || s_settings.seconds_hand_mode == 2);
  if (need_accel) {
    accel_tap_service_subscribe(accel_tap_handler);
  }

  // Open AppMessage
  app_message_open(APP_MSG_INBOX_SIZE, APP_MSG_OUTBOX_SIZE);
  app_message_register_inbox_received(inbox_received_handler);
}

static void window_unload(Window *window) {
  // Unload font
  if (s_cached_number_font) {
    fonts_unload_custom_font(s_cached_number_font);
    s_cached_number_font = NULL;
  }

  // Cancel timers
  if (s_shake_timer)       { app_timer_cancel(s_shake_timer);       s_shake_timer = NULL; }
  if (s_shake_delay_timer) { app_timer_cancel(s_shake_delay_timer); s_shake_delay_timer = NULL; }  // kept for safety
  if (s_seconds_timer)     { app_timer_cancel(s_seconds_timer);     s_seconds_timer = NULL; }

  // Unsubscribe
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  accel_tap_service_unsubscribe();

  // Destroy layers
  layer_destroy(s_seconds_layer);
  layer_destroy(s_minute_layer);
  layer_destroy(s_hour_layer);
  layer_destroy(s_complication_layer);
  layer_destroy(s_bg_layer);
}

// ─────────────────────────────────────────────────────────────────────────────
// App init / deinit
// ─────────────────────────────────────────────────────────────────────────────
static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

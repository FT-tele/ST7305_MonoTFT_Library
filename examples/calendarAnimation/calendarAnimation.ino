/*
 * calendarAnimation.ino
 * Left: 30-frame weather animation (storm → light rain → clouds → ROC sun)
 * Right: January 2025 calendar, rotated 90° clockwise (no overlap with animation),
 *        Jan 1 highlighted, "Happy New Year!" banner.
 *
 * Matches your frames.h:
 *   - FRAME0..FRAME29
 *   - #define NUM_FRAMES 30
 *   - typedef int (*putpixel_fn)(int,int,int);
 *   - void drawPic(const uint8_t*, int, int, putpixel_fn);
 *
 * Pins (per your request): DC=3, RST=6, CS=7, SCLK=2, SDIN=10
 */

#include <Arduino.h>
#include <SPI.h>
#include <ST73XX_UI.h>
#include <ST7305_2p9_BW_DisplayDriver.h>
#include <U8g2_for_ST73XX.h>
#include "DisplayConfig.h"     // DISPLAY_WIDTH / DISPLAY_HEIGHT
#include "frames.h"            // FRAME0..FRAME29, NUM_FRAMES, drawPic, putpixel_fn

// ---------- Pin definitions ----------
#define PIN_DC    3
#define PIN_RST   6
#define PIN_CS    7
#define PIN_SCLK  2
#define PIN_SDIN  10

// ---------- Globals ----------
ST7305_2p9_BW_DisplayDriver display(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN, SPI);
U8G2_FOR_ST73XX u8g2;

// Animation layout in our virtual **landscape** space
static const int ANIM_X = 10;     // left margin for animation in landscape coords
static const int ANIM_Y = 16;     // top margin
static const int ANIM_W = 120;    // frame width
static const int ANIM_H = 120;    // frame height

// Calendar panel bounds in **landscape** space (to the RIGHT of animation)
static const int CAL_X_MIN = ANIM_X + ANIM_W + 16;          // ensures no overlap
static const int CAL_X_MAX = (int)DISPLAY_HEIGHT - 4;       // landscape X spans 0..DISPLAY_HEIGHT-1
static const int CAL_Y_MIN = 6;
static const int CAL_Y_MAX = 162;

// ---------- Mapping helpers ----------
//
// Base mapping from our virtual **landscape** (x_land,y_land)
// to device portrait coordinates (x_dev,y_dev) is a 90° CLOCKWISE rotation:
//   x_dev = y_land
//   y_dev = DISPLAY_HEIGHT - 1 - x_land
//
static inline bool mapR90_land2dev(int x_land, int y_land, int* x_dev, int* y_dev) {
  int xd = y_land;
  int yd = (int)DISPLAY_HEIGHT - 1 - x_land;
  if (xd < 0 || xd >= (int)DISPLAY_WIDTH || yd < 0 || yd >= (int)DISPLAY_HEIGHT) return false;
  *x_dev = xd; *y_dev = yd;
  return true;
}

// ---------- Pixel setter for frames (matches putpixel_fn: int(int,int,int)) ----------
static int putpixel_landscape(int x_land, int y_land, int on) {
  int x_dev, y_dev;
  if (!mapR90_land2dev(x_land, y_land, &x_dev, &y_dev)) return 0;
  // Use the bool overload to avoid ambiguity
  display.writePoint((uint)x_dev, (uint)y_dev, (bool)(on != 0));
  return 0;
}

// ---------- Dotted lines & frame in **device** coords (force uint16_t overload) ----------
static void drawDottedH(int x0, int x1, int y) {
  if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
  for (int x = x0; x <= x1; ++x) {
    if ((x & 1) == 0) display.writePoint((uint)x, (uint)y, (uint16_t)ST7305_COLOR_BLACK);
  }
}
static void drawDottedV(int x, int y0, int y1) {
  if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
  for (int y = y0; y <= y1; ++y) {
    if ((y & 1) == 0) display.writePoint((uint)x, (uint)y, (uint16_t)ST7305_COLOR_BLACK);
  }
}
static void drawFrameDevice(int x0, int y0, int x1, int y1) {
  drawDottedH(x0, x1, y0);
  drawDottedH(x0, x1, y1);
  drawDottedV(x0, y0, y1);
  drawDottedV(x1, y0, y1);
}

// Convert a LANDSCAPE rectangle to DEVICE coords and draw a dotted frame
static void drawFrameLandscapeRect(int x0_land, int y0_land, int x1_land, int y1_land) {
  int a0x,a0y,a1x,a1y, b0x,b0y,b1x,b1y;
  mapR90_land2dev(x0_land, y0_land, &a0x, &a0y);
  mapR90_land2dev(x1_land, y0_land, &a1x, &a1y);
  mapR90_land2dev(x0_land, y1_land, &b0x, &b0y);
  mapR90_land2dev(x1_land, y1_land, &b1x, &b1y);
  // rectangle corners in device coords:
  int dx0 = min(a0x, b0x), dx1 = max(a1x, b1x);
  int dy0 = min(a0y, a1y), dy1 = max(b0y, b1y);
  drawFrameDevice(dx0, dy0, dx1, dy1);
}

// ---------- Calendar (Right Panel, rotated 90° CW via mapping) ----------
 // ---------- Calendar (Right Panel, 90° CW), weekdays above dates ----------
static void drawCalendarPanel() {
  // Use as much area as possible within CAL_* bounds to the RIGHT of animation
  const int rows = 6, cols = 7;

  // Vertical layout (in LANDSCAPE space) for the calendar area
  const int headerH = 22;   // "2025-01"
  const int weekH   = 16;   // "Mon Tue ... Sun"
  const int padY    = 2;    // small breathing space between sections

  const int usableW = CAL_X_MAX - CAL_X_MIN;
  const int usableH = CAL_Y_MAX - CAL_Y_MIN;

  const int yHeaderBase = CAL_Y_MIN + headerH;                 // baseline for header
  const int yWeekBase   = yHeaderBase + padY + weekH;          // baseline for weekdays
  const int gridY0      = yWeekBase   + padY;                  // top of date grid
  const int gridH       = (CAL_Y_MAX - gridY0);
  const int cellW       = max(1, usableW / cols);
  const int cellH       = max(1, gridH  / rows);

  // ---- Header: "2025-01" (clockwise rotated via mapping) ----
  {
    int xd, yd; mapR90_land2dev(CAL_X_MIN + 2, yHeaderBase - 6, &xd, &yd);
    u8g2.setFontDirection(3);               // your requested direction for header
    u8g2.setFont(u8g2_font_helvR18_tf);
    u8g2.setCursor(xd, yd);
    u8g2.print("From rain to sun");
  }

  // ---- Banner: Happy New Year! (smaller, right under header) ----
  {
    int xd, yd; mapR90_land2dev(CAL_X_MIN + 2, yHeaderBase + 10, &xd, &yd);
    u8g2.setFontDirection(3);               // render upright on the rotated mapping
    u8g2.setFont(u8g2_font_7x13B_mf);
    u8g2.setCursor(150, 380);
    u8g2.print("Happy New Year!😊❤️");
	
  }

  // ---- Weekday labels (Mon..Sun) ABOVE the date numbers ----
  static const char* WK[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  for (int c = 0; c < cols; ++c) {
    int xd, yd; mapR90_land2dev(CAL_X_MIN + c * cellW + 3, yWeekBase - 2, &xd, &yd);
    // Keep weekdays compact to avoid overlap with dates below
    u8g2.setFont((c >= 5) ? u8g2_font_7x13B_mf : u8g2_font_7x13_mf);
    u8g2.setCursor(xd, yd);
    u8g2.print(WK[c]);
  }

  // ---- Dotted grid (lightweight look on mono TFT) ----
  for (int c = 0; c <= cols; ++c) {
    int x_land = CAL_X_MIN + c * cellW;
    int y0_land = gridY0, y1_land = gridY0 + rows * cellH;
    int x0d, y0d, x1d, y1d;
    mapR90_land2dev(x_land, y0_land, &x0d, &y0d);
    mapR90_land2dev(x_land, y1_land, &x1d, &y1d);
    drawDottedV(x0d, y0d, y1d);
  }
  for (int r = 0; r <= rows; ++r) {
    int y_land = gridY0 + r * cellH;
    int x0_land = CAL_X_MIN, x1_land = CAL_X_MIN + cols * cellW;
    int x0d, y0d, x1d, y1d;
    mapR90_land2dev(x0_land, y_land, &x0d, &y0d);
    mapR90_land2dev(x1_land, y_land, &x1d, &y1d);
    drawDottedH(x0d, x1d, y0d);
  }

  // ---- January 2025 days (Mon=0 → Jan 1 is Wed → firstCol=2) ----
  const int firstCol = 2;
  const int daysInMonth = 31;
  int day = 1;

  for (int r = 0; r < rows && day <= daysInMonth; ++r) {
    for (int c = 0; c < cols && day <= daysInMonth; ++c) {
      if (r == 0 && c < firstCol) continue;

      const int x_cell = CAL_X_MIN + c * cellW;
      const int y_cell = gridY0 + r * cellH;

      // Highlight Jan 1 with a thin dotted frame
      if (day == 1) {
        drawFrameLandscapeRect(x_cell + 1, y_cell + 2,
                               x_cell + cellW - 2, y_cell + cellH - 2);
      }

      // Place the day number a little below the weekday row baseline
      int xd, yd; mapR90_land2dev(x_cell + 4, y_cell + min(16, cellH - 2), &xd, &yd);
      // Use normal font on weekdays, bold on weekends
      u8g2.setFont((c >= 5) ? u8g2_font_7x13B_mf : u8g2_font_7x13_mf);
      u8g2.setCursor(xd, yd);
      char buf[4]; snprintf(buf, sizeof(buf), "%d", day);
      u8g2.print(buf);

      ++day;
    }
  }

  // ---- Today label at the bottom inside the calendar area ----
  {
    int xd, yd; mapR90_land2dev(CAL_X_MIN + 2, CAL_Y_MAX - 4, &xd, &yd);
    u8g2.setFont(u8g2_font_7x13B_mf);
    u8g2.setCursor(xd, yd);
    u8g2.print("Today: 2025-01-01");
  }
}

// ---------- Frame list ----------
static const uint8_t* FRAMES[NUM_FRAMES] = {
  // 0..7: storm → light rain
  FRAME0, FRAME1, FRAME2, FRAME3, FRAME4, FRAME5, FRAME6, FRAME7,
  // 8..15: clouds
  FRAME8, FRAME9, FRAME10, FRAME11, FRAME12, FRAME13, FRAME14, FRAME15,
  // 16..21: transition
  FRAME16, FRAME17, FRAME18, FRAME19, FRAME20, FRAME21,
  // 22..29: ROC sun
  FRAME22, FRAME23, FRAME24, FRAME25, FRAME26, FRAME27, FRAME28, FRAME29
};

// Stage ranges for natural pacing
static const int STORM_START = 0,        STORM_END = 7;
static const int CLOUD_START = 8,        CLOUD_END = 15;
static const int TRANSITION_START = 16,  TRANSITION_END = 21;
static const int SUN_START = 22,         SUN_END = 29;

// Frame dwell time (ms) by stage
static uint16_t frameDelayFor(int idx) {
  if (idx >= STORM_START && idx <= STORM_END)           return 90;   // rapid storm
  if (idx >= CLOUD_START && idx <= CLOUD_END)           return 120;  // gentle
  if (idx >= TRANSITION_START && idx <= TRANSITION_END) return 140;  // easing
  /* SUN */                                              return 200;  // calm
}

// Subtle horizontal drift for clouds/sun
static int driftFor(int idx) {
  if (idx >= CLOUD_START && idx <= CLOUD_END)           return (idx & 3) - 1;   // -1..+2
  if (idx >= TRANSITION_START && idx <= TRANSITION_END) return (idx & 1);       // 0..1
  if (idx >= SUN_START && idx <= SUN_END)               return ((idx % 4) == 0) ? 1 : 0;
  return 0; // storm stable
}

// ---------- Arduino setup/loop ----------
void setup() {
  Serial.begin(115200);
  SPI.begin(PIN_SCLK, -1, PIN_SDIN, PIN_CS);

  display.initialize();
  display.High_Power_Mode();
  display.display_on(true);
  display.display_Inversion(false);

  u8g2.begin(display);
  u8g2.setFontMode(1);
  u8g2.setForegroundColor(ST7305_COLOR_BLACK);
  u8g2.setBackgroundColor(ST7305_COLOR_WHITE);

  display.clearDisplay();
  display.display();
}

void loop() {
  static int frame_idx = 0;

  // Clear frame
  display.clearDisplay();

  // ---- Draw animation on the LEFT (no overlap with calendar) ----
  int dx = driftFor(frame_idx);
  drawPic(FRAMES[frame_idx], ANIM_X + dx, ANIM_Y, putpixel_landscape);

  // ---- Draw calendar on the RIGHT, rotated 90° CW via mapping ----
  drawCalendarPanel();

  // Present to panel
  display.display();

  // Natural pacing
  delay(frameDelayFor(frame_idx));

  // Advance
  frame_idx = (frame_idx + 1) % NUM_FRAMES;
}

#ifndef _ADAFRUIT_GFX_AS_H
#define _ADAFRUIT_GFX_AS_H

#include "Load_fonts.h"
#include "esp_types.h"

class Adafruit_GFX_AS {
 public:
  Adafruit_GFX_AS(int16_t w, int16_t h); // Constructor

  // This MUST be defined by the subclass:
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;

  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void
    drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color),
    drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
    drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
    drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
    fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
    fillScreen(uint16_t color),
    invertDisplay(bool i);

  // These exist only with Adafruit_GFX_AS (no subclass overrides)
  void
    drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
    drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
      uint16_t color),
    fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
    fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
      int16_t delta, uint16_t color),
    drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color),
    fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
      int16_t x2, int16_t y2, uint16_t color),
    drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color),
    fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
      int16_t radius, uint16_t color),
    drawBitmap(int16_t x, int16_t y, const uint16_t *bitmap,
      int16_t w, int16_t h),
    setTextColor(uint16_t c),
    setTextColor(uint16_t c, uint16_t bg),
    setTextWrap(bool w),
    setRotation(uint8_t r);

    int drawUnicode(uint16_t uniCode, uint16_t x, uint16_t y, uint8_t size);
    int drawNumber(long long_num,uint16_t poX, uint16_t poY, uint8_t size);
    int drawChar(char c, uint16_t x, uint16_t y, uint8_t size);
    int drawString(const char *string, uint16_t poX, uint16_t poY, uint8_t size);
    int drawCentreString(const char *string, uint16_t dX, uint16_t poY, uint8_t size);
    int drawRightString(const char *string, uint16_t dX, uint16_t poY, uint8_t size);
    int drawFloat(float floatNumber,uint8_t decimal,uint16_t poX, uint16_t poY, uint8_t size);

  int16_t
    height(void),
    width(void);

  uint8_t getRotation(void);

 protected:
  const int16_t
    WIDTH, HEIGHT;   // This is the 'raw' display w/h - never changes
  int16_t
    _width, _height; // Display w/h as modified by current rotation
  uint16_t
    textcolor, textbgcolor;
  uint8_t
    rotation;
  bool
    wrap; // If set, 'wrap' text at right edge of display
};

#endif // _ADAFRUIT_GFX_AS_H

#pragma once

// General graphics utilities

typedef word tColour;

// If using monochrome, main screen is black and white, input area etc are 1-byte colours
#ifdef PDA_MONOCHROME
#define SCREEN_FG_COLOUR      RGB(0, 0, 0)
#define SCREEN_BG_COLOUR      RGB(255, 255, 255)
#define INPUT_AREA_COLOUR     0x0F  // 1 byte colours
#define GAP_COLOUR            0x0D
#else // full colour (slower)
#define SCREEN_FG_COLOUR      RGB(0, 0, 0)
#define SCREEN_BG_COLOUR      RGB(123, 140, 90)
#define INPUT_AREA_COLOUR     RGB(110, 141, 135)
#define GAP_COLOUR            RGB(120, 151, 145)
#endif

// Overall scale factor, 1 PDA pixel is 2x2 on the LCD
#define LCD_SCALE             2
#define PDA2LCD(_q)           (LCD_SCALE*(_q))
#define LCD2PDA(_q)           ((_q)/LCD_SCALE)

// ALL coordinates are LCD
namespace Graphics
{
  // Draw the 1-bit-per-pixel bitmap pData_P (which is PROGMEM) at x, y. The bitmap is w x h. Apply the given scale factor and use the foreground colour for 1's and background for 0's
  void Draw1BPP(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* pData_P, uint16_t scale, tColour fg, tColour bg);
  // Draw input details like "abc" and "123". 0-3 are corners; 4 & 5 are ticks, 6 & 7 are labels
  void DrawInputDetailElement(uint16_t x, uint16_t y, int idx);
  // Fill a solid rectangle. If roundScale non-0 adds corner details, alternate filled corners if roundScale is NEGATIVE
  void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tColour fg, tColour bg = SCREEN_BG_COLOUR, int8_t roundScale = 0);
  // Draws a hz or vt line of the thickness, optionally alternating (dotted)
  void DrawLine(uint16_t x, uint16_t y, uint16_t l, bool hz, tColour fg, tColour bg, bool alternate = false, uint16_t thickness = 1);
  // Draw a form frame, params as above.  cornerBitset controls corner details
  void DrawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tColour fg, tColour bg, bool alternate = false, uint16_t thickness = 1, int8_t roundScale = 0, uint8_t cornerBitset = 0x0F);
};

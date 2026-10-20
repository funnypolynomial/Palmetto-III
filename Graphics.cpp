#include <Arduino.h>
#include "PDA.h"

namespace Graphics
{

  // Small 1BPP details, created by hand
  // outline (button) and filled rounded corners, 3x3. top-L, top-R, bottom-L, bottom-R
  const uint16_t kRoundedCornerImagesSize = 3;
  static const uint8_t pRoundedCornerImages_P[] PROGMEM =
  {
    // button
    0b00000000,
    0b01100000,
    0b01000000,

    0b00000000,
    0b11000000,
    0b01000000,

    0b01000000,
    0b01100000,
    0b00000000,

    0b01000000,
    0b11000000,
    0b00000000,

    // button filled
    0b00000000,
    0b01100000,
    0b01100000,

    0b00000000,
    0b11000000,
    0b11000000,

    0b01100000,
    0b01100000,
    0b00000000,

    0b11000000,
    0b11000000,
    0b00000000,

    // double line filled
    0b00100000,
    0b01100000,
    0b11100000,

    0b10000000,
    0b11000000,
    0b11100000,

    0b11100000,
    0b01100000,
    0b00100000,

    0b11100000,
    0b11000000,
    0b10000000,
  };

  void Draw1BPP(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* pData_P, uint16_t scale, tColour fg, tColour bg)
  {
    // Draw the 1BPP data at pData as w x h pixels, scaled by size, in fg/bg.
    LCD_BEGIN_FILL(x, y, scale*w, scale*h);
    for (uint16_t row = 0; row < h; row++)
    {
      const uint8_t* pRow_P = pData_P;
      for (uint16_t s = 0; s < scale; s++)
      {
        uint8_t Mask = 0x80;
        pRow_P = pData_P;
        uint8_t Byte = pgm_read_byte(pRow_P++);
        for (uint16_t col = 0; col < w; col++)
        {
          PDA_FILL_COLOUR(scale, (Byte & Mask) ? fg : bg);
          Mask >>= 1;
          if (!Mask && (col < (w - 1))) // Get next byte in row
          {
            Mask = 0x80;
            Byte = pgm_read_byte(pRow_P++);
          }
        }
      }
      pData_P = pRow_P;
    }
  }

  void AddCorners(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tColour fg, tColour bg, uint8_t scale, const uint8_t* pCorners_P, uint8_t cornerBitset)
  {
    // cornerBitset is 0b0000ABCD where corners are:
    //   AB
    //   CD
    const uint8_t* pData = pCorners_P;
    const uint16_t kSize = kRoundedCornerImagesSize;
    if (cornerBitset & 0b00001000)
      Draw1BPP(x, y, kSize, kSize, pData, scale, fg, bg); // t-l
    pData += kSize;
    if (cornerBitset & 0b00000100)
      Draw1BPP(x + w - scale*kSize, y, kSize, kSize, pData, scale, fg, bg);  // t-r
    pData += kSize;
    if (cornerBitset & 0b00000010)
      Draw1BPP(x, y + h - scale*kSize, kSize, kSize, pData, scale, fg, bg);  // b-l
    pData += kSize;
    if (cornerBitset & 0b00000001)
      Draw1BPP(x + w - scale*kSize, y + h - scale*kSize, kSize, kSize, pData, scale, fg, bg); // b-r
  }

  void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tColour fg, tColour bg, int8_t roundScale)
  {
    // Filled rect, rounded corners optional
    // if roundScale non-0 adds corner details, alternate filled corners if roundScale is NEGATIVE :-)
    PDA_FILL_RECT(x, y, w, h, fg);
    if (roundScale != 0)
      AddCorners(x, y, w, h, fg, bg, abs(roundScale), (roundScale > 0)?pRoundedCornerImages_P + (4*kRoundedCornerImagesSize) : pRoundedCornerImages_P, 0b00001111);
  }

  void DrawLine(uint16_t x, uint16_t y, uint16_t len, bool hz, tColour fg, tColour bg, bool alternate, uint16_t thickness)
  {
    // Draw line, length l.if alternate, dotted
    PDA_FILL_RECT(x, y, hz ? len : thickness, hz ? thickness : len, fg);
    if (alternate)
    {
      // Choosing code size over speed, probably
      while (len)
      {
        if (!alternate)
          PDA_FILL_RECT(x, y, thickness, thickness, bg);
        if (hz)
          x += thickness;
        else
          y += thickness;
        alternate = !alternate;
        len -= thickness;
      }
    }
  }

  void DrawFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tColour fg, tColour bg, bool alternate, uint16_t t, int8_t roundScale, uint8_t cornerBitset)
  {
    // Draw rectangle outline
    // if roundScale non-0 adds corner details, FILLED corners if roundScale is NEGATIVE :-)
    DrawLine(x,         y,          w, true,  fg, bg, alternate, t); // top
    DrawLine(x,         y,          h, false, fg, bg, alternate, t); // left
    DrawLine(x,         y + h - t,  w, true,  fg, bg, alternate, t); // bot
    DrawLine(x + w - t, y,          h, false, fg, bg, alternate, t); // right
    if (roundScale != 0)
      AddCorners(x, y, w, h, fg, bg, abs(roundScale), (roundScale > 0)? pRoundedCornerImages_P:pRoundedCornerImages_P + (8*kRoundedCornerImagesSize), cornerBitset);
  }

}

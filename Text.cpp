#include "FontData.h"
#include "PDA.h"
#include "Text.h"

namespace Text
{
  // The fonts represent character definitions in fixed-size 1BPP blocks.
  // But, they are rendered proportionally, so we find the actual width,
  // the last column in the data, see CharWidth()
  struct tFontData
  {
    // Numbers are in PDA units
    const uint8_t*  _pFont_P = NULL;  // Primary font table, PROGMEM array of 1-BPP character data
    const char*     _pMap_P = NULL;   // Primary font map, PROGMEM string, chars in map are present in font table, index in map is position in table
    uint16_t        _width = 0;       // Nominal width of chars
    uint16_t        _height = 0;      // Nominal height of chars (including descender)
    // uint16_t        _descender = 0;   // Descender height *NOT USED*
    const uint8_t*  _pFont2_P = NULL; // Secondary font data...
    const char*     _pMap2_P = NULL;
    uint16_t        _width2 = 0;
  };

  const char pAMPMStr_P[] PROGMEM = MSTR("am") MSTR("pm");
  const char pTimeFormat_P[] PROGMEM = PREF_TIME_FORMAT;
  const char pDateFormat_P[] PROGMEM = PREF_DATE_FORMAT;
  const char pMonths_P[] PROGMEM = "JanFebMarAprMayJunJulAugSepOctNovDec";

  uint16_t CharWidth(char ch, const uint8_t* pFont_P, uint16_t width, uint16_t height, tFontID font)
  {
    // returns the actual width of the char, in pixel columns
    if (width > 16)
      return width; // not handled
    // Kerning is a black box, I'm doing it rule-based, but it's only an approximation, good enough. See KerningPair()
    if ('0' <= ch && ch <= '9')
    {
      // Make digits monospaced. Makes the CountDown screen ALMOST perfect
      if (font == LEDFont)
        return width;
      if (font == StandardFont)
        return width/2;
    }
    uint16_t combinedBits = 0; // build combined rows, max 16 bits wide
    for (uint16_t row = 0; row < height; row++)
    {
      uint16_t rowBits = pgm_read_byte(pFont_P++);
      if (width > 8) // next byte
        rowBits = ((rowBits << 8) | pgm_read_byte(pFont_P++)) >> (16 - width);
      combinedBits |= rowBits;
    }

    // find the RH bit
    if (combinedBits)
    {
      uint16_t cols = 0;
      while (!(combinedBits & 1))
      {
        combinedBits >>= 1;
        cols++;
      }
      return width  - cols;
    }
    else
      return 0;
  }

  bool KerningPair(char prevCh, char thisCh, tFontID font)
  {
    // Returns true if prevCh followed by thisCh should have thisChar 1 pixel closer to prevCh
    // This is a BUNCH of AD-HOC rules! Initially I was aiming for a pixel-perfect match with the
    // Palm but I've got over that. The kerning logic is opaque to me. Possibly a table of char vs char
    // Possibly based on character classification like: "bowl on the left" : acdegoq etc
    // In any case, limited to only these combinations.
    // Can be turned off entirely by defining PREF_NO_KERN, saves 150 bytes

    return
      ((prevCh == 'v' && thisCh == 'e') && font == StandardFont) // "Event"
      ||
      (prevCh == 'f' && thisCh == 's') //  "Prefs"
      ||
      (prevCh == 'f' && thisCh == 't') //  "Software"
      ||
      (prevCh == 'k' && thisCh == ' ') //  "Mark Wilson"
      ||
      (prevCh == ' ' && thisCh == '\xA9') //  "Copyright (c)"
      ||
      (prevCh == '\xA9' && thisCh == ' ') //  "(c) Mark"
      ||
      ((prevCh == ' ' && ('0' <= thisCh && thisCh <= '9')) && font == StandardFont) //  "Version 1"
      ;
  }

  int BytesPerChar(uint16_t height, uint16_t width)
  {
    // Returns the number of whole bytes in a char table entry
    return height*((width > 8) ? 2 : 1);
  }

  int16_t FindInMap(const char* pMap, unsigned char findCh)
  {
    // Like strchr, search for findCh in pMap string, returns effective offset into pMap (or -1)
    // But. Chars in pMap are ascending. Except that a *range* like "...ABCD..WXYZ..." can be
    // escaped (abbreviated) by having a *pair* that are descending: "...ZA..." 
    if (pMap)
    {
      int16_t idx = 0;
      uint8_t thisCh, prevCh = pgm_read_byte(pMap);
      while ((thisCh = pgm_read_byte(pMap++)) && findCh > thisCh)
      {
        if (thisCh < prevCh)
          idx += prevCh - thisCh;
        else
          idx++;
        prevCh = thisCh;
      }
      if (thisCh)
      {
        int16_t delta = (findCh - pgm_read_byte(pMap));
        if (0 <= delta && delta <= prevCh)
          return idx + delta;
        else if (findCh == thisCh)
          return idx;
      }
    }
    return -1;
  }

  uint16_t DrawFont(uint16_t x, uint16_t y, 
                    const char* str, 
                    tFontID font,
                    tFontData& data,
                    tColour fore, tColour back)
  {
    // general draw text with font table and alt table
    // does not draw if x == kNoDrawX
    // returns x of last pixel
    char prevCh = 0;
    bool draw = x != kNoDrawX;
    uint8_t scale = font == GraffitiFont_Unscaled ? 1 : LCD_SCALE;
    uint16_t clipH = data._height;
    uint16_t clipRow = (PDA2LCD(FORM_NORMAL_SIZE) - y)/scale;
    if (y < PDA2LCD(FORM_NORMAL_SIZE) && clipRow < clipH) // prevent controls close to the bottom from bleeding down. 
      clipH = clipRow;
    while (*str)
    {
      unsigned char thisCh = *str;
#ifdef PREF_NO_KERN
      (void)prevCh; // unused
#else
      if (KerningPair(prevCh, thisCh, font))
        x -= scale;
#endif
      int16_t idx = FindInMap(data._pMap_P, thisCh);
      const uint8_t* pData = data._pFont_P;
      uint16_t w = 0;
      if (idx >= 0)
      {
        pData += BytesPerChar(data._height, data._width)*idx;
        w = CharWidth(thisCh, pData, data._width, data._height, font);
        if (draw)
          Graphics::Draw1BPP(x, y, w, clipH, pData, scale, fore, back);
      }
      else if (data._pFont2_P)
      {
        idx = FindInMap(data._pMap2_P, thisCh);
        if (idx >= 0)
        {
          pData = data._pFont2_P + BytesPerChar(data._height, data._width2)*idx;
          w = CharWidth(thisCh, pData, data._width2, data._height, font);
          if (draw)
            Graphics::Draw1BPP(x, y, w, clipH, pData, scale, fore, back);
        }
      }

      if (w)
      {
        // gap after char
        x += scale*w;
        if (draw)
          PDA_FILL_RECT(x, y, scale, clipH*scale, back);
        x += scale;
      }
      else
      {
        // Space is half the nominal width?  Or kerning -- see BoldFont below? Good enough!
        // Also unknown chars
        uint16_t span = scale*(max(data._width, data._width2)/2 - 1);
        if (font == BoldFont)
          span = scale*2;
        if (draw)
          PDA_FILL_RECT(x, y, span, clipH*scale, back);
        x += span;
      }
      prevCh = *(str++);
    }
    return x;
  }

  void GetFontData(tFontID font, tFontData& data)
  {
    // Fill in the font data
    uint32_t props = 0, props2 = 0;
    switch (font)
    {
      case StandardFont:
      {
        data._pFont_P =   FontData::pStandardFont_P;
        data._pMap_P =    FontData::pStandardMap_P;
        props =           FontData::kStandardProps;
        break;
      }
      case LargeFont:
      {
        data._pFont_P =   FontData::pLargeFont_P;
        data._pMap_P =    FontData::pLargeMap_P;
        props =           FontData::kLargeProps;
        data._pFont2_P =  FontData::pLargeWideFont_P;
        data._pMap2_P =   FontData::pLargeWideMap_P;
        props2 =          FontData::kLargeWideProps;
        break;
      }
      case BoldFont:
      {
        data._pFont_P =   FontData::pBoldFont_P;
        data._pMap_P =    FontData::pBoldMap_P;
        props =           FontData::kBoldProps;
        data._pFont2_P =  FontData::pBoldWideFont_P;
        data._pMap2_P =   FontData::pBoldWideMap_P;
        props2 =          FontData::kBoldWideProps;
        break;
      }
      case LEDFont:
      {
        data._pFont_P =   FontData::pLEDFont_P;
        data._pMap_P =    FontData::pLEDMap_P;
        props =           FontData::kLEDProps;
        break;
      }
      case LargeBoldFont:
      {
        data._pFont_P =   FontData::pLargeBoldFont_P;
        data._pMap_P =    FontData::pLargeBoldMap_P;
        props =           FontData::kLargeBoldProps;
        break;
      }
      case GraffitiFont_Unscaled:
      {
        data._pFont_P =   FontData::pGraffitiFont_P;
        data._pMap_P =    FontData::pGraffitiMap_P;
        props =           FontData::kGraffitiProps;
        data._pFont2_P =  FontData::pGraffitiThinFont_P;
        data._pMap2_P =   FontData::pGraffitiThinMap_P;
        props2 =          FontData::kGraffitiThinProps;
        break;
      }
      default:
        break;
    }
    data._width = props & 0xFF;
    data._height = (props >> 8)  & 0xFF;
    // data._descender = (props >> 16)  & 0xFF; *NOT USED*
    data._width2 = props2 & 0xFF;
  }


  uint16_t Draw(tFontID font, uint16_t x, uint16_t y, const char* str, tColour fore, tColour back)
  {
    // Draw text at PDA coords using font
    // Does not draw if x == kNoDrawX
    // Returns x of last pixel

    tFontData fontData;
    GetFontData(font, fontData);
    return DrawFont(x, y, str, font, fontData, fore, back);
  }

  uint16_t FontHeight(tFontID font)
  {
    // Returns the font height
    tFontData fontData;
    GetFontData(font, fontData);
    return fontData._height;
  }

  uint16_t StringWidth(tFontID font, const char* str)
  {
    // String width in LCD pixels
    return Draw(font, kNoDrawX, 0, str, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR) - kNoDrawX;
  }

  void FormatNN(int value, char* pBuff)
  {
    // Format 2-digit POSITIVE number, with leading 0
    *pBuff++ = (value / 10) + '0';
    *pBuff++ = (value % 10) + '0';
    *pBuff = 0;
  }

  const char* FormatTime(uint8_t hour24, uint8_t minute, char* pBuff)
  {
    // Format has 'h'=12-hour, 'H'=24-hour, 'm'=minutes, 'x'=am/pm, 'X'=AM/PM. Other chars are copied
    const char* pFormat_P = pTimeFormat_P;
    char ch = pgm_read_byte(pFormat_P++);
    uint8_t Hour12 = hour24;
    char buff[8];
    bool PM = hour24 >= 12;
    if (Hour12 > 12)
      Hour12 -= 12;
    if (!Hour12)
      Hour12 = 12;
    *pBuff = 0;
    while (ch)
    {
      if (ch == 'h') // H
        ::itoa(Hour12, buff, 10);
      else if (ch == 'H') // HH
        FormatNN(hour24, buff);
      else if (ch == 'm') // MM
        FormatNN(minute, buff);
      else if (ch == 'x' || ch == 'X')  // am/pm/AM/PM
        strcpy_P(buff, MSTR_StrN(pAMPMStr_P, PM ? 1 : 0));
      else // just copy char
      {
        buff[0] = ch;
        buff[1] = 0;
      }
      if (ch == 'X')
      {
        // make uppercase
        buff[0] -= 32;
        buff[1] -= 32;
      }
      strcat(pBuff, buff);
      ch = pgm_read_byte(pFormat_P++);
    }
    return pBuff;
  }

  const char* FormatDate(uint16_t yearYYYY, uint8_t month, uint8_t date, char* pBuff)
  {
    // Format has 'y'=year mod 100, 'Y'=year, 'm'=month, 'M'=month leading 0, 'x'=Jan/Feb, 'X'=JAN/FEB, 'd'=date, 'D'=date leading 0. Other chars are copied
    const char* pFormat_P = pDateFormat_P;
    char ch = pgm_read_byte(pFormat_P++);
    char buff[8];
    *pBuff = 0;
    while (ch)
    {
      if (ch == 'y')  // YY
        FormatNN(yearYYYY % 100, buff);
      else if (ch == 'Y') // YYYY
        ::itoa(yearYYYY, buff, 10);
      else if (ch == 'm') // M
        ::itoa(month, buff, 10);
      else if (ch == 'M') // MM
        FormatNN(month, buff);
      else if (ch == 'X' || ch == 'x') // MMM Mmm
      {
        strncpy_P(buff, pMonths_P + 3*(month-1), 3);
        buff[3] = 0;
      }
      else if (ch == 'd') // D
        ::itoa(date, buff, 10);
      else if (ch == 'D') // DD
        FormatNN(date, buff);
      else // just copy char
      {
        buff[0] = ch;
        buff[1] = 0;
      }
      if (ch == 'X')
      {
        // make uppercase
        buff[1] -= 32;
        buff[2] -= 32;
      }
      strcat(pBuff, buff);
      ch = pgm_read_byte(pFormat_P++);
    }
    return pBuff;
  }

  const char* MSTR_StrN(const char* multiStr_P, uint8_t n)
  {
    // return the Nth string in the given multi-string
    // the first is #0
    while (n-- && pgm_read_byte(multiStr_P))
      while (pgm_read_byte(multiStr_P++))
        ;
    return multiStr_P;
  }

}

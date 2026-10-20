#pragma once
// General Text utilities

// https://palm.wiki/development/docs/601/PalmOSReference/Font.html
enum tFontID 
{
  // Note that not all chars are present, eg LargeBoldFont has only "IPaelmot" (Palmetto III)
  StandardFont = 0, BoldFont, LargeFont, LEDFont = 6, LargeBoldFont, 
  GraffitiFont_Unscaled = 10
};

#define TEXT_ELLIPSIS       "\x85"
#define TEXT_CHEVRON_RIGHT  "\xBB"

// Define PROGMEM Multi-strings.  See MSTR_StrN
#undef MSTR
#define MSTR(s) s "\0"  

namespace Text
{
  // Indicates text shouldn't be draw but length computed
  static const uint16_t kNoDrawX = 1024;
  // Draws text, returns x of last pixel. x & y are LCD
  uint16_t Draw(tFontID font, uint16_t x, uint16_t y, const char* str, tColour fore, tColour back);

  // Returns the font height in PDA units
  uint16_t FontHeight(tFontID font);
  // Returns string width in LCD pixels
  uint16_t StringWidth(tFontID font, const char* str);

  // Formats POSITIVE value as two digits with leading 0
  void FormatNN(int value, char* pBuff);
  // Formats the time according to PREF_TIME_FORMAT
  const char* FormatTime(uint8_t hour24, uint8_t minute, char* pBuff);
  // Formats the date according to PREF_DATE_FORMAT
  const char* FormatDate(uint16_t yearYYYY, uint8_t month, uint8_t date, char* pBuff);

  // Extract the nth string from a PROGMEM multi-string. See MSTR()
  const char* MSTR_StrN(const char* multiStr_P, uint8_t n);
};


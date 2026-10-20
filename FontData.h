#pragma once

namespace FontData
{
  // PROPS is 0xDDHHWW WW=char nominal width, HH=full height, DD=height of descender (not used)
  #define PROPS(_descender, _height, _width) (unsigned long int)((((unsigned long int)_descender)<<16) | (((unsigned long int)_height)<<8) | ((unsigned long int)_width))

  // Standard
  static const unsigned long kStandardProps = PROPS(2, 10, 8);
  extern const char          pStandardMap_P[];
  extern const unsigned char pStandardFont_P[];

  // Large
  static const unsigned long kLargeProps = PROPS(2, 13, 8);
  extern const char          pLargeMap_P[];
  extern const unsigned char pLargeFont_P[];

  // LargeWide
  static const unsigned long kLargeWideProps = PROPS(2, 13, 10);
  extern const char          pLargeWideMap_P[];
  extern const unsigned char pLargeWideFont_P[];

  // Bold
  static const unsigned long kBoldProps = PROPS(2, 10, 8);
  extern const char          pBoldMap_P[];
  extern const unsigned char pBoldFont_P[];

  // BoldWide
  static const unsigned long kBoldWideProps = PROPS(2, 10, 10);
  extern const char          pBoldWideMap_P[];
  extern const unsigned char pBoldWideFont_P[];

  // LargeBold
  static const unsigned long kLargeBoldProps = PROPS(3, 10, 8);
  extern const char          pLargeBoldMap_P[];
  extern const unsigned char pLargeBoldFont_P[];

  // LED
  static const unsigned long kLEDProps = PROPS(0, 14, 8);
  extern const char          pLEDMap_P[];
  extern const unsigned char pLEDFont_P[];

  // Graffiti
  static const unsigned long kGraffitiProps = PROPS(0, 17, 14);
  extern const char          pGraffitiMap_P[];
  extern const unsigned char pGraffitiFont_P[];

  // GraffitiThin
  static const unsigned long kGraffitiThinProps = PROPS(0, 17, 8);
  extern const char          pGraffitiThinMap_P[];
  extern const unsigned char pGraffitiThinFont_P[];

};

#pragma once

// No splash etc
//#define DEBUG

// If defined, the screen is drawn black and white (and 1-byte colour), for faster rendering.
// Otherwise "realistic" colours are used.
//#define PDA_MONOCHROME

// Format has 'h'=12-hour, 'H'=24-hour, 'm'=minutes, 'x'=am/pm, 'X'=AM/PM. 
// Other chars are copied (but note fonts are limited).
#define PREF_TIME_FORMAT "h:m x"

// Format has 'y'=year mod 100, 'Y'=year, 'm'=month, 'M'=month leading 0, 'x'=Jan/Feb, 'X'=JAN/FEB, 'd'=date, 'D'=date leading 0.
// Other chars are copied (but note fonts are limited).
#define PREF_DATE_FORMAT "d-m-y"

// If defined, a sound is made on stylus down.
//#define PREF_AUDIBLE_TOUCH

// If defined, keyboard is ABCDEF, else QWERTY.
#define PREF_KEYBOARD_ABCDEF

// If defined, keyboard uses Graffiti glyphs, else StandardFont chars (which saves ~120 program storage bytes).
#define PREF_KEYBOARD_GRAFFITI

// If defined, the limited ad-hoc kerning is turned off entirely (saves ~150 program storage bytes).
#define PREF_NO_KERN
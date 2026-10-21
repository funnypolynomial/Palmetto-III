#include <Arduino.h>
// Libraries:
#include <SoftwareI2C.h>

#include "LCD_Direct.h"
#include "RTC.h"
#include "PDA.h"
#include "AppsForm.h"
#include "PrefsApp.h"

//   P a l m e t t o - I I I
//   -----------------------
// "Small palm" :-) 	
// *NOT* the Palm OS! Simulation *NOT* emulation.
// The aim of this project was to create *just enough* functionality on an Uno with a 320x480 touch LCD shield to
// simulate the *look and feel* of a Palm III program I wrote in *NINETEEN NINETY-EIGHT*. 
//
// === Why
// I was going through a box of old miscellaneous stuff in a wardrobe when I unearthed my old Palm III.
// This got me thinking about "CountDown", a program I wrote almost THIRTY years ago:
//   https://www.funnypolynomial.com/software/palm/index.html
// Having recently retired, I've found myself reminiscing about things from my past, sometimes revisiting them.
// The Palm OS infrastructure is long gone so there's no easy way to get CountDown running on a Palm (and no
// real desire to).  I thought it would be an interesting project to try recreating it on an Arduino instead.
// Retro-themed graphics on an Arduino LCD shield is something I've done a fair bit of, and enjoy:
//   MacPaint:    https://hackaday.io/project/204744-lackpaint
//   Amiga Ball:  https://hackaday.io/project/204504-ameagreball
//   Asteroids:   https://hackaday.io/project/202157-rasteroids
//   Battlezone:  https://hackaday.io/project/185693-littlezone
//   Elite:       https://hackaday.io/project/183107-elitepetite
//
// === What
// It's a simulation of enough of the look and the feel of a Palm III to revive the ghost of CountDown. Quite a lot of
// code -- the Uno is nearly full.  It's vaguely "event" driven, or at least event call-back driven.  There is a class
// hierarchy of controls (well, actually struct's for simplicity). I originally aimed for a pixel-perfect match with 
// some old screen-captures, but I relaxed that, seems there's some kerning I can't reproduce (see PREF_NO_KERN).
// The 320x480 LCD works nicely with the Palm's 160x160 display by doubling-up.  The lower "graffiti" area ends up 
// proportionally a little bigger. Of the four icons on the graffiti area, only Apps and Menu work (note that I don't
// include the labels, they just look bad at this scale).  Hitting Apps always goes back to the apps screen.  
// Hitting Menu doesn't nothing except show an About box, if applicable. 
// There is no graffiti support. As an homage to it, text entry is via a keyboard in the graffiti area showing 
// graffiti glyphs (optionally, see PREF_KEYBOARD_GRAFFITI). Text entry (of event names) is basic -- append or delete characters.
// Entry is limited to 20 letters, numbers or spaces.
// The field classes render themselves and do hit-testing.  Stylus hits include a "element" number, this is used to distinguish
// up and down on a spin control and also to identify the key on the Keyboard control.
// The Prefs app is much simplified and bears little resemblance to the Palm version, just time, date and digitizer (touch).
// Other preferences are compile-time. Prefs.h has general settings (e.g. PREF_TIME_FORMAT). CountDown has no Prefs form, it defines
// one setting: PREF_TQCD_SHOW_NEGATIVE_COUNTS. Note that touching the screen while the code starts up resets the touch calibration.
// Font and image pixel data is encoded in PROGMEM code files from input images using Python scripts (see the Assets subdirectory).
// A driving concern in the writing of this has been keeping the code/memory footprint low since the Uno was my primary target.
// The fonts are limited, to save program space, not all chars are defined. For example LargeBold only has "IPaelmot" for 
// "Palmetto-III" on the splash/about.
//
// === What it's *NOT*
// This isn't the Palm OS!  It's a simulation of a tiny fraction of it, only the bits I needed to scratch the itch
// of recreating CountDown.
// Things NOT implemented include:
//  * Menus. The Menu button just shows About, if applicable. Apps goes directly to the apps form.
//  * Graffiti! There's a little keyboard instead.
//  * Full text field editing.  All you can do is append and backspace (no selection, insertion etc).
//  * Controls like check-boxes and pop-up triggers
//  * Prefs is much simplified to just time, date and digitiser and they are my own versions.
//  * Calc and Find buttons.
//  * App categories
// 
// === CountDown App
// The main screen shows 4 Events. Each event occupies two lines.  On the left of the first line is the event's name (if set).  
// On the right is the event's Details button. If the event is more than a day away its date is shown here, otherwise
// its time is displayed (if one is set).  The <chevron> character points toward future events and away from past events.
// "Event n Details..." is shown for unused events.  The second line of an event is the count down/up display, in 
// Days (*elapsed*, not calendar), Hours, Minutes and Seconds.  If the event has no specific time (only a date), just the 
// Day number changes and the Seconds display simply cycles through 0..9.  Clicking on the Details button brings up the 
// Event Details form. This has a Name field and buttons for setting the event's Date and Time.  These bring up my 
// version of the "standard" forms.  The time-setting form has an optional "None" button. 
// The Count Down logic has been written from scratch, I paid little attention to my original code, other than the resources. 
// I'm not faking original API calls.
//
// === Triquetral? 
// Note that the About box refers to "Triquetral Software" and has an image of the bones of the hand with one highlighted.
//   The triquetral bone is located in the wrist on the medial side of the proximal row of the carpus between the lunate and pisiform bones.
//   - https://en.wikipedia.org/wiki/Triquetral_bone
// It's in the *hand* (if not exactly the *palm*) and has *three* in the name. In 1998 I was extremely pleased with myself for coming up
// with this name for my nascent software empire.
//
// === Prefs App
// The Prefs form lets you set the date and time via small custom forms. The date's year is split into decades and units to
// reduce clicking, time is always 24-hour mode.  "Spin" controls are used for simplicity.  
// The digitizer (touch screen) can be calibrated, this is different from the Palm: it is full-screen and only two clicks.
//
// === Configuration
// A number of things are configured at compile time by defines in Prefs.h, for example:
//  * Date & Time formats
//  * Colour/monochrome (monochrome draws faster)
//  * Audible clicks
//
// === Hardware
// This runs on an Uno with an ILI948x 320x480 touch LCD shield (ID=0x6814).  
// There are some variations see LCD_FIXED_ORIGIN.
// It also works with a Mega, with room to spare.
// I have stacked the shield with another blank one between the Uno and the LCD. This has 
//  * a DS1307 RTC using SoftwareI2C with SDA=pin 12 & SCL=pin 11
//  * a passive piezo buzzer on pin 10
//
// ==== Speed
// Screen updates need to be snappy.
// The code uses *direct register manipulation* to drive the display, for speed.
// On the Mega the data pins distribution across (3) registers is less convenient, so I *shuffle* the colour bits at
// *compile-time* to simplify (and accelerate) the code (see LCD_TRANSFORM).
// For both devices, using 1-byte colour is faster too.  The 8 data pins are set then simply clocked twice. See PDA_MONOCHROME.
// This gives black and white (0x0000 and 0xFFFF) and also a few "pastels", see Assets\Screens\LCD_FILL_BYTE.png
//
// === Resources:
//  * My original source and screen-snaps and my Palm III
//  * https://cloudpilot-emu.github.io/ which let me run a copy of CountDown.prc I had preserved all these years.
//  * https://en.wikipedia.org/wiki/Palm_III etc, etc
//  * http://www.sealiesoftware.com/palmfonts/ for the font pixels (Thanks Greg Parker!)
//
// I chose not to use AI on the long winding road to this result.  It's not my job, so I don't have to be efficient.
// It's about the journey as much as the destination.
//
// === Author
// Mark Wilson October 2026

void setup() 
{
#ifdef DEBUG
  Serial.begin(38400);
  while (!Serial)
    ;
  Serial.println("; Palmetto-III");
  SERIALISE_ON(true);
#endif  
  LCD_INIT();
  LCD_FILL_BYTE(LCD_BEGIN_FILL(0, 0, LCD_WIDTH, LCD_HEIGHT), 0x00);
  if (LCD_CHECK_TOUCH()) // touch at startup: reset calib
    PrefsAppForm::ResetDigitizer();
  rtc.Setup();
  PDA::Init();
#ifndef DEBUG
  AppsAboutForm::RunForm(true); // splash
#endif  
  AppsForm::RunForm();
}

void loop()
{
}

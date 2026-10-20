#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>

// Overall PDA logic

#include "Prefs.h"
#include "LCD_Direct.h"
#include "Graphics.h"
#include "Text.h"
#include "Form.h"
#include "Controls.h"


namespace PDA
{
  // Init the device
  void Init();
  // Draw empty screen and input
  void DrawEntireScreen();
  // Draw the input area, if not full, clears the box
  void DrawInputBox(bool full);

  // Called when the stylus is lowered at (x_LCD, y_LCD)
  void OnStylusDown(uint16_t x_LCD, uint16_t y_LCD);
  // Called when the stylus is lifted at (x_LCD, y_LCD)
  void OnStylusUp(uint16_t x_LCD, uint16_t y_LCD);
  // Called when the stylus is dragged to (x_LCD, y_LCD)
  void OnStylusMove(uint16_t x_LCD, uint16_t y_LCD);
  // Called when the RTC second rolls over
  void OnSecond();
  // Called to optionally (See PREF_AUDIBLE_TOUCH) make a sound on stylus down
  void Buzz();

  // The main for event loop. 
  uint8_t FormLoop(struct Form* pForm);
  // Calle to exit the loop with the given code
  void ExitLoop(uint8_t code);
  // The code the form loop exited with
  extern uint8_t _LoopExitCode;
};

#define ASSERT(_expression) // if (!(_expression)) Serial.println(__LINE__)
#define DBG(_x) { Serial.print(#_x);Serial.print(":");Serial.println(_x); }

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define LARGE_RAM
#else
#undef LARGE_RAM
#endif

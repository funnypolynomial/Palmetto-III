#pragma once

// The Prefs app sets time and date and calibrates the touch screen

struct PrefsAppForm :Form
{
  PrefsAppForm();
  Title _TITLE;
  Label _TIME;
  Selector _TIMESELECTOR;
  Label _DATE;
  Selector _DATESELECTOR;
  Label _CALIB;
  Selector _DIGITIZER;
  void OnSecond() override;
  const char* GetText(tResID id, bool& progmem) override;

  // Perform app functions
  static void* AppFunc(AppsForm::tAppAction action);
  // Run the form
  static void RunForm();

  // Reset digitisere to hard-wired default fall-back values
  static void ResetDigitizer();

  uint8_t _DisplayedMinute;
};

// Run the date-setting form. True if set. If SetCurrent, setting today's date (year is 2020+, otherwise 1900+)
extern bool RunDateForm(uint16_t& Year, uint8_t& Month, uint8_t& Date, bool SetCurrent);
// Run the time-setting form.  True if set.  
// If pNoTime is non-null choosing <No Time is enabled and the pointed-to bool is displayed and set.
extern bool RunTimeForm(uint8_t& Hour24, uint8_t& Minute, bool* pNoTime);

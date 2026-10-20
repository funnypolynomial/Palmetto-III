#pragma once
// The CountDown app form

// If defined, shows future events with a -ve number of days (no app Prefs form)
//#define PREF_TQCD_SHOW_NEGATIVE_COUNTS

struct TQCDAppForm :Form
{
  TQCDAppForm();

  Title _TITLE;

  Field _DESCVALUE1;
  Selector _SELECTOR1;
  Field _DAYSVALUE1;
  Label _DAYSLABEL1;
  Field _HRSVALUE1;
  Label _HRSLABEL1;
  Field _MINSVALUE1;
  Label _MINSLABEL1;
  Field _SECSVALUE1;
  Label _SECSLABEL1;

  Field _DESCVALUE2;
  Selector _SELECTOR2;
  Field _DAYSVALUE2;
  Label _DAYSLABEL2;
  Field _HRSVALUE2;
  Label _HRSLABEL2;
  Field _MINSVALUE2;
  Label _MINSLABEL2;
  Field _SECSVALUE2;
  Label _SECSLABEL2;

  Field _DESCVALUE3;
  Selector _SELECTOR3;
  Field _DAYSVALUE3;
  Label _DAYSLABEL3;
  Field _HRSVALUE3;
  Label _HRSLABEL3;
  Field _MINSVALUE3;
  Label _MINSLABEL3;
  Field _SECSVALUE3;
  Label _SECSLABEL3;

  Field _DESCVALUE4;
  Selector _SELECTOR4;
  Field _DAYSVALUE4;
  Label _DAYSLABEL4;
  Field _HRSVALUE4;
  Label _HRSLABEL4;
  Field _MINSVALUE4;
  Label _MINSLABEL4;
  Field _SECSVALUE4;
  Label _SECSLABEL4;

  void OnIcon(uint8_t icon) override;
  const char* GetText(tResID id, bool& progmem) override;
  void OnSecond() override;
  const char* FormatValue(tResID id);
  void UpdateField(uint8_t resID);

  static void* AppFunc(AppsForm::tAppAction action);
  static void RunForm();
};


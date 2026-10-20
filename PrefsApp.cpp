#include "PDA.h"
#include "Database.h"
#include "RTC.h"
#include "Time.h"
#include "AppsForm.h"
#include "PrefsApp.h"
#include "PrefsGraphicsData.h"

// Signature used to identify presence of settings in EEPROM
const uint32_t kPrefsSignature = 0x70726566; // "pref"
// Location of settings in EEPROM
const uint16_t kDatabaseStartIdx = 256;
// Touch screen calib values stored in EEPROM
struct tCalib
{
  int _minX, _maxX, _minY, _maxY;
};

void IncrementAndConstrain(int8_t& value, bool inc, int8_t min, int8_t max)
{
  // inc/dec value wrapping around min..max
  value += inc?+1:-1;
  if (value > max)
    value = min;
  else if (value < min)
    value = max;
}

/////////////////////////////////////////////////
// Time
const char pTimeStaticText_P[] PROGMEM = MSTR("Set Time") MSTR(":") MSTR("None");
struct TimeForm :Form
{
  enum
  {
    // static/progmem
    STR_TITLE,
    STR_COLON,
    STR_NOTIME,

    STR_HOUR,
    STR_MINUTE,
    IDC_SPIN
  };
  Title       _TITLE;
  PushButton  _HOUR;
  PushButton  _MINUTE;
  PushButton  _NOTIME;
  Label       _COLON;
  Spin        _SPIN;
  Button      _OK;
  Button      _CANCEL;

  int8_t _Hour24;
  int8_t _Minute;
  bool* _pNoTime;

  TimeForm(uint8_t& Hour24, uint8_t& Minute, bool* pNoTime) :
    Form(FORM_MODAL_BORDER, 100, FORM_MODAL_SIZE, 58, true),
    _TITLE(STR_TITLE),
    _HOUR(40, 20, 20, 30, STR_HOUR, USABLE, StandardFont),
    _MINUTE(65, 20, 20, 30, STR_MINUTE, USABLE, StandardFont),
    _NOTIME(110, 20, 25, 30, STR_NOTIME, USABLE, StandardFont),
    _COLON(60, 20, STR_COLON, USABLE, BoldFont),
    _SPIN(90, 18, 20, 30, IDC_SPIN),
    _OK(6, 40, AUTO, AUTO, IDC_OK, USABLE, StandardFont),
    _CANCEL(37, 40, AUTO, AUTO, IDC_Cancel, USABLE, StandardFont)
  {
    _Hour24 = Hour24;
    _Minute = Minute;
    _pNoTime = pNoTime;
    if (pNoTime && *pNoTime)
      _NOTIME._Selected = true;
    else
      _HOUR._Selected = true;
    if (pNoTime) // modal on mondal, don't do top corners
      _CornerBitset = 0b00000011;
  }

  void OnControlHighlight(bool hit, Control* pCtrl, uint8_t element) override
  {
    if (STR_NOTIME <= pCtrl->_ResID && pCtrl->_ResID <= STR_MINUTE)
      PushButton::UpdateGroup(hit ? pCtrl : NULL, STR_NOTIME, STR_MINUTE, false);
    else
      Form::OnControlHighlight(hit, pCtrl, element);
  }

  void OnControlPressed(Control* pCtrl, uint8_t element) override
  {
    if (STR_NOTIME <= pCtrl->_ResID && pCtrl->_ResID <= STR_MINUTE)
      PushButton::UpdateGroup(pCtrl, STR_NOTIME, STR_MINUTE, true);
    else if (pCtrl == &_SPIN)
    {
      // inc/dec the selected value
      if (_HOUR._Selected)
        IncrementAndConstrain(_Hour24, !element, 0, 23);
      else if (_MINUTE._Selected)
        IncrementAndConstrain(_Minute, !element, 0, 59);
      PushButton::UpdateGroup(NULL, STR_NOTIME, STR_MINUTE, false);
    }
    else
      Form::OnControlPressed(pCtrl, element);
  }


  const char* GetText(tResID id, bool& progmem) override
  {
    progmem = id == STR_TITLE || id == STR_COLON || id == STR_NOTIME;
    if (id == STR_NOTIME && !_pNoTime)
      return NULL;
    if (progmem)
      return Text::MSTR_StrN(pTimeStaticText_P, id);
    else
    {
      Text::FormatNN(id == STR_HOUR?_Hour24:_Minute, Form::_pTextBuffer);
      return Form::_pTextBuffer;
    }
  }

};

bool RunTimeForm(uint8_t& Hour24, uint8_t& Minute, bool* pNoTime)
{
  TimeForm time(Hour24, Minute, pNoTime);
  uint8_t res = PDA::FormLoop(&time);
  if (res == IDC_OK || res == TimeForm::STR_NOTIME)
  {
    Hour24 = time._Hour24;
    Minute = time._Minute;
    if (pNoTime)
      *pNoTime = time._NOTIME._Selected;
    return true;
  }
  return false;
}


/////////////////////////////////////////////////
// Date

const char pDateStaticText_P[] PROGMEM = MSTR("Set Date") MSTR("-");
struct DateForm :Form
{
  enum
  {
    // static/progmem
    STR_TITLE,
    STR_DASH,

    STR_DECADE,
    STR_YEAR,
    STR_MONTH,
    STR_DATE,
    IDC_SPIN
  };
  Title       _TITLE;
  PushButton  _YEAR_DECADES;
  PushButton  _YEAR_UNITS;
  PushButton  _MONTH;
  PushButton  _DATE;
  Label       _DASH1;
  Label       _DASH2;
  Spin        _SPIN;
  Button      _OK;
  Button      _CANCEL;

  // Year, eg 2026, is split into decades, 202 and units, 6.
  // Decade is offset from 1900 (see ref date in Time.cpp)
  static const int16_t kDecadeBase = 190;

  int8_t _YearDecades;  // 2026 is 202
  int8_t _YearUnits;    // 20206 is 6
  int8_t _Month;
  int8_t _Date;
  bool _SetCurrent;     // 


  DateForm(uint16_t& Year, uint8_t& Month, uint8_t& Date, bool SetCurrent) :
    Form(FORM_MODAL_BORDER, 100, FORM_MODAL_SIZE, 58, true),
    _TITLE(STR_TITLE),
    _YEAR_DECADES(40, 20, 20, 30, STR_DECADE, USABLE, StandardFont),
    _YEAR_UNITS(62, 20, 20, 30, STR_YEAR, USABLE, StandardFont),
    _MONTH(83, 20, 20, 30, STR_MONTH, USABLE, StandardFont),
    _DATE(109, 20, 20, 30, STR_DATE, USABLE, StandardFont),
    _DASH1(76, 20, STR_DASH, USABLE, BoldFont),
    _DASH2(102, 20, STR_DASH, USABLE, BoldFont),
    _SPIN(134, 18, 20, 30, IDC_SPIN),
    _OK(6, 40, AUTO, AUTO, IDC_OK, USABLE, StandardFont),
    _CANCEL(37, 40, AUTO, AUTO, IDC_Cancel, USABLE, StandardFont)
  {
    _YearDecades = Year/10 - kDecadeBase;
    _YearUnits = Year % 10;
    _Month = Month;
    _Date = Date;
    _SetCurrent = SetCurrent;
    _DATE._Selected = true;
    if (!SetCurrent) // modal on modal, don't do top corners
      _CornerBitset = 0b00000011;
  }

  void OnControlHighlight(bool hit, Control* pCtrl, uint8_t element) override
  {
    if (STR_DECADE <= pCtrl->_ResID && pCtrl->_ResID <= STR_DATE)
      PushButton::UpdateGroup(hit ? pCtrl : NULL, STR_DECADE, STR_DATE, false);
    else
      Form::OnControlHighlight(hit, pCtrl, element);
  }

  void OnControlPressed(Control* pCtrl, uint8_t element) override
  {
    if (STR_DECADE <= pCtrl->_ResID && pCtrl->_ResID <= STR_DATE)
      PushButton::UpdateGroup(pCtrl, STR_DECADE, STR_DATE, true);
    else if (pCtrl == &_SPIN)
    {
      // inc/dec the selected value
      if (_YEAR_DECADES._Selected)
        IncrementAndConstrain(_YearDecades, !element, _SetCurrent? 202 - kDecadeBase:190 - kDecadeBase, 209 - kDecadeBase);
      else if (_YEAR_UNITS._Selected)
        IncrementAndConstrain(_YearUnits, !element, 0, 9);
      else if (_MONTH._Selected)
        IncrementAndConstrain(_Month, !element, 1, 12);
      uint8_t maxDate = Time::DaysInMonth(_Month, (_YearDecades*10 + _YearUnits)%100);
      if (_Date > maxDate)
        _Date = maxDate;
      if (_DATE._Selected)
        IncrementAndConstrain(_Date, !element, 1, maxDate);
      PushButton::UpdateGroup(NULL, STR_DECADE, STR_DATE, false);
    }
    else
      Form::OnControlPressed(pCtrl, element);
  }


  const char* GetText(tResID id, bool& progmem) override
  {
    progmem = id == STR_TITLE || id == STR_DASH;
    if (progmem)
      return Text::MSTR_StrN(pDateStaticText_P, id);
    else if (id == STR_MONTH || id == STR_DATE)
    {
      Text::FormatNN(id == STR_MONTH ? _Month : _Date, Form::_pTextBuffer);
      return Form::_pTextBuffer;
    }
    else
      return ::itoa(id == STR_DECADE ? (_YearDecades + kDecadeBase) : _YearUnits, Form::_pTextBuffer, 10);
  }
};

bool RunDateForm(uint16_t& Year, uint8_t& Month, uint8_t& Date, bool SetCurrent)
{
  DateForm date(Year, Month, Date, SetCurrent);
  if (PDA::FormLoop(&date) == IDC_OK)
  {
    Year = (date._YearDecades + DateForm::kDecadeBase)*10 + date._YearUnits;
    Month = date._Month;
    Date = date._Date;
    return true;
  }
  return false;
}
/////////////////////////////////////////////////
// Digitizer
// Special case, full-screen, not a real form
// Don't show arrows, redundant, the form is different anyway, and it saves ~100 bytes program space
#ifdef LARGE_RAM 
#define SHOW_ARROWS
#else
#undef SHOW_ARROWS
#endif
#define ARROW_WIDTH   15
#define ARROW_HEIGHT  11

const char pTapTarget_P[] PROGMEM = "Tap center of target";
struct DigitizerForm
{
  const uint16_t kTargetSize_PDA = 21;
  uint8_t _step = 0;
  void Draw()
  {
    // note that colours are inverted because pixel at (0,0) is foreground
    Graphics::FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, SCREEN_BG_COLOUR);
    ::strcpy_P(Form::_pTextBuffer, pTapTarget_P);
    if (_step == 0)
    {
      Graphics::Draw1BPP(PDA2LCD(0), PDA2LCD(0), kTargetSize_PDA, kTargetSize_PDA, PrefsGraphicsData::pTargetImage_P, LCD_SCALE, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
#ifdef SHOW_ARROWS
      Graphics::Draw1BPP(PDA2LCD(25), PDA2LCD(25), ARROW_WIDTH, ARROW_HEIGHT, PrefsGraphicsData::pTopLeftArrowImage_P, LCD_SCALE, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
      Text::Draw(BoldFont, PDA2LCD(43), PDA2LCD(28), Form::_pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
#else
      Text::Draw(BoldFont, PDA2LCD(43 - ARROW_WIDTH), PDA2LCD(28 - ARROW_HEIGHT), Form::_pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
#endif
    }
    else
    {
      Graphics::Draw1BPP(LCD_WIDTH - PDA2LCD(kTargetSize_PDA), LCD_HEIGHT - PDA2LCD(kTargetSize_PDA), kTargetSize_PDA, kTargetSize_PDA, PrefsGraphicsData::pTargetImage_P, LCD_SCALE, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
#ifdef SHOW_ARROWS
      Graphics::Draw1BPP(PDA2LCD(120), PDA2LCD(204), ARROW_WIDTH, ARROW_HEIGHT, PrefsGraphicsData::pBotRightArrowImage_P, LCD_SCALE, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
      Text::Draw(BoldFont, PDA2LCD(13), PDA2LCD(201), Form::_pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
#else
      Text::Draw(BoldFont, PDA2LCD(13 + ARROW_WIDTH), PDA2LCD(201 + ARROW_HEIGHT), Form::_pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
#endif
    }
  }

  void Run()
  {
    int rawX0 = 0, rawY0 = 0;
    int rawX1 = 0, rawY1 = 0;

    while (_step < 2)
    {
      Draw();
      int X, Y;
      // Wait for up:
      while (LCD_GET_TOUCH(X, Y))
        ;
      // Wait for down:
      while (!LCD_GET_TOUCH(X, Y))
        ;
      PDA::Buzz();
      // Wait for up:
      while (LCD_GET_TOUCH(X, Y))
        ;
      // Grab the last good raw values
#ifdef LCD_PORTRAIT_BOT
      if (_step == 0)
        LCD_GET_LAST_RAW_TOUCH(rawX1, rawY1);
      else
        LCD_GET_LAST_RAW_TOUCH(rawX0, rawY0);
#elif defined(LCD_PORTRAIT_TOP)
      if (_step == 0)
        LCD_GET_LAST_RAW_TOUCH(rawX0, rawY0);
      else
        LCD_GET_LAST_RAW_TOUCH(rawX1, rawY1);
#endif    
      _step++;
    }
    // the calib is for Portrait
    tCalib calib;
    // edge ADC = map(pixel edge,  touch pixel range, touch pixel range,  touch ADC range, touch ADC range)
    calib._minX = map(LCD_HEIGHT - 1, PDA2LCD(kTargetSize_PDA)/2, LCD_HEIGHT - PDA2LCD(kTargetSize_PDA)/2,  rawX0, rawX1);
    calib._maxX = map(0,              PDA2LCD(kTargetSize_PDA)/2, LCD_HEIGHT - PDA2LCD(kTargetSize_PDA)/2,  rawX0, rawX1);
    calib._minY = map(LCD_WIDTH - 1,  PDA2LCD(kTargetSize_PDA)/2, LCD_WIDTH  - PDA2LCD(kTargetSize_PDA)/2,  rawY0, rawY1);
    calib._maxY = map(0,              PDA2LCD(kTargetSize_PDA)/2, LCD_WIDTH  - PDA2LCD(kTargetSize_PDA)/2,  rawY0, rawY1);
    uint16_t idx = kDatabaseStartIdx;
    uint32_t sig = kPrefsSignature;
    uint16_t size = (uint16_t)sizeof(calib);
    Database::Write(idx, (uint8_t*)&sig, sizeof(sig));
    Database::Write(idx, (uint8_t*)&size, sizeof(size));
    Database::Write(idx, (uint8_t*)&calib, sizeof(calib));
    LCD_SET_TOUCH_CALIB(calib._minX, calib._maxX, calib._minY, calib._maxY);
    PDA::DrawEntireScreen();
  }
};

/////////////////////////////////////////////////
// Prefs

// See ResetDigitizer()
const char pResetText_P[] PROGMEM = " Reset!";

enum
{
  // static/progmem
  STR_PREFS,
  IDC_SetTime,
  IDC_SetDate,
  IDC_Calibrate,
  IDC_DIGITIZER,

  // dynamic
  STR_DYN__FIRST,
  STR_DATE_VALUE = STR_DYN__FIRST,
  STR_TIME_VALUE
};

const char pPrefsStaticText_P[] PROGMEM = MSTR("Prefs") MSTR("Preferences") MSTR("Set Time:") MSTR("Set Date:") MSTR("Calibrate:") MSTR("Digitizer");
PrefsAppForm::PrefsAppForm() :
  Form(FORM_NORMAL_BORDER, FORM_NORMAL_BORDER, FORM_NORMAL_SIZE, FORM_NORMAL_SIZE, false),
  _TITLE(STR_PREFS),
  _TIME(39, 30, IDC_SetTime, USABLE, BoldFont),
  _TIMESELECTOR(100, 29, 1, 13, STR_TIME_VALUE, USABLE | LEFTANCHOR, StandardFont),
  _DATE(39, 48, IDC_SetDate, USABLE, BoldFont),
  _DATESELECTOR(100, 47, 1, 13, STR_DATE_VALUE, USABLE | LEFTANCHOR, StandardFont),
  _CALIB(39, 48+18, IDC_Calibrate, USABLE, BoldFont),
  _DIGITIZER(100, 65, 1, 13, IDC_DIGITIZER, USABLE | LEFTANCHOR, StandardFont),
  _DisplayedMinute(0xFF)
{

}

void PrefsAppForm::OnSecond()
{
  // Update the time selector. 
  // Probably not running over midnight so no date upate
  uint8_t minute = rtc.ReadMinute();
  if (minute != _DisplayedMinute)
  {
    _DisplayedMinute = minute;
    _TIMESELECTOR.Draw(Control::eNormal);
  }
}

const char* PrefsAppForm::GetText(tResID id, bool& progmem)
{
  // Text for controls
  const char* str = Form::GetText(id, progmem);
  if (str)
    return str;
  progmem = id < STR_DYN__FIRST;
  if (progmem)
    return Text::MSTR_StrN(pPrefsStaticText_P, id + 1);
  else if (id == STR_DATE_VALUE)
  {
    rtc.ReadTime(true);
    return Text::FormatDate(rtc.m_Year + 1900, rtc.m_Month, rtc.m_DayOfMonth, _pTextBuffer);
  }
  else
  {
    rtc.ReadTime(false);
    return Text::FormatTime(rtc.m_Hour24, rtc.m_Minute, _pTextBuffer);
  }
}

void PrefsAppForm::RunForm()
{
  // Do the form loop
  uint8_t res = 0;
  while (PDA::_LoopExitCode != ID_APPS_ICON)
  {
    {
      PrefsAppForm prefs;
      res = PDA::FormLoop(&prefs);
    }
    if (res == IDC_DIGITIZER)         // Digitizer form
    {
      DigitizerForm digitizer;
      digitizer.Run();
    }
    else if (res == STR_TIME_VALUE)   // Time form
    {
      rtc.ReadTime(true);
      uint8_t Hour24 = rtc.m_Hour24;
      uint8_t Minute = rtc.m_Minute;
      if (RunTimeForm(Hour24, Minute, NULL))
      {
        rtc.m_Hour24 = Hour24;
        rtc.m_Minute = Minute;
        rtc.WriteTime();
      }
    }
    else if (res == STR_DATE_VALUE)   // Date form
    {
      rtc.ReadTime(true);
      uint16_t Year = 2000 + rtc.m_Year;
      uint8_t Month = rtc.m_Month;
      uint8_t Date = rtc.m_DayOfMonth;
      if (RunDateForm(Year, Month, Date, true))
      {
        rtc.m_Year = Year % 100;
        rtc.m_Month = Month;
        rtc.m_DayOfMonth = Date;
        rtc.WriteTime();
      }
    }
  }
}

void* PrefsAppForm::AppFunc(AppsForm::tAppAction action)
{
  // Perform the app func actions
  switch (action)
  {
    case AppsForm::eAppInit:
    {
      // Init: load the calib settings from EEPROM
      // data is, sig, size of calib struct, calib struct
      uint16_t idx = kDatabaseStartIdx;
      uint32_t sig;
      uint16_t size;
      tCalib calib;
      Database::Read(idx, (uint8_t*)&sig, sizeof(sig));
      Database::Read(idx, (uint8_t*)&size, sizeof(size));
      if (sig == kPrefsSignature && size == (uint16_t)sizeof(calib))
      {
        // It's there, read it
        Database::Read(idx, (uint8_t*)&calib, sizeof(calib));
        LCD_SET_TOUCH_CALIB(calib._minX, calib._maxX, calib._minY, calib._maxY);
      }
      else
        LCD_SET_TOUCH_CALIB(-1, 0, 0, 0); // reset
      break;
    }
    case AppsForm::eAppName:
      return (void*)pPrefsStaticText_P;
    case AppsForm::eAppIcon:
      return (void*)PrefsGraphicsData::pPrefsImage_P;
    case AppsForm::eAppRun:
      RunForm();
      break;
    default:
      break;
  }
  return NULL;
}

void PrefsAppForm::ResetDigitizer()
{
  // reset the stored touch calib to fall back on hard-wired values
  ::strcpy_P(Form::_pTextBuffer, Text::MSTR_StrN(pPrefsStaticText_P, 5));
  ::strcat_P(Form::_pTextBuffer, pResetText_P);
  Text::Draw(LargeFont, 85, 140, Form::_pTextBuffer, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
  uint16_t idx = kDatabaseStartIdx;
  Database::Write(idx, (uint8_t*)&idx, sizeof(idx)); // zap the EEPROM data
  delay(5000); // make sure it's seen
}


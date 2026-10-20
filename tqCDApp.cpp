#include <Arduino.h>
#include "RTC.h"
// This is the original resource file for Count Down (with a few edits):
#include "tqCDResource.h"

#include "PDA.h"
#include "Database.h"
#include "Time.h"
#include "AppsForm.h"
#include "PrefsApp.h"
#include "tqCDApp.h"
#include "tQCDGraphicsData.h"

void RunAboutForm();
void RunDetailsForm(uint8_t EventNum);

/*
*   The original resource file (.rcp) started like this for the main form.
    TITLE "Count Down"
    FIELD ID IDC_DESCVALUE1 AT (COL_DESCVALUE ROW_DESC1 NUM_DESCCHARS*FONT_DESCWIDTH AUTO) USABLE LEFTALIGN FONT FONT_DESC NONEDITABLE
                         SINGLELINE MAXCHARS NUM_DESCCHARS
      SELECTORTRIGGER  "" ID IDC_SELECTOR1 AT (COL_SELECTOR ROW_SEL1 1 SELECTOR_HEIGHT) USABLE RIGHTANCHOR FONT FONT_SELECTOR
    FIELD ID IDC_DAYSVALUE1 AT (COL_DAYSVALUE ROW_VALUE1 NUM_DAYPIXELS FONT_BIGHEIGHT ) USABLE RIGHTALIGN FONT FONT_BIG NONEDITABLE
                          SINGLELINE MAXCHARS NUM_DAYCHARS
      LABEL "d" ID IDC_DAYSLABEL1 AT (COL_DAYSLABEL ROW_LABEL1 ) USABLE FONT FONT_LABEL
    FIELD ID IDC_HRSVALUE1 AT (COL_HRSVALUE ROW_VALUE1 NUM_TIMECHARS*FONT_BIGWIDTH FONT_BIGHEIGHT ) USABLE RIGHTALIGN FONT FONT_BIG NONEDITABLE
                          SINGLELINE MAXCHARS NUM_TIMECHARS
      LABEL "h" ID IDC_HRSLABEL1 AT (COL_HRSLABEL ROW_LABEL1 ) USABLE FONT FONT_LABEL
    FIELD ID IDC_MINSVALUE1 AT (COL_MINSVALUE ROW_VALUE1 NUM_TIMECHARS*FONT_BIGWIDTH FONT_BIGHEIGHT ) USABLE RIGHTALIGN FONT FONT_BIG NONEDITABLE
                          SINGLELINE MAXCHARS NUM_TIMECHARS
      LABEL "m" ID IDC_MINSLABEL1 AT (COL_MINSLABEL ROW_LABEL1 ) USABLE FONT FONT_LABEL
    FIELD ID IDC_SECSVALUE1 AT (COL_SECSVALUE ROW_VALUE1 NUM_TIMECHARS*FONT_BIGWIDTH FONT_BIGHEIGHT ) USABLE RIGHTALIGN FONT FONT_BIG NONEDITABLE
                          SINGLELINE MAXCHARS NUM_TIMECHARS
      LABEL "s" ID IDC_SECSLABEL1 AT (COL_SECSLABEL ROW_LABEL1 ) USABLE FONT FONT_LABEL
    ...
*/

enum
{
  // PROGMEM static text
  STR_COUNTDOWN,
  STR_D,
  STR_H,
  STR_M,
  STR_S,

  // Dynamic text
  STR_DYN__FIRST,
  STR_DESCVALUE1 = STR_DYN__FIRST,
  STR_DESCVALUE2,
  STR_DESCVALUE3,
  STR_DESCVALUE4,
  // Clickable
  STR_SELECTOR1,
  STR_SELECTOR2,
  STR_SELECTOR3,
  STR_SELECTOR4,

  // Numerical values
  NUM__FIRST__,
  NUM_DAYSVALUE1 = NUM__FIRST__, NUM_HRSVALUE1, NUM_MINSVALUE1, NUM_SECSVALUE1,
  NUM_DAYSVALUE2, NUM_HRSVALUE2, NUM_MINSVALUE2, NUM_SECSVALUE2,
  NUM_DAYSVALUE3, NUM_HRSVALUE3, NUM_MINSVALUE3, NUM_SECSVALUE3,
  NUM_DAYSVALUE4, NUM_HRSVALUE4, NUM_MINSVALUE4, NUM_SECSVALUE4,
};

const uint8_t kNoEventYear = 0; // year value that means NO EVENT
const uint8_t kNoTimeMinute = 0xFF; // minute value that means NO TIME
// Timestamp for midnight this morning, intervals are computed from this (plus time)
uint32_t _TodayTimestampSeconds = 0; // excludes the time

// Info for an event
struct tEvent
{
  // Date & time, it applicable
  uint16_t _YearYYYY;
  uint8_t _Month, _Date, _Hour24, _Minute;
  // Text description
  char _Description[Form::kMaxEditLen + 1];

  // The event's date & time as seconds since reference
  uint32_t _EventTimestampSeconds;

  void ComputeTimestamp()
  {
    // Compute the timestampSeconds for the event
    if (_YearYYYY != kNoEventYear)
    {
      if (_Minute == kNoTimeMinute)
        _EventTimestampSeconds = Time::SecondsSinceReference(_YearYYYY, _Month, _Date);
      else
        _EventTimestampSeconds = Time::SecondsSinceReference(_YearYYYY, _Month, _Date) + Time::TimeAsSeconds(_Hour24, _Minute, 0);
    }
    else
      _EventTimestampSeconds = 0UL;
  }
};

// Info about an interval (to/from and event)
struct tInterval
{
  // Days, hours, minutes & seconds to/from the event
  uint16_t _Days;
  uint8_t  _Hours;
  uint8_t  _Minutes;
  uint8_t  _Seconds;
  bool     _Future; // true if event in in the future

  void ComputeInterval(tEvent& event)
  {
    // Compute the event's interval
    _Days = 0;
    _Hours = _Minutes = _Seconds = 0;
    _Future = false;
    uint32_t timestampSeconds = event._EventTimestampSeconds;
    if (timestampSeconds)
    {
      uint32_t now;
      if (event._Minute == kNoTimeMinute)
        now = _TodayTimestampSeconds;
      else
        now = _TodayTimestampSeconds + Time::TimeAsSeconds(rtc.m_Hour24, rtc.m_Minute, rtc.m_Second);
      _Future = now < timestampSeconds;
      uint32_t diff = now - timestampSeconds;
      if (_Future)
        diff = timestampSeconds - now;
      uint32_t divisor = Time::kSecondsPerDay;
      _Days = diff / divisor;
      diff %= divisor;
      divisor /= 24UL;
      _Hours = diff / divisor;
      diff %= divisor;
      divisor /= 60UL;
      _Minutes = diff / divisor;
      diff %= divisor;
      _Seconds = (event._Minute == kNoTimeMinute) ? rtc.m_Second % 10 : diff;
    }
  }
};

// Signature used to identify presence of settings in EEPROM
const uint32_t kTQCDSignature = 0x74714344; // "tqCD"
// Location of settings in EEPROM
// (Avoid dodgy first few EEPROM bytes on my Uno)?
const uint16_t kDatabaseStartIdx = 8;


// PROGMEM static labels
const char pMainStaticText_P[] PROGMEM = MSTR("Count Down") MSTR("d") MSTR("h") MSTR("m") MSTR("s");
// Base for Event n text
const char pEventNDetailsText_P[] PROGMEM = "Event 1 Details";

// The events
tEvent _Events[4];
// Current date, used to note a change and an update
uint8_t _CurrentDate = 0xFF;

// The intervals
tInterval _Intervals[4];

char* GetEventNTitleStr(uint8_t n, bool title)
{
  // Load event n's title/text into _pTextBuffer
  // If title or no event, text is "Event n details".
  // If a selector, "Event n details..." if no event, 
  //  otherwise formatted date with chevron
  //  or time with chevron if there is a time and the interval is less than 1 day
  if (_Events[n]._YearYYYY == kNoEventYear || title)
  {
    ::strcpy_P(Form::_pTextBuffer, pEventNDetailsText_P);
    Form::_pTextBuffer[6] += n;
    if (!title)
      ::strcat(Form::_pTextBuffer, TEXT_ELLIPSIS);
  }
  else // selector
  {
    // There is an event
    char* pBuff = Form::_pTextBuffer;
    if (_Intervals[n]._Future)
    {
      // future:leading chevron
      ::strcpy(pBuff, TEXT_CHEVRON_RIGHT);
      pBuff += ::strlen(pBuff);
    }
    if (_Events[n]._Minute != kNoTimeMinute && _Intervals[n]._Days < 1)
      Text::FormatTime(_Events[n]._Hour24, _Events[n]._Minute, pBuff);
    else
      Text::FormatDate(_Events[n]._YearYYYY, _Events[n]._Month, _Events[n]._Date, pBuff);
    if (!_Intervals[n]._Future)  // past:trailing chevron
      ::strcat(pBuff, TEXT_CHEVRON_RIGHT);
  }
  return Form::_pTextBuffer;
}


// A close apporimation to the original form resource, FWIW
TQCDAppForm::TQCDAppForm() :
  Form(FORM_NORMAL_BORDER, FORM_NORMAL_BORDER, FORM_NORMAL_SIZE, FORM_NORMAL_SIZE, false),
  _TITLE(STR_COUNTDOWN),

  _DESCVALUE1(COL_DESCVALUE, ROW_DESC1, NUM_DESCCHARS* FONT_DESCWIDTH, AUTO, STR_DESCVALUE1, USABLE | LEFTALIGN, FONT_DESC),
  _SELECTOR1(COL_SELECTOR, ROW_SEL1, 1, SELECTOR_HEIGHT, STR_SELECTOR1, USABLE | RIGHTANCHOR, FONT_SELECTOR),
  _DAYSVALUE1(COL_DAYSVALUE, ROW_VALUE1, NUM_DAYPIXELS, FONT_BIGHEIGHT, NUM_DAYSVALUE1, USABLE | RIGHTALIGN, FONT_BIG),
  _DAYSLABEL1(COL_DAYSLABEL, ROW_LABEL1, STR_D, USABLE, FONT_LABEL),
  _HRSVALUE1(COL_HRSVALUE, ROW_VALUE1, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_HRSVALUE1, USABLE | RIGHTALIGN, FONT_BIG),
  _HRSLABEL1(COL_HRSLABEL, ROW_LABEL1, STR_H, USABLE, FONT_LABEL),
  _MINSVALUE1(COL_MINSVALUE, ROW_VALUE1, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_MINSVALUE1, USABLE | RIGHTALIGN, FONT_BIG),
  _MINSLABEL1(COL_MINSLABEL, ROW_LABEL1, STR_M, USABLE, FONT_LABEL),
  _SECSVALUE1(COL_SECSVALUE, ROW_VALUE1, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_SECSVALUE1, USABLE | RIGHTALIGN, FONT_BIG),
  _SECSLABEL1(COL_SECSLABEL, ROW_LABEL1, STR_S, USABLE, FONT_LABEL),

  _DESCVALUE2(COL_DESCVALUE, ROW_DESC2, NUM_DESCCHARS* FONT_DESCWIDTH, AUTO, STR_DESCVALUE2, USABLE | LEFTALIGN, FONT_DESC),
  _SELECTOR2(COL_SELECTOR, ROW_SEL2, 1, SELECTOR_HEIGHT, STR_SELECTOR2, USABLE | RIGHTANCHOR, FONT_SELECTOR),
  _DAYSVALUE2(COL_DAYSVALUE, ROW_VALUE2, NUM_DAYPIXELS, FONT_BIGHEIGHT, NUM_DAYSVALUE2, USABLE | RIGHTALIGN, FONT_BIG),
  _DAYSLABEL2(COL_DAYSLABEL, ROW_LABEL2, STR_D, USABLE, FONT_LABEL),
  _HRSVALUE2(COL_HRSVALUE, ROW_VALUE2, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_HRSVALUE2, USABLE | RIGHTALIGN, FONT_BIG),
  _HRSLABEL2(COL_HRSLABEL, ROW_LABEL2, STR_H, USABLE, FONT_LABEL),
  _MINSVALUE2(COL_MINSVALUE, ROW_VALUE2, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_MINSVALUE2, USABLE | RIGHTALIGN, FONT_BIG),
  _MINSLABEL2(COL_MINSLABEL, ROW_LABEL2, STR_M, USABLE, FONT_LABEL),
  _SECSVALUE2(COL_SECSVALUE, ROW_VALUE2, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_SECSVALUE2, USABLE | RIGHTALIGN, FONT_BIG),
  _SECSLABEL2(COL_SECSLABEL, ROW_LABEL2, STR_S, USABLE, FONT_LABEL),

  _DESCVALUE3(COL_DESCVALUE, ROW_DESC3, NUM_DESCCHARS* FONT_DESCWIDTH, AUTO, STR_DESCVALUE3, USABLE | LEFTALIGN, FONT_DESC),
  _SELECTOR3(COL_SELECTOR, ROW_SEL3, 1, SELECTOR_HEIGHT, STR_SELECTOR3, USABLE | RIGHTANCHOR, FONT_SELECTOR),
  _DAYSVALUE3(COL_DAYSVALUE, ROW_VALUE3, NUM_DAYPIXELS, FONT_BIGHEIGHT, NUM_DAYSVALUE3, USABLE | RIGHTALIGN, FONT_BIG),
  _DAYSLABEL3(COL_DAYSLABEL, ROW_LABEL3, STR_D, USABLE, FONT_LABEL),
  _HRSVALUE3(COL_HRSVALUE, ROW_VALUE3, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_HRSVALUE3, USABLE | RIGHTALIGN, FONT_BIG),
  _HRSLABEL3(COL_HRSLABEL, ROW_LABEL3, STR_H, USABLE, FONT_LABEL),
  _MINSVALUE3(COL_MINSVALUE, ROW_VALUE3, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_MINSVALUE3, USABLE | RIGHTALIGN, FONT_BIG),
  _MINSLABEL3(COL_MINSLABEL, ROW_LABEL3, STR_M, USABLE, FONT_LABEL),
  _SECSVALUE3(COL_SECSVALUE, ROW_VALUE3, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_SECSVALUE3, USABLE | RIGHTALIGN, FONT_BIG),
  _SECSLABEL3(COL_SECSLABEL, ROW_LABEL3, STR_S, USABLE, FONT_LABEL),

  _DESCVALUE4(COL_DESCVALUE, ROW_DESC4, NUM_DESCCHARS* FONT_DESCWIDTH, AUTO, STR_DESCVALUE4, USABLE | LEFTALIGN, FONT_DESC),
  _SELECTOR4(COL_SELECTOR, ROW_SEL4, 1, SELECTOR_HEIGHT, STR_SELECTOR4, USABLE | RIGHTANCHOR, FONT_SELECTOR),
  _DAYSVALUE4(COL_DAYSVALUE, ROW_VALUE4, NUM_DAYPIXELS, FONT_BIGHEIGHT, NUM_DAYSVALUE4, USABLE | RIGHTALIGN, FONT_BIG),
  _DAYSLABEL4(COL_DAYSLABEL, ROW_LABEL4, STR_D, USABLE, FONT_LABEL),
  _HRSVALUE4(COL_HRSVALUE, ROW_VALUE4, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_HRSVALUE4, USABLE | RIGHTALIGN, FONT_BIG),
  _HRSLABEL4(COL_HRSLABEL, ROW_LABEL4, STR_H, USABLE, FONT_LABEL),
  _MINSVALUE4(COL_MINSVALUE, ROW_VALUE4, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_MINSVALUE4, USABLE | RIGHTALIGN, FONT_BIG),
  _MINSLABEL4(COL_MINSLABEL, ROW_LABEL4, STR_M, USABLE, FONT_LABEL),
  _SECSVALUE4(COL_SECSVALUE, ROW_VALUE4, NUM_TIMECHARS* FONT_BIGWIDTH, FONT_BIGHEIGHT, NUM_SECSVALUE4, USABLE | RIGHTALIGN, FONT_BIG),
  _SECSLABEL4(COL_SECSLABEL, ROW_LABEL4, STR_S, USABLE, FONT_LABEL)
{
  rtc.ReadTime(true);
  _CurrentDate = rtc.m_DayOfMonth;
  _TodayTimestampSeconds = Time::SecondsSinceReference(2000 + rtc.m_Year, rtc.m_Month, rtc.m_DayOfMonth);
  for (int event = 0; event < 4; event++)
    _Intervals[event].ComputeInterval(_Events[event]);
}

const char* TQCDAppForm::FormatValue(tResID id)
{
  // Format the number into Form::_TextBuffer 
  int col = (id - NUM__FIRST__) % 4;
  tInterval* pInterval = _Intervals + (id - NUM__FIRST__)/4;
  if (col)
  {
    int value = pInterval->_Hours;
    if (col == 2)
      value = pInterval->_Minutes;
    if (col == 3)
      value = pInterval->_Seconds;
    Text::FormatNN(value, _pTextBuffer);
  }
  else
  {
#ifdef PREF_TQCD_SHOW_NEGATIVE_COUNTS
    char* pBuff = _pTextBuffer;
    if (pInterval->_Future)
      *pBuff++ = '-';
    ::itoa(pInterval->_Days, pBuff, 10);
#else
    ::itoa(pInterval->_Days, _pTextBuffer, 10);
#endif
  }
  return _pTextBuffer;
}

void TQCDAppForm::UpdateField(uint8_t resID)
{
  // Redraw the field with the given ID
  Control* pCtrl = this->_pControlHead;
  while (pCtrl)
    if (pCtrl->_ResID == resID)
    {
      pCtrl->Draw(Control::eNormal);
      break;
    }
    else
      pCtrl = pCtrl->_pNextCtrl;
}

void TQCDAppForm::OnIcon(uint8_t icon)
{
  // If an icon was clicked (Apps, Menu) exit the loop to process
  PDA::ExitLoop(icon);
}

const char* TQCDAppForm::GetText(tResID id, bool& progmem)
{
  // Text for control
  progmem = id < STR_DYN__FIRST;
  if (progmem)
    return Text::MSTR_StrN(pMainStaticText_P, id);
  else if (STR_DESCVALUE1 <= id && id <= STR_DESCVALUE4)
    return ::strcpy(Form::_pTextBuffer, _Events[id - STR_DESCVALUE1]._Description);
  else if (STR_SELECTOR1 <= id && id <= STR_SELECTOR4)
    return GetEventNTitleStr(id - STR_SELECTOR1, false);
  else
    return FormatValue(id);
}

void TQCDAppForm::OnSecond()
{
  // Update all the intervals
  rtc.ReadTime(true);
  if (_CurrentDate != rtc.m_DayOfMonth) // new day!
  {
    // update cached _TodayTimestampSeconds
    _CurrentDate = rtc.m_DayOfMonth;
    _TodayTimestampSeconds = Time::SecondsSinceReference(2000 + rtc.m_Year, rtc.m_Month, rtc.m_DayOfMonth);
  }

  // update the intervals and fields
  tInterval* pInterval = _Intervals;
  for (int event = 0; event < 4; event++)
  {
    tInterval interval = *pInterval;
    pInterval->ComputeInterval(_Events[event]);
    // Redraw those that have changed
    if (interval._Days != pInterval->_Days)
      UpdateField(NUM_DAYSVALUE1 + 4*event);
    if (interval._Hours != pInterval->_Hours)
      UpdateField(NUM_HRSVALUE1 + 4*event);
    if (interval._Minutes != pInterval->_Minutes)
      UpdateField(NUM_MINSVALUE1 + 4*event);
    if (interval._Seconds != pInterval->_Seconds)
      UpdateField(NUM_SECSVALUE1 + 4*event);
    if (interval._Future != pInterval->_Future)
      UpdateField(STR_SELECTOR1 + event);
    pInterval++;
  }
  SERIALISE_ON(false);
}

void TQCDAppForm::RunForm()
{
  // Run the form
  uint8_t res = 0;
  while (PDA::_LoopExitCode != ID_APPS_ICON)
  {
    {
      TQCDAppForm tqCD;
      res = PDA::FormLoop(&tqCD);
    }
    if (res == ID_MENU_ICON)
      RunAboutForm();
    else if (STR_SELECTOR1 <= res && res <= STR_SELECTOR4)
      RunDetailsForm(res - STR_SELECTOR1);
  }
}

void WriteEventsToDatabase()
{
  // Save events in EEPROM
  // data is, sig, size of event structs, event structs

  uint16_t idx = kDatabaseStartIdx;
  uint32_t sig = kTQCDSignature;
  uint16_t size = (uint16_t)sizeof(_Events);
  Database::Write(idx, (uint8_t*)&sig, sizeof(sig));
  Database::Write(idx, (uint8_t*)&size, sizeof(size));
  Database::Write(idx, (uint8_t*)&_Events, sizeof(_Events));
}

void* TQCDAppForm::AppFunc(AppsForm::tAppAction action)
{
  // Perform the app func actions
  switch (action)
  {
    case AppsForm::eAppInit:
    {
      // Load events from EEPROM
      // data is, sig, size of event structs, event structs
      ::memset(_Events, 0, sizeof(_Events));
      ::memset(_Intervals, 0, sizeof(_Intervals));
      uint16_t idx = kDatabaseStartIdx;
      uint32_t sig;
      uint16_t size;
      Database::Read(idx, (uint8_t*)&sig, sizeof(sig));
      Database::Read(idx, (uint8_t*)&size, sizeof(size));
      if (sig == kTQCDSignature && size == (uint16_t)sizeof(_Events))
        Database::Read(idx, (uint8_t*)&_Events, sizeof(_Events)); // It's there, read it
      else
        WriteEventsToDatabase();  // Write the defaults
      break;
    }
    case AppsForm::eAppName:
      return (void*)Text::MSTR_StrN(pMainStaticText_P, STR_COUNTDOWN);
    case AppsForm::eAppIcon:
      return (void*)tqCDGraphicsData::pTQCDImage_P;
    case AppsForm::eAppRun:
      RunForm();
      break;
    default:
      break;
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////
// About

// Static labels etc
const char pAboutStaticText_P[] PROGMEM = MSTR("About Count Down") MSTR("Triquetral Software") MSTR("Count Down") MSTR("Version 1.00d") MSTR("Copyright \xA9 Mark Wilson, 1998"); // 0xA9 is (c).  I don't consider this copyright any more!

struct CDAboutForm :Form
{
  enum
  {
    // static/progmem
    STR_ABOUT_COUNTDOWN,
    STR_TRIQUETRAL,
    STR_COUNTDOWN2,
    STR_VERSION,
    STR_COPYRIGHT,

    // bmps
    BMP_LOGO,
    BMP_URL,
  };

  CDAboutForm();

  Title _TITLE;
  Bitmap _HAND;
  Label _TQ;
  Label _CD;
  Label _VER;
  Label _COPYRIGHT;
  Bitmap _URL;
  Button _OK;

  const char* GetText(tResID id, bool& progmem) override;
  const uint8_t* GetBitmap_P(tResID id, uint16_t& width, uint16_t& height) override;
};

CDAboutForm::CDAboutForm() :
  Form(FORM_MODAL_BORDER, FORM_MODAL_BORDER, FORM_MODAL_SIZE, FORM_MODAL_SIZE, true),
  _TITLE(STR_ABOUT_COUNTDOWN),
  _HAND(5, 20, BMP_LOGO, USABLE),
  _TQ(PREVLEFT(5) + 43, 20, STR_TRIQUETRAL, USABLE, 1),
  _CD(PREVLEFT(48) + 10, 30 + (3), STR_COUNTDOWN2, USABLE, 2),
  _VER(PREVLEFT(58), 40 + (7), STR_VERSION, USABLE, 0),
  _COPYRIGHT(5, 20+64+5, STR_COPYRIGHT, USABLE, 0),
  _URL(5, 20+64+5+12, BMP_URL, USABLE),
  _OK(CENTER(65), 140, AUTO, AUTO, IDC_OK, USABLE, 0)
{
}

const char* CDAboutForm::GetText(tResID id, bool& progmem)
{
  // Everything is static, just go to the MSTR
  progmem = true;
  return Text::MSTR_StrN(pAboutStaticText_P, id);
}

const uint8_t* CDAboutForm::GetBitmap_P(tResID id, uint16_t& width, uint16_t& height)
{
  // Return the Triquetral logo or the obsolete bigfoot URL
  if (id == BMP_LOGO)
  {
    width = 42;
    height = 65;
    return tqCDGraphicsData::pLogoImage_P;
  }
  else if (id == BMP_URL)
  {
    width = 112;
    height = 8;
    return tqCDGraphicsData::pURLImage_P;
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////
// Details

// Static labels
const char pDetailsStaticText_P[] PROGMEM = MSTR("Name:") MSTR("Delete") MSTR("Time:") MSTR("Date:");
const char pNoTimeStaticText_P[] PROGMEM = "None";

struct CDDetailsForm :Form
{
  enum
  {
    // static/progmem
    STR_NAME,
    STR_DELETE,
    IDC_Time,
    IDC_Date,

    // dynamic
    STR_DYN__FIRST,
    STR_DETAILS_VALUE = STR_DYN__FIRST,
    STR_NAME_VALUE,
    STR_DATE_VALUE,
    STR_TIME_VALUE
  };

  CDDetailsForm(uint8_t EventNum, struct tEvent* pEvent);

  Title _TITLE;
  Label _NAME;
  Field _NAMEVALUE;
  Label _DATE;
  Selector _DATESELECTOR;
  Label _TIME;
  Selector _TIMESELECTOR;
  Button _OK;
  Button _CANCEL;
  Button _DELETE;

  Keyboard _KBD;

  uint8_t _EventNum;
  struct tEvent* _pEvent;

  const char* GetText(tResID id, bool& progmem) override;
  void OnControlPressed(Control* pCtrl, uint8_t element) override;
  void OnSecond() override;
};


CDDetailsForm::CDDetailsForm(uint8_t EventNum, struct tEvent* pEvent) :
  Form(FORM_MODAL_BORDER, 60, FORM_MODAL_SIZE, 98, true),
  _TITLE(STR_DETAILS_VALUE),
  _NAME(5, 20, STR_NAME, USABLE, 1),
  _NAMEVALUE(PREVRIGHT(35)+5, PREVTOP(18), 110, 13, STR_NAME_VALUE, LEFTALIGN | EDITABLE | UNDERLINED | AUTOSHIFT, 0), // MAXCHARS is N/A, always kMaxEditLen
  _DATE(5, PREVBOTTOM(32)+5, IDC_Date, USABLE, 1),
  _DATESELECTOR(PREVRIGHT(40)+5, PREVTOP(37), 1, SELECTOR_HEIGHT, STR_DATE_VALUE, USABLE | LEFTANCHOR, 0),
  _TIME(5, PREVBOTTOM(50)+5, IDC_Time, USABLE, 1),
  _TIMESELECTOR(PREVRIGHT(40)+5, PREVTOP(55), 1, SELECTOR_HEIGHT, STR_TIME_VALUE, USABLE | LEFTANCHOR, 0),
  _OK(6, 80, AUTO, AUTO, IDC_OK, USABLE, 0),
  _CANCEL(PREVRIGHT(31)+6, PREVTOP(80), AUTO, AUTO, IDC_Cancel, USABLE, 0),
  _DELETE(PREVRIGHT(75)+6, PREVTOP(80), AUTO, AUTO, STR_DELETE, USABLE, 0),
  _KBD(&_NAMEVALUE),
  _EventNum(EventNum),
  _pEvent(pEvent)
{
}

const char* CDDetailsForm::GetText(tResID id, bool& progmem)
{
  // Text for the controls
  const char* str = Form::GetText(id, progmem);
  if (str)
    return str;
  progmem = id < STR_DYN__FIRST;
  if (progmem)
    return Text::MSTR_StrN(pDetailsStaticText_P, id);
  else if (id == STR_DETAILS_VALUE)
    return GetEventNTitleStr(_EventNum, true);
  else if (id == STR_NAME_VALUE)
    return ::strcpy(_pTextBuffer, _pEvent->_Description);
  else if (id == STR_DATE_VALUE)
  {
    if (_pEvent->_YearYYYY == kNoEventYear)
    {
      // no event, use now
      rtc.ReadTime(true);
      _pEvent->_YearYYYY = rtc.m_Year + 2000;
      _pEvent->_Month = rtc.m_Month;
      _pEvent->_Date = rtc.m_DayOfMonth;
      _pEvent->_Hour24 = rtc.m_Hour24;
      _pEvent->_Minute = rtc.m_Minute;
    }
    return Text::FormatDate(_pEvent->_YearYYYY, _pEvent->_Month, _pEvent->_Date, _pTextBuffer);
  }
  else if (id == STR_TIME_VALUE)
  {
    if (_pEvent->_Minute != kNoTimeMinute)  // have a time
      return Text::FormatTime(_pEvent->_Hour24, _pEvent->_Minute, _pTextBuffer);
    else
    {
      progmem = true;
      return pNoTimeStaticText_P;
    }
  }
  return NULL;
}

void CDDetailsForm::OnControlPressed(Control* pCtrl, uint8_t element)
{
  // Handle control pressed, at this level just the KBD
  if (pCtrl == &_KBD)
    _KBD.OnKeyPressed(element, _pEvent->_Description);
  else if (pCtrl == &_NAMEVALUE)
    _KBD.SetField(&_NAMEVALUE);
  else
  {
    _KBD.SetField(NULL);
    // Pass to base, which will exit
    Form::OnControlPressed(pCtrl, element);
  }
}

void CDDetailsForm::OnSecond()
{
  // Pass to KBD to blink cursor if applicable
  _KBD.OnSecond();
}

void RunAboutForm()
{
  // Show the CountDown About form
  CDAboutForm about;
  PDA::FormLoop(&about);
}

void RunDetailsForm(uint8_t EventNum)
{
  // Edit the details for the EventNum-th event
  tEvent event = _Events[EventNum];
  CDDetailsForm details(EventNum, &event);
  details._KBD._CAPSOn = !::strlen(event._Description);
  while (PDA::_LoopExitCode != ID_APPS_ICON)
  {
    uint8_t res = PDA::FormLoop(&details);
    if (res == CDDetailsForm::STR_DELETE)           // Delete
    {
      // Clear/reset the event
      ::memset(&_Events[EventNum], 0, sizeof(_Events[EventNum]));
      ::memset(&_Intervals[EventNum], 0, sizeof(_Intervals[EventNum]));
      WriteEventsToDatabase();
    }
    else if (res == IDC_OK)                         // OK, save
    {
      // Accept the event details
      event.ComputeTimestamp();
      _Events[EventNum] = event;
      WriteEventsToDatabase();
    }
    else if (res == CDDetailsForm::STR_DATE_VALUE)  // Date form
    {
      // Run the date form
      uint16_t Year = event._YearYYYY;
      uint8_t Month = event._Month, Date = event._Date;
      if (RunDateForm(Year, Month, Date, false))
      {
        event._YearYYYY = Year;
        event._Month = Month;
        event._Date = Date;
      }
      Form::_pCurrentForm = &details; // go back to editing
      details._KBD.SetField(&details._NAMEVALUE);
      continue;
    }
    else if (res == CDDetailsForm::STR_TIME_VALUE)  // Time form
    {
      // Run the Time form
      uint8_t Hour24 = event._Hour24;
      uint8_t Minute = event._Minute;
      bool NoTime = (event._Minute == kNoTimeMinute);
      if (NoTime)
      {
        // If time is N/A show now
        rtc.ReadTime(false);
        Hour24 = rtc.m_Hour24;
        Minute = rtc.m_Month;
      }
      // Allow "No Time"
      if (RunTimeForm(Hour24, Minute, &NoTime))
      {
        event._Hour24 = Hour24;
        event._Minute = Minute;
        if (NoTime)
          event._Minute = kNoTimeMinute;
      }
      Form::_pCurrentForm = &details; // go back to editing
      details._KBD.SetField(&details._NAMEVALUE);
      continue;
    }
    break;
  }
  details._KBD.SetField(NULL);  // make sure kbd is gone
}


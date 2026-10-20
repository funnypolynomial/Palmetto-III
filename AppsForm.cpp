#include "PDA.h"
#include "RTC.h"
#include "AppsForm.h"
#include "PrefsApp.h"
#include "tqCDApp.h"

///////////////////////////////////////////////////////////////////////
// About
const char pAboutText_P[] PROGMEM = "About";
const char pPalmettoText_P[] PROGMEM = "Palmetto III";
const char pMarkWilsonText_P[] PROGMEM = "Mark Wilson. MMXXVI";

AppsAboutForm::AppsAboutForm(bool Splash) :
  Form(FORM_MODAL_BORDER, FORM_MODAL_BORDER, FORM_MODAL_SIZE, FORM_MODAL_SIZE, true),
  _TITLE(0),
  _OK(CENTER(65), 140, AUTO, AUTO, IDC_OK, USABLE, 0)
{
  // counter controls splash version
  _SplashCtr = Splash?3:0;
}

void AppsAboutForm::OnDraw()
{
  if (!_SplashCtr)
    Form::OnDraw(); // draw the frame if not splash
  ::strcpy_P(_pTextBuffer, pPalmettoText_P);
  tFontID font = LargeBoldFont;
  uint16_t height = Text::FontHeight(font);
  uint16_t y = FORM_NORMAL_SIZE/2 - height;
  Text::Draw(font, PDA2LCD((FORM_NORMAL_SIZE - LCD2PDA(Text::StringWidth(font, _pTextBuffer)))/2), PDA2LCD(y), _pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);

  y += height + 4;
  font = StandardFont;
  ::strcpy_P(_pTextBuffer, pMarkWilsonText_P);
  Text::Draw(font, PDA2LCD((FORM_NORMAL_SIZE - LCD2PDA(Text::StringWidth(font, _pTextBuffer)))/2), PDA2LCD(y), _pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
  // lines
  Graphics::FillRect(192, 151, 5, 2, SCREEN_FG_COLOUR);
  Graphics::FillRect(195, 153, 2, 11, SCREEN_FG_COLOUR);
  Graphics::FillRect(197, 162, 19, 2, SCREEN_FG_COLOUR);
}

void AppsAboutForm::OnSecond()
{
  // Count down the seconds. This is RTC not elapsed so there will be some variation in how long the splash is visible
  if (_SplashCtr)
  {
    _SplashCtr--;
    if (!_SplashCtr)
      PDA::ExitLoop(IDC_OK); // exit
  }
}

const char* AppsAboutForm::GetText(tResID, bool& progmem)
{
  progmem = true;
  return pAboutText_P;
}

void AppsAboutForm::RunForm(bool Splash)
{
  AppsAboutForm about(Splash);
  PDA::FormLoop(&about);
}

///////////////////////////////////////////////////////////////////////
// App
enum
{
  ICON_APP_0,
  ICON_APP_LAST = ICON_APP_0 + AppsForm::kMaxApps - 1, // reserve app icons

  STR_TIME, // the form title is the current time
};

uint8_t displayedMinute = 0xFF; // detect time updates
uint8_t AppsForm::_NumApps = 0; // number of registered apps
// App functions for each registered app
AppsForm::tAppActionFunc AppsForm::_pAppsAction[kMaxApps];

AppsForm::AppsForm() :
  Form(FORM_NORMAL_BORDER, FORM_NORMAL_BORDER, FORM_NORMAL_SIZE, FORM_NORMAL_SIZE, false),
  _TITLE(STR_TIME),
  _APP_ICONS{{0, 0, ICON_APP_0}, 
             {0, 1, ICON_APP_0 + 1}}
{
  for (int app = 0; app < kMaxApps; app++)
    _pAppsAction[app] = NULL;
}

void AppsForm::RegisterApp(tAppActionFunc appFunc)
{
  ASSERT(_NumApps < kMaxApps);
  // Register app form, add to array and initialise
  _pAppsAction[_NumApps++] = appFunc;
  appFunc(eAppInit);
}

void AppsForm::OnIcon(uint8_t icon)
{
  // Exit if the Menu icon (to show About)
  if (icon == ID_MENU_ICON)
    PDA::ExitLoop(icon);
}

void AppsForm::OnSecond()
{
  // Check updating the time on the title bar
  uint8_t m = rtc.ReadMinute();
  if (m != displayedMinute)
    _TITLE.Draw();
}

const char* AppsForm::GetText(tResID id, bool& progmem)
{
  // Handle request for the title-bar time text, or an app icon caption
  if (id == STR_TIME)
  {
    progmem = false;
    rtc.ReadTime(false);
    Text::FormatTime(rtc.m_Hour24, rtc.m_Minute, _pTextBuffer);
    displayedMinute = rtc.m_Minute;
    return _pTextBuffer;
  }
  else if (id < _NumApps)
  {
    // Call the app func
    ASSERT(_pAppsAction[id]);
    progmem = true;
    return static_cast<const char*>(_pAppsAction[id](eAppName));
  }
  return NULL;
}

const uint8_t* AppsForm::GetBitmap_P(tResID id, uint16_t& width, uint16_t& height)
{
  // Call the app func to get the app's icon
  if (id < _NumApps)
  {
    ASSERT(_pAppsAction[id]);
    width = height = 21;
    return static_cast<const uint8_t*>(_pAppsAction[id](eAppIcon));
  }
  return NULL;
}

void AppsForm::RunForm()
{
  // Run the main app form
  AppsForm apps;

  // Registered in alphabetical order:
  AppsForm::RegisterApp(TQCDAppForm::AppFunc);
  AppsForm::RegisterApp(PrefsAppForm::AppFunc);

  while (true)
  {
    uint8_t res = PDA::FormLoop(&apps);
    if (res == ID_MENU_ICON) // About
      AppsAboutForm::RunForm(false);
    else if (res <= ICON_APP_LAST)  // Run the app
      _pAppsAction[res - ICON_APP_0](eAppRun);
  }
}


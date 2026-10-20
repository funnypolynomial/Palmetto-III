#pragma once

// The main form showing the app icons
struct AppsForm:Form
{
  // "App" forms provide an action func to perform various operations: 
  enum tAppAction {
                    eAppInit, // Init the app (load prefs etc)
                    eAppName, // Return non-PROGMEM ptr to App name string
                    eAppIcon, // Return PROGMEM ptr to App icon data (21x21)
                    eAppRun   // Run the App form
                  };
  typedef void* (*tAppActionFunc)(tAppAction action);

  AppsForm();

  // Called to register the App with the form and add it's icon to the 
  // display.
  static void RegisterApp(tAppActionFunc appFunc);

  void OnIcon(uint8_t icon) override;
  void OnSecond() override;

  static const int kMaxApps = 2; // Total apps
  Title _TITLE;
  AppIcon  _APP_ICONS[kMaxApps];
  static uint8_t _NumApps;
  static tAppActionFunc _pAppsAction[kMaxApps];
  const char* GetText(tResID id, bool& progmem) override;
  const uint8_t* GetBitmap_P(tResID id, uint16_t& width, uint16_t& height) override;

  static void RunForm();
};

// About (& splash) for Palmetto-III
struct AppsAboutForm :Form
{
  AppsAboutForm(bool Splash);
  void OnDraw() override;
  void OnSecond() override;
  const char* GetText(tResID id, bool& progmem) override;
  Title _TITLE;
  Button _OK;
  uint8_t _SplashCtr;

  static void RunForm(bool Splash);
};


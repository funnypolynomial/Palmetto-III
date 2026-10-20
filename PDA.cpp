#include "Pins.h"
#include "Prefs.h"
#include "PDA.h"
#include "RTC.h"
#include "PDAGraphicsData.h"
#include "AppsForm.h"
#include "PrefsApp.h"
#include "tqCDApp.h"

// Numbers are LCD pixels
// The upper graphics area
#define SCREEN_ORG_X          0
#define SCREEN_ORG_Y          0
#define SCREEN_WIDTH          LCD_WIDTH
#define SCREEN_HEIGHT         SCREEN_WIDTH
// Gap between areas
#define GAP_ORG_X             0
#define GAP_ORG_Y             (SCREEN_ORG_Y + SCREEN_HEIGHT)
#define GAP_WIDTH             LCD_WIDTH
#define GAP_HEIGHT            5
// Input/graffiti area
#define INPUT_AREA_ORG_X      0
#define INPUT_AREA_ORG_Y      (GAP_ORG_Y + GAP_HEIGHT)
#define INPUT_AREA_WIDTH      LCD_WIDTH
#define INPUT_AREA_HEIGHT     (LCD_HEIGHT - INPUT_AREA_ORG_Y)

#define INPUT_ICON_SIZE       40

#define INPUT_BOX_COLOUR      RGB(0, 0, 0)
#define INPUT_BOX_WIDTH       (LCD_WIDTH - 59 - INPUT_ICON_SIZE)
#define INPUT_BOX_DY          8
#define INPUT_BOX_HEIGHT      (INPUT_AREA_HEIGHT - 2*INPUT_BOX_DY)
#define INPUT_BOX_ORG_Y       (INPUT_AREA_ORG_Y + (INPUT_AREA_HEIGHT - INPUT_BOX_HEIGHT)/2)
#define INPUT_BOX_ORG_X       (INPUT_AREA_ORG_X + (INPUT_AREA_WIDTH - INPUT_BOX_WIDTH)/2)
#define INPUT_BOX_TICK_OFFSET 2*INPUT_BOX_WIDTH/3

#define INPUT_ICON_DX         (((INPUT_AREA_WIDTH - INPUT_BOX_WIDTH)/2 - INPUT_ICON_SIZE)/2)

namespace PDA
{
  // currently selected icon (of Apps, Menu)
  uint8_t selectedIcon = ID_NULL_ICON;

  // If non-null, the code the active form exited with
  uint8_t _LoopExitCode = ID_NULL_RESOURCE;

  // Check RTC seconds rollover
  uint8_t _prevSecond = 0xFF;
  uint32_t _secondCheckMS = 0;
  // Track LCD touches
  bool _touching = false;
  int _touchX, _touchY;

  // Small 1BPP details, created by hand
  // rounded corners, 3x3. 4:top-L, top-R, bottom-L, bottom-R
  // and ticks, 3x3. 2: top & bottom
  static const uint8_t pInputCornerImages_P[] PROGMEM =
  {
    0b00100000,
    0b01000000,
    0b10000000,

    0b10000000,
    0b01000000,
    0b00100000,

    0b10000000,
    0b01000000,
    0b00100000,

    0b00100000,
    0b01000000,
    0b10000000,

    0b11111000,
    0b01110000,
    0b00100000,

    0b00100000,
    0b01110000,
    0b11111000,
  };

  void DrawInputDetailElement(uint16_t x, uint16_t y, int idx)
  {
    // draw round corners etc
    // idx: 0-3 are corners; 4 & 5 are ticks, 6 & 7 are labels
    const uint16_t kSize = 3;
    const uint16_t kLabelSize = 21;
    const uint16_t kLabelOffset = 4;
    if (idx == 1 || idx == 3)
      x -= kSize;
    if (idx == 2 || idx == 3 || idx == 5)
      y -= kSize;
    if (idx < 4)
      Graphics::Draw1BPP(x, y, kSize, kSize, pInputCornerImages_P + idx*kSize, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
    else if (idx < 6)
      Graphics::Draw1BPP(x, y, 5, kSize, pInputCornerImages_P + 4*kSize + kSize*(idx - 4), 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
    else if (idx == 6) // abc
      Graphics::Draw1BPP(x + kLabelOffset, y - kLabelOffset - kLabelSize, kLabelSize, kLabelSize, PDAGraphicsData::pABCImage_P, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
    else if (idx == 7) // 123
      Graphics::Draw1BPP(x - kLabelOffset - kLabelSize, y - kLabelOffset - kLabelSize, kLabelSize, kLabelSize, PDAGraphicsData::p123Image_P, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
  }

  void DrawEntireScreen()
  {
    // Main area fills
    PDA_FILL_RECT(SCREEN_ORG_X, SCREEN_ORG_Y, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BG_COLOUR);
    PDA_FILL_RECT(INPUT_AREA_ORG_X, INPUT_AREA_ORG_Y, INPUT_AREA_WIDTH, INPUT_AREA_HEIGHT, INPUT_AREA_COLOUR);

    // Gap
    PDA_FILL_RECT(GAP_ORG_X, GAP_ORG_Y, GAP_WIDTH, GAP_HEIGHT, GAP_COLOUR);

    // Input box
    DrawInputBox(true);

    // The icons
    uint16_t x0 = INPUT_AREA_ORG_X + INPUT_ICON_DX;
    uint16_t x1 = INPUT_AREA_ORG_X + INPUT_AREA_WIDTH - INPUT_ICON_SIZE - INPUT_ICON_DX;
    uint16_t y0 = INPUT_AREA_ORG_Y + INPUT_BOX_DY;
    uint16_t y1 = INPUT_AREA_ORG_Y + INPUT_AREA_HEIGHT - INPUT_ICON_SIZE - INPUT_BOX_DY;
    Graphics::Draw1BPP(x0, y0, INPUT_ICON_SIZE, INPUT_ICON_SIZE, PDAGraphicsData::pAppsImage_P, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
    Graphics::Draw1BPP(x1, y0, INPUT_ICON_SIZE, INPUT_ICON_SIZE, PDAGraphicsData::pCalcImage_P, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
    Graphics::Draw1BPP(x0, y1, INPUT_ICON_SIZE, INPUT_ICON_SIZE, PDAGraphicsData::pMenuImage_P, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
    Graphics::Draw1BPP(x1, y1, INPUT_ICON_SIZE, INPUT_ICON_SIZE, PDAGraphicsData::pFindImage_P, 1, SCREEN_FG_COLOUR, INPUT_AREA_COLOUR);
  }

  void DrawInputBox(bool full)
  {
    // Draw Input/Graffiti box 
    // If full, draws the whole thing (frame, ticks labels), otherwise just clears the interior
    Graphics::FillRect(INPUT_BOX_ORG_X, INPUT_BOX_ORG_Y, INPUT_BOX_WIDTH, INPUT_BOX_HEIGHT, INPUT_AREA_COLOUR);
    if (full)
    {
      Graphics::DrawFrame(INPUT_BOX_ORG_X, INPUT_BOX_ORG_Y, INPUT_BOX_WIDTH, INPUT_BOX_HEIGHT, INPUT_BOX_COLOUR, INPUT_AREA_COLOUR);
      // Corners
      DrawInputDetailElement(INPUT_BOX_ORG_X, INPUT_BOX_ORG_Y, 0);
      DrawInputDetailElement(INPUT_BOX_ORG_X + INPUT_BOX_WIDTH, INPUT_BOX_ORG_Y, 1);
      DrawInputDetailElement(INPUT_BOX_ORG_X, INPUT_BOX_ORG_Y + INPUT_BOX_HEIGHT, 2);
      DrawInputDetailElement(INPUT_BOX_ORG_X + INPUT_BOX_WIDTH, INPUT_BOX_ORG_Y + INPUT_BOX_HEIGHT, 3);

      // Ticks
      DrawInputDetailElement(INPUT_BOX_ORG_X + INPUT_BOX_TICK_OFFSET, INPUT_BOX_ORG_Y + 1, 4);
      DrawInputDetailElement(INPUT_BOX_ORG_X + INPUT_BOX_TICK_OFFSET, INPUT_BOX_ORG_Y  + INPUT_BOX_HEIGHT - 1, 5);
      // Labels
      DrawInputDetailElement(INPUT_BOX_ORG_X, INPUT_BOX_ORG_Y + INPUT_BOX_HEIGHT, 6);
      DrawInputDetailElement(INPUT_BOX_ORG_X + INPUT_BOX_WIDTH, INPUT_BOX_ORG_Y + INPUT_BOX_HEIGHT, 7);
    }
  }

  void Init()
  {
    // Initialisation, buzzer if applicable, draw basic screen
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    DrawEntireScreen();
  }

  uint8_t IconHitTest(uint16_t x_LCD, uint16_t y_LCD)
  {
    // Returns icon hit at (x_LCD, y_LCD)
    if (Control::HitTest(x_LCD, y_LCD, INPUT_AREA_ORG_X + INPUT_ICON_DX, INPUT_AREA_ORG_Y + INPUT_BOX_DY, INPUT_ICON_SIZE, INPUT_ICON_SIZE))
      return ID_APPS_ICON;
    if (Control::HitTest(x_LCD, y_LCD, INPUT_AREA_ORG_X + INPUT_ICON_DX, INPUT_AREA_ORG_Y + INPUT_AREA_HEIGHT - INPUT_ICON_SIZE - INPUT_BOX_DY, INPUT_ICON_SIZE, INPUT_ICON_SIZE))
      return ID_MENU_ICON;
    return ID_NULL_ICON;
  }

  bool ControlHitTest(uint16_t x_LCD, uint16_t y_LCD, Control*& pHitCtrl, uint8_t& hitElement)
  {
    // Hit-test the controls on the current form, true if found, details in pHitCtrl and hitElement
    ASSERT(Form::_pCurrentForm);
    uint8_t element;
    Control* pCtrl = Form::_pCurrentForm->_pControlHead;
    while (pCtrl)
    {
      if (pCtrl->HitTest(x_LCD, y_LCD, element))
      {
        pHitCtrl = pCtrl;
        hitElement = element;
        return true;
      }
      pCtrl = pCtrl->_pNextCtrl;
    }
    return false;
  }

  void OnStylusDown(uint16_t x_LCD, uint16_t y_LCD)
  {
    // Handle stylus down (touch started)
    // Make a noise, if applicable
    Buzz();
    // Note which icon the Down happened over
    selectedIcon = IconHitTest(x_LCD, y_LCD);
    // Check the form's controls
    if (selectedIcon == ID_NULL_ICON && ControlHitTest(x_LCD, y_LCD, Form::_pSelectedControl, Form::_SelectedControlElement))
    {
      Form::_pCurrentForm->OnControlHighlight(true, Form::_pSelectedControl, Form::_SelectedControlElement);
      Form::_SelectedControlHighlighted = true;
    }
  }

  void OnStylusUp(uint16_t x_LCD, uint16_t y_LCD)
  {
    // Was the up over the selected icon?
    ASSERT(Form::_pCurrentForm);
    uint8_t icon = IconHitTest(x_LCD, y_LCD);
    if (icon != ID_NULL_ICON && icon == selectedIcon) // yes -- notify
      Form::_pCurrentForm->OnIcon(icon);
    selectedIcon = ID_NULL_ICON;
    // Check the form's controls
    if (Form::_pSelectedControl)
    {
      Control* pCtrl = NULL;
      uint8_t element = 0;
      bool pressed = ControlHitTest(x_LCD, y_LCD, pCtrl, element) && pCtrl == Form::_pSelectedControl && element == Form::_SelectedControlElement && Form::_SelectedControlHighlighted;
      Form::_pSelectedControl->Draw(Control::eNormal, Form::_SelectedControlElement);
      Form::_pSelectedControl = NULL;
      Form::_SelectedControlElement = 0;
      Form::_SelectedControlHighlighted = false;
      if (pressed)
        Form::_pCurrentForm->OnControlPressed(pCtrl, element);
    }
  }

  void OnStylusMove(uint16_t x_LCD, uint16_t y_LCD)
  {
    // Stylus dragged, update display of selected control
    if (Form::_pSelectedControl)
    {
      // dragging with a control selected, are we still over it?
      Control* pCtrl = NULL;
      uint8_t element = 0;
      bool hit = ControlHitTest(x_LCD, y_LCD, pCtrl, element) && pCtrl == Form::_pSelectedControl && element == Form::_SelectedControlElement;
      if (hit != Form::_SelectedControlHighlighted)
      {
        // if different, redraw selection
        Form::_pCurrentForm->OnControlHighlight(hit, Form::_pSelectedControl, Form::_SelectedControlElement);
        Form::_SelectedControlHighlighted = hit;
      }
    }
  }

  void OnSecond()
  {
    // Pass to current form
    ASSERT(Form::_pCurrentForm);
    Form::_pCurrentForm->OnSecond();
  }

  void Buzz()
  {
    // Make a noise, if applicable
#ifdef PREF_AUDIBLE_TOUCH
    analogWrite(PIN_BUZZER, 16);
    delay(5);
    analogWrite(PIN_BUZZER, 0);
#endif    
  }

  void ExitLoop(uint8_t code)
  {
    // Exit the form animation with the given code
    ASSERT(code != ID_NULL_RESOURCE);
    _LoopExitCode = code;
    Form::_pCurrentForm = NULL;
  }

  uint8_t FormLoop(struct Form* pForm)
  {
    // Run the form. Returns the exit code
    ASSERT(pForm);
    _LoopExitCode = ID_NULL_RESOURCE;
    Form::_pCurrentForm = pForm;
    pForm->Activate();
    while (_LoopExitCode == ID_NULL_RESOURCE)
    {
      if (!_touching) // no other updates if stylus held down
      {
        // check RTC seconds rollover
        uint32_t nowMS = millis();
        if (nowMS - _secondCheckMS > 100UL)
        {
          _secondCheckMS = nowMS;
          uint8_t s = rtc.ReadSecond();
          if (s != _prevSecond)
          {
            _prevSecond = s;
            OnSecond();
          }
        }
      }

      if (LCD_GET_TOUCH(_touchX, _touchY))
      {
        if (_touching)
          OnStylusMove(_touchX, _touchY);
        else
          OnStylusDown(_touchX, _touchY);
        _touching = true;
      }
      else if (_touching)
      {
        _touching = false;
        OnStylusUp(_touchX, _touchY);
      }
    }
    return _LoopExitCode;
  }
}

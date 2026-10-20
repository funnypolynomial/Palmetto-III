#pragma once

// Forms 

struct Control;
// Note: All numbers are in PDA units, unless _LCD

// ID's don't need to be unique, but they are used as identity when required and to get resources like text
typedef uint8_t tResID;

// Common/Standard resource IDs
enum
{
  IDC_FIRST_COMMON = 0x80,
  // Standard Buttons, fulfilled by Form base class
  IDC_OK = IDC_FIRST_COMMON,
  IDC_Cancel,
  IDC_Keyboard,

  // Icons on imput area:
  ID_APPS_ICON,
  ID_CALC_ICON,
  ID_MENU_ICON,
  ID_FIND_ICON,
  ID_NULL_ICON,

  // To indicate N/A exit code
  ID_NULL_RESOURCE = 0xFF
};

// No support for these resource compiler macros:
#define PREVLEFT(_x)    (_x)
#define PREVRIGHT(_x)   (_x)
#define PREVBOTTOM(_y)  (_y)
#define PREVTOP(_y)     (_y)
#define CENTER(_x)      (_x)

#define FORM_NORMAL_BORDER   0
#define FORM_NORMAL_SIZE     160
#define FORM_MODAL_BORDER    2
#define FORM_MODAL_SIZE      156

struct Form
{
  static const int kMaxTextLen = 32; // size of buffer for labels, titles etc
  static const int kMaxEditLen = 20; // Max len of Field text

  // The currently active form
  static Form* _pCurrentForm;
  // The list of controls
  Control* _pControlHead = NULL;
  Control* _pControlTail = NULL;
  // Form's position and size, PDA numbers
  uint8_t _x_PDA = 0, _y_PDA = 0, _w_PDA = 0, _h_PDA = 0;
  bool _modal = false;
  // bitset is 0b0000ABCD where corners are:
  //   AB
  //   CD
  uint8_t _CornerBitset;  

  // Control slection tracking
  static Control* _pSelectedControl;
  static uint8_t  _SelectedControlElement;
  static bool     _SelectedControlHighlighted;

  Form(uint8_t x_PDA, uint8_t y_PDA, uint8_t w_PDA, uint8_t h_PDA, bool modal);

  // Start the form
  void Activate();

  // Called to draw the form and its controls
  virtual void OnDraw();
  // Called when an Icon is clicked
  virtual void OnIcon(uint8_t icon);
  // Called the the RTC second rolls over
  virtual void OnSecond();
  // Called when a control (pCtrl) is pressed (Stylus up). element is as provided by the hit test
  virtual void OnControlPressed(Control* pCtrl, uint8_t element);
  // Called when the highlight status of the control changes. hit is true if highlighted (Stylus is over control and down)
  virtual void OnControlHighlight(bool hit, Control* pCtrl, uint8_t element);


  // Resource helpers
  // Return a pointer to the text for the resource ID, indicating if it is in PROGMEM
  virtual const char* GetText(tResID id, bool& progmem);
  // Return a PROGMEM pointer to the bitmap data for the resource ID, and provide dims
  virtual const uint8_t* GetBitmap_P(tResID id, uint16_t& width_PDA, uint16_t& height_PDA);

  // Load the text for the resource ID into _pTextBuffer, handling PROGMEM
  // True is found, otherwise control will not be drawn
  static bool LoadText(tResID id);
  // Buffer for Form text operations -- display and editing
  static char _pTextBuffer[kMaxTextLen + 1];

  // Coord Helpers
  // Transform PDA coord to LCD
  static uint16_t CtrlX_LCD(uint8_t x_PDA);
  static uint16_t CtrlY_LCD(uint8_t y_PDA);
  static uint16_t Width_LCD();

  // Draw the form's frame and filled client area
  void DrawFrame();
};
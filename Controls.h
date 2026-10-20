#pragma once

// Controls on forms

// If defined, draw the hitbox and touch-point
//#define DBG_DRAW_CTRL_HITBOX

#define AUTO        0     // auto width place-holder
// Few of these flags are actually implemented
#define USABLE      0x00  // place-holder
#define LEFTALIGN   0x01  // N/A
#define LEFTANCHOR  0x02  // N/A
#define RIGHTANCHOR 0x04  // Control coords are RHS
#define RIGHTALIGN  0x08  // Right-align text within control
#define EDITABLE    0x10  // Can be edited
#define UNDERLINED  0x20  // Drawn with a dotted underline
#define AUTOSHIFT   0x40  // N/A


// Note: All numbers are in PDA units EXCEPT those marked _LCD

// Base for all controls
struct Control
{
  // Draw control normally, or highlighted
  enum tDrawMode {eNormal, eHighlighted};
  // Position and size
  uint8_t _x, _y, _w, _h;
  // Resource ID, used to request text etc and as identity
  tResID _ResID;
  // Format/Layout flags
  uint8_t _Flags;
  // Text font
  uint8_t _Font;
  // Pointer to next control on a form
  Control* _pNextCtrl;

  Control(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font);

  // Draw control in given mode, optional element
  virtual void Draw(tDrawMode mode, uint8_t element = 0);
  // True if the control is clickable and the test coords fall in it
  virtual bool HitTest(uint16_t testX_LCD, uint16_t testY_LCD, uint8_t& element);
  // Hit test helper
  static bool HitTest(uint16_t testX_LCD, uint16_t testY_LCD, uint16_t boxX_LCD, uint16_t boxY_LCD, uint16_t boxW_LCD, uint16_t boxH_LCD);
};

// Base for controls which can be clicked on
struct ClickableControl:Control
{
  // Control bounding box
  uint16_t _hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD;

  ClickableControl(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font);
  bool HitTest(uint16_t testX_LCD, uint16_t testY_LCD, uint8_t& element) override;
};

// Title-bar control
struct Title:Control
{
  Title(tResID resID);
  void Draw(tDrawMode mode = eNormal, uint8_t element = 0) override;
};

// Static label control
struct Label:Control
{
  Label(uint8_t x, uint8_t y, tResID resID, uint8_t flags, uint8_t font);
};

// Edit field control
struct Field:ClickableControl
{
  Field(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font);
  void Draw(tDrawMode mode, uint8_t element = 0) override;
  bool HitTest(uint16_t testX_LCD, uint16_t testY_LCD, uint8_t& element) override;
  uint16_t _rightMostX;
};

// Selector, a clickable text box
struct Selector:ClickableControl
{
  Selector(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font);
  void Draw(tDrawMode mode, uint8_t element = 0) override;
};

// Control showing a bitmap
struct Bitmap:Control
{
  Bitmap(uint8_t x, uint8_t y, tResID resID, uint8_t flags);
  void Draw(tDrawMode mode, uint8_t element = 0) override;
};

// Clickable button
struct Button:ClickableControl
{
  Button(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font);
  void Draw(tDrawMode mode, uint8_t element = 0) override;
  void DoDraw(tDrawMode mode, bool pushButton);
};

// A clickable button, usually part of a group of mutually-exclusive PushButtons. One is selected
struct PushButton:Button
{
  PushButton(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font);
  void Draw(tDrawMode mode, uint8_t element = 0) override;
  // Process mutual-exclusivity
  static void UpdateGroup(Control* pBtn, tResID first, tResID last, bool set);
  // Selected state
  bool _Selected = false;
};

// An app icon
struct AppIcon:ClickableControl
{
  static const uint8_t kWidth  = 51;
  static const uint8_t kHeight = 35;
  AppIcon(uint8_t row, uint8_t col, tResID resID);
  void Draw(tDrawMode mode, uint8_t element = 0) override;
};

// A pair of small <Up>/<Down> buttons.
// Hix boxen are slightly enlarged.
struct Spin:Control
{
  Spin(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID);
  void Draw(tDrawMode mode, uint8_t element) override;
  bool HitTest(uint16_t testX, uint16_t testY, uint8_t& element) override;
};

// A special control which displays a keyboard and edits text in a Field
struct Keyboard:Control
{
  Keyboard(Field* pField);
  void Draw(tDrawMode mode, uint8_t element) override;
  bool HitTest(uint16_t testX, uint16_t testY, uint8_t& element) override;
  void SetField(Field* pField);
  void OnKeyPressed(uint8_t element, char* pBuffer);
  void DrawCell(uint8_t row, uint8_t col, char* pBuff, tColour fg, tColour bg);
  void OnSecond();
  bool _CAPSOn = true;
  uint8_t _CursorOn = 0xFF;
  Field* _pField;
};
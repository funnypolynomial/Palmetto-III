#include "PDA.h"
#include "Controls.h"

// -------- Base -------
Control::Control(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font) :
  _x(x),
  _y(y),
  _w(w),
  _h(h),
  _ResID(resID),
  _Flags(flags),
  _Font(font),
  _pNextCtrl(NULL)
{
  // add to form's list
  if (Form::_pCurrentForm->_pControlHead)
    Form::_pCurrentForm->_pControlTail->_pNextCtrl = this;
  else
    Form::_pCurrentForm->_pControlHead = this;
  Form::_pCurrentForm->_pControlTail = this;
}

void Control::Draw(tDrawMode, uint8_t)
{
  // Base draw just draws the text, if loaded
  if (Form::LoadText(_ResID))
    Text::Draw((tFontID)_Font, Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y + 1), Form::_pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
}

bool Control::HitTest(uint16_t, uint16_t, uint8_t&)
{
  // Base never hits
  return false;
}

bool Control::HitTest(uint16_t testX, uint16_t testY, uint16_t boxX, uint16_t boxY, uint16_t boxW, uint16_t boxH)
{
  // Returns true if (textX, textY) is in (boxX, boxY)-(boxX + boxW, boxY + boxH)
#ifdef DBG_DRAW_CTRL_HITBOX
  if (boxH)
  {
    // 0xE8 gives a reasonable red even when monochrome
    Graphics::DrawLine(testX - 1, testY, 3, true, RGB(0xE8, 0, 0), SCREEN_BG_COLOUR);
    Graphics::DrawLine(testX, testY - 1, 3, false, RGB(0xE8, 0, 0), SCREEN_BG_COLOUR);
    Graphics::DrawFrame(boxX, boxY, boxW, boxH, RGB(0xE8, 0, 0), SCREEN_BG_COLOUR);
  }
#endif
  return boxH && boxX <= testX && testX <= (boxX + boxW) && boxY <= testY && testY <= (boxY + boxH);
}

// ------- ClickableControl --------
ClickableControl::ClickableControl(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font) :
  Control(x, y, w, h, resID, flags, font),
  _hitX_LCD(Form::CtrlX_LCD(x)),
  _hitY_LCD(Form::CtrlY_LCD(y)),
  _hitW_LCD(PDA2LCD(w)),
  _hitH_LCD(PDA2LCD(h))
{
  // by default hitbox is position and size
}

bool ClickableControl::HitTest(uint16_t testX, uint16_t testY, uint8_t& element)
{
  // Default hit test with box members
  element = 0;
  return Control::HitTest(testX, testY, _hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD);
}

// ------- Title --------
const uint16_t kTitleHeight = 15;
Title::Title(tResID resID) :
  Control(0, 0,
    Form::_pCurrentForm->_w_PDA, kTitleHeight,
    resID, 0, BoldFont)
{
}

void Title::Draw(tDrawMode, uint8_t)
{
  // Draw the titlebar
  const uint16_t kLine = 2;
  const uint16_t kText = 3;
  if (Form::_pCurrentForm->_modal)
  {
    // Title is centred
    Graphics::FillRect(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y), PDA2LCD(_w), PDA2LCD(kTitleHeight - 1 - FORM_MODAL_BORDER), SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
    if (Form::LoadText(_ResID))
      Text::Draw((tFontID)_Font, Form::CtrlX_LCD(_x) + (Form::Width_LCD() - Text::StringWidth((tFontID)_Font, Form::_pTextBuffer))/2, Form::CtrlY_LCD(_y) + PDA2LCD(kText - FORM_MODAL_BORDER), Form::_pTextBuffer, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
  }
  else
  {
    // Title is on left
    uint16_t x = kText;
    Graphics::FillRect(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y), PDA2LCD(kText), PDA2LCD(kTitleHeight - kLine), SCREEN_FG_COLOUR); // left end
    Graphics::FillRect(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y + kTitleHeight - kLine), Form::Width_LCD(), PDA2LCD(kLine), SCREEN_FG_COLOUR); // hz line
    if (Form::LoadText(_ResID))
      x = Text::Draw((tFontID)_Font, Form::CtrlX_LCD(_x + kText), Form::CtrlY_LCD(_y + kText), Form::_pTextBuffer, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR);
    Graphics::FillRect(x, Form::CtrlY_LCD(_y), PDA2LCD(kText - 1), PDA2LCD(kTitleHeight - kLine), SCREEN_FG_COLOUR); // rt end
    Graphics::FillRect(Form::CtrlX_LCD(_x + kText), Form::CtrlY_LCD(_y), x - Form::CtrlX_LCD(_x + kText), PDA2LCD(kText), SCREEN_FG_COLOUR); // above text
    x += PDA2LCD(kText - 1);
    Graphics::FillRect(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y), PDA2LCD(1), PDA2LCD(1), SCREEN_BG_COLOUR); // left corner
    Graphics::FillRect(x - PDA2LCD(1), Form::CtrlY_LCD(_y), PDA2LCD(1), PDA2LCD(1), SCREEN_BG_COLOUR); // right corner
    Graphics::FillRect(x, Form::CtrlY_LCD(_y), Form::Width_LCD() - x, PDA2LCD(kTitleHeight - kLine), SCREEN_BG_COLOUR); // clear to end
  }
}

// -------- Label --------
Label::Label(uint8_t x, uint8_t y, tResID resID, uint8_t flags, uint8_t font) :
  Control(x, y, AUTO, AUTO,
    resID, flags, font)
{
}

// -------- Field --------
Field::Field(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font) :
  ClickableControl(x, y, w, h, resID, flags, font),
  _rightMostX(0)
{
}

void Field::Draw(tDrawMode mode, uint8_t element)
{
  // Draw the field, text on a dotted line if element = 0
  // If element = 1 or 2, draw just the "cursor" on/off. 
  // Cursor is always at the end of the text
  if (Form::LoadText(_ResID))
  {
    if (!element)
    {
      uint16_t x_LCD = Form::CtrlX_LCD(_x);
      if (_Flags & RIGHTALIGN)
        x_LCD += PDA2LCD(_w) - Text::StringWidth((tFontID)_Font, Form::_pTextBuffer);
      if (_Flags & EDITABLE)
        Graphics::FillRect(_hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD, SCREEN_BG_COLOUR); // clear background
      _rightMostX = Text::Draw((tFontID)_Font, x_LCD, Form::CtrlY_LCD(_y + 2), Form::_pTextBuffer, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
      if (_Flags & UNDERLINED)
        Graphics::DrawLine(x_LCD, Form::CtrlY_LCD(_y + Text::FontHeight((tFontID)_Font) + 2), PDA2LCD(_w), true, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR, true, PDA2LCD(1));
    }
    if ((element || mode == eHighlighted) && _rightMostX) // cursor
      Graphics::FillRect(_rightMostX, _hitY_LCD, PDA2LCD(2), _hitH_LCD - PDA2LCD(1), (element==1)?SCREEN_FG_COLOUR: SCREEN_BG_COLOUR);
  }
}

bool Field::HitTest(uint16_t testX_LCD, uint16_t testY_LCD, uint8_t& element)
{
  // By default ClickableControl-derived can be hit, but a Field needs to be editable
  return (_Flags & EDITABLE) && ClickableControl::HitTest(testX_LCD, testY_LCD, element);
}

// -------- Selector --------
Selector::Selector(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font) :
  ClickableControl(x, y, w, h, resID, flags, font)
{
}

void Selector::Draw(tDrawMode mode, uint8_t)
{
  // Draw the selector, text in a dotted rectangle
  if (Form::LoadText(_ResID))
  {
    tColour fg = (mode == eHighlighted) ? SCREEN_BG_COLOUR : SCREEN_FG_COLOUR;
    tColour bg = (mode == eHighlighted) ? SCREEN_FG_COLOUR : SCREEN_BG_COLOUR;
    const uint16_t kTextHz = 4;
    const uint16_t kTextVt = 3;
    uint16_t x_LCD;
    uint16_t len = Text::StringWidth((tFontID)_Font, Form::_pTextBuffer);
    if (_Flags & RIGHTANCHOR)
      x_LCD = PDA2LCD(Form::_pCurrentForm->_x_PDA) + Form::Width_LCD() - len - PDA2LCD(kTextHz);
    else
      x_LCD = Form::CtrlX_LCD(_x - kTextHz);
    uint16_t y_LCD = Form::CtrlY_LCD(_y + 2);
    // update hitbox
    _hitX_LCD = x_LCD - PDA2LCD(kTextHz);
    _hitY_LCD = y_LCD - PDA2LCD(kTextVt);
    _hitW_LCD = len + PDA2LCD(2*kTextHz - 1);
    _hitH_LCD = PDA2LCD(Text::FontHeight((tFontID)_Font)) + PDA2LCD(2*kTextVt - 1);
    if (!((_hitW_LCD / LCD_SCALE) % 2))
    {
      // adjust width to make the corners even
      if (_Flags & RIGHTANCHOR)
        _hitX_LCD -= LCD_SCALE;
      _hitW_LCD += LCD_SCALE;
    }
    Graphics::FillRect(_hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD, bg);   // clear rect
    Text::Draw((tFontID)_Font, x_LCD, y_LCD, Form::_pTextBuffer, fg, bg); // text
    Graphics::DrawFrame(_hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR, true, PDA2LCD(1));  // frame
  }
}

// -------- Bitmap --------
Bitmap::Bitmap(uint8_t x, uint8_t y, tResID resID, uint8_t flags) :
  Control(x, y, 0, 0, resID, flags, 0)
{
}

void Bitmap::Draw(tDrawMode, uint8_t)
{
  uint16_t width, height;
  const uint8_t* pBMP_P = Form::_pCurrentForm->GetBitmap_P(_ResID, width, height);
  if (pBMP_P)
    Graphics::Draw1BPP(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y), width, height, pBMP_P, LCD_SCALE, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR);
}

// -------- Button --------
Button::Button(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font) :
  ClickableControl(x, y, w, h, resID, flags, font)
{
}

void Button::Draw(tDrawMode mode, uint8_t)
{
  // Draw the button, text centred in a rounded rectangle
  DoDraw(mode, false);
}

void Button::DoDraw(tDrawMode mode, bool pushButton)
{
  // Draw the button, text centred in a rounded rectangle or square rectangle if pushButton and smaller margins
  if (Form::LoadText(_ResID))
  {
    tColour fg = (mode == eHighlighted) ? SCREEN_BG_COLOUR : SCREEN_FG_COLOUR;
    tColour bg = (mode == eHighlighted) ? SCREEN_FG_COLOUR : SCREEN_BG_COLOUR;
    uint16_t kTextHz = pushButton ? 3 : 6;
    const uint16_t kTextVt = 2;
    int8_t scale = pushButton ? 0 : +LCD_SCALE;
    _hitX_LCD = Form::CtrlX_LCD(_x - 1);
    _hitY_LCD = Form::CtrlY_LCD(_y - 1);
    _hitW_LCD = Text::StringWidth((tFontID)_Font, Form::_pTextBuffer) + PDA2LCD(2*kTextHz + 2);
    _hitH_LCD = PDA2LCD(Text::FontHeight((tFontID)_Font)) + PDA2LCD(2*kTextVt);
    Graphics::FillRect(_hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD, bg, fg, scale);  // filled frame
    if (mode == eNormal)
      Graphics::DrawFrame(_hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD, fg, bg, false, PDA2LCD(1), scale);  // outer frame
    Text::Draw((tFontID)_Font, _hitX_LCD + PDA2LCD(kTextHz + 1), _hitY_LCD + PDA2LCD(kTextVt), Form::_pTextBuffer, fg, bg);        // text
  }
}

// -------- PushButton --------
PushButton::PushButton(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID, uint8_t flags, uint8_t font) :
  Button(x, y, w, h, resID, flags, font)
{
};

void PushButton::Draw(tDrawMode mode, uint8_t)
{
  mode = _Selected ? eHighlighted : mode;
  DoDraw(mode, true);
}

void PushButton::UpdateGroup(Control* pBtn, tResID first, tResID last, bool set)
{
  // Update a group of push buttons, 
  // if not set, draws all in range unselected except pCtrl if non-null, otherwise the _Selected one
  // if set, sets all in range to be _Selected false, pCtrl->_selected is true and drawn
  for (Control* pCtrl = Form::_pCurrentForm->_pControlHead; pCtrl; pCtrl = pCtrl->_pNextCtrl)
  {
    if (first <= pCtrl->_ResID && pCtrl->_ResID <= last)
    {
      PushButton* pPB = static_cast<PushButton*>(pCtrl);
      tDrawMode mode = (pPB == pBtn) ? eHighlighted : eNormal;
      if (set)
        pPB->_Selected = (pPB == pBtn);
      else
        if (pBtn)
          mode = pPB == pBtn ? eHighlighted : eNormal;
        else
          mode = pPB->_Selected ? eHighlighted : eNormal;
      pPB->DoDraw(mode, true);
    }
  }
}

// -------- AppIcon --------
AppIcon::AppIcon(uint8_t row, uint8_t col, tResID resID) :
  ClickableControl(col* kWidth, kTitleHeight + 2 + row*kHeight, kWidth, kHeight, resID, 0, StandardFont)
{
}

void AppIcon::Draw(tDrawMode mode, uint8_t)
{
  tColour fg = (mode == eHighlighted) ? SCREEN_BG_COLOUR : SCREEN_FG_COLOUR;
  tColour bg = (mode == eHighlighted) ? SCREEN_FG_COLOUR : SCREEN_BG_COLOUR;
  uint16_t iconW, iconH;
  const uint8_t* pBMP_P = Form::_pCurrentForm->GetBitmap_P(_ResID, iconW, iconH);
  if (pBMP_P && Form::LoadText(_ResID))
  {
    _hitX_LCD = Form::CtrlX_LCD(_x);
    _hitY_LCD = Form::CtrlY_LCD(_y);
    _hitW_LCD = PDA2LCD(kWidth);
    _hitH_LCD = PDA2LCD(kHeight);
    Graphics::FillRect(_hitX_LCD, _hitY_LCD, _hitW_LCD, _hitH_LCD, bg, fg);
    Graphics::Draw1BPP(_hitX_LCD + PDA2LCD((kWidth - iconW)/2 - 1), _hitY_LCD + PDA2LCD(2), iconW, iconH, pBMP_P, LCD_SCALE, fg, bg);
    Text::Draw(StandardFont, _hitX_LCD + (_hitW_LCD - Text::StringWidth(StandardFont, Form::_pTextBuffer))/2, _hitY_LCD + PDA2LCD(2 + iconH + 2), Form::_pTextBuffer, fg, bg);
    if (mode == eHighlighted)
    {
      // fix the corners, overwriting text pixels
      PDA_FILL_RECT(_hitX_LCD, _hitY_LCD, PDA2LCD(2), LCD_SCALE, fg); PDA_FILL_RECT(_hitX_LCD, _hitY_LCD + LCD_SCALE, LCD_SCALE, LCD_SCALE, fg); // top-left
      PDA_FILL_RECT(_hitX_LCD + _hitW_LCD - PDA2LCD(2), _hitY_LCD, PDA2LCD(2), LCD_SCALE, fg); PDA_FILL_RECT(_hitX_LCD + _hitW_LCD - LCD_SCALE, _hitY_LCD + LCD_SCALE, LCD_SCALE, LCD_SCALE, fg); // top-right
      uint16_t y = _hitY_LCD + _hitH_LCD - LCD_SCALE;
      PDA_FILL_RECT(_hitX_LCD, y, PDA2LCD(2), LCD_SCALE, fg); PDA_FILL_RECT(_hitX_LCD, y - LCD_SCALE, LCD_SCALE, LCD_SCALE, fg); // bot-left
      PDA_FILL_RECT(_hitX_LCD + _hitW_LCD - PDA2LCD(2), y, PDA2LCD(2), LCD_SCALE, fg); PDA_FILL_RECT(_hitX_LCD + _hitW_LCD - LCD_SCALE, y - LCD_SCALE, LCD_SCALE, LCD_SCALE, fg); // bot-right
    }
  }
}

// -------- Spin  --------
// A single control, probably two single-symbol-char buttons on a real Palm
// Upper (increment) element is 0, lower (decrement) is 1
// The hit areas are slightly enlarged
static const uint8_t pSpinArrows_P[] PROGMEM =
{
  0b00000000, 0b00000000,
  0b00000010, 0b00000000,
  0b00000111, 0b00000000,
  0b00001111, 0b10000000,
  0b00011111, 0b11000000,
  0b00111111, 0b11100000,
  0b01111111, 0b11110000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b01111111, 0b11110000,
  0b00111111, 0b11100000,
  0b00011111, 0b11000000,
  0b00001111, 0b10000000,
  0b00000111, 0b00000000,
  0b00000010, 0b00000000,
  0b00000000, 0b00000000,
};

Spin::Spin(uint8_t x, uint8_t y, uint8_t w, uint8_t h, tResID resID) :
  Control(x, y, w, h, resID, 0, StandardFont)
{
}

void Spin::Draw(tDrawMode mode, uint8_t element)
{
  // Draw the whole thing if unselected, otherwise top or bottom arrow, selected
  ASSERT(element <= 1);
  if (mode == eNormal)
    Graphics::Draw1BPP(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y), 13, 17, pSpinArrows_P, LCD_SCALE, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR); // both, normal
  else
    Graphics::Draw1BPP(Form::CtrlX_LCD(_x), Form::CtrlY_LCD(_y + element*9), 13, 8, pSpinArrows_P + element*18, LCD_SCALE, SCREEN_BG_COLOUR, SCREEN_FG_COLOUR); // top or bottom, highlighted
}

bool Spin::HitTest(uint16_t testX, uint16_t testY, uint8_t& element)
{
  // Hit test with a slightly enlarged hitbox
  const uint16_t kEnlargement = 5;  // LCD pixels to expand left, right and away from centre

  if (Control::HitTest(testX, testY, Form::CtrlX_LCD(_x) - kEnlargement, Form::CtrlY_LCD(_y) - kEnlargement, 26 + 2*kEnlargement, 17 + kEnlargement))
  {
    element = 0;
    return true;
  }
  if (Control::HitTest(testX, testY, Form::CtrlX_LCD(_x) - kEnlargement, Form::CtrlY_LCD(_y) + 17, 26 + 2*kEnlargement, 17 + kEnlargement))
  {
    element = 1;
    return true;
  }
  return false;
}

// -------- Keyboard  --------
// A pseudo-control which drives editing a text field, displays a "graffiti keyboard" in the input area.
// Text can only be appended to by hitting a key, or trimmed by hitting BS.
// The idea is this could be expanded to a letters/numbers "page" and a symbols page, toggling between
// them via the key slot currently set to kNoChar.
#define KBD_X       49
#define KBD_Y       333
#define KBD_W       220
#define KBD_H       138
#define KBD_COLS    10
#define KBD_ROWS    4
#define KBD_CELL_W  (KBD_W/KBD_COLS)
#define KBD_CELL_H  (KBD_H/KBD_ROWS)

static const uint8_t kBSChar    = 0x08; // Backspace key in map
static const uint8_t kCAPSChar  = 0x0F; // Caps lock key/indicator in map
static const uint8_t kNoChar    = 0xFF; // A gap, N/A char in map

#ifdef PREF_KEYBOARD_ABCDEF
const char pKeyboardLayout_P[] PROGMEM = 
  "0123456789"
  "ABCDEFGHIJ"
  "KLMNOPQRS "
  "TUVWXYZ\x0F\x08\xFF";
#else
const char pKeyboardLayout_P[] PROGMEM =
  "1234567890"
  "QWERTYUIOP"
  "ASDFGHJKL "
  "ZXCVBNM\x0F\x08\xFF";
#endif

Keyboard::Keyboard(Field* pField) :
#ifdef PREF_KEYBOARD_GRAFFITI
  Control(0, 0, 0, 0, IDC_Keyboard, 0, GraffitiFont_Unscaled),
#else
  Control(0, 0, 0, 0, IDC_Keyboard, 0, StandardFont),
#endif
  _pField(pField)
{
}

void Keyboard::Draw(tDrawMode mode, uint8_t element)
{
  // Draw the keyboard, if applicable (if there is an active _pField)
  // if eNormal and element = 0, draw the whole thing, otherwise
  // just the cell/key indicated by element-1
  if (!_pField)
    return;
  char buff[8];
  buff[1] = 0;
  tColour fg = (mode == eHighlighted) ? INPUT_AREA_COLOUR : SCREEN_FG_COLOUR;
  tColour bg = (mode == eHighlighted) ? SCREEN_FG_COLOUR : INPUT_AREA_COLOUR;
  if (mode == eNormal && element == 0)
  {
    // Clear the Graffiti area
    PDA::DrawInputBox(false);
    // draw whole grid
    bool omitBRCorner = pgm_read_byte(pKeyboardLayout_P + ::strlen_P(pKeyboardLayout_P) - 1) == kNoChar; // EXCEPT the bottom-right corner if N/A
    for (uint8_t r = 0; r <= KBD_ROWS; r++)
      Graphics::DrawLine(KBD_X, KBD_Y + r*KBD_CELL_H, KBD_COLS*KBD_CELL_W + 1 - ((omitBRCorner && r == KBD_ROWS)?KBD_CELL_W:0), true, fg, bg);   // Hz
    for (uint8_t c = 0; c <= KBD_COLS; c++)
      Graphics::DrawLine(KBD_X + c*KBD_CELL_W, KBD_Y, KBD_ROWS*KBD_CELL_H - ((omitBRCorner && c == KBD_COLS)?KBD_CELL_H:0), false, fg, bg);  // Vt
    const char* pChar = pKeyboardLayout_P;
    for (uint8_t r = 0; r < KBD_ROWS; r++)
      for (uint8_t c = 0; c < KBD_COLS; c++)
      {
        buff[0] = pgm_read_byte(pChar++);
        DrawCell(r, c, buff, fg, bg);
      }
    _CursorOn = 1;
    _pField->Draw(eNormal, 1); // cursor
  }
  else
  {
    // just the cell
    element--;
    buff[0] = pgm_read_byte(pKeyboardLayout_P + element);
    uint8_t r = element / KBD_COLS;
    uint8_t c = element % KBD_COLS;
    Graphics::FillRect(KBD_X + c*KBD_CELL_W + 1, KBD_Y + r*KBD_CELL_H + 1, KBD_CELL_W - 1, KBD_CELL_H - 1, bg);
    DrawCell(r, c, buff, fg, bg);
  }
}

bool Keyboard::HitTest(uint16_t testX, uint16_t testY, uint8_t& element)
{
  // true if (testX, testY) on a cell, sets element to the index in the map, +1
  if (!_pField)
    return false;
  element = 0;
#ifdef DBG_DRAW_CTRL_HITBOX
  for (uint8_t r = 0; r < KBD_ROWS; r++)
    for (uint8_t c = 0; c < KBD_COLS; c++)
      if (Control::HitTest(testX, testY, KBD_X + c*KBD_W/KBD_COLS, KBD_Y + r*KBD_H/KBD_ROWS, KBD_W/KBD_COLS, KBD_H/KBD_ROWS))
        element = 1 + r*KBD_COLS + c;
#else
  int r = (testY - KBD_Y)/KBD_CELL_H;
  int c = (testX - KBD_X)/KBD_CELL_W;
  if (0 <= r && r < KBD_ROWS && 0 <= c && c < KBD_COLS)
  {
    element = r*KBD_COLS + c;
    if (element < strlen_P(pKeyboardLayout_P) && pgm_read_byte(pKeyboardLayout_P + element) != kNoChar)
      element++;
    else
      element = 0;
  }
#endif
  return element;
}

void Keyboard::SetField(Field* pField)
{
  // Set the field to be edited, if NULL editing is off and the keyboard is erased
  _pField = pField;
  _CursorOn = 0xFF;
  if (pField)
    Draw(eNormal, 0);
  else // Restore the Graffiti area
    PDA::DrawInputBox(true);
}

void Keyboard::OnKeyPressed(uint8_t element, char* pBuffer)
{
  // Called when a key in the keyboard has been pressed
  // the key's location in the map is element-1
  // pBuffer is the text being edited
  if (!element || !_pField)
    return;
  element--;
  uint16_t charIdx = (uint16_t)strlen(pBuffer);
  uint16_t idx = charIdx;
  bool caps = _CAPSOn;
  if (element < ::strlen_P(pKeyboardLayout_P))
  {
    // valid key
    uint8_t ch = pgm_read_byte(pKeyboardLayout_P + element);
    if (ch == kBSChar)
    {
      // BS
      if (charIdx)
      {
        charIdx--;
        pBuffer[charIdx] = 0;
      }
    }
    else if (ch == kCAPSChar)
    {
      // toggle shift
      _CAPSOn = !_CAPSOn;
    }
    else if (ch != kNoChar && charIdx < Form::kMaxEditLen)
    {
      // append
      if (!_CAPSOn && 'A' <= ch && ch <= 'Z')
        ch += 32; // make lowercase, map is uppercase
      _CAPSOn = false; // unset shift
      pBuffer[charIdx++] = ch;
      pBuffer[charIdx] = 0;
    }
    if (caps != _CAPSOn)
    {
      ASSERT(strchr_P(pKeyboardLayout_P, kCAPSChar));
#ifdef PREF_KEYBOARD_GRAFFITI
      // update CAPS display
      Draw(eNormal, (uint16_t)(::strchr_P(pKeyboardLayout_P, kCAPSChar) - pKeyboardLayout_P)  + 1);
#else
      Draw(eNormal, 0); // redraw the whole keyboard to update the case of the letters
#endif
    }
    if (idx != charIdx)
    {
      // update cursor string
      _pField->Draw(eNormal, 0);
      _CursorOn = 1;
      _pField->Draw(eNormal, 1); // and cursor
    }
  }
}

const char pBSText_P[]        PROGMEM = "bs";
const char pUppercaseText_P[] PROGMEM = "up";
const char pLowercaseText_P[] PROGMEM = "lo";

void Keyboard::DrawCell(uint8_t row, uint8_t col, char* pBuff, tColour fg, tColour bg)
{
  // Draw just the cell/key at (row, col)
  bool CAPS = (uint8_t)pBuff[0] == kCAPSChar;
#ifdef PREF_KEYBOARD_GRAFFITI
  // If the cell is CAPS, draw an inner rectangle showing the state
  if (CAPS)
    Graphics::DrawFrame(KBD_X + col*KBD_CELL_W + 1, KBD_Y + row*KBD_CELL_H + 1, KBD_CELL_W - 1, KBD_CELL_H - 1, _CAPSOn ? fg : bg, _CAPSOn ? bg : fg);
  Text::Draw((tFontID)_Font, KBD_X + col*KBD_CELL_W + (KBD_CELL_W - Text::StringWidth((tFontID)_Font, pBuff))/2 + 1, KBD_Y + row*KBD_CELL_H + 9, pBuff, fg, bg);
#else
  // A bit of a hack, replace special chars with 2-char strings, there's very little room
  if (CAPS)
    ::strcpy_P(pBuff, _CAPSOn?pLowercaseText_P:pUppercaseText_P);
  else if ((uint8_t)pBuff[0] == kBSChar)
    ::strcpy_P(pBuff, pBSText_P);
  else if (!_CAPSOn && pBuff[0] >= 'A')
    pBuff[0] += 32; // make lowercase, map is only alphanumeric
  Text::Draw((tFontID)_Font, KBD_X + col*KBD_CELL_W + (KBD_CELL_W - Text::StringWidth((tFontID)_Font, pBuff))/2 + 1, KBD_Y + row*KBD_CELL_H + 8, pBuff, fg, bg);
  pBuff[1] = 0;// Undo the hack, put the NUL back
#endif
}

void Keyboard::OnSecond()
{
  // Blink the cursor
  if (_pField)
  {
    if (_CursorOn == 0xFF)
      _CursorOn = 1; // make the cursor dwell ON for a little since the 1s call is on the RTC rollover, not synchronised
    else
      _CursorOn = _CursorOn?0:1;
    _pField->Draw(eNormal, _CursorOn ? 1 : 2); // cursor on/off
  }
}


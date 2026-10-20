#include "PDA.h"
#include "Form.h"
#include "Controls.h"

// The form base class

// GLOBAL holding the current form.  Controls find the form they belong to via this, so care needs to be taken
Form* Form::_pCurrentForm = NULL;
// Utility buffer for text
char Form::_pTextBuffer[kMaxTextLen + 1];
// Common button text
const char pStandardStaticText_P[] PROGMEM = MSTR("OK")  MSTR("Cancel");
// Details about the currently selected/highlighted control
Control* Form::_pSelectedControl = NULL;
uint8_t  Form::_SelectedControlElement = 0;
bool     Form::_SelectedControlHighlighted = false;

Form::Form(uint8_t x_PDA, uint8_t y_PDA, uint8_t w_PDA, uint8_t h_PDA, bool modal) :
  _x_PDA(x_PDA),
  _y_PDA(y_PDA),
  _w_PDA(w_PDA),
  _h_PDA(h_PDA),
  _modal(modal),
  _CornerBitset(0b00001111) // all 4 corners, by default
{
  _pCurrentForm = this;
}

void Form::Activate()
{
  // Make this form the active one, and draw it
  _pCurrentForm = this;
  _pSelectedControl = NULL;
  _SelectedControlElement = 0;
  _SelectedControlHighlighted = false;
  OnDraw();
}

void Form::OnDraw()
{
  // Draw the form's frame and controls
  DrawFrame();
  Control* pCtrl = _pControlHead;
  while (pCtrl)
  {
    pCtrl->Draw(Control::eNormal);
    pCtrl = pCtrl->_pNextCtrl;
  }
}

void Form::OnIcon(uint8_t icon)
{
  // Handle clicking on a icon
  if (icon == ID_APPS_ICON) // by default, exit on Apps, ignore Menu
    PDA::ExitLoop(ID_APPS_ICON);
}

void Form::OnSecond()
{
  // nothing by default
}

void Form::OnControlPressed(Control* pCtrl, uint8_t element)
{
  // Default handling of a control being pressed (Stylus up after a down on it)
  // Draw it unhighlighted and exit the loop
  ASSERT(pCtrl);
  pCtrl->Draw(Control::eNormal, element);
  PDA::ExitLoop(pCtrl->_ResID);
}

void Form::OnControlHighlight(bool hit, Control* pCtrl, uint8_t element)
{
  // By default, draw the control highlighted or not, based on hit
  ASSERT(pCtrl);
  pCtrl->Draw(hit?Control::eHighlighted:Control::eNormal, element);
}

bool Form::LoadText(tResID id)
{
  // Loads the given text into the _TextBuffer, true if found
  ASSERT(_pCurrentForm);
  bool progmem = false;
  // try the base first (for OK, Cancel etc)
  const char* pText = _pCurrentForm->Form::GetText(id, progmem);
  if (!pText)
    pText = _pCurrentForm->GetText(id, progmem);
  if (pText)
  {
    if (progmem)
      ::strcpy_P(_pTextBuffer, pText);
    else if (pText != _pTextBuffer)
      ::strcpy(_pTextBuffer, pText);
  }
  return pText;
}

const char* Form::GetText(tResID id, bool& progmem)
{
  // Returns a pointer to the string corresponding to the id, or NULL
  // Sets progmem if that is where the string is stored
  progmem = IDC_OK <= id && id <= IDC_Cancel;
  if (progmem)
    return Text::MSTR_StrN(pStandardStaticText_P, id - IDC_OK);
  return NULL;
}

const uint8_t* Form::GetBitmap_P(tResID, uint16_t& width_PDA, uint16_t& height_PDA)
{
  // Nothing by default
  width_PDA = height_PDA = 0;
  return NULL;
}

uint16_t Form::CtrlX_LCD(uint8_t x_PDA)
{
  // Return x_PDA, which is relative to the form, as an LCD coord
  ASSERT(_pCurrentForm);
  return PDA2LCD(_pCurrentForm->_x_PDA + x_PDA);
}

uint16_t Form::CtrlY_LCD(uint8_t y_PDA)
{
  // Return y_PDA, which is relative to the form, as an LCD coord
  ASSERT(_pCurrentForm);
  return PDA2LCD(_pCurrentForm->_y_PDA + y_PDA);
}

uint16_t Form::Width_LCD()
{
  // Return the width of the form in LCD units
  ASSERT(_pCurrentForm);
  return PDA2LCD(_pCurrentForm->_w_PDA);
}

void Form::DrawFrame()
{
  // Draw the form's frame
  uint16_t margin = (_modal) ? FORM_MODAL_BORDER : 0;

  uint16_t x_LCD = PDA2LCD(_pCurrentForm->_x_PDA - margin);
  uint16_t y_LCD = PDA2LCD(_pCurrentForm->_y_PDA - margin);
  uint16_t w_LCD = PDA2LCD(_pCurrentForm->_w_PDA + 2*margin);
  uint16_t h_LCD = PDA2LCD(_pCurrentForm->_h_PDA + 2*margin);
  // Fill the background, non-modal forms have no frame
  Graphics::FillRect(x_LCD, y_LCD, w_LCD, h_LCD, SCREEN_BG_COLOUR);
  if (_modal) // Draw the modal frame, with the requested corner details
    Graphics::DrawFrame(x_LCD, y_LCD, w_LCD, h_LCD, SCREEN_FG_COLOUR, SCREEN_BG_COLOUR, false, PDA2LCD(2), -LCD_SCALE, _CornerBitset);
}


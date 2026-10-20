#include <Arduino.h>
#include <avr/pgmspace.h>
#include "ILI948x.h"

#ifdef LCD_PORTRAIT_BOT
#define MADCTL0x36 B00001000
#elif defined LCD_PORTRAIT_TOP
#define MADCTL0x36 B11001000
#elif defined LCD_LANDSCAPE_LEFT
#define MADCTL0x36 B10101000
#else
#define MADCTL0x36 B01101000
#endif   
//                  ||||||||  
//                  | B7 Page Address Order
//                   | B6 Column Address Order
//                    | B5 Page/Column Order
//                     | B4 Line Address Order
//                      | B3 RGB/BGR Order
//                       | B2 Display Data Latch Data Order
//                        | B1 Reserved Set to 0
//                         | B0 Reserved Set to 0
byte ILI948x::m_MADCTL0x36 = 0;

#ifdef SERIALISE
bool ILI948x::_serialise = false;
#define SERIALISE_INIT(_w,_h,_s) if (ILI948x::_serialise) { Serial.print(_w);Serial.print(',');Serial.print(_h);Serial.print(',');Serial.println(_s);}
#define SERIALISE_BEGINFILL(_x,_y,_w,_h) if (ILI948x::_serialise) { Serial.print(_x);Serial.print(',');Serial.print(_y);Serial.print(',');Serial.print(_w);Serial.print(',');Serial.println(_h);}
#define SERIALISE_FILLCOLOUR(_len,_colour) if (ILI948x::_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour);}
#define SERIALISE_FILLBYTE(_len,_colour) if (ILI948x::_serialise) { Serial.print(_len);Serial.print(',');Serial.println((word)(_colour | (_colour << 8)));}
#else
#define SERIALISE_INIT(_w,_h,_s)
#define SERIALISE_BEGINFILL(_x,_y,_w,_h)
#define SERIALISE_FILLCOLOUR(_len,_colour)
#define SERIALISE_FILLBYTE(_len,_colour)
#endif

#define LCD_OR_CTRL_PORT B00100000 // Keep A5 INPUT_PULLUP (?)
/////////// UNO/NANO
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#define LCD_OR_PORTB  B00011000 // Keep  11, 12, (SDA & SCL) HIGH
#define LCD_AND_PORTB B00000100 // Preserve 10
#define CTRL_PORT PORTC
#define CTRL_PIN  PINC
// Blasts into TX, RX on PORTD and Pins 10, 11, 12, 13 on PORTB, pins on PORTC, EXCEPT what is set high by LCD_OR_PORTB & LCD_OR_CTRL_PORT
// Stop press: preserve pin 10 (Buzzer)
#define CMD(_cmd)   { PORTD =  (_cmd); PORTB = (PORTB & LCD_AND_PORTB) | (LCD_OR_PORTB |  (_cmd & ~LCD_AND_PORTB)); CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT;              PINC = LCD_WR_BIT; }
#define DATA(_data) { PORTD = (_data); PORTB = (PORTB & LCD_AND_PORTB) | (LCD_OR_PORTB | (_data & ~LCD_AND_PORTB)); CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT; PINC = LCD_WR_BIT; }
#endif

/////////// MEGA
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define MEGA
// Pin  1111 
//      321098 76543210 
// Reg  BBBBHH HHEGEEEE
//      765465 43355410
//// optimised code, specific to Mega
#define CTRL_PORT PORTF
#define CTRL_PIN  PINF
// byte = 0bQQXYPPZZ
// PORTE  0b__PPX___
// PORTG  0b__Y_____
// PORTH  0b_ZZQQ___
// #define DATA_PINS(_d)   PORTE = (PORTE & 0b11000111) | (((_d) & 0b00001100) << 2) | (((_d) & 0b00100000) >> 2);
//                         PORTG = (PORTG & 0b11011111) | (((_d) & 0b00010000) << 1);                              
//                         PORTH = (PORTH & 0b10000111) | (((_d) & 0b11000000) >> 3) | ( (_d)               << 5);

#ifdef LCD_TRANSFORM
// As below but _d has been transformed at compile-time to simplify, other pins are set arbitrarily
//      _d=0bGHHHHEEE
// PORTE = 0b__EEE___
// PORTG = 0b__G_____
// PORTH = 0b_HHHH___
#define DATA_PINS(_d)   PORTE = ((_d) << 3); \
                        PORTG = ((_d) >> 2); \
                        PORTH = (_d)
#else                        
// Overwrite ports with correct bits, other pins set to LOW
#define DATA_PINS(_d)   PORTE = (((_d) & 0b00001100) << 2) | (((_d) & 0b00100000) >> 2); \
                        PORTG = (((_d) & 0b00010000) << 1);                              \
                        PORTH = (((_d) & 0b11000000) >> 3) | ( (_d)               << 5)
#endif

#ifdef LCD_MONOCHROME
// Overwrite ports bits all 1 or all 0, other pins set to LOW
#define DATA_PINS_MONO(_d)   PORTE = PORTG = PORTH = (_d) & 0b01111000 // protect the pins TX & RX on PORTE (etc), everything else gets blasted
#else
#define DATA_PINS_MONO(_d)   {DATA_PINS(_d);}
#endif

#define CMD(_cmd) Cmd(_cmd)
#define DATA(_data) DataByte(_data)
#endif

#define T(_d) LCD_TRANSFORM_BYTE(_d)

// Delay analog read after pin mode switch:
#define TOUCH_DELAY_READ delayMicroseconds(100)

// Show touches
//#define DBG_TOUCH

const byte PROGMEM ILI948x::initialisation[]  = 
{
// len, cmd,   data...
// 255, sleep MS
     3, T(0xC0),  T(0x19), T(0x1A), // Power Control 1 "VREG1OUT +ve VREG2OUT -ve"
     3, T(0xC1),  T(0x45), T(0x00), // Power Control 2 "VGH,VGL    VGH>=14V."
     2, T(0xC2),  T(0x33),       // Power Control 3
     3, T(0xC5),  T(0x00), T(0x28), // VCOM Control "VCM_REG[7:0]. <=0X80."
     3, T(0xB1),  T(0x90), T(0x11), // Frame Rate Control "OSC Freq set. T(0xA0=62HZ,0XB0 =70HZ, <=0XB0."
     2, T(0xB4),  T(0x02),       // Display Inversion Control "2 DOT FRAME MODE,F<=70HZ."
     4, T(0xB6),  T(0x00), T(0x42), T(0x3B), // Display Function Control "0 GS SS SM ISC[3:0];"
     2, T(0xB7),  T(0x07),       // Entry Mode Set
     // Positive Gamma Control:
    16, T(0xE0),  T(0x1F), T(0x25), T(0x22), T(0x0B), T(0x06), T(0x0A), T(0x4E), T(0xC6), T(0x39), T(0x00), T(0x00), T(0x00), T(0x00), T(0x00), T(0x00),
     // Negative Gamma Control:
    16, T(0xE1),  T(0x1F), T(0x3F), T(0x3F), T(0x0F), T(0x1F), T(0x0F), T(0x46), T(0x49), T(0x31), T(0x05), T(0x09), T(0x03), T(0x1C), T(0x1A), T(0x00),
     2, T(0x36),  T(MADCTL0x36), // Memory Access Control BGR (row,col,exch)
     2, T(0x3A),  T(0x55),       // Interface Pixel Format = 16 bits
     1, T(0x11),              // Sleep OUT
   255, 120,               // delay
     //1, T(0x29,              // Display ON, no, wait until we're ready
     0  // END
};

void ILI948x::Init()
{
  for(int p=2;p<10;p++)
  {
    pinMode(p,OUTPUT);
  }
  // All registers to OUTPUT, HIGH:
#ifdef MEGA
  bitSet(DDRE, 4);  // D2
  bitSet(DDRE, 5);  // D3
  bitSet(DDRG, 5);  // D4
  bitSet(DDRE, 3);  // D5
  bitSet(DDRH, 3);  // D6
  bitSet(DDRH, 4);  // D7
  bitSet(DDRH, 5);  // D8
  bitSet(DDRH, 6);  // D9  
 
  DDRF  |= (LCD_RD_PIN | LCD_WR_PIN | LCD_RS_PIN | LCD_CS_PIN | LCD_RST_PIN);
#else  
  DDRC = 0b00011111;
#endif  
  CTRL_PORT =  (LCD_RD_PIN | LCD_WR_PIN | LCD_RS_PIN | LCD_CS_PIN | LCD_RST_PIN);

  digitalWrite(LCD_RST_PIN,HIGH);
  delay(50); 
  digitalWrite(LCD_RST_PIN,LOW);
  delay(150);
  digitalWrite(LCD_RST_PIN,HIGH);
  delay(150);

  digitalWrite(LCD_CS_PIN,HIGH);
  digitalWrite(LCD_WR_PIN,HIGH);
  digitalWrite(LCD_CS_PIN,LOW);

  const byte* init = initialisation;
  do
  {
    byte len = pgm_read_byte(init++);
    if (len == 255)
    {
      delay(pgm_read_byte(init++));
    }
    else if (len)
    {
      Cmd(pgm_read_byte(init++));
      while (--len)
        DataByte(pgm_read_byte(init++));
    }
    else
    {
      break;
    }
    
  } while (true);

  m_MADCTL0x36 = MADCTL0x36;
  SERIALISE_COMMENT("*** START")
  SERIALISE_INIT(LCD_WIDTH, LCD_HEIGHT, 1);
}

void ILI948x::DisplayOn()
{
   CMD(T(0x29));
}

void ILI948x::Cmd(byte cmd)
{
#ifdef MEGA
  DATA_PINS(cmd);
#else  
  PORTD = cmd & B11111100; 
  PORTB = (PORTB & LCD_AND_PORTB) | (LCD_OR_PORTB | (cmd & B00000011)); 
#endif  
  CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT;
  CTRL_PORT |= LCD_WR_BIT;
}


void ILI948x::DataByte(byte data)
{
#ifdef MEGA
  DATA_PINS(data);
#else  
  PORTD = data & B11111100; 
  PORTB = (PORTB & LCD_AND_PORTB) | (LCD_OR_PORTB | (data & B00000011)); 
#endif  
  CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  CTRL_PORT |= LCD_WR_BIT;
}

void ILI948x::DataWord(word data)
{
  DATA(T((byte)(data >> 8)));
  DATA(T((byte)(data)));
}

void ILI948x::ColourByte(byte colour, unsigned long count)
{
  SERIALISE_FILLBYTE(count, colour);
  if (count)
  {
#ifdef MEGA
    if (colour == 0xFF || colour == 0x00)
      DATA_PINS_MONO(colour)
    else
      DATA_PINS(colour);
#else
    PORTD = colour;
    PORTB = (PORTB & LCD_AND_PORTB) | (LCD_OR_PORTB | (colour & ~LCD_AND_PORTB));
#endif  
    CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
    while (count--)
    {
      CTRL_PIN = LCD_WR_BIT;
      CTRL_PIN = LCD_WR_BIT;
      CTRL_PIN = LCD_WR_BIT;
      CTRL_PIN = LCD_WR_BIT;
    }
  }
//  Serial.println(" OK");
}

void ILI948x::ColourWord(word colour, unsigned long count)
{
  SERIALISE_FILLCOLOUR(count, colour);
  byte hi = colour >> 8;
  while (count--)
  {
    DATA(hi);
    DATA(colour);
  }
}


unsigned long ILI948x::Window(word x,word y,word w,word h)
{
  SERIALISE_BEGINFILL(x, y, w, h);
#ifdef LCD_X_OFFSET
  x += LCD_X_OFFSET;
#endif
#ifdef LCD_Y_OFFSET
  y += LCD_Y_OFFSET;
#endif
  word x2 = x + w - 1;
  word y2 = y + h - 1;
  
#ifdef LCD_FIXED_ORIGIN
  // The coordinates of the window sent to the LCD DO NOT adjust for the orientation?
  #define SWAP(_a, _b) {auto temp = _a; _a = _b; _b = temp;}
  #ifdef LCD_PORTRAIT_BOT
    // no adjustment, origin is top left
  #elif defined LCD_PORTRAIT_TOP
    x  = LCD_WIDTH - x;
    x2 = LCD_WIDTH - x2;
    SWAP(x, x2);
    y  = LCD_HEIGHT - y;
    y2 = LCD_HEIGHT - y2;
    SWAP(y, y2);    
  #elif defined LCD_LANDSCAPE_LEFT
    x  = LCD_WIDTH - x;
    x2 = LCD_WIDTH - x2;
    SWAP(x, x2);
  #else  
    y  = LCD_HEIGHT - y;
    y2 = LCD_HEIGHT - y2;
    SWAP(y, y2);
  #endif
#endif
  
  CMD(T(0x2A));
  DataWord(x);
  DataWord(x2);
  
  CMD(T(0x2B));
  DataWord(y);
  DataWord(y2);
  
  CMD(T(0x2C));

  unsigned long count = w;
  count *= h;
  return count;  
}


#ifdef LCD_HAS_TOUCH
#define Z_THRESHOLD 725
#define HOLD_TIME_MS 25
#define MAX_COORD_JUMP 20
#define MIN_COORD_JUMP 5
#define TOUCH_CALIB_X_MIN 140
#define TOUCH_CALIB_X_MAX 870
#define TOUCH_CALIB_Y_MIN 140
#define TOUCH_CALIB_Y_MAX 870

int XP = 7, YP = A2, XM = A1, YM = 6; // Data5, RS, WR, Data4 on LCD
bool m_bPrevReading = false;
bool m_bPrevState = true;
unsigned long m_iTransitionTimeMS = 0UL;

int TouchCalibMinX = TOUCH_CALIB_X_MIN;
int TouchCalibMaxX = TOUCH_CALIB_X_MAX;
int TouchCalibMinY = TOUCH_CALIB_Y_MIN;
int TouchCalibMaxY = TOUCH_CALIB_Y_MAX;
const int kNullCoord = -999;
int _prevX = kNullCoord, _prevY;
bool _FilterTouch = true;

// x and y are along long and short axis respectively (USB on right)
int _LastRawX = 0, _LastRawY = 0;
bool GetRawTouch(int& rawX, int& rawY)
{
  // debounced touch, true if touch
  bool touch = false;
  digitalWrite(LCD_CS_PIN, HIGH); // deselect LCD

  // Set X+ to ground
  pinMode(XP, OUTPUT);
  digitalWrite(XP, LOW);

  // Set Y- to VCC
  pinMode(YM, OUTPUT);
  digitalWrite(YM, HIGH);

  // Hi-Z X- and Y+
  pinMode(XM, INPUT_PULLUP);
  pinMode(YP, INPUT_PULLUP);

  TOUCH_DELAY_READ;
  int z1 = analogRead(XM);
  int z2 = analogRead(YP);

  bool ThisReading = (z2 - z1) < Z_THRESHOLD;
  if (ThisReading != m_bPrevReading && _FilterTouch)
  {
      // state change, reset the timer
      m_bPrevReading = ThisReading;
      m_iTransitionTimeMS = millis();
  }
  else if (((millis() - m_iTransitionTimeMS) >= HOLD_TIME_MS) | _FilterTouch)
  {
      // consistent for long enough, read the values
      if (ThisReading)
      {
          pinMode(YP, INPUT_PULLUP);
          pinMode(YM, INPUT_PULLUP);
          pinMode(XP, OUTPUT);
          digitalWrite(XP, HIGH);
          pinMode(XM, OUTPUT);
          digitalWrite(XM, LOW);
          TOUCH_DELAY_READ;
          rawX = analogRead(YP);

          pinMode(XP, INPUT_PULLUP);
          pinMode(XM, INPUT_PULLUP);
          pinMode(YP, OUTPUT);
          digitalWrite(YP, HIGH);
          pinMode(YM, OUTPUT);
          digitalWrite(YM, LOW);
          TOUCH_DELAY_READ;
          rawY = analogRead(XM);

          pinMode(XP, OUTPUT);
          pinMode(YM, OUTPUT);
      }
      touch = ThisReading;
      m_bPrevState = ThisReading;
  }

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(LCD_CS_PIN, LOW); // re-select LCD
  return touch;    
}

bool ILI948x::CheckTouch()
{
  int x, y;
  _FilterTouch = false;
  bool touch = GetRawTouch(x, y);
  _FilterTouch = true;
  return touch;
}

bool ILI948x::GetTouch(int& x, int& y)
{
  // returns true if touch, x & y are transformed pixel coords
  int rawX = 0, rawY = 0;
  if (GetRawTouch(rawX, rawY))
  {
#if defined(LCD_PORTRAIT_TOP)
    // pixel = map(measured ADC,  calib ADC range, calib ADC range,  LCD pixel range, LCD pixel range)
    x = map(rawY, TouchCalibMinY, TouchCalibMaxY, LCD_WIDTH - 1, 0);
    y = map(rawX, TouchCalibMinX, TouchCalibMaxX, LCD_HEIGHT - 1, 0);
#elif defined(LCD_PORTRAIT_BOT) 
    x = map(rawY, TouchCalibMinY, TouchCalibMaxY, 0, LCD_WIDTH - 1);
    y = map(rawX, TouchCalibMinX, TouchCalibMaxX, 0, LCD_HEIGHT - 1);
#elif defined(LCD_LANDSCAPE_LEFT) 
    x = map(rawX, TouchCalibMinX, TouchCalibMaxX, LCD_WIDTH - 1, 0);
    y = map(rawY, TouchCalibMinY, TouchCalibMaxY, 0, LCD_HEIGHT - 1);
#else    
    x = map(rawX, TouchCalibMinX, TouchCalibMaxX, 0, LCD_WIDTH - 1);
    y = map(rawY, TouchCalibMinY, TouchCalibMaxY, LCD_HEIGHT - 1, 0);
#endif
    int dX = abs(_prevX - x);
    int dY = abs(_prevY - y);
    if (_prevX != kNullCoord && ((dX > MAX_COORD_JUMP || dY > MAX_COORD_JUMP) || (dX < MIN_COORD_JUMP && dY < MIN_COORD_JUMP)))
    {
      // ignore large or small position jumps
      x = _prevX;
      y = _prevY;
    }
    else
    {
      _LastRawX = rawX;
      _LastRawY = rawY;
    }
    if (_prevX != kNullCoord)
    {
      // average last two readings, and discard LSB
      int avgX = (_prevX + x)/4;
      int avgY = (_prevY + y)/4;
      _prevX = x;
      _prevY = y;
      x = avgX*2;
      y = avgY*2;
    }
    else
    {
      _prevX = x;
      _prevY = y;
    }
#ifdef DBG_TOUCH    
    //Serial.print(x);Serial.print(",");Serial.println(y);
    ColourWord(RGB(255, 0, 0), Window(x, 0, 1, LCD_HEIGHT));
    ColourWord(RGB(255, 0, 0), Window(0, y, LCD_WIDTH, 1));
#endif    
    return true;
  }
  else
    _prevX = kNullCoord;
  return false;
}

void ILI948x::SetTouchCalib(int minX, int maxX, int minY, int maxY)
{
  if (minX < 0)
  {
    // reset!
    TouchCalibMinX = TOUCH_CALIB_X_MIN;
    TouchCalibMaxX = TOUCH_CALIB_X_MAX;
    TouchCalibMinY = TOUCH_CALIB_Y_MIN;
    TouchCalibMaxY = TOUCH_CALIB_Y_MAX;
  }
  else
  {
    TouchCalibMinX = minX;
    TouchCalibMaxX = maxX;
    TouchCalibMinY = minY;
    TouchCalibMaxY = maxY;
  }
}

void ILI948x::GetLastRawTouch(int& rawX, int& rawY)
{
  // Returns last (good) raw values, debounced/filtered
  rawX = _LastRawX;
  rawY = _LastRawY;
}

#else
bool ILI948x::GetTouch(int& , int& )
{
  return false;
}
bool ILI948x::Checkouch()
{
  return false;
}
void ILI948x::GetLastRawTouch(int& , int& )
{
}
void ILI948x::SetTouchCalib(int , int , int , int )
{
}

#endif

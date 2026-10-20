#pragma once

// *** ORIENTATION ***
// Orientation is Landscape right (USB bottom right) UNLESS one of these is defined:
#define LCD_PORTRAIT_BOT    // Portrait, USB connector at bottom
//#define LCD_PORTRAIT_TOP    // Portrait, USB connector at top
//#define LCD_LANDSCAPE_LEFT  // Landscape, USB connector at top left

// *** ADJUSTMENTS ***
// Try defining this if the display is jumbled.
#define LCD_FIXED_ORIGIN
#ifdef LCD_PORTRAIT_TOP
// The AliExpress large touch LCD's seem to have the origin shifted left 1 pixel?
#define LCD_X_OFFSET 1
// shifted up too?
#define LCD_Y_OFFSET 1
#endif

// *** TOUCH ***
// AliExpress large touch LCD
#define LCD_HAS_TOUCH

// optionally dump graphics cmds to serial:
//#define SERIALISE
#ifdef SERIALISE
#define SERIALISE_ON(_on) ILI948x::_serialise=_on;
#define SERIALISE_COMMENT(_c) if (ILI948x::_serialise) { Serial.print("; ");Serial.println(_c);}
#else
#define SERIALISE_ON(_on)
#define SERIALISE_COMMENT(_c)
#endif

#if defined(LCD_PORTRAIT_BOT) || defined(LCD_PORTRAIT_TOP)
#define LCD_WIDTH  320
#define LCD_HEIGHT 480
#else
#define LCD_WIDTH  480
#define LCD_HEIGHT 320
#endif

#define LCD_RD_BIT   B00000001  // A0
#define LCD_WR_BIT   B00000010
#define LCD_RS_BIT   B00000100
#define LCD_CS_BIT   B00001000
#define LCD_RST_BIT  B00010000  // A4

#define LCD_RD_PIN   A0 // Read
#define LCD_WR_PIN   A1 // Write  
#define LCD_RS_PIN   A2 // Register select (data/cmd)
#define LCD_CS_PIN   A3 // Chip select    
#define LCD_RST_PIN  A4
// BUTTON            A5

// SD card not used
#define SD_SCK_PIN   13 // LED_BUILTIN
#define SD_DO_PIN    12 // SDA
#define SD_DI_PIN    11 // SCL
#define SD_SS_PIN    10 // BUZZER

#define RGB_565(_r, _g, _b) (word)(((_b & 0x00F8) >> 3) | ((_g & 0x00FC) << 3) | ((_r & 0x00F8) << 8))

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
// Accelerate the LCD updates by transforming the distribution of bits across the three registers
#define LCD_TRANSFORM
#endif

#ifdef LCD_TRANSFORM
// reorder 0bQQXYPPZZ -> 
//         0bYZZQQPPX 
// Reg    (0bGHHHHEEE)
// PORTE = 0b__PPX___
// PORTG = 0b__Y_____
// PORTH = 0b_ZZQQ___
#define LCD_TRANSFORM_BYTE(_b) ((((_b) & 0b00010000) << 3) | (((_b) & 0b00001100) >> 1) | (((_b) & 0b00000011) << 5) | (((_b) & 0b11000000) >> 3) | (((_b) & 0b00100000) >> 5))
#define RGB(_r, _g, _b) (word(LCD_TRANSFORM_BYTE(RGB_565(_r, _g, _b)) | (LCD_TRANSFORM_BYTE(RGB_565(_r, _g, _b) >> 8) << 8)))
#else
#define LCD_TRANSFORM_BYTE(_b) (_b)
#define RGB(_r, _g, _b) RGB_565(_r, _g, _b)
#endif

class ILI948x
{
  public:
    static void Init();
    static void DisplayOn();
    static void Cmd(byte cmd);
    static void Cmd_T(byte cmd);
    static void DataByte(byte data);
    static void DataWord(word data);
    static void ColourWord(word colour, unsigned long count);
    static void ColourByte(byte colour, unsigned long count);
    static unsigned long Window(word x,word y,word w,word h);
    static byte m_MADCTL0x36;

    static bool GetTouch(int& x, int& y);
    static bool CheckTouch();
    static void GetLastRawTouch(int& rawX, int& rawY);
    static void SetTouchCalib(int minX, int maxX, int minY, int maxY);

#ifdef SERIALISE
     static bool _serialise;
#endif
    
private:
    static const byte initialisation[];
};

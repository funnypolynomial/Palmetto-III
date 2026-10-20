#include <EEPROM.h>
#include "Database.h"

namespace Database
{
  void Read(uint16_t& idx, uint8_t* pData, size_t Size)
  {
    // read Size bytes from idx into pData, update idx
    while (Size--)
      *pData++ = EEPROM.read(idx++);
  }

  void Write(uint16_t& idx, uint8_t* pData, size_t Size)
  {
    // write Size bytes from pDate to idx, update idx
    while (Size--)
      EEPROM.write(idx++, *pData++);
  }
};

#pragma once
// Just a simple layer onto EEPROM for persistent storage
namespace Database
{
  // Reads Size EEPROM bytes, starting at EEPROM[idx] into pData. Increments idx
  void Read(uint16_t& idx, uint8_t* pData, size_t Size);
  // Writes Size EEPROM bytes, starting at EEPROM[idx] from pData. Increments idx
  void Write(uint16_t& idx, uint8_t* pData, size_t Size);
};


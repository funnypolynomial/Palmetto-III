#pragma once
// Performs time calculations
namespace Time
{
  static const uint32_t kSecondsPerDay = 24UL*60UL*60UL;
  // Returns days in the month (1..12) of the year (1900..2099)
  uint8_t DaysInMonth(uint8_t month, uint16_t yearYYYY);
  // Returns days in the year (1900..2099)
  uint16_t DaysInYear(uint16_t yearYYYY);
  // Returns the time as seconds since midnight
  uint32_t TimeAsSeconds(uint8_t hour24, uint8_t minute, uint8_t second);
  // Returns the seconds to the date from the reference
  uint32_t SecondsSinceReference(uint16_t yearYYYY, uint8_t month, uint8_t date);
}

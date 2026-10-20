#include <Arduino.h>
#include "Time.h"

namespace Time
{
  // Ref is Jan 1 1900 00:00. uint32_t Seconds spans ~136 years
  const int kRefYear = 1900;
  const int kRefMonth = 1;
  const int kRefDate = 1;

  uint8_t DaysInMonth(uint8_t month, uint16_t yearYYYY)  // 1..12, 1900..2099
  {
    // Return days in the month, of the yearYYYY
    // <Days-in-month>-28 encoded as 2bits/month:
    //                             0b00DDNN...          ...FFJJ00
    //                                 eeoo                eeaa
    //                                 ccvv                bbnn    
    const uint32_t MONTH_LENGTHS = 0b0011101110111110111011001100UL;
    bool leapYear = (!(yearYYYY % 4) && (yearYYYY % 100)) || !(yearYYYY % 400);
    int Increment = (MONTH_LENGTHS >> (month*2) & 0x03);
    if (!leapYear || Increment)
      return 28 + Increment;
    else
      return 29;// Leap year AND Feb
  }

  uint16_t DaysInYear(uint16_t yearYYYY)  // 1900..2099
  {
    // Days in the yearYYYY
    return 337 + DaysInMonth(2, yearYYYY);
  }

  uint32_t TimeAsSeconds(uint8_t hour24, uint8_t minute, uint8_t second)
  {
    // Convert time to seconds after midnight
    return (hour24*60UL + minute)*60UL + second;
  }

  uint32_t SecondsSinceReference(uint16_t yearYYYY, uint8_t month, uint8_t date)
  {
    // Return the number of seconds since the reference date
    uint32_t days = 0;
    // whole days
    while (date-- > kRefDate)
      days++;
    // whole months
    while (month-- > kRefMonth)
      days += DaysInMonth(month, yearYYYY);
    // whole years
    while (yearYYYY-- > kRefYear)
      days += DaysInYear(yearYYYY);
    return kSecondsPerDay*days;
  }
}

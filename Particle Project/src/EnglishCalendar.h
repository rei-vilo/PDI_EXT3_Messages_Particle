///
/// @file		EnglishCalendar.h
/// @brief		Header
/// @details	Constants for English date
/// @n
/// @n @b		Project PDI_EXT3_Messages_Particle
///
/// @author     Rei Vilo
/// @author     https://embeddedcomputing.weebly.com
/// @date       03 Mar 2021
/// @version    201
///
/// @copyright  (c) Rei Vilo, 2020
/// @copyright  CC = BY SA NC
///


// Core library for code-sense - IDE-based
#if (ENERGIA) // LaunchPad specific
#include "Energia.h"
#else // General case
#include "Arduino.h"
#endif // end IDE


#ifndef ENGLISH_CALENDAR_RELEASE
///
/// @brief	Release
///
#define ENGLISH_CALENDAR_RELEASE

const String LocalDay[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const String LocalMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};



#endif // ENGLISH_CALENDAR_RELEASE

/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2008 SYSRA
   
   EyeDB is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   EyeDB is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA 
*/

/*
   Author: Laurent Pereira
*/


#ifndef _CALENDAR_CONVERTER_H
#define _CALENDAR_CONVERTER_H

// forward declarations
#include <eyedb/eyedb.h>

namespace eyedb {

  /**
   * An utility class that performs some calendar conversion 
   * from and to the julian day.
   *
   * This is an abstract class.
   */
  class CalendarConverter
  {
  public :

    /**
     * Computes the calendar date (year, month, day) from the julian day.
     * @param julian The julian day
     * @param year The returned year
     * @param month The returned month
     * @param day The returned day of month
     */
    virtual void jday2calendar(const eyedblib::int32 julian, eyedblib::int32 * year, eyedblib::int16 * month, eyedblib::int16 * day) = 0;

    /**
     * Computes the julian day from a calendar date (year, month, day) 
     * @param julian The returned julian day or 0 if the date is invalid.
     * @param year The year
     * @param month The month
     * @param day The day of month
     */
    virtual void calendar2jday(eyedblib::int32 * julian, const eyedblib::int32 year, const eyedblib::int16 month, const eyedblib::int16 day) = 0;

    /**
     * Computes the day of year from the julian day.
     * @param julian The julian day
     * @return The day of year
     */
    virtual eyedblib::int16 jday2day_of_year(const eyedblib::int32 julian) = 0;

    /**
     * Check if a year is a leap year from the julian day.
     * @param julian The julian day
     * @return True if is a leap year, idbfalse otherwise
     */
    virtual Bool jday2leap_year(const eyedblib::int32 julian) = 0;

    /**
     * Computes the week day (monday, tuesday...) from the julian day.
     * @param julian The julian day
     * @return The week day
     */
    virtual Weekday::Type jday2weekday(const eyedblib::int32 julian) = 0;

    /**
     * Computes the julian day from an ASCII date. 
     * @param date The ASCII date in ISO format "YYYY-MM-DD"
     * @returns The julian day or 0 if date was not valid.
     */
    virtual eyedblib::int32 ascii2jday(const char * date) = 0;

    /**
     * Computes an ASCII date from the julian day.
     * @param julian The julian day
     * @return The ASCII date in ISO format "YYY-MM-DD". It must be
     *         deallocated (delete) by the client.
     */
    virtual char * jday2ascii(const eyedblib::int32 julian) = 0;

    /**
     * Gets the current julian day.
     */
    virtual eyedblib::int32 current_date() = 0;

  };
}

#endif

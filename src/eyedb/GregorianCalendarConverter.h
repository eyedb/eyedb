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


#ifndef _GREGORIAN_CALENDAR_CONVERTER_H
#define _GREGORIAN_CALENDAR_CONVERTER_H

#include <eyedb/CalendarConverter.h>

namespace eyedb {

  /**
   * An utility class that performs some calendar conversion 
   * from and to the julian day.
   */
  class GregorianCalendarConverter : public CalendarConverter
  {
  private:
    char * string_date;
    eyedblib::int32 tmp_year;
    eyedblib::int16 tmp_month;
    eyedblib::int16 tmp_day;
  
  public :

    ///
    GregorianCalendarConverter();
    ///
    ~GregorianCalendarConverter();
  

    /**
     * Computes the calendar date (year, month, day) from the julian day.
     * The conversion algorithm comes from Hermetic Systems at
     * http://hermetic.magnet.ch/cal_stud/jdn.htm (Henry F. Fliegel 
     * and Thomas C. Van Flandern).
     * This method is not thread-safe.
     * @param julian The julian day (negative values will result in -4713-11-25)
     * @param year The returned year ( > -4713)
     * @param month The returned month (1-12)
     * @param day The returned day of month (1-31)
     */
    virtual void jday2calendar(const eyedblib::int32 julian, eyedblib::int32 * year, eyedblib::int16 * month, eyedblib::int16 * day);

    /**
     * Computes the julian day from a calendar date (year, month, day).
     * The conversion algorithm comes from Hermetic Systems at
     * http://hermetic.magnet.ch/cal_stud/jdn.htm (Henry F. Fliegel 
     * and Thomas C. Van Flandern). 
     * This method is not thread-safe.
     * @param julian The returned julian day or -1 if the date is invalid
     * @param year The year ( > -4713)
     * @param month The month
     * @param day The day of month
     */
    virtual void calendar2jday(eyedblib::int32 * julian, const eyedblib::int32 year, const eyedblib::int16 month, const eyedblib::int16 day);

    /// This method is not thread-safe.
    virtual eyedblib::int16 jday2day_of_year(const eyedblib::int32 julian);

    /// This method is not thread-safe.
    virtual Bool jday2leap_year(const eyedblib::int32 julian);

    virtual Weekday::Type jday2weekday(const eyedblib::int32 julian);

  
    /// This method is not thread-safe.
    virtual eyedblib::int32 ascii2jday(const char * date);

    /// This method is not thread-safe.
    virtual char * jday2ascii(const eyedblib::int32 julian);

    ///
    virtual eyedblib::int32 current_date();

  };

}

#endif

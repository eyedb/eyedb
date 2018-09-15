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



#ifndef _EYEDB_CLOCK_CONVERTER_H
#define _EYEDB_CLOCK_CONVERTER_H

#include <eyedb/eyedb.h>

namespace eyedb {

  /**
   * An utility class that performs some clock conversion
   * from and to the usec (microsecond) time.
   *
   * This is an abstract class.
   */
  class ClockConverter
  {
  
  private:
    char * string_clock;
    char * string_zone;


  public:

    ClockConverter();
    virtual ~ClockConverter();




    /**
     * Computes the clock time from the usec time.
     * @param usec The elapsed microseconds since 00:00h
     * @param hour The returned hour (0->23)
     * @param min The returned minute (0->60)
     * @param sec The returned second (0->60)
     * @param ms The returned millisecond (0->999)
     * @param us The returned microsecond (0->999)
     */
    void usec2clock(const eyedblib::int64 usec, eyedblib::int16 * hour = 0, eyedblib::int16 * min = 0, eyedblib::int16 *sec = 0, eyedblib::int16 * ms = 0, eyedblib::int16 *us = 0);
    /**
     * Computes the usec time from the clock.
     * @param usec The elapsed microseconds since 00:00h
     * @param hour The hour (0->23)
     * @param min The minute (0->60)
     * @param sec The second (0->60)
     * @param ms The millisecond (0->999)
     * @param us The microsecond (0->999)
     */
    void clock2usec(eyedblib::int64 *usec, const eyedblib::int16 hour = 0, const eyedblib::int16 min = 0, const eyedblib::int16 sec = 0, const eyedblib::int16 ms = 0, const eyedblib::int16 us = 0);

  
    /**
     * Convert an usec time into days.
     * @param usec The time in microsecond
     * @return The number of days into usec
     */
    eyedblib::int32 usec2day(eyedblib::int64 usec);


    /**
     * Computes timezone (minute) -> (hour,minute).
     */
    void tz2clock(const eyedblib::int16 tz, eyedblib::int16 * hour, eyedblib::int16 * min);

    /**
     * Computes (hour, minute) -> timezone (minute)
     */
    void clock2tz(eyedblib::int16 * tz, const eyedblib::int16 hour, const eyedblib::int16 min);


    /**
     * Convert a string time into an usec time.
     * This method is not thread-safe.
     * @param t The string time in the format HH:MM[:ss[,mmm[,uuu]]]
     * @return The usec time or 0 if the string is not valid.
     */
    eyedblib::int64 ascii2usec(const char * t);

    /**
     * Convert an usec time in ascii format (same as in ascii2usec).
     * The client must deallocate (delete) the returned string.
     */
    char * usec2ascii(const eyedblib::int64 usec);


    /**
     * Convert a string into a timezone value.
     * This method is not thread-safe.
     * @param tz The string timezone in the format GMT(+|-)HH[:MM]
     * @return The timezone value or 0 if the string is not valid.
     */
    eyedblib::int16 ascii2tz(const char * tz);

    /**
     * Convert a timezone value in ascii format (same as in ascii2tz).
     * The client must deallocate (delete) the returned string.
     */
    char * tz2ascii(const eyedblib::int16 tz);




    /**
     * Gets the current GMT time
     */
    eyedblib::int64 current_time();
 
  
    /**
     * Gets the local timezone.
     * @return The local timezone in minutes
     */
    eyedblib::int16 local_timezone();

  };


  //
  // Constants
  //

  extern const eyedblib::int64 USEC_OF_DAY;
  extern const eyedblib::int64 USEC_OF_HOUR;
  extern const eyedblib::int32 USEC_OF_MINUTE;
  extern const eyedblib::int32 USEC_OF_SECOND;
  extern const eyedblib::int16 USEC_OF_MILLISECOND;
  extern const eyedblib::int16 MAX_TZ;
  extern const eyedblib::int16 MIN_TZ;
}

#endif

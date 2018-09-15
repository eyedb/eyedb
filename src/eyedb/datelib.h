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


#ifndef _DATELIB_H
#define _DATELIB_H

namespace eyedb {

  //forward declarations
  class CalendarConverter;
  class ClockConverter;

  /**
   * This class is a repository for date conversion algorithms.
   *
   * This is a singleton OO pattern.
   */

  class DateAlgorithmRepository
  {
  private:
    CalendarConverter * t_cal;
    ClockConverter * t_clock;

  public:
    DateAlgorithmRepository();
    virtual ~DateAlgorithmRepository();

    /**
     * Get the unique instance of this class.
     * This method is not thread-safe.
     */
    static const DateAlgorithmRepository& instance();

    /**
     * Get the default CalendarConverter.
     * This method is not thread-safe.
     */
    static CalendarConverter * getDefaultCalendarConverter();
    /**
     * This method is not thread-safe.
     * Get the default ClockConverter
     */
    static ClockConverter * getDefaultClockConverter();
  };

}

#endif

/* 
   EyeDB Object Database Management System
   Copyright (C) 1994-2018 SYSRA
   
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



#include "CalendarConverter.h"
#include "datelib.h"
#include "eyedb/internals/ObjectPeer.h"

namespace eyedb {
  //
  // Constants
  //

  const eyedblib::int32 MIN_JULIAN = 0;
  const eyedblib::int32 MAX_JULIAN = 211202371;

  //
  // Date user methods
  //

  void
  Date::userCopy(const Object &)
  {
    setClientData();
  }

  void
  Date::userInitialize()
  {
    setClientData();
  }

  void
  Date::setClientData()
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    char * s_date = cal->jday2ascii(getJulian());  
    strcpy(this->string_date, s_date);
  
    delete [] s_date;
  }



  eyedblib::int32
  Date::year() const
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int32 y;
    cal->jday2calendar(getJulian(), &y, 0, 0);

    return y;
  }

  eyedblib::int16
  Date::month() const 
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int16 m;
    cal->jday2calendar(getJulian(), 0, &m, 0);

    return m;
  }

  eyedblib::int16
  Date::day() const
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int16 d;
    cal->jday2calendar(getJulian(), 0, 0, &d);

    return d;
  }


  eyedblib::int16
  Date::day_of_year() const
  {

    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    return cal->jday2day_of_year(getJulian());
  }

  Month::Type
  Date::month_of_year() const
  {
    return Month::Type(month());
  }

  Weekday::Type
  Date::day_of_week() const
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    return cal->jday2weekday(getJulian());
  }

  Bool
  Date::is_leap_year() const
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    return cal->jday2leap_year(getJulian());
  }

  Bool
  Date::is_equal(const Date & d) const
  {
  
    if(getJulian() == d.getJulian())
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Date::is_greater(const Date & d) const
  {

  
    if(getJulian() > d.getJulian())
      {
	return True;
      }
    else
      {
	return False;
      }

  }

  Bool
  Date::is_greater_or_equal(const Date & d) const 
  {
    if(getJulian() >= d.getJulian())
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Date::is_less(const Date & d) const
  {
    if(getJulian() < d.getJulian())
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Date::is_less_or_equal(const Date & d) const
  {
    if(getJulian() <= d.getJulian())
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Date::is_between(const Date & d1, const Date & d2) const
  {
    eyedblib::int32 j = getJulian();

    if( ( j > d1.getJulian()
	  && j < d2.getJulian() )
	|| ( j > d2.getJulian()
	     && j < d1.getJulian()) )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Date &
  Date::next(Weekday::Type day)
  {
    Weekday::Type today = day_of_week();

    eyedblib::int16 diff = day - today;
    if( diff < 0 )
      {
	diff = 7 + diff;
      }

    set_julian(getJulian() + diff);

    return *this;
  }

  Date &
  Date::previous(Weekday::Type day)
  {
    Weekday::Type today = day_of_week();

    eyedblib::int16 diff = day - today;
    if( diff > 0 )
      {
	diff = diff - 7;
      }

    set_julian(getJulian() + diff);


    return *this;
  }



  Date &
  Date::add_days(eyedblib::int32 days)
  {
    set_julian(getJulian() + days);

    return *this;
  }

  Date &
  Date::substract_days(eyedblib::int32 days)
  {

    set_julian(getJulian() - days); 

    return *this;
  }

  eyedblib::int32
  Date::substract_date(const Date & d) const
  {
    return (getJulian() - d.getJulian());
  }

  Status
  Date::set_julian(eyedblib::int32 julian_day)
  {

    if(julian_day < MIN_JULIAN
       || julian_day > MAX_JULIAN)
      {
	return Exception::make(IDB_ERROR, "date out of range");
      }

    Status s = setJulian(julian_day);

    // update client data
    setClientData();

    return s;
  }

  Date *
  Date::date(Database * db)
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();
  
    Date * d = new Date(db);
    d->set_julian(cal->current_date());

    return d;
  }

  Date *
  Date::date(Database * db, const Date & date)
  {
    Date * d = new Date(db);
    d->set_julian(date.getJulian());
    return  d;
  }

  Date * 
  Date::date(Database * db, eyedblib::int32 julian_day)
  {
    Date * d = new Date(db);
    d->set_julian(julian_day);

    return  d;
  }

  Date * 
  Date::date(Database * db, eyedblib::int32 year, Month::Type m, eyedblib::int16 day)
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();
  
    Date * d = new Date(db);
    eyedblib::int32 julian = 0;
    cal->calendar2jday(&julian, year, m, day);
    d->set_julian(julian);
    return  d;
  }

  Date *
  Date::date(Database * db, const char * s)
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();
  
    Date * d = new Date(db);
    d->set_julian(cal->ascii2jday(s));


    return d;
  }

  const char *
  Date::toString() const 
  {
    return this->string_date;
  }

 
  eyedblib::int32
  Date::julian(const char * d)
  {
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();
    return cal->ascii2jday(d);

  }

}

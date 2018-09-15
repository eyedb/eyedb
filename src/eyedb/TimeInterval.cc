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

#include <eyedb/syscls.h>
#include "ClockConverter.h"
#include "datelib.h"

namespace eyedb {
  //
  // TimeInterval user methods
  //

  void
  TimeInterval::userCopy(const Object &)
  {
    setClientData();
  }

  void
  TimeInterval::userInitialize()
  {
    setClientData();
  }

  void
  TimeInterval::setClientData()
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    char * s_time = clock->usec2ascii(getUsecs());

    strcpy(string_time_interval, s_time);

    delete [] s_time;
  }


  eyedblib::int32
  TimeInterval::day() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    return clock->usec2day(getUsecs());
  }

  eyedblib::int16
  TimeInterval::hour() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    clock->usec2clock( getUsecs(), &hour);

    return hour % 24;
  }

  eyedblib::int16
  TimeInterval::minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    clock->usec2clock( getUsecs(), 0, &min);

    return min;
  }

  eyedblib::int16
  TimeInterval::second() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 sec = 0;
    clock->usec2clock( getUsecs(), 0, 0, &sec);

    return sec;
  }

  eyedblib::int16
  TimeInterval::millisecond() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 ms = 0;
    clock->usec2clock( getUsecs(), 0, 0, 0, &ms);

    return ms;
  }


  eyedblib::int16
  TimeInterval::microsecond() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 us = 0;
    clock->usec2clock( getUsecs(), 0, 0, 0, 0, &us);

    return us;
  }


  Bool
  TimeInterval::is_zero() const
  {
    if( getUsecs() == 0 ) 
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  TimeInterval &
  TimeInterval::plus(const TimeInterval & i)
  {
    set_usecs( getUsecs() + i.getUsecs() );

    return *this;
  }

  TimeInterval &
  TimeInterval::minus(const TimeInterval & i)
  {
    set_usecs( getUsecs() - i.getUsecs() );

    return *this;
  }

  TimeInterval &
  TimeInterval::product(eyedblib::int64 val)
  {
    set_usecs( getUsecs() * val );

    return *this;
  }

  TimeInterval &
  TimeInterval::quotient(eyedblib::int64 val)
  {
    set_usecs( getUsecs() / val );

    return *this;
  }

  Bool
  TimeInterval::is_equal(const TimeInterval & i) const
  {
    if( getUsecs() == i.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  TimeInterval::is_greater(const TimeInterval & i) const
  {
    if( getUsecs() > i.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  TimeInterval::is_greater_or_equal(const TimeInterval & i) const
  {
    if( getUsecs() >= i.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  TimeInterval::is_less(const TimeInterval & i) const
  {
    if( getUsecs() < i.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  TimeInterval::is_less_or_equal(const TimeInterval & i) const
  {
    if( getUsecs() <= i.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Status
  TimeInterval::set_usecs(eyedblib::int64 usecs)
  {
    Status s = setUsecs(usecs);
  
    if(!s)
      {
	setClientData();
      }

    return s;
  }

  TimeInterval *
  TimeInterval::time_interval(Database * db, eyedblib::int64 usecs)
  {
    TimeInterval * i = new TimeInterval(db);
    i->set_usecs(usecs);

    return i;
  }

  TimeInterval *
  TimeInterval::time_interval(Database * db, eyedblib::int32 day, eyedblib::int16 hours, eyedblib::int16 min, eyedblib::int16 sec, eyedblib::int16 msec, eyedblib::int16 usec)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    TimeInterval * i = new TimeInterval(db);

    eyedblib::int64 usecs = 0;
    eyedblib::int64 total_hours = day * 24 + hours;
    clock->clock2usec(&usecs, total_hours, min, sec, msec, usec);

    i->set_usecs(usecs);
   
    return i;
  }

  const char *
  TimeInterval::toString() const
  {
    return string_time_interval;
  }

}

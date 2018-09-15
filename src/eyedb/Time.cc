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

#include "eyedb/syscls.h"
#include "ClockConverter.h"
#include "datelib.h"

//
// Utility functions
//

namespace eyedb {

  eyedblib::int64
  gmt2local_time(eyedblib::int64 gmt_time, eyedblib::int16 time_zone)
  {
  
    eyedblib::int64 local_time = gmt_time + (eyedblib::int64)(time_zone) * USEC_OF_MINUTE;

    local_time = local_time % USEC_OF_DAY;

    if( local_time < 0 )
      {
	local_time = USEC_OF_DAY + local_time;
      }

    return local_time;
  }


  void parse_time(const char * time, eyedblib::int64 * gmt_usec, eyedblib::int16 * timezone)
  {

    // separate the string in 2 tokens : time and timezone
    char * s = strdup(time);

    char * s_usec = strtok(s, " ");
    char * s_tz = strtok(0, " ");

    // parse the 2 tokens

    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int64 local_usec = clock->ascii2usec(s_usec);

    if( s_tz == 0 )
      {
	*timezone = clock->local_timezone();
      }
    else
      {
	*timezone = clock->ascii2tz(s_tz);
      }

    // convert local time to gmt time


    *gmt_usec = local_usec - ((eyedblib::int64)(*timezone) * USEC_OF_MINUTE);


    *gmt_usec = *gmt_usec % USEC_OF_DAY;

    if( *gmt_usec < 0 )
      {
	*gmt_usec = USEC_OF_DAY + *gmt_usec;
      }
  


    free(s);
  }



  //
  // Time Methods
  //

  void
  Time::userCopy(const Object &)
  {
    setClientData();
  }

  void
  Time::userInitialize()
  {
    setClientData();
  }

  void
  Time::setClientData()
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
  
    char * s_clock = clock->usec2ascii( gmt2local_time(getUsecs(), getTz()) );
    char * s_zone = clock->tz2ascii(getTz());
  
    this->string_time[0] = '\0';
    strcat(this->string_time, s_clock);
    strcat(this->string_time, " ");
    strcat(this->string_time, s_zone);

    delete [] s_clock;
    delete [] s_zone;
  }



  eyedblib::int16
  Time::hour() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    eyedblib::int64 local_time = gmt2local_time(getUsecs(), getTz());
    clock->usec2clock( local_time, &hour);

    return hour;
  }

  eyedblib::int16
  Time::minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    eyedblib::int64 local_time = gmt2local_time(getUsecs(), getTz());
    clock->usec2clock( local_time, 0, &min);

    return min;
  }

  eyedblib::int16
  Time::second() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 sec = 0;
    clock->usec2clock(getUsecs(), 0, 0, &sec);

    return sec;
  }

  eyedblib::int16
  Time::millisecond() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 ms = 0;
    clock->usec2clock(getUsecs(), 0, 0, 0, &ms);

    return ms;
  }

  eyedblib::int16
  Time::tz_hour() const
  {

    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    clock->tz2clock(getTz(), &hour, 0);

    return hour;
  }

  eyedblib::int16
  Time::tz_minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    clock->tz2clock(getTz(), 0, &min);

    return min;

  }

  Bool
  Time::is_equal(const Time & t) const
  {
    if( getUsecs() == t.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Time::is_greater(const Time & t) const
  {
    if( getUsecs() > t.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Time::is_greater_or_equal(const Time & t) const
  {
    if( getUsecs() >= t.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Time::is_less(const Time & t) const
  {
    if( getUsecs() < t.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  Time::is_less_or_equal(const Time & t) const
  {
    if( getUsecs() <= t.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }

  }

  Bool
  Time::is_between(const Time & t1, const Time &t2) const
  {
    eyedblib::int64 usecs = getUsecs();
  
    if( ( usecs > t1.getUsecs() 
	  && usecs < t2.getUsecs() )
	|| ( usecs < t1.getUsecs()
	     && usecs > t2.getUsecs() ) )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Time &
  Time::add_interval(const TimeInterval & i)
  {
    eyedblib::int64 usecs = getUsecs() + i.getUsecs();
  
    usecs = usecs % USEC_OF_DAY;  

    if( usecs < 0 )
      {
	usecs = USEC_OF_DAY + usecs;
      }

    set_usecs(usecs, getTz());


    return *this;
  }

  Time &
  Time::substract_interval(const TimeInterval & i)
  {
    eyedblib::int64 usecs = getUsecs() - i.getUsecs();
  
    usecs = usecs % USEC_OF_DAY;

    if( usecs < 0 )
      {
	usecs = USEC_OF_DAY + usecs;
      }
  
    set_usecs(usecs, getTz());

    return *this;
  }

  TimeInterval *
  Time::substract_time(const Time & t)
  {
    eyedblib::int64 interval =  getUsecs() - t.getUsecs();

    if( interval < 0 )
      {
	interval = USEC_OF_DAY + interval;
      }

    return  TimeInterval::time_interval(getDatabase(), interval);
  }

  eyedblib::int16
  Time::gmt_hour() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    clock->usec2clock(getUsecs(), &hour);

    return hour;
  }

  eyedblib::int16
  Time::gmt_minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    clock->usec2clock(getUsecs(), 0, &min);

    return min;
  }

  eyedblib::int16
  Time::microsecond() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 us = 0;
    clock->usec2clock(getUsecs(), 0, 0, 0, 0, &us);

    return us;
  }

  Status
  Time::set_usecs(eyedblib::int64 usecs, eyedblib::int16 tz)
  {
    if(usecs < 0
       || usecs >= USEC_OF_DAY)
      {
	return Exception::make(IDB_ERROR, "time out of range");
      }
    if(tz < MIN_TZ
       || tz > MAX_TZ)
      {
	return Exception::make(IDB_ERROR, "time_zone out of range");
      }

    Status s;
    s = setUsecs(usecs);

    // update client data
    setClientData();

  
    if(s)
      {
	return s;
      }

    s = setTz(tz);

    // update client data
    setClientData();

    return s;
  }

  void
  Time::get_local_time_zone(eyedblib::int16 * tz_hour, eyedblib::int16 * tz_min)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    clock->tz2clock( clock->local_timezone(), tz_hour, tz_min);
  }

  Time *
  Time::gmt_time(Database * db)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);
    t->set_usecs(clock->current_time(), 0);

    return t;
  }

  Time *
  Time::local_time(Database * db)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);
    t->set_usecs(clock->current_time(), clock->local_timezone());

    return t;
  }

  Time *
  Time::time(Database * db, const Time & time)
  { 
    Time * t = new Time(db);
    t->set_usecs(time.getUsecs(), time.getTz());

    return t;
  }

  Time *
  Time::time(Database * db, eyedblib::int64 usec)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);
    t->set_usecs(usec, clock->local_timezone());
  
    return t;
  }

  Time *
  Time::time(Database * db, eyedblib::int64 usec, eyedblib::int16 tz)
  {
    Time * t = new Time(db);
    t->set_usecs(usec, tz);

    return t;
  }

  Time *
  Time::time(Database * db, eyedblib::int16 hours, eyedblib::int16 min, eyedblib::int16 sec, eyedblib::int16 msec)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);

    eyedblib::int64 usec_time = 0;
    clock->clock2usec(&usec_time, hours, min, sec, msec, 0);
    t->set_usecs(usec_time, clock->local_timezone());
  
    return t;

  }


  Time *
  Time::time(Database * db, eyedblib::int16 hours, eyedblib::int16 min, eyedblib::int16 sec, eyedblib::int16 msec,  eyedblib::int16 usec)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);

    eyedblib::int64 usec_time = 0;
    clock->clock2usec(&usec_time, hours, min, sec, msec, usec);
    t->set_usecs(usec_time, clock->local_timezone());
  
    return t;

  }



  Time *
  Time::time(Database * db, eyedblib::int16 hours, eyedblib::int16 min, eyedblib::int16 sec, eyedblib::int16 msec, eyedblib::int16 tz_hour, eyedblib::int16 tz_minute)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);

    eyedblib::int64 usec_time = 0;
    eyedblib::int16 tz = 0;
    clock->clock2usec(&usec_time, hours, min, sec, msec, 0);
    clock->clock2tz(&tz, tz_hour, tz_minute);

    t->set_usecs(usec_time, tz);
  
    return t;

  }

  Time *
  Time::time(Database * db, eyedblib::int16 hours, eyedblib::int16 min, eyedblib::int16 sec, eyedblib::int16 msec, eyedblib::int16 usec, eyedblib::int16 tz_hour, eyedblib::int16 tz_minute)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    Time * t = new Time(db);

    eyedblib::int64 usec_time = 0;
    eyedblib::int16 tz = 0;
    clock->clock2usec(&usec_time, hours, min, sec, msec, usec);
    clock->clock2tz(&tz, tz_hour, tz_minute);

    t->set_usecs(usec_time, tz);
  
    return t;

  }

  Time *
  Time::time(Database * db, const char * s)
  {
    eyedblib::int64 gmt_usec = 0;
    eyedblib::int16 timezone = 0;

    parse_time(s, &gmt_usec, &timezone);

    Time * t = new Time(db);
    t->set_usecs(gmt_usec, timezone);

    return t;
  }

  const char *
  Time::toString() const
  {
    return this->string_time;
  }

  eyedblib::int64
  Time::usec_time(const char * t)
  {
    eyedblib::int64 gmt_usec;
    eyedblib::int16 timezone;

    parse_time(t, &gmt_usec, &timezone);

    return gmt_usec;
  }

}

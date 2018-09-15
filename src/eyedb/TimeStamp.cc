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
#include "CalendarConverter.h"
#include "datelib.h"

namespace eyedb {

  //
  // vars
  //

  std::string timeStamp_string;

  //
  // Utility functions
  //



  eyedblib::int64
  gmt2local_timeStamp(eyedblib::int64 gmt_time, eyedblib::int16 time_zone, eyedblib::int16 * day_offset = 0)
  {

    //
    // Computes the local time
    //
    eyedblib::int64 local_time = (gmt_time % USEC_OF_DAY) + (eyedblib::int64)(time_zone) * USEC_OF_MINUTE;

    //
    // Computes the day offset
    //
    if( day_offset != 0 )
      {
	if( local_time >= USEC_OF_DAY )
	  {
	    *day_offset = 1;
	  }
	else if ( local_time < 0 )
	  {
	    *day_offset = -1;
	  }
	else
	  {
	    *day_offset = 0;
	  }
      }

  
    //
    // Give local time a valid value ( [0, USEC_OF_DAY[ )
    //
    local_time = local_time % USEC_OF_DAY;

    if( local_time < 0 )
      {
        local_time = USEC_OF_DAY + local_time;    
      }


    return local_time;
  }

  void parse_timeStamp(const char * time, eyedblib::int64 * gmt_usec, eyedblib::int16 * timezone)
  {

    // separate the string in 3 tokens : date, time and timezone
    char * s = strdup(time);

    char * s_julian = strtok(s, " ");
    char * s_usec = strtok(0, " ");
    char * s_tz = strtok(0, " ");


    // parse the tokens

    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int32 julian = cal->ascii2jday(s_julian);
  
    eyedblib::int64 local_usec = 0;
    if( s_usec != 0 )
      {
	local_usec = clock->ascii2usec(s_usec);
      }

    if( s_tz == 0 )
      {
	*timezone = clock->local_timezone();
      }
    else
      {
	*timezone = clock->ascii2tz(s_tz);
      }



    // convert local time to gmt time


    *gmt_usec = julian * USEC_OF_DAY +
      local_usec - ((eyedblib::int64)(*timezone) * USEC_OF_MINUTE);
 
  


    free(s);
  }



  //
  // TimeStamp methods
  //


  void
  TimeStamp::userCopy(const Object &)
  {
    setClientData();
  }

  void
  TimeStamp::userInitialize()
  {
    setClientData();
  }

  void
  TimeStamp::setClientData()
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    //
    // set the string value
    //


    // preliminary 
    eyedblib::int16 day_offset = 0;
    eyedblib::int64 local_time = gmt2local_timeStamp(getUsecs(), getTz(), &day_offset);
    eyedblib::int32 local_julian = clock->usec2day(getUsecs()) + day_offset;

    char * s_date = cal->jday2ascii( local_julian );
    char * s_time = clock->usec2ascii( local_time );
    char * s_zone = clock->tz2ascii(getTz());

    // convert to string

    this->string_time_stamp[0] = '\0';
    strcat(this->string_time_stamp, s_date);
    strcat(this->string_time_stamp, " ");
    strcat(this->string_time_stamp, s_time);
    strcat(this->string_time_stamp, " ");
    strcat(this->string_time_stamp, s_zone);

    // release temporary data
    delete [] s_date;
    delete [] s_time;
    delete [] s_zone;
  
    //
    // Set date and time values
    //  

    ts_date.set_julian( clock->usec2day(getUsecs()) );
    ts_time.set_usecs(getUsecs() % USEC_OF_DAY, getTz());

 
  }



  const Date &
  TimeStamp::date() const
  {
    return ts_date;
  }

  const Time &
  TimeStamp::time() const
  {
    return ts_time;
  }


  eyedblib::int32
  TimeStamp::year() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int32 julian = clock->usec2day(getUsecs());

    eyedblib::int32 year = 0;
    cal->jday2calendar(julian, &year, 0, 0);

    return  year;
  }

  eyedblib::int16
  TimeStamp::month() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int32 julian = clock->usec2day(getUsecs());

    eyedblib::int16 month = 0;
    cal->jday2calendar(julian, 0, &month, 0);

    return  month;
  }

  eyedblib::int16
  TimeStamp::day() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    eyedblib::int32 julian = clock->usec2day(getUsecs());

    eyedblib::int16 day = 0;
    cal->jday2calendar(julian, 0, 0, &day);

    return  day;

  }

  eyedblib::int16
  TimeStamp::hour() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    eyedblib::int64 local_time = gmt2local_timeStamp(getUsecs(), getTz());
    clock->usec2clock(local_time, &hour);

    return hour;
  }

  eyedblib::int16
  TimeStamp::minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    eyedblib::int64 local_time = gmt2local_timeStamp(getUsecs(), getTz());
    clock->usec2clock(local_time, 0, &min);

    return min;
  }

  eyedblib::int16
  TimeStamp::second() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 sec = 0;
    clock->usec2clock(getUsecs(), 0, 0, &sec);

    return sec;
  }

  eyedblib::int16
  TimeStamp::millisecond() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 ms = 0;
    clock->usec2clock(getUsecs(), 0, 0, 0, &ms);

    return ms;
  }

  eyedblib::int16
  TimeStamp::tz_hour() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    clock->tz2clock(getTz(), &hour, 0);

    return hour;
  }

  eyedblib::int16
  TimeStamp::tz_minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    clock->tz2clock(getTz(), 0, &min);

    return min;
  }

  TimeStamp &
  TimeStamp::plus(const TimeInterval & i)
  {
    set_usecs(getUsecs() + i.getUsecs(), getTz());

    return *this;
  }

  TimeStamp &
  TimeStamp::minus(const TimeInterval & i)
  {
    set_usecs(getUsecs() - i.getUsecs(), getTz());

    return *this;
  }

  Bool
  TimeStamp::is_equal(const TimeStamp & ts) const
  {
    if( getUsecs() == ts.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  TimeStamp::is_greater(const TimeStamp & ts) const
  {
    if( getUsecs() > ts.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
    return True;
  }

  Bool
  TimeStamp::is_greater_or_equal(const TimeStamp & ts) const
  {
    if( getUsecs() >= ts.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
    return True;
  }

  Bool
  TimeStamp::is_less(const TimeStamp & ts) const
  {
    if( getUsecs() < ts.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
    return True;
  }

  Bool
  TimeStamp::is_less_or_equal(const TimeStamp & ts) const
  {
    if( getUsecs() <= ts.getUsecs() )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  Bool
  TimeStamp::is_between(const TimeStamp & ts1, const TimeStamp & ts2) const
  {
    eyedblib::int64 usec = getUsecs();

    if( (usec < ts1.getUsecs()
	 && usec > ts2.getUsecs() ) 
	|| ( usec > ts1.getUsecs()
	     && usec < ts2.getUsecs() ) )
      {
	return True;
      }
    else
      {
	return False;
      }
  }

  eyedblib::int16
  TimeStamp::gmt_hour() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 hour = 0;
    clock->usec2clock(getUsecs() % USEC_OF_DAY, &hour);

    return hour;
  }

  eyedblib::int16
  TimeStamp::gmt_minute() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 min = 0;
    clock->usec2clock(getUsecs(), 0, &min);

    return min;
  }

  eyedblib::int16
  TimeStamp::microsecond() const
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    eyedblib::int16 us = 0;
    clock->usec2clock(getUsecs(), 0, 0, 0, 0, &us);

    return us;
  }

  Status
  TimeStamp::set_usecs(eyedblib::int64 usec, eyedblib::int16 tz)
  {
  
    if(usec < 0)
      {
	return Exception::make("time_stamp out of range");
      }
    if(tz < MIN_TZ
       || tz > MAX_TZ)
      {
	return Exception::make("time_zone out of range");
      }

    Status s;
    s = setUsecs(usec);  

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



  TimeInterval *
  TimeStamp::substract(const TimeStamp & ts)
  {
    return TimeInterval::time_interval(getDatabase(), getUsecs() - ts.getUsecs());
 
  }

  TimeStamp *
  TimeStamp::gmt_time_stamp(Database * db)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(clock->current_time() + cal->current_date() * USEC_OF_DAY, 0);

    return ts;
  }

  TimeStamp *
  TimeStamp::local_time_stamp(Database * db)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();
    CalendarConverter * cal = DateAlgorithmRepository::getDefaultCalendarConverter();

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(clock->current_time() + cal->current_date() * USEC_OF_DAY, 
		  clock->local_timezone());

    return ts;

  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, const TimeStamp & time_stamp)
  {
  
    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(time_stamp.getUsecs(), ts->getTz());


    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, eyedblib::int64 usec)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(usec, clock->local_timezone());

    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, eyedblib::int64 usec, eyedblib::int16 tz)
  {
    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(usec, tz);

    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, eyedblib::int32 julian_day, eyedblib::int64 usec)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(usec + julian_day * USEC_OF_DAY, 
		  clock->local_timezone());
 

    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, eyedblib::int32 julian_day, eyedblib::int64 usec, eyedblib::int16 tz)
  {
    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(usec + julian_day * USEC_OF_DAY, 
		  tz);

    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, const Date & d, const Time & t)
  {
    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(t.getUsecs() + d.getJulian() * USEC_OF_DAY, 
		  t.getTz());
 

    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, const Date & d)
  {
    ClockConverter * clock = DateAlgorithmRepository::getDefaultClockConverter();

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(d.getJulian() * USEC_OF_DAY, 
		  clock->local_timezone());
 
    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, const Date & d, eyedblib::int16 tz_hour, eyedblib::int16 tz_min)
  {

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(d.getJulian() * USEC_OF_DAY, 
		  tz_hour * 60 + tz_min);


    return ts;
  }

  TimeStamp *
  TimeStamp::time_stamp(Database * db, const char * s)
  {
    eyedblib::int64 gmt_usec = 0;
    eyedblib::int16 timezone = 0;

    parse_timeStamp(s, &gmt_usec, &timezone);

    TimeStamp * ts = new TimeStamp(db);
    ts->set_usecs(gmt_usec, timezone);

    return ts;
  }

  const char *
  TimeStamp::toString() const
  {
    return this->string_time_stamp;
  }

  eyedblib::int64
  TimeStamp::usec_time_stamp(const char * ts)
  {
    eyedblib::int64 gmt_usec;
    eyedblib::int16 timezone;

    parse_timeStamp(ts, &gmt_usec, &timezone);

    return gmt_usec;
  }
}

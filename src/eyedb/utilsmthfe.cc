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
   Author: Eric Viara <viara@sysra.com>
*/


#include <eyedb/eyedb.h>

//
// Date & OString contribution
//

using namespace eyedb;

#define PACK_INIT(_db)

#define IdbDate_c(X) ((Date *)(X))
#define IdbTime_c(X) ((Time *)(X))
#define IdbTimeStamp_c(X) ((TimeStamp *)(X))
#define IdbTimeInterval_c(X) ((TimeInterval *)(X))
#define IdbOString_c(X) ((OString *)(X))

#//
// int32 Date::year() [datemthfe.cc]
//

Status
__method__OUT_int32_year_date(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->year();

  return Success;
}

//
// int16 Date::month() [datemthfe.cc]
//

Status
__method__OUT_int16_month_date(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->month();

  return Success;
}

//
// int16 Date::day() [datemthfe.cc]
//

Status
__method__OUT_int16_day_date(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->day();

  return Success;
}

//
// int16 Date::day_of_year() [datemthfe.cc]
//

Status
__method__OUT_int16_day_of_year_date(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->day_of_year();

  return Success;
}

//
// month Date::month_of_year() [datemthfe.cc]
//

Status
__method__OUT_month_month_of_year_date(Database *_db, FEMethod_C *_m, Object *_o, Month::Type &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = (Month::Type)date->month_of_year();

  return Success;
}

//
// weekday Date::day_of_week() [datemthfe.cc]
//

Status
__method__OUT_weekday_day_of_week_date(Database *_db, FEMethod_C *_m, Object *_o, Weekday::Type &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->day_of_week();

  return Success;
}

//
// int16 Date::is_leap_year() [datemthfe.cc]
//

Status
__method__OUT_int16_is_leap_year_date(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_leap_year();

  return Success;
}

//
// int16 Date::is_equal(in date*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_equal_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_equal(*d);

  return Success;
}

//
// int16 Date::is_greater(in date*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_greater(*d);

  return Success;
}

//
// int16 Date::is_greater_or_equal(in date*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_or_equal_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_greater_or_equal(*d);

  return Success;
}

//
// int16 Date::is_less(in date*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_less(*d);

  return Success;
}

//
// int16 Date::is_less_or_equal(in date*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_or_equal_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_less_or_equal(*d);

  return Success;
}

//
// int16 Date::is_between(in date*, in date*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_between_date__IN_date_REF___IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d1, Date * d2, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->is_between(*d1, *d2);

  return Success;
}

//
// date* Date::next(in weekday) [datemthfe.cc]
//

Status
__method__OUT_date_REF__next_date__IN_weekday(Database *_db, FEMethod_C *_m, Object *_o, const Weekday::Type day, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  date->next(day);
  retarg = date;

  return Success;
}

//
// date* Date::previous(in weekday) [datemthfe.cc]
//

Status
__method__OUT_date_REF__previous_date__IN_weekday(Database *_db, FEMethod_C *_m, Object *_o, const Weekday::Type day, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  Date * date = IdbDate_c(_o);
  date->previous(day);
  retarg = date;

  return Success;
}

//
// date* Date::add_days(in int32) [datemthfe.cc]
//

Status
__method__OUT_date_REF__add_days_date__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 days, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  date->add_days(days);
  retarg = date;

  return Success;
}

//
// date* Date::substract_days(in int32) [datemthfe.cc]
//

Status
__method__OUT_date_REF__substract_days_date__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 days, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //


  Date * date = IdbDate_c(_o);
  date->substract_days(days);
  retarg = date;

  return Success;

}

//
// int32 Date::substract_date(in date*) [datemthfe.cc]
//

Status
__method__OUT_int32_substract_date_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Object *_o, Date * d, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);
  retarg = date->substract_date(*d);

  return Success;

}

//
// void Date::set_julian(in int32) [datemthfe.cc]
//

Status
__method__OUT_void_set_julian_date__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 julian_day)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Date * date = IdbDate_c(_o);

  Status s;
  s = date->set_julian(julian_day);

  if(s)
    {
      return s;
    }

  return Success;
}

//
// date* Date::date() [datemthfe.cc]
//

Status
__method_static_OUT_date_REF__date_date(Database *_db, FEMethod_C *_m, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Date::date(_db);

  return Success;
}

//
// date* Date::date(in date*) [datemthfe.cc]
//

Status
__method_static_OUT_date_REF__date_date__IN_date_REF_(Database *_db, FEMethod_C *_m, Date * d, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Date::date(_db, *d);

  return Success;
}

//
// date* Date::date(in int32) [datemthfe.cc]
//

Status
__method_static_OUT_date_REF__date_date__IN_int32(Database *_db, FEMethod_C *_m, const eyedblib::int32 julian_day, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Date::date(_db, julian_day);

  return Success;
}

//
// date* Date::date(in int32, in month, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_date_REF__date_date__IN_int32__IN_month__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int32 year, const Month::Type m, const eyedblib::int16 day, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Date::date(_db, year, m, day);

  return Success;
}

//
// date* Date::date(in string) [datemthfe.cc]
//

Status
__method_static_OUT_date_REF__date_date__IN_string(Database *_db, FEMethod_C *_m, const char * d, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Date::date(_db, d);

  return Success;
}

//
// string Date::toString() [datemthfe.cc]
//

Status
__method__OUT_string_toString_date(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  
  Date * date = IdbDate_c(_o);
  
  retarg = Argument::dup(date->toString());

  return Success;
}

//
// int32 Date::julian(in string) [datemthfe.cc]
//

Status
__method_static_OUT_int32_julian_date__IN_string(Database *_db, FEMethod_C *_m, const char * d, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Date::julian(d);

  return Success;
}

//
// int16 Time::hour() [datemthfe.cc]
//

Status
__method__OUT_int16_hour_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->hour();

  return Success;
}

//
// int16 Time::minute() [datemthfe.cc]
//

Status
__method__OUT_int16_minute_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->minute();

  return Success;
}

//
// int16 Time::second() [datemthfe.cc]
//

Status
__method__OUT_int16_second_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->second();

  return Success;
}

//
// int16 Time::millisecond() [datemthfe.cc]
//

Status
__method__OUT_int16_millisecond_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->millisecond();

  return Success;
}

//
// int16 Time::tz_hour() [datemthfe.cc]
//

Status
__method__OUT_int16_tz_hour_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->tz_hour();

  return Success;
}

//
// int16 Time::tz_minute() [datemthfe.cc]
//

Status
__method__OUT_int16_tz_minute_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->tz_minute();

  return Success;
}

//
// int16 Time::is_equal(in time*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_equal_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->is_equal(*t);
 
  return Success;
}

//
// int16 Time::is_greater(in time*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->is_greater(*t);
 
  return Success;
}

//
// int16 Time::is_greater_or_equal(in time*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_or_equal_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->is_greater_or_equal(*t);
 
  return Success;
}

//
// int16 Time::is_less(in time*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->is_less(*t);
 
  return Success;
}

//
// int16 Time::is_less_or_equal(in time*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_or_equal_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->is_less_or_equal(*t);
 
  return Success;
}

//
// int16 Time::is_between(in time*, in time*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_between_time__IN_time_REF___IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t1, Time * t2, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->is_between(*t1, *t2);

  return Success;
}

//
// time* Time::add_interval(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_time_REF__add_interval_time__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  time->add_interval(*i);
  
  retarg = time;

  return Success;
}

//
// time* Time::substract_interval(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_time_REF__substract_interval_time__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  time->substract_interval(*i);
  
  retarg = time;

  return Success;
}

//
// time_interval* Time::substract_time(in time*) [datemthfe.cc]
//

Status
__method__OUT_time_interval_REF__substract_time_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Object *_o, Time * t, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->substract_time(*t);

  return Success;
}

//
// int16 Time::gmt_hour() [datemthfe.cc]
//

Status
__method__OUT_int16_gmt_hour_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->gmt_hour();

  return Success;
}

//
// int16 Time::gmt_minute() [datemthfe.cc]
//

Status
__method__OUT_int16_gmt_minute_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->gmt_minute();

  return Success;
}

//
// int16 Time::microsecond() [datemthfe.cc]
//

Status
__method__OUT_int16_microsecond_time(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = time->microsecond();

  return Success;
}

//
// void Time::set_usecs(in int64, in int16) [datemthfe.cc]
//

Status
__method__OUT_void_set_usecs_time__IN_int64__IN_int16(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int64 usecs, const eyedblib::int16 tz)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  
  Time * time = IdbTime_c(_o);
  
  Status s;
  s = time->set_usecs(usecs, tz);
  
  if(s)
    {
      return s;
    }

  return Success;
}

//
// void Time::get_local_time_zone(out int16, out int16) [datemthfe.cc]
//

Status
__method_static_OUT_void_get_local_time_zone_time__OUT_int16__OUT_int16(Database *_db, FEMethod_C *_m, eyedblib::int16 &tz_hour, eyedblib::int16 &tz_min)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time::get_local_time_zone(&tz_hour, &tz_min);

  return Success;
}

//
// time* Time::gmt_time() [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__gmt_time_time(Database *_db, FEMethod_C *_m, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::gmt_time(_db);

  return Success;
}

//
// time* Time::local_time() [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__local_time_time(Database *_db, FEMethod_C *_m, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::local_time(_db);

  return Success;
}

//
// time* Time::time(in time*) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_time_REF_(Database *_db, FEMethod_C *_m, Time * t, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, *t);

  return Success;
}

//
// time* Time::time(in int64) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_int64(Database *_db, FEMethod_C *_m, const eyedblib::int64 usec, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, usec);

  return Success;
}

//
// time* Time::time(in int64, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_int64__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int64 usec, const eyedblib::int16 tz, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, usec, tz);

  return Success;
}

//
// time* Time::time(in int16, in int16, in int16, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_int16__IN_int16__IN_int16__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int16 hours, const eyedblib::int16 min, const eyedblib::int16 sec, const eyedblib::int16 msec, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, hours, min, sec, msec);

  return Success;
}

//
// time* Time::time(in int16, in int16, in int16, in int16, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int16 hours, const eyedblib::int16 min, const eyedblib::int16 sec, const eyedblib::int16 msec, const eyedblib::int16 usec, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, hours, min, sec, msec, usec);

  return Success;
}

//
// time* Time::time(in int16, in int16, in int16, in int16, in int16, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int16 hours, const eyedblib::int16 min, const eyedblib::int16 sec, const eyedblib::int16 msec, const eyedblib::int16 tz_hour, const eyedblib::int16 tz_minute, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, hours, min, sec, msec, tz_hour, tz_minute);

  return Success;
}

//
// time* Time::time(in int16, in int16, in int16, in int16, in int16, in int16, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int16 hours, const eyedblib::int16 min, const eyedblib::int16 sec, const eyedblib::int16 msec, const eyedblib::int16 usec, const eyedblib::int16 tz_hour, const eyedblib::int16 tz_minute, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, hours, min, sec, msec, usec, tz_hour, tz_minute);

  return Success;
}

//
// time* Time::time(in string) [datemthfe.cc]
//

Status
__method_static_OUT_time_REF__time_time__IN_string(Database *_db, FEMethod_C *_m, const char * t, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = Time::time(_db, t);

  return Success;
}

//
// string Time::toString() [datemthfe.cc]
//

Status
__method__OUT_string_toString_time(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  Time * time = IdbTime_c(_o);
  retarg = Argument::dup(time->toString());

  return Success;
}

//
// int64 Time::usec_time(in string) [datemthfe.cc]
//

Status
__method_static_OUT_int64_usec_time_time__IN_string(Database *_db, FEMethod_C *_m, const char * t, eyedblib::int64 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  
  retarg = Time::usec_time(t);

  return Success;
}

//
// date* TimeStamp::date() [datemthfe.cc]
//

Status
__method__OUT_date_REF__date_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, Date * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = Date::date(_db, time_stamp->date());

  return Success;
}

//
// time* TimeStamp::time() [datemthfe.cc]
//

Status
__method__OUT_time_REF__time_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, Time * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = Time::time(_db, time_stamp->time());

  return Success;
}

//
// int32 TimeStamp::year() [datemthfe.cc]
//

Status
__method__OUT_int32_year_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->year();

  return Success;
}

//
// int16 TimeStamp::month() [datemthfe.cc]
//

Status
__method__OUT_int16_month_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->month();

  return Success;
}

//
// int16 TimeStamp::day() [datemthfe.cc]
//

Status
__method__OUT_int16_day_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->day();

  return Success;
}

//
// int16 TimeStamp::hour() [datemthfe.cc]
//

Status
__method__OUT_int16_hour_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->hour();

  return Success;
}

//
// int16 TimeStamp::minute() [datemthfe.cc]
//

Status
__method__OUT_int16_minute_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->minute();

  return Success;
}

//
// int16 TimeStamp::second() [datemthfe.cc]
//

Status
__method__OUT_int16_second_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->second();

  return Success;
}

//
// int16 TimeStamp::millisecond() [datemthfe.cc]
//

Status
__method__OUT_int16_millisecond_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->millisecond();

  return Success;
}

//
// int16 TimeStamp::tz_hour() [datemthfe.cc]
//

Status
__method__OUT_int16_tz_hour_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->tz_hour();

  return Success;
}

//
// int16 TimeStamp::tz_minute() [datemthfe.cc]
//

Status
__method__OUT_int16_tz_minute_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->tz_minute();

  return Success;
}

//
// time_stamp* TimeStamp::plus(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_time_stamp_REF__plus_time_stamp__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  time_stamp->plus(*i);
  retarg = time_stamp;

  return Success;
}

//
// time_stamp* TimeStamp::minus(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_time_stamp_REF__minus_time_stamp__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  time_stamp->minus(*i);
  retarg = time_stamp;

  return Success;
}

//
// int16 TimeStamp::is_equal(in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_equal_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->is_equal(*ts);

  return Success;
}

//
// int16 TimeStamp::is_greater(in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->is_greater(*ts);

  return Success;
}

//
// int16 TimeStamp::is_greater_or_equal(in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_or_equal_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->is_greater_or_equal(*ts);

  return Success;
}

//
// int16 TimeStamp::is_less(in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->is_less(*ts);

  return Success;
}

//
// int16 TimeStamp::is_less_or_equal(in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_or_equal_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->is_less_or_equal(*ts);

  return Success;
}

//
// int16 TimeStamp::is_between(in time_stamp*, in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_between_time_stamp__IN_time_stamp_REF___IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts1, TimeStamp * ts2, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->is_between(*ts1, ts2);

  return Success;
}

//
// int16 TimeStamp::gmt_hour() [datemthfe.cc]
//

Status
__method__OUT_int16_gmt_hour_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->gmt_hour();

  return Success;
}

//
// int16 TimeStamp::gmt_minute() [datemthfe.cc]
//

Status
__method__OUT_int16_gmt_minute_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->gmt_minute();

  return Success;
}

//
// int16 TimeStamp::microsecond() [datemthfe.cc]
//

Status
__method__OUT_int16_microsecond_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->microsecond();

  return Success;
}

//
// void TimeStamp::set_usecs(in int64, in int16) [datemthfe.cc]
//

Status
__method__OUT_void_set_usecs_time_stamp__IN_int64__IN_int16(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int64 usec, const eyedblib::int16 tz)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);

  Status s;
  s = time_stamp->set_usecs(usec, tz);

  if(s)
    {
      return s;
    }

  return Success;
}

//
// time_interval* TimeStamp::substract(in time_stamp*) [datemthfe.cc]
//

Status
__method__OUT_time_interval_REF__substract_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeStamp * ts, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = time_stamp->substract(*ts);

  return Success;
}

//
// time_stamp* TimeStamp::gmt_time_stamp() [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__gmt_time_stamp_time_stamp(Database *_db, FEMethod_C *_m, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::gmt_time_stamp(_db);

  return Success;
}

//
// time_stamp* TimeStamp::local_time_stamp() [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__local_time_stamp_time_stamp(Database *_db, FEMethod_C *_m, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::local_time_stamp(_db);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in time_stamp*) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_time_stamp_REF_(Database *_db, FEMethod_C *_m, TimeStamp * ts, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, *ts);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in int64) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_int64(Database *_db, FEMethod_C *_m, const eyedblib::int64 usec, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, usec);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in int64, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_int64__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int64 usec, const eyedblib::int16 tz, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, usec, tz);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in int32, in int64) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_int32__IN_int64(Database *_db, FEMethod_C *_m, const eyedblib::int32 julian_day, const eyedblib::int64 usec, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, julian_day, usec);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in int32, in int64, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_int32__IN_int64__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int32 julian_day, const eyedblib::int64 usec, const eyedblib::int16 tz, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, julian_day, usec, tz);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in date*, in time*) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_date_REF___IN_time_REF_(Database *_db, FEMethod_C *_m, Date * d, Time * t, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, *d, *t);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in date*) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_date_REF_(Database *_db, FEMethod_C *_m, Date * d, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, *d);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in date*, in int16, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_date_REF___IN_int16__IN_int16(Database *_db, FEMethod_C *_m, Date * d, const eyedblib::int16 tz_hour, const eyedblib::int16 tz_min, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, *d, tz_hour, tz_min);

  return Success;
}

//
// time_stamp* TimeStamp::time_stamp(in string) [datemthfe.cc]
//

Status
__method_static_OUT_time_stamp_REF__time_stamp_time_stamp__IN_string(Database *_db, FEMethod_C *_m, const char * ts, TimeStamp * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::time_stamp(_db, ts);

  return Success;
}

//
// string TimeStamp::toString() [datemthfe.cc]
//

Status
__method__OUT_string_toString_time_stamp(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeStamp * time_stamp = IdbTimeStamp_c(_o);
  retarg = Argument::dup(time_stamp->toString());

  return Success;
}

//
// int64 TimeStamp::usec_time_stamp(in string) [datemthfe.cc]
//

Status
__method_static_OUT_int64_usec_time_stamp_time_stamp__IN_string(Database *_db, FEMethod_C *_m, const char * ts, eyedblib::int64 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeStamp::usec_time_stamp(ts);

  return Success;
}

//
// int32 TimeInterval::day() [datemthfe.cc]
//

Status
__method__OUT_int32_day_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->day();
  
  return Success;
}

//
// int16 TimeInterval::hour() [datemthfe.cc]
//

Status
__method__OUT_int16_hour_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->hour();

  return Success;
}

//
// int16 TimeInterval::minute() [datemthfe.cc]
//

Status
__method__OUT_int16_minute_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->minute();

  return Success;
}

//
// int16 TimeInterval::second() [datemthfe.cc]
//

Status
__method__OUT_int16_second_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->second();

  return Success;
}

//
// int16 TimeInterval::millisecond() [datemthfe.cc]
//

Status
__method__OUT_int16_millisecond_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->millisecond();

  return Success;
}

//
// int16 TimeInterval::microsecond() [datemthfe.cc]
//

Status
__method__OUT_int16_microsecond_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->microsecond();

  return Success;
}

//
// int16 TimeInterval::is_zero() [datemthfe.cc]
//

Status
__method__OUT_int16_is_zero_time_interval(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->is_zero();

  return Success;
}

//
// time_interval* TimeInterval::plus(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_time_interval_REF__plus_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  time_interval->plus(*i);
  
  retarg = time_interval;

  return Success;
}

//
// time_interval* TimeInterval::minus(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_time_interval_REF__minus_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  time_interval->minus(*i);
  
  retarg = time_interval;

  return Success;
}

//
// time_interval* TimeInterval::product(in int64) [datemthfe.cc]
//

Status
__method__OUT_time_interval_REF__product_time_interval__IN_int64(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int64 val, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  time_interval->product(val);
  
  retarg = time_interval;

  return Success;
}

//
// time_interval* TimeInterval::quotient(in int64) [datemthfe.cc]
//

Status
__method__OUT_time_interval_REF__quotient_time_interval__IN_int64(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int64 val, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  time_interval->quotient(val);
  
  retarg = time_interval;

  return Success;
}

//
// int16 TimeInterval::is_equal(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_equal_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->is_equal(*i);

  return Success;
}

//
// int16 TimeInterval::is_greater(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->is_greater(*i);

  return Success;
}

//
// int16 TimeInterval::is_greater_or_equal(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_greater_or_equal_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->is_greater_or_equal(*i);

  return Success;
}

//
// int16 TimeInterval::is_less(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->is_less(*i);

  return Success;
}

//
// int16 TimeInterval::is_less_or_equal(in time_interval*) [datemthfe.cc]
//

Status
__method__OUT_int16_is_less_or_equal_time_interval__IN_time_interval_REF_(Database *_db, FEMethod_C *_m, Object *_o, TimeInterval * i, eyedblib::int16 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = time_interval->is_less_or_equal(*i);

  return Success;
}

//
// void TimeInterval::set_usecs(in int64) [datemthfe.cc]
//

Status
__method__OUT_void_set_usecs_time_interval__IN_int64(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int64 usecs)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);

  Status s;
  s = time_interval->set_usecs(usecs);

  if(s)
    {
      return s;
    }

  return Success;
}

//
// time_interval* TimeInterval::time_interval(in int64) [datemthfe.cc]
//

Status
__method_static_OUT_time_interval_REF__time_interval_time_interval__IN_int64(Database *_db, FEMethod_C *_m, const eyedblib::int64 usecs, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeInterval::time_interval(_db, usecs);

  return Success;
}

//
// time_interval* TimeInterval::time_interval(in int32, in int16, in int16, in int16, in int16, in int16) [datemthfe.cc]
//

Status
__method_static_OUT_time_interval_REF__time_interval_time_interval__IN_int32__IN_int16__IN_int16__IN_int16__IN_int16__IN_int16(Database *_db, FEMethod_C *_m, const eyedblib::int32 day, const eyedblib::int16 hours, const eyedblib::int16 min, const eyedblib::int16 sec, const eyedblib::int16 msec, const eyedblib::int16 usec, TimeInterval * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = TimeInterval::time_interval(_db, day, hours, min, sec, msec, usec);

  return Success;
}

//
// string TimeInterval::toString() [datemthfe.cc]
//

Status
__method__OUT_string_toString_time_interval(Database *_db, FEMethod_C *_m, Object *_o, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  TimeInterval * time_interval = IdbTimeInterval_c(_o);
  retarg = Argument::dup(time_interval->toString());

  return Success;
}

//
// ostring* OString::ostring() [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring(Database *_db, FEMethod_C *_m, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db);

  return Success;
}

//
// ostring* OString::ostring(in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_string(Database *_db, FEMethod_C *_m, const char * s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, s);

  return Success;
}

//
// ostring* OString::ostring(in string, in int32) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, const char * s, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, s, len);

  return Success;
}

//
// ostring* OString::ostring(in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, const char * s, const eyedblib::int32 offset, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, s, offset, len);

  return Success;
}

//
// ostring* OString::ostring(in ostring*) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_ostring_REF_(Database *_db, FEMethod_C *_m, OString * s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, *s);

  return Success;
}

//
// ostring* OString::ostring(in char) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_char(Database *_db, FEMethod_C *_m, const char s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, s);

  return Success;
}

//
// ostring* OString::ostring(in int32) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_int32(Database *_db, FEMethod_C *_m, const eyedblib::int32 s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, s);

  return Success;
}

//
// ostring* OString::ostring(in float) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__ostring_ostring__IN_float(Database *_db, FEMethod_C *_m, const double s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::ostring(_db, s);

  return Success;
}

//
// int32 OString::strlen(in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_int32_strlen_ostring__IN_string(Database *_db, FEMethod_C *_m, const char * s, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = strlen(s);

  return Success;
}

//
// int32 OString::strcmp(in string, in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_int32_strcmp_ostring__IN_string__IN_string(Database *_db, FEMethod_C *_m, const char * s1, const char * s2, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  
  retarg = strcmp(s1, s2);

  return Success;
}

//
// int32 OString::strstr(in string, in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_int32_strstr_ostring__IN_string__IN_string(Database *_db, FEMethod_C *_m, const char * s1, const char * s2, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  
  char * match = (char *)strstr(s1, s2);

  if( match == 0 )
    {
      retarg = -1;
    }
  else
    {
      retarg = match - s1;
    }

  return Success;
}

//
// string OString::substr(in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method_static_OUT_string_substr_ostring__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, const char * s, const eyedblib::int32 offset, const eyedblib::int32 len, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //  
  
  char * sub = OString::substr(s, offset, len);
  retarg = Argument::dup(sub);
  
  delete sub;

  return Success;
}

//
// string OString::toLower(in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_string_toLower_ostring__IN_string(Database *_db, FEMethod_C *_m, const char * s, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  char * s2  = OString::toLower(s);
  retarg = Argument::dup(s2);

  delete s2;

  return Success;
}

//
// string OString::toUpper(in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_string_toUpper_ostring__IN_string(Database *_db, FEMethod_C *_m, const char * s, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  char * s2  = OString::toUpper(s);
  retarg = Argument::dup(s2);

  delete s2;

  return Success;
}

//
// string OString::rtrim(in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_string_rtrim_ostring__IN_string(Database *_db, FEMethod_C *_m, const char * s, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  char * s2 = OString::rtrim(s);
  retarg = Argument::dup(s2);
  
  delete s2;

  return Success;
}

//
// string OString::ltrim(in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_string_ltrim_ostring__IN_string(Database *_db, FEMethod_C *_m, const char * s, char * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  char * s2 = OString::ltrim(s);
  retarg = Argument::dup(s2);
  
  delete s2;

  return Success;
}

//
// ostring* OString::concat(in string, in string) [ostringmthfe.cc]
//

Status
__method_static_OUT_ostring_REF__concat_ostring__IN_string__IN_string(Database *_db, FEMethod_C *_m, const char * s1, const char * s2, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  retarg = OString::concat(_db, s1, s2);

  return Success;
}



//
// ostring* OString::append(in string) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__append_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->append(s);

  retarg = os;

  return Success;
}

//
// ostring* OString::append(in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__append_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //


  OString * os = IdbOString_c(_o);

  os->append(s, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::append(in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__append_ostring__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 offset, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->append(s, offset, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::prepend(in string) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__prepend_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->prepend(s);

  retarg = os;

  return Success;
}

//
// ostring* OString::prepend(in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__prepend_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->prepend(s, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::prepend(in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__prepend_ostring__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 offset, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->prepend(s, offset, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::insert(in int32, in string) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__insert_ostring__IN_int32__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const char * s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->insert(offset, s);

  retarg = os;

  return Success;
}

//
// ostring* OString::insert(in int32, in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__insert_ostring__IN_int32__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const char * s, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->insert(offset, s, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::insert(in int32, in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__insert_ostring__IN_int32__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const char * s, const eyedblib::int32 offset2, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->insert(offset, s, offset2, len);

  retarg = os;

  return Success;
}

//
// int32 OString::first(in string) [ostringmthfe.cc]
//

Status
__method__OUT_int32_first_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * s, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->first(s);

  return Success;
}

//
// int32 OString::last(in string) [ostringmthfe.cc]
//

Status
__method__OUT_int32_last_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * arg1, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  return Exception::make(IDB_ERROR, "Not implemented");
}

//
// int32 OString::find(in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_int32_find_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 offset, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->find(s, offset);

  return Success;
}

//
// ostring* OString::substr(in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__substr_ostring__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->substr(offset, len);
  retarg->setDatabase(_db);

  return Success;
}

//
// ostring* OString::substr(in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__substr_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * regexp, const eyedblib::int32 offset, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->substr(regexp, offset);
  retarg->setDatabase(_db);

  return Success;
}

//
// ostring* OString::erase(in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__erase_ostring__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->erase(offset, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::replace(in int32, in int32, in string) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__replace_ostring__IN_int32__IN_int32__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const eyedblib::int32 len, const char * s, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->replace(offset, len, s);
  
  retarg = os;
  
  return Success;
}

//
// ostring* OString::replace(in int32, in int32, in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__replace_ostring__IN_int32__IN_int32__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const eyedblib::int32 len, const char * s, const eyedblib::int32 len2, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->replace(offset, len, s, len2);
  
  retarg = os;

  return Success;
}

//
// ostring* OString::replace(in int32, in int32, in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__replace_ostring__IN_int32__IN_int32__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const eyedblib::int32 offset, const eyedblib::int32 len, const char * s, const eyedblib::int32 offset2, const eyedblib::int32 len2, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->replace(offset, len, s, offset2, len2);
  
  retarg = os;

  return Success;
}

//
// ostring* OString::replace(in string, in string) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__replace_ostring__IN_string__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * s1, const char * s2, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->replace(s1, s2);
  
  retarg = os;

  return Success;
}

//
// void OString::reset() [ostringmthfe.cc]
//

Status
__method__OUT_void_reset_ostring(Database *_db, FEMethod_C *_m, Object *_o)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);
  os->reset();

  return Success;
}

//
// ostring* OString::assign(in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__assign_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->assign(s, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::assign(in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__assign_ostring__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 offset, const eyedblib::int32 len, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->assign(s, offset, len);

  retarg = os;

  return Success;
}

//
// ostring* OString::toLower() [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__toLower_ostring(Database *_db, FEMethod_C *_m, Object *_o, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);
  
  os->toLower();

  retarg = os;

  return Success;
}

//
// ostring* OString::toUpper() [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__toUpper_ostring(Database *_db, FEMethod_C *_m, Object *_o, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);
  
  os->toUpper();

  retarg = os;

  return Success;
}

//
// ostring* OString::rtrim() [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__rtrim_ostring(Database *_db, FEMethod_C *_m, Object *_o, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //
  
  OString * os = IdbOString_c(_o);

  os->rtrim();

  retarg = os;

  return Success;
}

//
// ostring* OString::ltrim() [ostringmthfe.cc]
//

Status
__method__OUT_ostring_REF__ltrim_ostring(Database *_db, FEMethod_C *_m, Object *_o, OString * &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  os->ltrim();

  retarg = os;

  return Success;
}

//
// int32 OString::compare(in string) [ostringmthfe.cc]
//

Status
__method__OUT_int32_compare_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * s, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->compare(s);
  
  return Success;
}

//
// int32 OString::compare(in string, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_int32_compare_ostring__IN_string__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 to, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->compare(s, to);

  return Success;
}

//
// int32 OString::compare(in string, in int32, in int32) [ostringmthfe.cc]
//

Status
__method__OUT_int32_compare_ostring__IN_string__IN_int32__IN_int32(Database *_db, FEMethod_C *_m, Object *_o, const char * s, const eyedblib::int32 from, const eyedblib::int32 to, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->compare(s, from, to);

  return Success;
}

//
// int32 OString::is_null() [ostringmthfe.cc]
//

Status
__method__OUT_int32_is_null_ostring(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->is_null();

  return Success;
}

//
// int32 OString::match(in string) [ostringmthfe.cc]
//

Status
__method__OUT_int32_match_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * regexp, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);

  retarg = os->match(regexp);
 
  return Success;
}

//
// int32 OString::length() [ostringmthfe.cc]
//

Status
__method__OUT_int32_length_ostring(Database *_db, FEMethod_C *_m, Object *_o, eyedblib::int32 &retarg)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * o = IdbOString_c(_o);
  retarg = o->length();

  return Success;
}

//
// string[] OString::split(in string) [ostringmthfe.cc]
//

Status
__method__OUT_string_ARRAY__split_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * separator, char * * &retarg, int &retarg_cnt)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);
  
  int token_count = 0;
  
  char ** tokens = os->split(separator, token_count);

  retarg = Argument::dup(tokens, token_count);
  retarg_cnt = token_count;

  for(int i=0; i < token_count; ++i)
    {
      delete tokens[i];
    }
  delete tokens;

  return Success;
}

//
// string[] OString::regexp_split(in string) [ostringmthfe.cc]
//

Status
__method__OUT_string_ARRAY__regexp_split_ostring__IN_string(Database *_db, FEMethod_C *_m, Object *_o, const char * regexp_separator, char * * &retarg, int &retarg_cnt)
{
  PACK_INIT(_db);

  //
  // User Implementation
  //

  OString * os = IdbOString_c(_o);
  
  int token_count = 0;
  
  char ** tokens = os->regexp_split(regexp_separator, token_count);

  retarg = Argument::dup(tokens, token_count);
  retarg_cnt = token_count;

  for(int i=0; i < token_count; ++i)
    {
      delete tokens[i];
    }
  delete tokens;

  return Success;
}


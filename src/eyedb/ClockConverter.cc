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


#include <eyedbconfig.h>

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#include <iostream>

#include "ClockConverter.h"

//
// Constants
//

namespace eyedb {

  const eyedblib::int64 USEC_OF_DAY = 86400000000LL;
  const eyedblib::int64 USEC_OF_HOUR = 3600000000LL;
  const eyedblib::int32 USEC_OF_MINUTE = 60000000;
  const eyedblib::int32 USEC_OF_SECOND = 1000000;
  const eyedblib::int16 USEC_OF_MILLISECOND = 1000;
  const eyedblib::int16 MAX_TZ = 720;
  const eyedblib::int16 MIN_TZ = -720;

  const int MAX_STRING_CLOCK = 32;   // 32 > (H{0,10})HH + : + MM + : + ss + , + mmm + , + uuu + \0
  //  (10 +) 2 + 1 + 2  + 1 +  2 + 1 +   3 + 1 +   3 + 1
  //  > 17
  const int MAX_STRING_ZONE = 16;    // 16 > GMT + (+|-) + (H{0,2})HH + : + MM + \0
  //   3 +   1   +  (2+)2 + 1 +  2 +  1
  //  > 10 

  //
  // ClockConverter methods
  //


  ClockConverter::ClockConverter()
  {
    this->string_clock = new char[MAX_STRING_CLOCK];  
    this->string_zone = new char[MAX_STRING_ZONE]; 
  }


  ClockConverter::~ClockConverter()
  {
    delete [] this->string_clock;
    delete [] this->string_zone;
  }



  void
  ClockConverter::usec2clock(const eyedblib::int64 usec, eyedblib::int16 * hour, eyedblib::int16 * min, eyedblib::int16 *sec, eyedblib::int16 * ms, eyedblib::int16 *us)
  {
    eyedblib::int64 usec_res = 0;

    if(hour != 0)
      {
	*hour = (eyedblib::int16) (usec/USEC_OF_HOUR);
      }
    usec_res = usec % USEC_OF_HOUR;

    if(min != 0)
      {
	*min = (eyedblib::int16) (usec_res/USEC_OF_MINUTE);
      }
    usec_res = usec_res % USEC_OF_MINUTE;
  
    if(sec != 0)
      {
	*sec = (eyedblib::int16) (usec_res/USEC_OF_SECOND);
      }
    usec_res = usec_res % USEC_OF_SECOND;
  
    if(ms != 0)
      {
	*ms = (eyedblib::int16) (usec_res/USEC_OF_MILLISECOND);
      }
    usec_res = usec_res % USEC_OF_MILLISECOND;

    if(us != 0)
      {
	*us = (eyedblib::int16) usec_res;
      }

  }


  void
  ClockConverter::clock2usec(eyedblib::int64 *usec, const eyedblib::int16 hour, const eyedblib::int16 min, const eyedblib::int16 sec, const eyedblib::int16 ms, const eyedblib::int16 us)
  {
    *usec = hour * USEC_OF_HOUR +
      (eyedblib::int64)min * USEC_OF_MINUTE +
      (eyedblib::int64)sec * USEC_OF_SECOND + 
      (eyedblib::int64)ms * USEC_OF_MILLISECOND +
      us;

  }


  eyedblib::int32
  ClockConverter::usec2day(eyedblib::int64 usec)
  {
    return usec / USEC_OF_DAY;
  }





  void
  ClockConverter::tz2clock(const eyedblib::int16 tz, eyedblib::int16 * hour, eyedblib::int16 * min)
  {
    if( hour != 0)
      {
	*hour = (tz/60)%24;
      }
    if( min != 0 )
      {
	*min = tz%60;
      }
  }

  void
  ClockConverter::clock2tz(eyedblib::int16 * tz, const eyedblib::int16 hour, const eyedblib::int16 min)
  {
    *tz = (hour%24) * 60 + min;
  }





  eyedblib::int64
  ClockConverter::ascii2usec(const char * t)
  {

    //
    // Basic string checking
    //

    if(strlen(t) > 16
       || strlen(t) < 5)
      {
	return 0;
      }


    //
    // Parse the string
    //

    strcpy(this->string_clock, t);

    char * null_string = (char*)""; /*@@@@warning cast*/
    char * s_hour = null_string;
    char * s_min = null_string;
    char * s_sec = null_string;
    char * s_ms = null_string;
    char * s_us = null_string;

    switch( strlen(string_clock) )
      {
      case 16 : 
	{
	  string_clock[12] = '\0';
	  s_us = string_clock + 13;
	}
      case 12 :
	{
	  string_clock[8] = '\0';
	  s_ms = string_clock + 9;
	}
      case 8 :
	{
	  string_clock[5] = '\0';
	  s_sec = string_clock + 6;
	}
      case 5 :
	{
	  string_clock[2] = '\0';
	  s_min = string_clock + 3;
	  s_hour = string_clock;
	}    
      }

  
    //
    // Convert to numerical values
    //
  
    eyedblib::int16 hour = atoi(s_hour);;
    eyedblib::int16 min = atoi(s_min);
    eyedblib::int16 sec = atoi(s_sec);
    eyedblib::int16 ms = atoi(s_ms);
    eyedblib::int16 us = atoi(s_us);


    //
    // Compute the usec
    //
  
    eyedblib::int64 usec = 0;
    clock2usec(&usec, hour, min, sec, ms, us);


    return usec;
  }

  char *
  ClockConverter::usec2ascii(const eyedblib::int64 usec)
  {
    eyedblib::int16 hour = 0;
    eyedblib::int16 min = 0;
    eyedblib::int16 sec = 0;
    eyedblib::int16 ms = 0;
    eyedblib::int16 us = 0;

    usec2clock(usec, &hour, &min, &sec, &ms, &us);

    char * return_value = new char[MAX_STRING_CLOCK];
    sprintf(return_value, "%.2d:%.2d:%.2d,%.3d,%.3d", hour, min, sec, ms, us);
  
    return return_value;
  }



  eyedblib::int16
  ClockConverter::ascii2tz(const char * tz)
  {

    //
    // basic
    //
  
    if(strlen(tz) > 9
       || strlen(tz) < 6)
      {
	return 0;
      }

  
    //
    // Parse the string
    //

    strcpy(this->string_zone, tz);

    char * null_string = (char*)""; /*@@@@warning cast*/
    char * s_hour = null_string;
    char * s_min = null_string;
    char sign = 0;

    switch( strlen(string_zone))
      {
      case 9 :
	{
	  string_zone[6] = '\0';
	  s_min = string_zone + 7;	
	}
      case 6 :
	{
	  s_hour = string_zone + 4;
	  sign = string_zone[3];
	  break;
	}
      }


  
    //
    // Convert to numerical values
    //
  
    eyedblib::int16 hour = atoi(s_hour);;
    eyedblib::int16 min = atoi(s_min);


    if( sign == '-' )
      {
	hour = -hour;
	min = -min;
      }


    //
    // Compute the tz
    //
  
    eyedblib::int16 timezone = 0;
    clock2tz(&timezone, hour, min);
  
    return timezone;
  }

  char *
  ClockConverter::tz2ascii(const eyedblib::int16 tz)
  {
    eyedblib::int16 hour = 0;
    eyedblib::int16 min = 0;
    char sign;

    if( tz < 0 )
      {
	sign = '-';
      }
    else
      {
	sign = '+';
      }


    tz2clock(tz, &hour, &min);

  
    char * return_value = new char[MAX_STRING_ZONE];
    sprintf(return_value, "GMT%c%.2d:%.2d", sign, abs(hour), abs(min)); 

    return return_value;
  }




  eyedblib::int64
  ClockConverter::current_time()
  {
    struct timeval now;
    gettimeofday(&now, 0);

    struct tm * time = gmtime((time_t*)&(now.tv_sec));

    eyedblib::int64 usec = 0;
    clock2usec(&usec, time->tm_hour, time->tm_min, time->tm_sec, 0, 0);
    usec += now.tv_usec;

    return usec;
  }
 



  eyedblib::int16
  ClockConverter::local_timezone()
  {
#if defined(HAVE_TZSET) && defined(HAVE_VAR_LONG_TIMEZONE)
    tzset();

    return -(timezone / 60);
#else
    std::cerr << "ClockConverter::local_timezone() not implemented\n";
    return 0;
#endif
  }

}
  

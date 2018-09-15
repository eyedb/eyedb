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



#include "GregorianCalendarConverter.h"

#include <ctype.h>

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

namespace eyedb {
  //
  // Constants
  //

  // The julian day for the day 1970-01-01
  const eyedblib::int32 JULIAN_19700101 = 2440588;

  const int MAX_STRING_DATE = 16;     // 16 > (-)YYYYYY + - + MM + - + dd + '\0'
  //      =  1 + 6    + 1 +  2 + 1 +  2 +  1
  //      = 14

  //
  // GregorianCalendarConverter methods
  //

  GregorianCalendarConverter::GregorianCalendarConverter()
  {
    this->string_date = new char[MAX_STRING_DATE]; 
    this->tmp_year = 0;
    this->tmp_day = 0;
    this->tmp_month = 0;
  }


  GregorianCalendarConverter::~GregorianCalendarConverter()
  {
    delete this->string_date;  
  }


  // algorithms from http://hermetic.magnet.ch/cal_stud/jdn.htm
  // ref : Henry F. Fliegel and Thomas C. Van Flandern
  void
  GregorianCalendarConverter::jday2calendar(const eyedblib::int32 julian, eyedblib::int32 * year, eyedblib::int16 * month, eyedblib::int16 * day)
  {
    // check return values

    if(year == 0)
      {
	year = &tmp_year;
      }
    if(month == 0) 
      {
	month = &tmp_month;
      }
    if(day == 0)
      {
	day = &tmp_day;
      }

    // check the julian day number
    if( julian < 0 )
      {
	// set the date to julian=0
	*year = -4713;
	*month = 11;
	*day = 24;

	return;
      }

  
    // run the conversion algorithm

    eyedblib::int32 l = 0;
    eyedblib::int32 n = 0;
    eyedblib::int32 i = 0;
    eyedblib::int32 j = 0;  

    l = julian + 68569;
    n = ( 4 * l ) / 146097;
    l = l - ( 146097 * n + 3 ) / 4;
    i = ( 4000 * ( l + 1 ) ) / 1461001;
    l = l - ( 1461 * i ) / 4 + 31;
    j = ( 80 * l ) / 2447;
    *day = l - ( 2447 * j ) / 80;
    l = j / 11;
    *month = j + 2 - ( 12 * l );
    *year = 100 * ( n - 49 ) + i + l;


  }



  void
  GregorianCalendarConverter::calendar2jday(eyedblib::int32 * julian, const eyedblib::int32 year, const eyedblib::int16 month, const eyedblib::int16 day)
  {
    // check the year
    if(year < -4713)
      {
	*julian=-1;
	return;
      }
  

    // run the conversion algorithm
  
    *julian = ( 1461 * ( year + 4800 + ( month - 14 ) / 12 ) ) / 4 + 
      ( 367 * ( month - 2 - 12 * ( ( month - 14 ) / 12 ) ) ) / 12 - 
      ( 3 * ( ( year + 4900 + ( month - 14 ) / 12 ) / 100 ) ) / 4 + day - 32075;
 

    // check the value
    if( *julian < 0 )
      {
	*julian = -1;
      }
   
   
  }



  eyedblib::int16
  GregorianCalendarConverter::jday2day_of_year(const eyedblib::int32 julian)
  {  
    jday2calendar(julian, &(this->tmp_year), 0, 0);
    eyedblib::int32 first_day_julian = 0;
    calendar2jday(&first_day_julian, tmp_year, 1, 0);
  
    return julian - first_day_julian;
  }

  Bool
  GregorianCalendarConverter::jday2leap_year(const eyedblib::int32 julian)
  {
    jday2calendar(julian, &(this->tmp_year), 0, 0);
 

    if( !(tmp_year % 4) 
	&& ( tmp_year % 100
	     || !(tmp_year % 400) ) )
      {
	return True;
      }
    else
      {
	return False;
      }
  }



  Weekday::Type
  GregorianCalendarConverter::jday2weekday(const eyedblib::int32 julian)
  {
    return (Weekday::Type)((julian + 1) % 7);
  }

  eyedblib::int32
  GregorianCalendarConverter::ascii2jday(const char * date)
  {
    //
    // Parsing
    //

    // check the length
    int len = strlen(date);
    if(len > 13 
       || len < 5)
      {
	return -1;
      }

    strcpy(string_date, date);


    // break the string between year and month
    // by paying attention to the leading '-' that would appear
    // if the year was < 0
    strtok(string_date + 1, "-");
                                            
    // get month and day strings
    char * s_month = strtok(0, "-");
    char * s_day = strtok(0, "-");


    // check the tokens
    if(s_month == 0
       || s_day == 0
       || (isdigit(string_date[0]) == 0 && string_date[0] != '-')
       || isdigit(s_month[0]) == 0
       || isdigit(s_day[0]) == 0 )
      {
	return -1;
      }
  

    //
    // Convert strings into numerical values
    //
    tmp_year = atol(string_date);
    tmp_month = atoi(s_month);
    tmp_day = atoi(s_day);


    //
    // Computes the julian day
    //
    eyedblib::int32 julian = 0;
    calendar2jday(&julian, tmp_year, tmp_month, tmp_day);


    return julian;
  }

  char *
  GregorianCalendarConverter::jday2ascii(const eyedblib::int32 julian)
  {
 
    jday2calendar(julian, &tmp_year, &tmp_month, &tmp_day);


    char * return_value = new char[MAX_STRING_DATE];
    sprintf(return_value, "%ld-%.2d-%.2d", tmp_year, tmp_month, tmp_day);

    return return_value;
  }

  eyedblib::int32
  GregorianCalendarConverter::current_date()
  {
    struct timeval now;
    gettimeofday(&now, 0);

    eyedblib::int32 julian = JULIAN_19700101 + (now.tv_sec / (3600*24));

    return julian;
  }
}

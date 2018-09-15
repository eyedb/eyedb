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

#include "datelib.h"
#include "ClockConverter.h"
#include "GregorianCalendarConverter.h"

namespace eyedb {

  DateAlgorithmRepository the_instance;

  DateAlgorithmRepository::DateAlgorithmRepository()
  {
    this->t_cal = 0;
    this->t_clock = 0;
  }


  DateAlgorithmRepository::~DateAlgorithmRepository()
  {
    if( t_cal != 0 )
      {
	delete t_cal;
      }

    if( t_clock != 0 )
      {
	delete t_clock;
      }

  }


  const DateAlgorithmRepository&
  DateAlgorithmRepository::instance()
  {
    return the_instance;
  }


  CalendarConverter *
  DateAlgorithmRepository::getDefaultCalendarConverter()
  {
    DateAlgorithmRepository::instance();

    if(the_instance.t_cal == 0)
      {
	the_instance.t_cal = new GregorianCalendarConverter();
      }
    return the_instance.t_cal;
  }


  ClockConverter *
  DateAlgorithmRepository::getDefaultClockConverter()
  {
    DateAlgorithmRepository::instance();

    if(the_instance.t_clock == 0)
      {
	the_instance.t_clock = new ClockConverter();
      }
    return the_instance.t_clock;

  }
}
  

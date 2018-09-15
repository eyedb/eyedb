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


#include "eyedb/eyedb.h"
#include "ThreadPoolManager.h"

eyedblib::ThreadPool *ThreadPoolManager::thrpool = 0;

#define DEFERRED_CONSTRUCT

namespace eyedbsm {
  extern void setThreadPool(eyedblib::ThreadPool *);
  extern void setThreadPoolGet(eyedblib::ThreadPool *(*)());
}

static eyedblib::ThreadPool *
get_thread_pool()
{
  return ThreadPoolManager::getThrPool();
}

void
ThreadPoolManager::init()
{
#ifdef DEFERRED_CONSTRUCT
  eyedbsm::setThreadPoolGet(get_thread_pool);
#else
  delete thrpool;

  unsigned int thrcnt;
  const char *thrcnt_str = eyedb::getConfigValue("thread_count");
  thrcnt = thrcnt_str ? atoi(thrcnt_str) : 1;

  Thread::initCallingThread();
  thrpool = new eyedblib::ThreadPool(thrcnt);

  const char *thrprofile_str = eyedb::getConfigValue("thread_profile");
  bool thrprofile = (thrprofile_str && !strcasecmp(thrprofile_str, "true"));
    
  if (thrprofile)
    thrpool->setProfile(true);

  eyedbsm::setThreadPool(thrpool);
#endif
}

eyedblib::ThreadPool *
ThreadPoolManager::getThrPool()
{
#ifndef DEFERRED_CONSTRUCT
  return thrpool;
#else
  if (thrpool) return thrpool;
  unsigned int thrcnt;
  const char *thrcnt_str = eyedb::ServerConfig::getSValue("thread_count");
  thrcnt = thrcnt_str ? atoi(thrcnt_str) : 1;

  eyedblib::Thread::initCallingThread();
  thrpool = new eyedblib::ThreadPool(thrcnt);

  const char *thrprofile_str = eyedb::ServerConfig::getSValue("thread_profile");
  bool thrprofile = (thrprofile_str && !strcasecmp(thrprofile_str, "true"));
    
  if (thrprofile)
    thrpool->setProfile(true);

  eyedbsm::setThreadPool(thrpool);
  return thrpool;
#endif
}

void
ThreadPoolManager::_release()
{
  delete thrpool;
  thrpool = 0;
}

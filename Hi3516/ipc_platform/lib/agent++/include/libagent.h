/*_############################################################################
  _## 
  _##  AGENT++ 4.5 - libagent.h.in  
  _## 
  _##  Copyright (C) 2000-2021  Frank Fock and Jochen Katz (agentpp.com)
  _##  
  _##  Licensed under the Apache License, Version 2.0 (the "License");
  _##  you may not use this file except in compliance with the License.
  _##  You may obtain a copy of the License at
  _##  
  _##      http://www.apache.org/licenses/LICENSE-2.0
  _##  
  _##  Unless required by applicable law or agreed to in writing, software
  _##  distributed under the License is distributed on an "AS IS" BASIS,
  _##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  _##  See the License for the specific language governing permissions and
  _##  limitations under the License.
  _##  
  _##########################################################################*/
#ifndef __LIBAGENT_H_INCLUDED__
#define __LIBAGENT_H_INCLUDED__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if 1
# include <sys/types.h>
#endif
#if 1
# include <sys/stat.h>
#endif

#if 1
# include <cctype>
# include <cerrno>
# include <climits>
# include <csignal>
# include <cstddef>
# include <cstdio>
# include <cstdlib>
# include <cstring>
# include <ctime>
#else
# include <stdio.h>
# if 1
#  include <stdlib.h>
#  include <stddef.h>
# else
#  if 1
#   include <stdlib.h>
#  endif
# endif
# if 1
#  if ! 1 && 1
#   include <memory.h>
#  endif
#  include <string.h>
# endif
# if 1
#  include <strings.h>
# endif
# if 1
#  include <inttypes.h>
# endif
# if 1
#  include <ctype.h>
# endif
# if 1
#  include <signal.h>
# endif
# if 1
#  include <errno.h>
# endif
# if 1
#  include <time.h>
#endif
#endif

#if 1
# include <unistd.h>
#elif 0
# include <sys/unistd.h>
#endif
#if 1
# include <stdint.h>
#endif

#if 1
#include <sys/time.h>
#endif
#if 1
#include <sys/timeb.h> // and _ftime
#endif

#if 0
# include <winsock2.h>
# if 0
#  include <ws2tcpip.h>
# endif
# if 0
#  include <wspiapi.h>
# endif
#elif 0
  /* IIRC winsock.h must be included before windows.h */
# include <winsock.h>
#else
# if 1
#  include <netdb.h>
# endif
# if 1
#  include <sys/socket.h>
# endif
# if 1
#  include <arpa/inet.h>
# endif
# if 1
#  include <netinet/in.h>
# endif
#endif

#if 1
# include <poll.h>
#endif
#if 1
# include <sys/select.h>
#endif

#ifdef _WIN32
# if 0
#  include <io.h>
# endif
# if 0
#  include <process.h>
# endif
# include <windows.h>
#endif

#if ! 0
# if 0
#  define getpid _getpid
# endif
#endif

#if ! 0
# if 0
#  define ss_family __ss_family
# endif
#endif

/* Minimum of signed integral types.  */
#ifndef INT8_MIN
# define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
# define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
# define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT64_MIN
# define INT64_MIN              (-int64_t(9223372036854775807)-1)
#endif
/* Maximum of signed integral types.  */
#ifndef INT_MAX
# define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
# define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
# define INT32_MAX              (2147483647)
#endif
#ifndef INT64_MAX
# define INT64_MAX              (int64_t(9223372036854775807))
#endif

/* Maximum of unsigned integral types.  */
#ifndef UINT8_MAX
# define UINT8_MAX              (255)
#endif
#ifndef UINT16_MAX
# define UINT16_MAX             (65535)
#endif
#ifndef UINT32_MAX
# define UINT32_MAX             (4294967295U)
#endif
#ifndef UINT64_MAX
# define UINT64_MAX             (uint64_t(18446744073709551615))
#endif

#ifndef NULL
#define NULL	0
#endif

#if 1
# include <iostream>
#else
# include <iostream.h>
#endif

#include <snmp_pp/config_snmp_pp.h>
#include <snmp_pp/log.h>
#ifdef _THREADS
#ifndef _WIN32THREADS
#include <pthread.h>
#endif
#endif

#include <agent_pp/agent++.h>

#endif /* ?__LIBAGENT_H_INCLUDED__ */

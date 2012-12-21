/*
 * CPlatform.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CPLATFORM_H_
#define CPLATFORM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>

#if defined(_WIN32)
#	include <windows.h>
#	include <direct.h>
#	include <io.h>
#else // __linux__
#	include <unistd.h>
#	include <pthread.h>
#	include <sys/stat.h>
#	include <sys/types.h>
#	include <sys/time.h>
#endif

// log interface
#ifdef _DUMP_DEBUG
#define CLogD(format, ...)      printf(format, ##__VA_ARGS__)	// log dump debug
#define CLogI(format, ...)      printf(format, ##__VA_ARGS__)	// log info
#define CLogE(format, ...)      printf(format, ##__VA_ARGS__)	// log error
#define CLog(format, ...)		CLogI(format, ##__VA_ARGS__)	// shortcut for log info
#elif defined(_DEBUG) || defined(_DUMP_LOG)
#define CLogD(format, ...)
#define CLogI(format, ...)      printf(format, ##__VA_ARGS__)
#define CLogE(format, ...)      printf(format, ##__VA_ARGS__)
#define CLog(format, ...)		CLogI(format, ##__VA_ARGS__)
#else // release
#define CLogD(format, ...) 
#define CLogD(format, ...)
#define CLogI(format, ...)
#define CLogE(format, ...)      printf(format, ##__VA_ARGS__)
#define CLog(format, ...)
#endif

#if defined(_WIN32)

// struct for gettimeofday
struct timeval_t
{
	long    tv_sec;        // seconds
	long    tv_usec;    // microSeconds
};
#else // __linux__

typedef timeval timeval_t;

#endif // defined(_WIN32)



#endif /* CPLATFORM_H_ */

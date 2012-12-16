/*
 * CUtils.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CUTILS_H_
#define CUTILS_H_

#include "CPlatform.h"
#include "stdlib.h"

namespace cells
{

class CUtils
{
public:
#if defined(_WIN32)
	inline static void sleep(unsigned int millisec)
	{
		Sleep(millisec);
	}
	inline static void yield()
	{
		Sleep(0);
	}
#else // __linux__
	inline static void sleep(unsigned int millisec)
	{
		usleep(millisec * 1000);
	}
	inline static void yield()
	{
		pthread_yield();
	}
#endif

	inline static int atoi(const char* str)
	{
		//return 0;
		return str == NULL ? 0 : ::atoi(str);
	}
};

} /* namespace cells */
#endif /* CUTILS_H_ */

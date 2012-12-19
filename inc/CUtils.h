/*
 * CUtils.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CUTILS_H_
#define CUTILS_H_

#include "CPlatform.h"
#include <stdlib.h>
#include <string>

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

	// zlib compress & decompress
	static int compress(const char* file_in, const char* file_out, int level = -1);
	static int compress_fd(FILE* fin, FILE* fout, int level = -1);
	static int decompress(const char* file_in, const char* file_out);

	// hash utils
	static std::string filehash_md5str(FILE* fp, char* buf, size_t buf_size);
};

} /* namespace cells */
#endif /* CUTILS_H_ */

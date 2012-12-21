/*
 * CUtils.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CUTILS_H_
#define CUTILS_H_

#include "CPlatform.h"
#include "md5.h"
#include "zpip.h"

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

	// get time of day
	static bool gettimeofday(timeval_t* tv, void* tz);

	// get elapsed seconds quickly
	static double gettime_seconds();

	// zlib compress & decompress
	static int compress(const char* file_in, const char* file_out, int level = -1);
	static int compress_fd(FILE* fin, FILE* fout, int level = -1);
	static int decompress(const char* file_in, const char* file_out);

	// hash utils
	static std::string filehash_md5str(FILE* fp, char* buf, size_t buf_size);

	// directory access
	static bool access(const char* path, int mode);

	// make directory
	static bool mkdir(const char* path);

	// build path directorys
	static bool builddir(const char* path);

	// trim
	static std::string str_trim(std::string& s);

	// replace char
	static size_t str_replace_ch(std::string& str, char which, char to);


};

} /* namespace cells */
#endif /* CUTILS_H_ */

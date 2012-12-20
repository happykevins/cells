/*
 * CUtils.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CUtils.h"

#include <stdio.h>
#include <sstream>
#include <algorithm>
#include "md5.h"
#include "zpip.h"

#if defined(_WIN32)
	#include <direct.h>
	#include <io.h>
#else // __linux__ 
　　#include <sys/stat.h>
　　#include <sys/types.h>
	#include <unistd.h>
#endif


namespace cells
{

int CUtils::compress(const char* file_in, const char* file_out, int level /*= -1*/)
{
	FILE* fin = fopen(file_in, "rb");
	if ( !fin )
	{
		printf("CUtils::compress: open input file failed %s\n", file_in);
		return -1;
	}
	FILE* fout = fopen(file_out, "wb+");
	if ( !fout )
	{
		printf("CUtils::compress: open output file failed %s\n", file_out);
		fclose(fin);
		return -1;
	}

	int ret = def(fin, fout, level);

	fclose(fout);
	fclose(fin);
	return ret;
}

int CUtils::compress_fd(FILE* fin, FILE* fout, int level)
{
	int ret = def(fin, fout, level);

	fclose(fout);
	fclose(fin);
	return ret;
}

int CUtils::decompress(const char* file_in, const char* file_out)
{
	FILE* fin = fopen(file_in, "rb");
	if ( !fin )
	{
		printf("CUtils::decompress: open input file failed %s\n", file_in);
		return -1;
	}
	FILE* fout = fopen(file_out, "wb+");
	if ( !fout )
	{
		printf("CUtils::decompress: open output file failed %s\n", file_out);
		fclose(fin);
		return -1;
	}

	int ret = inf(fin, fout);

	fclose(fout);
	fclose(fin);
	return ret;
}

std::string CUtils::filehash_md5str(FILE* fp, char* buf, size_t buf_size)
{
	md5_state_t state;
	md5_byte_t digest[16];
	char hex_output[16*2 + 1];
	size_t file_size = 0;
	md5_init(&state);
	do
	{
		size_t readsize = fread(buf, 1, buf_size, fp);
		file_size += readsize;
		md5_append(&state, (const md5_byte_t *)buf, readsize);
	} while( !feof(fp) && !ferror(fp) );
	md5_finish(&state, digest);

	for (int di = 0; di < 16; ++di)
		sprintf(hex_output + di * 2, "%02x", digest[di]);

	hex_output[16*2] = 0;

	return std::string(hex_output);
}

// directory access
bool CUtils::access(const char* path, int mode)
{
	return ::access(path, mode) == 0;
}

// make directory
bool CUtils::mkdir(const char* path)
{
#if defined(_WIN32)
	return ::mkdir(path) == 0;
#else
	return ::mkdir(path, 777) == 0;
#endif
}


// build path directorys
bool CUtils::builddir(const char* path)
{
	std::string str(path);

	str_replace_ch(str, '\\', '/');

	size_t end = str.find_last_not_of('/');
	bool dummy = false;
	for ( size_t i = 0; i < str.size(); i++ )
	{
		if ( str[i] == '/' && !dummy )
		{
			std::string bpath = str.substr(0, i);
			if ( !CUtils::access(bpath.c_str(), 0) )
			{
				if ( !CUtils::mkdir(bpath.c_str()) )
				{
					return false;
				}
			}
			dummy = true;
		}
		else
		{
			dummy = false;
		}
	}

	return true;
}

// replace char
size_t CUtils::str_replace_ch(std::string& str, char which, char to)
{
	size_t num = 0;
	for ( size_t i = 0; i < str.size(); i++ )
	{
		if ( str[i] == which )
		{
			str[i] = to;
			num++;
		}
	}
	return num;
}

std::string CUtils::str_trim(std::string& s)
{
	if (s.length() == 0) return s;
	size_t beg = s.find_first_not_of(" \a\b\f\n\r\t\v");
	size_t end = s.find_last_not_of(" \a\b\f\n\r\t\v");
	if (beg == std::string::npos) return "";
	return std::string(s, beg, end - beg + 1);
}


} /* namespace cells */

/*
 * CUtils.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CUtils.h"

#include <stdio.h>
#include "md5.h"
#include "zpip.h"


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

} /* namespace cells */

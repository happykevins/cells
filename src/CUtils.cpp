/*
 * CUtils.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CUtils.h"

#include <stdio.h>
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

} /* namespace cells */

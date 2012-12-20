/*
* cellstool
*
*  Created on: 2012-12-19
*      Author: happykevins@gmail.com
*/

//
// TODO: build工具的过滤列表
// TODO: 比对工具
//


#ifndef TOOLS_H_
#define TOOLS_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h> 
#include <io.h>
#include <stdio.h>
#include <direct.h>
#include <map>
#include <vector>
#include <string.h>

#include "CUtils.h"

using namespace std;
using namespace cells;

#define BUFFER_SIZE 16384
extern "C" string s_folders_outputfile;
extern "C" string s_files_outputfile;
extern "C" char s_data_buf[BUFFER_SIZE];
extern "C" string s_hash_suffix;

extern "C" void error_msg(const char* msg);
extern "C" void build_cells(string input_path, string output_path, bool compress, int compress_level, string suffix);
extern "C" void build_cdf(string input_path, string output_path, bool compress, int compress_level, string suffix);


#ifdef _DEBUG
#define assert_break(exp) assert(exp)
#else
#define assert_break(exp) if ( !exp ) { break; }
#endif

#endif//#define TOOLS_H_
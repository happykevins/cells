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
#include <set>
#include <vector>
#include <string.h>

#include "CUtils.h"

using namespace std;
using namespace cells;

struct cell_attr
{
	cell_attr() : osize(0), zsize(0), in_cdf(false), is_cdf(false) {}
	string omd5;
	string zmd5;
	size_t osize;
	size_t zsize;
	bool in_cdf;
	bool is_cdf;
};
typedef map<string, cell_attr*> idxmap_t;
typedef map<string, string> args_t;

extern "C" args_t g_cmd_args;

#define BUFFER_SIZE 16384
extern "C" string s_folders_outputfile;
extern "C" string s_files_outputfile;
extern "C" char s_data_buf[BUFFER_SIZE];
extern "C" string s_hash_suffix;
extern "C" string s_cdf_suffix;

extern "C" string s_freefiles_name;
extern "C" string s_freefiles_cdf_name;
extern "C" string s_cdf_input_idxfile;

extern "C" string s_modified_files;
extern "C" string s_added_files;
extern "C" string s_deleted_files;
extern "C" string s_unchanged_files;

extern "C" string s_log_path;
extern "C" string s_process_log;
extern "C" string s_build_process_log;
extern "C" string s_cdf_process_log;
extern "C" string s_version_process_log;

extern "C" bool parse_line(FILE* fp, vector<string>& out);
extern "C" bool check_bom(FILE* fp);
extern "C" void error_msg(const char* msg);
extern "C" void build_cells(string input_path, string output_path, bool compress, int compress_level);
extern "C" void build_cdf(string input_path, string output_path, bool compress, int compress_level);
extern "C" void build_version_patch(string old_path, string new_path, string output_path);


#ifdef _DEBUG
#define assert_break(exp) assert(exp)
#else
#define assert_break(exp) if ( !exp ) { break; }
#endif

#endif//#define TOOLS_H_
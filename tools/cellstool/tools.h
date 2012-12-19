/*
* cellstool
*
*  Created on: 2012-12-19
*      Author: happykevins@gmail.com
*/

//
// TODO: build工具的过滤列表
// TODO: 比对工具
// TODO: 生成cdf的策略及工具
//


#ifndef TOOLS_H_
#define TOOLS_H_

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <map>

#include "CUtils.h"

using namespace std;
using namespace cells;

extern "C" void error_msg(const char* msg);
extern "C" void build_cells(string input_path, string output_path, bool compress, int compress_level, string suffix);

#endif//#define TOOLS_H_
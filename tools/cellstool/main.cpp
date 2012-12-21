/*
* cellstool
*
*  Created on: 2012-12-19
*      Author: happykevins@gmail.com
*/

#include "tools.h"


// global setting
string s_folders_outputfile = "cells_folders.csv";
string s_files_outputfile = "cells_files.csv";
char s_data_buf[BUFFER_SIZE];
string s_hash_suffix = ".hash";

typedef map<string, string> args_t;

void error_msg(const char* msg)
{
	printf("[error]: %s\n", msg);
	exit(0);
}

void print_help()
{
	printf("*** tools for cells ***\n");
	
	printf("\t-h --help : for this help list.\n");
	printf("\n");

	printf("\t-b --build : build specified path for cells system.\n");
	printf("\t\t-i=... : input path; no default value.\n");
	printf("\t\t-o=... : output path: no default value. \n");
	printf("\t\t-z[=level] : compress file use zlib. can specify level for compress level, default is -1. \n");
	printf("\n");

	printf("\t-c --cdf : create cdf files for cells system.\n");
	printf("\t\t-i=... : input path; no default value.\n");
	printf("\t\t-o=... : output path: no default value. \n");
	printf("\t\t-z[=level] : compress cdf use zlib. can specify level for compress level, default is -1. \n");
	printf("\n");
}



int main(int argc, char *argv[])
{
	args_t cmd_args;
	for ( int i = 0; i < argc; i++ )
	{
		std::string v(argv[i]);
		size_t split_pos = v.find('=');
		if ( split_pos == -1 )
		{
			cmd_args.insert(std::make_pair(v, ""));
		}
		else
		{
			std::pair<string, string> ap = std::make_pair(v.substr(0, split_pos), v.substr(split_pos+1, v.size()));
			cmd_args.insert(ap);
			//printf("%s = %s\n", ap.first.c_str(), ap.second.c_str());
		}
	}

	// help
	if ( argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 )
	{
		print_help();
	}
	// build
	else if ( strcmp(argv[1], "--build") == 0 || strcmp(argv[1], "-b") == 0 )
	{
		string input_path = cmd_args["-i"];
		if ( input_path.empty() )
		{
			error_msg("-i input path can't be null!");
		}
		string output_path = cmd_args["-o"];
		if ( output_path.empty() )
		{
			error_msg("-o output path can't be null!");
		}
		bool compress = cmd_args.find("-z") != cmd_args.end();
		int compress_level = -1;
		if ( !cmd_args["-z"].empty() )
		{
			compress_level = CUtils::atoi(cmd_args["-z"].c_str());
		}
		string suffix = cmd_args["-suffix"];
		if ( suffix.empty() )
		{
			suffix = ".hash";
		}

		build_cells(input_path, output_path, compress, compress_level, suffix);
	}
	// build
	else if ( strcmp(argv[1], "--cdf") == 0 || strcmp(argv[1], "-c") == 0 )
	{
		string input_path = cmd_args["-i"];
		if ( input_path.empty() )
		{
			error_msg("-i input path can't be null!");
		}
		string output_path = cmd_args["-o"];
		if ( output_path.empty() )
		{
			error_msg("-o output path can't be null!");
		}
		bool compress = cmd_args.find("-z") != cmd_args.end();
		int compress_level = -1;
		if ( !cmd_args["-z"].empty() )
		{
			compress_level = CUtils::atoi(cmd_args["-z"].c_str());
		}
		string suffix = cmd_args["-suffix"];
		if ( suffix.empty() )
		{
			suffix = ".xml";
		}

		build_cdf(input_path, output_path, compress, compress_level, suffix);
	}
	else
	{
		error_msg("invalid command!");
	}

	return 0;
}


/*
* cellstool
*
*  Created on: 2012-12-19
*      Author: happykevins@gmail.com
*/

#include "tools.h"


// ===== global settings =====

string s_process_log;
string s_log_path = ".";

// build tool settings
string s_build_process_log = "_cells_build_logs.log";
string s_folders_outputfile = "_cells_build_allfolders.csv";
string s_files_outputfile = "_cells_build_allfiles.csv";
char s_data_buf[BUFFER_SIZE];
string s_hash_suffix = ".hash";
string s_cdf_suffix = ".xml";

// cdf tool settings
string s_cdf_process_log = "_cells_cdf_logs.log";
string s_freefiles_name = "_cells_cdf_freefiles.csv";
string s_freefiles_cdf_name = "_cells_cdf_freefiles.xml";
string s_cdf_input_idxfile = "cdf_index.txt";

// version tool settings
string s_version_process_log = "_cells_version_logs.log";
string s_modified_files = "_cells_ver_modified_files.csv";
string s_added_files = "_cells_ver_added_files.csv";
string s_deleted_files = "_cells_ver_deleted_files.csv";
string s_unchanged_files = "_cells_ver_unchanged_files.csv";

// ============================

args_t g_cmd_args;

void error_msg(const char* msg)
{
	printf("[error]: %s\n", msg);
	exit(0);
}

bool check_bom(FILE* fp)
{
	assert(fp);
	int bom_bytes[3] = {0xEF, 0xBB, 0xBF};

	for ( size_t i = 0; i < 3; i++ )
	{
		int ch = fgetc(fp);
		if ( ch == EOF )
		{
			fseek(fp, 0, SEEK_SET);
			return false;
		}
		if ( bom_bytes[i] != ch )
		{
			fseek(fp, 0, SEEK_SET);
			return false;
		}
	}

	return true;
}

bool parse_line(FILE* fp, vector<string>& out)
{
	for( int w = 0, ch = fgetc(fp); true; ch = fgetc(fp) )
	{
		if ( ch == '\n' || ch == ',' || ch == EOF )
		{
			s_data_buf[w] = '\0';
			out.push_back(string(s_data_buf));

			if ( ch == '\n' )
			{
				if ( out.size() == 1 && out[0].empty() )
				{
					// skip empty line
					out.clear();
				}
				return true;
			}

			if ( ch == EOF )
				break;

			w = 0;
			continue;
		}

		s_data_buf[w] = (char)ch;
		w++;
	}

	if ( out.size() == 1 && out[0].empty() )
	{
		// skip empty line
		out.clear();
	}

	return false;
}

void print_help()
{
	printf("*** tools for cells ***\n");
	
	printf("\t--- global settings --- \n");
	printf("\t\t-log=... : log file name; default cells_*.log.\n");
	printf("\t\t-logpath=... : log file path; default is working path.\n");
	printf("\t\t-hashsuffix=... : hash file suffix name; default '%s'. \n", s_hash_suffix.c_str());
	printf("\t\t-cdfsuffix=... : cdf file suffix name; default '%s'. \n", s_cdf_suffix.c_str());
	printf("\n");

	printf("\t-h --help : for this help list.\n");
	printf("\n");

	printf("\t-b --build : build specified path for cells system.\n");
	printf("\t\t*notice: a sub directory start with '_' will never be processed! \n");
	printf("\t\t-i=... : input path; no default value.\n");
	printf("\t\t-o=... : output path: no default value. \n");
	printf("\t\t-z[=level] : compress file use zlib. can specify level for compress level, default is -1. \n");
	printf("\n");

	printf("\t-c --cdf : create cdf files for cells system.\n");
	printf("\t\t-i=... : input config files path; no default value.\n");
	printf("\t\t-o=... : object path: no default value. \n");
	printf("\t\t-cdfidx=... : index file name for cdf building tool; default '%s'. \n", s_cdf_input_idxfile.c_str());
	printf("\t\t-freecdf=... : free files cdf name; default '%s'. \n", s_freefiles_cdf_name.c_str());
	printf("\t\t-z[=level] : compress cdf use zlib. can specify level for compress level, default is -1. \n");
	printf("\n");

	printf("\t-v --version : create patch between versions.\n");
	printf("\t\t-io=... : input old version path; no default value.\n");
	printf("\t\t-in=... : input new version path: no default value. \n");
	printf("\t\t-o=... : output patch path: no default value. \n");
	printf("\n");
}



int main(int argc, char *argv[])
{
	for ( int i = 0; i < argc; i++ )
	{
		std::string v(argv[i]);
		size_t split_pos = v.find('=');
		if ( split_pos == -1 )
		{
			g_cmd_args.insert(std::make_pair(v, ""));
		}
		else
		{
			std::pair<string, string> ap = std::make_pair(v.substr(0, split_pos), v.substr(split_pos+1, v.size()));
			g_cmd_args.insert(ap);
			//printf("%s = %s\n", ap.first.c_str(), ap.second.c_str());
		}
	}

	// global args
	if ( !g_cmd_args["-log"].empty() )
	{
		s_process_log = g_cmd_args["-log"];
	}
	if ( !g_cmd_args["-logpath"].empty() )
	{
		s_log_path = g_cmd_args["-logpath"];
	}
	if ( !g_cmd_args["-hashsuffix"].empty() )
	{
		s_hash_suffix = g_cmd_args["-hashsuffix"];
	}
	if ( !g_cmd_args["-cdfsuffix"].empty() )
	{
		s_cdf_suffix = g_cmd_args["-cdfsuffix"];
	}


	// help
	if ( argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 )
	{
		print_help();
	}
	// build
	else if ( strcmp(argv[1], "--build") == 0 || strcmp(argv[1], "-b") == 0 )
	{
		string input_path = g_cmd_args["-i"];
		if ( input_path.empty() )
		{
			error_msg("-i input path can't be null!");
		}
		string output_path = g_cmd_args["-o"];
		if ( output_path.empty() )
		{
			error_msg("-o output path can't be null!");
		}
		bool compress = g_cmd_args.find("-z") != g_cmd_args.end();
		int compress_level = -1;
		if ( !g_cmd_args["-z"].empty() )
		{
			compress_level = CUtils::atoi(g_cmd_args["-z"].c_str());
		}

		if ( s_process_log.empty() )
		{
			s_process_log = s_build_process_log;
		}

		build_cells(input_path, output_path, compress, compress_level);
	}
	// cdf
	else if ( strcmp(argv[1], "--cdf") == 0 || strcmp(argv[1], "-c") == 0 )
	{
		string input_path = g_cmd_args["-i"];
		if ( input_path.empty() )
		{
			error_msg("-i input path can't be null!");
		}
		string output_path = g_cmd_args["-o"];
		if ( output_path.empty() )
		{
			error_msg("-o output path can't be null!");
		}
		bool compress = g_cmd_args.find("-z") != g_cmd_args.end();
		int compress_level = -1;
		if ( !g_cmd_args["-z"].empty() )
		{
			compress_level = CUtils::atoi(g_cmd_args["-z"].c_str());
		}
		if ( !g_cmd_args["-cdfidx"].empty() )
		{
			s_cdf_input_idxfile = g_cmd_args["-cdfidx"];
		}

		if ( !g_cmd_args["-freecdf"].empty() )
		{
			s_freefiles_cdf_name = g_cmd_args["-freecdf"];
		}

		if ( s_process_log.empty() )
		{
			s_process_log = s_cdf_process_log;
		}

		build_cdf(input_path, output_path, compress, compress_level);
	}
	// version
	else if ( strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0 )
	{
		string input_path1 = g_cmd_args["-io"];
		if ( input_path1.empty() )
		{
			error_msg("-io old version path can't be null!");
		}
		string input_path2 = g_cmd_args["-in"];
		if ( input_path2.empty() )
		{
			error_msg("-in new version path can't be null!");
		}
		string output_path = g_cmd_args["-o"];
		if ( output_path.empty() )
		{
			error_msg("-o patch output path can't be null!");
		}

		if ( s_process_log.empty() )
		{
			s_process_log = s_version_process_log;
		}

		build_version_patch(input_path1, input_path2, output_path);
	}
	else
	{
		error_msg("invalid command!");
	}

	return 0;
}


#include "tools.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h> 
#include <direct.h>

#ifdef _DEBUG
#define assert_break(exp) assert(exp)
#else
#define assert_break(exp) if ( !exp ) { break; }
#endif

static string s_inputpath;
static string s_outputpath;
static bool s_compress = false;
static int s_compress_level = -1;
static string s_suffix;
static size_t s_file_counter = 0;
static size_t s_file_err_counter = 0;
static size_t s_folder_counter = 0;
static size_t s_folder_err_counter = 0;
static string s_process_log = "cells_build_logs.txt";
static string s_folders_outputfile = "cells_folders.csv";
static string s_files_outputfile = "cells_files.csv";
static FILE* s_process_log_fp = NULL;
static FILE* s_folders_outputfile_fp = NULL;
static FILE* s_files_outputfile_fp = NULL;
char s_data_buf[16384];

bool process_folder(string full_path, string rel_path, string name)
{
	string output_folder = s_outputpath + rel_path + name;

	if ( access(output_folder.c_str(), 0) != 0 && mkdir(output_folder.c_str()) != 0 )
	{
		// error!
		fprintf(s_process_log_fp, "[error]can't create folder: %s\n", string(rel_path + name).c_str());

		s_folder_err_counter++;

		return false;
	}
	else
	{
		// success!
		fprintf(s_process_log_fp, "[info]build folder: %s\n", string(rel_path + name).c_str());
		fprintf(s_folders_outputfile_fp, "%s\n", string(rel_path + name).c_str());
		
		s_folder_counter++;

		return true;
	}
}

bool process_file(string full_path, string rel_path, string name)
{
	string input_file = full_path + name;
	string output_file = s_outputpath + rel_path + name;
	string hash_file = output_file + s_suffix;
	string rel_file = rel_path + name;
	int ret = 0;

	FILE* input_fp = NULL;
	FILE* output_fp = NULL;
	FILE* hash_fp = NULL;

	do {

	input_fp = fopen(input_file.c_str(), "rb");
	if ( input_fp == NULL )
	{
		// error!
		fprintf(s_process_log_fp, "[error]can't access file: %s\n", input_file.c_str() );
		s_file_err_counter++;
		return false;
	}

	output_fp = fopen(output_file.c_str(), "wb+");
	if ( output_fp == NULL )
	{
		// error!
		fclose(input_fp);
		fprintf(s_process_log_fp, "[error]can't create file: %s\n", output_file.c_str() );
		s_file_err_counter++;
		return false;
	}

	hash_fp = fopen(hash_file.c_str(), "w+");
	if ( hash_fp == NULL )
	{
		// error!
		fclose(input_fp);
		fclose(output_fp);
		fprintf(s_process_log_fp, "[error]can't create hash file: %s\n", hash_file.c_str() );
		s_file_err_counter++;
		return false;
	}

	string of_md5str = CUtils::filehash_md5str(input_fp, s_data_buf, sizeof(s_data_buf));
	string zf_md5str;
	ret = fseek(input_fp, 0, SEEK_END);  
	assert_break(ret == 0);
	size_t of_size = ftell(input_fp); 
	size_t zf_size = 0;

	if ( of_size == 0 )
	{
		fclose(input_fp);
		fclose(output_fp);
		fclose(hash_fp);
		fprintf(s_process_log_fp, "[error] file size is zero!: %s\n", input_file.c_str() );
		s_file_err_counter++;
		return false;
	}

	ret = fseek(input_fp, 0, SEEK_SET);
	assert_break(ret == 0);

	if ( s_compress )
	{
		ret = CUtils::compress_fd(input_fp, output_fp, s_compress_level);
		assert_break(ret == 0);
	}
	else
	{
		while ( feof(input_fp) == 0 && ferror(input_fp) == 0 && ferror(output_fp) == 0 )
		{
			size_t read_size = fread(s_data_buf, 1, sizeof(s_data_buf), input_fp);
			size_t write_size = fwrite(s_data_buf, 1, read_size, output_fp);
			assert_break(read_size == write_size);
		}
	}

	fprintf(hash_fp, "%s\n%d\n", of_md5str.c_str(), of_size);
	if ( s_compress )
	{
		fclose(output_fp);
		output_fp = fopen(output_file.c_str(), "rb");
		assert_break(output_fp);

		zf_md5str = CUtils::filehash_md5str(output_fp, s_data_buf, sizeof(s_data_buf));

		ret = fseek(output_fp, 0, SEEK_END);  
		assert_break(ret == 0);
		zf_size = ftell(output_fp); 

		fprintf(hash_fp, "%s\n%d\n", zf_md5str.c_str(), zf_size);
	}

	// success!
	fprintf(s_files_outputfile_fp, "%s,%s,%d,", rel_file.c_str(), of_md5str.c_str(), of_size);	
	s_compress ? 
		fprintf(s_files_outputfile_fp, "%s,%d\n", zf_md5str.c_str(), zf_size) :
		fprintf(s_files_outputfile_fp, ",\n");
	fprintf(s_process_log_fp, "[info]build file: %s\n", rel_file.c_str());
	s_file_counter++;

	} while(0); // do

	fclose(input_fp);
	fclose(output_fp);
	fclose(hash_fp);

	if ( ret != 0 )
	{
		fprintf(s_process_log_fp, "[error]error occured when build file: %s\n", rel_file.c_str() );
		s_file_err_counter++;
		return false;
	}

	return true;
}

void visit_build(string path, string relpath = "")
{    
	struct _finddata_t file_info;
	long handle;

	if( (handle=_findfirst(string(path + "/*").c_str(),&file_info)) == -1L ) 
	{
		printf("-- no file --\n");
	}
	else
	{
		while( !_findnext(handle,&file_info) ) // first is "."
		{
			if( (file_info.attrib & _A_SUBDIR) == _A_SUBDIR  && strcmp(file_info.name, "..") )
			{   
				printf("folder:%s", string(relpath + "/"+ file_info.name).c_str());
				process_folder(path + "/", relpath + "/", file_info.name) ? 
					printf("[OK]\n") : printf("[FAILED]\n");
				visit_build(path + "/" + file_info.name, relpath + "/" + file_info.name);            
			}
			else if(!(file_info.attrib & _A_SUBDIR))
			{ 
				printf("file:%s", string(relpath + "/"+ file_info.name).c_str());
				process_file(path + "/", relpath + "/", file_info.name) ? 
					printf("[OK]\n") : printf("[FAILED]\n");
			} 
		}     
		_findclose(handle);        
	} 
}

void build_cells(string input_path, string output_path, bool compress, int compress_level, string suffix)
{
	// setup output path
	if ( access(output_path.c_str(), 0) != 0 && mkdir(output_path.c_str()) != 0 )
	{
		error_msg("can't create output path!");
	}
	// open log file
	s_process_log_fp = fopen(string(output_path + "/" + s_process_log).c_str(), "w+");
	if ( !s_process_log_fp )
	{
		error_msg("can't create log file!");
	}

	printf("***start building:\n --- in=%s, out=%s, zlib=%s, zlevel=%d, suffix=%s\n",
		input_path.c_str(), output_path.c_str(), compress ? "true" : "false", compress_level, suffix.c_str());

	fprintf(s_process_log_fp, "***start building:\n --- in=%s, out=%s, zlib=%s, zlevel=%d, suffix=%s\n",
		input_path.c_str(), output_path.c_str(), compress ? "true" : "false", compress_level, suffix.c_str());

	s_inputpath = input_path;
	s_outputpath = output_path;
	s_compress = compress;
	s_compress_level = compress_level;
	s_suffix = suffix;



	// open output file
	s_folders_outputfile_fp = fopen(string(s_outputpath + "/" + s_folders_outputfile).c_str(), "w+");
	if ( !s_folders_outputfile_fp )
	{
		error_msg("can't open folders list file!");
	}
	s_files_outputfile_fp = fopen(string(s_outputpath + "/" + s_files_outputfile).c_str(), "w+");
	if ( !s_files_outputfile_fp )
	{
		error_msg("can't open files list file!");
	}

	// visit process files
	visit_build(s_inputpath);

	// close
	fclose(s_files_outputfile_fp);
	fclose(s_folders_outputfile_fp);
	
	printf("***end building: \n --- (%d) files success! (%d) folders success!\n --- (%d) file errors! (%d) folder errors!\n", 
		s_file_counter, s_folder_counter,
		s_file_err_counter, s_folder_err_counter);

	fprintf(s_process_log_fp, "***end building: \n --- (%d) files success! (%d) folders success!\n --- (%d) file errors! (%d) folder errors!\n", 
		s_file_counter, s_folder_counter,
		s_file_err_counter, s_folder_err_counter);

	fclose(s_process_log_fp);
}



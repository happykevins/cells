#include "tools.h"

static string s_inputpath;
static string s_outputpath;
static bool s_compress = false;
static int s_compress_level = -1;
static size_t s_error_counter = 0;


static size_t s_cdf_error_counter = 0;
static size_t s_file_counter = 0;
static size_t s_file_err_counter = 0;
static size_t s_folder_counter = 0;
static size_t s_folder_err_counter = 0;

static string s_cdf_input_idxfile = "cells_cdf_index.txt";
static string s_process_log = "cells_cdf_logs.log";
static string s_freefiles_name = "cells_freefiles.csv";
static string s_freefiles_cdf_name = "cells_cdf_freefiles.xml";
static string s_temp_suffix = ".tmp";
static FILE* s_process_log_fp = NULL;
static FILE* s_folders_outputfile_fp = NULL;
static FILE* s_files_outputfile_fp = NULL;

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
static idxmap_t s_idxmap;

struct task_attr
{
	string name;
	string input_name;
	string output_name;
	vector<string> attrs;
};
vector<task_attr*> s_tasks;

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

void setup_index()
{
	// open index file
	s_folders_outputfile_fp = fopen(string(s_inputpath + "/" + s_folders_outputfile).c_str(), "r");
	if ( !s_folders_outputfile_fp )
	{
		error_msg("can't open folders list file!");
	}
	s_files_outputfile_fp = fopen(string(s_inputpath + "/" + s_files_outputfile).c_str(), "r");
	if ( !s_files_outputfile_fp )
	{
		error_msg("can't open files list file!");
	}

	check_bom(s_folders_outputfile_fp);
	check_bom(s_files_outputfile_fp);

	bool has_next = false;
	do
	{
		vector<string> strs;
		has_next = parse_line(s_files_outputfile_fp, strs);
		if ( strs.empty() ) break;

		if ( strs.size() < 3 )
		{
			s_error_counter++;
			fprintf(s_process_log_fp, "[error]invalid attributes for: %s\n", strs[0].c_str() );
			continue;
		}
		cell_attr* attr = new cell_attr;
		attr->omd5 = strs[1];
		attr->osize = CUtils::atoi(strs[2].c_str());
		if ( strs.size() == 5 )
		{
			attr->zmd5 = strs[3];
			attr->zsize = CUtils::atoi(strs[4].c_str());
		}
		s_idxmap.insert(std::make_pair(strs[0], attr));
	} while(has_next);

	// close
	fclose(s_files_outputfile_fp);
	fclose(s_folders_outputfile_fp);

	fprintf(s_process_log_fp, "[info]setup index from files list, count=%d\n", s_idxmap.size() );
}

void setup_tasks()
{
	// open index file
	FILE* in_cdf_index_fp = fopen(string(s_inputpath + "/" + s_cdf_input_idxfile).c_str(), "r");
	if ( !s_process_log_fp )
	{
		error_msg("can't open input cdf index file!");
	}

	check_bom(in_cdf_index_fp);
	
	bool has_next = false;
	do
	{
		vector<string> strs;
		has_next = parse_line(in_cdf_index_fp, strs);
		if ( strs.empty() ) break;

		if ( strs.size() < 2 )
		{
			s_error_counter++;
			fprintf(s_process_log_fp, "[error]invalid input cdf index for: %s\n", strs[0].c_str() );
			continue;
		}

		task_attr* attrs = new task_attr;
		attrs->input_name = s_inputpath + "/" + strs[0];
		attrs->output_name = s_outputpath + "/" + strs[1];
		attrs->name = strs[1];
		// ∏Ωº” Ù–‘
		for ( size_t i = 2; i < strs.size(); i++ )
		{
			attrs->attrs.push_back(strs[i]);
		}

		s_tasks.push_back(attrs);
	} while(has_next);

	fprintf(s_process_log_fp, "[info]create %d cdf tasks from file %s\n", s_tasks.size(),  s_cdf_input_idxfile.c_str());

	fclose(in_cdf_index_fp);
}

bool process_cdf(string input_file, string output_file, string rel_file)
{
	string hash_file = output_file + s_hash_suffix;

	int ret = 0;

	FILE* input_fp = NULL;
	FILE* output_fp = NULL;
	FILE* hash_fp = NULL;

	do {

		input_fp = fopen(input_file.c_str(), "rb");
		if ( input_fp == NULL )
		{
			// error!
			fprintf(s_process_log_fp, "[error]can't open input cdf file: %s\n", input_file.c_str() );
			s_error_counter++;
			return false;
		}

		output_fp = fopen(output_file.c_str(), "wb+");
		if ( output_fp == NULL )
		{
			// error!
			fclose(input_fp);
			fprintf(s_process_log_fp, "[error]can't create output cdf file: %s\n", output_file.c_str() );
			s_error_counter++;
			return false;
		}

		hash_fp = fopen(hash_file.c_str(), "w+");
		if ( hash_fp == NULL )
		{
			// error!
			fclose(input_fp);
			fclose(output_fp);
			fprintf(s_process_log_fp, "[error]can't create cdf hash file: %s\n", hash_file.c_str() );
			s_error_counter++;
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
			fprintf(s_process_log_fp, "[error] input cdf file size is zero!: %s\n", input_file.c_str() );
			s_error_counter++;
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

		// insert to idxmap
		cell_attr* attr = new cell_attr;
		attr->omd5 = of_md5str;
		attr->osize = of_size;
		attr->is_cdf = true;
		if ( s_compress )
		{
			attr->zmd5 = zf_md5str;
			attr->zsize = zf_size;
		}
		s_idxmap.insert(std::make_pair(rel_file, attr));

		fprintf(s_process_log_fp, "[info]create cdf file %s [OK]\n", rel_file.c_str());
	} while(0); // do

	fclose(input_fp);
	fclose(output_fp);
	fclose(hash_fp);

	if ( ret != 0 )
	{
		fprintf(s_process_log_fp, "[error]error occured when build cdf file: %s\n", rel_file.c_str() );
		s_error_counter++;
		return false;
	}

	return true;
}


void setup_cdf(task_attr* task)
{
	string input_name = task->input_name;
	string temp_name = input_name + s_temp_suffix;
	string output_name = task->output_name;

	// create cdf
	FILE* input_fp = fopen(input_name.c_str(), "r");
	if ( input_fp == NULL )
	{
		// error!
		fprintf(s_process_log_fp, "[error]can't open input cdf file: %s\n", input_name.c_str() );
		s_error_counter++;
		return;
	}
	check_bom(input_fp);

	FILE* output_fp = fopen(temp_name.c_str(), "w+");
	if ( output_fp == NULL )
	{
		// error!
		fclose(input_fp);
		fprintf(s_process_log_fp, "[error]can't create output cdf file: %s\n", temp_name.c_str() );
		s_error_counter++;
		return;
	}

	// write header
	fprintf(output_fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

	// write begin cells section
	fprintf(output_fp, "<cells");
	// write attrs
	for ( size_t ai = 0; ai < task->attrs.size(); ai++)
	{
		fprintf(output_fp, " %s ", task->attrs[ai].c_str());
	}
	fprintf(output_fp, ">\n");

	// write cell
	bool has_next = false;
	do
	{
		vector<string> strs;
		has_next = parse_line(input_fp, strs);
		if ( strs.empty() ) break;

		idxmap_t::iterator it = s_idxmap.find(strs[0]);
		if ( it == s_idxmap.end() )
		{
			// error!
			fprintf(s_process_log_fp, "[error]cdf %s desire a non-exsit file %s\n", task->input_name.c_str(), strs[0].c_str() );
			s_error_counter++;
			continue;
		}
		// mark is in cdf
		it->second->in_cdf = true;

		// write cell begin
		fprintf(output_fp, "\t<cell ");

		// write name
		fprintf(output_fp, "name=\"%s\" ", it->first.c_str());
		// write md5
		fprintf(output_fp, "hash=\"%s\" ", it->second->omd5.c_str());
		// write size
		fprintf(output_fp, "size=\"%d\" ", it->second->osize);

		// write zip attr
		if ( !it->second->zmd5.empty() )
		{
			// write md5
			fprintf(output_fp, "zhash=\"%s\" ", it->second->zmd5.c_str());
			// write size
			fprintf(output_fp, "zsize=\"%d\" ", it->second->zsize);
		}

		// is cdf file
		if ( it->second->is_cdf )
		{
			// mark cdf
			fprintf(output_fp, "cdf=\"1\" ");
		}

		// write user attr
		for ( size_t ui = 1; ui < strs.size(); ui++ )
		{
			fprintf(output_fp, "%s ", strs[ui].c_str());
		}

		// write cell end
		fprintf(output_fp, "/>\n");

	} while (has_next);

	// write end cells section
	fprintf(output_fp, "</cells>");

	fclose(output_fp);
	fclose(input_fp);

	// process file
	process_cdf(temp_name, output_name, task->name);

	// delete tmp file
	if ( remove(temp_name.c_str()) != 0 )
	{
		fprintf(s_process_log_fp, "[error]can't delete temp cdf file: %s\n", temp_name.c_str() );
		s_error_counter++;
	}
}

void setup_cdfs()
{
	for ( size_t i = 0; i < s_tasks.size(); i++  )
	{
		task_attr* task = s_tasks[i];
		assert(task);

		setup_cdf(task);
	}
}

void setup_freefiles()
{
	string freelist_path = s_outputpath + "/" + s_freefiles_name;
	FILE* freelist_fp = fopen(freelist_path.c_str(), "w+");
	if ( !freelist_fp )
	{
		fprintf(s_process_log_fp, "[error]can't create freefiles list file: %s\n", freelist_path.c_str() );
		s_error_counter++;
		return;
	}

	// create freefiles list file
	size_t free_counter = 0;
	for( idxmap_t::iterator it = s_idxmap.begin(); it != s_idxmap.end(); it++ )
	{
		if ( !it->second->in_cdf )
		{
			free_counter++;
			fprintf(freelist_fp, "%s\n", it->first.c_str());
		}
	}

	fclose(freelist_fp);

	fprintf(s_process_log_fp, "[info] create cdf %s for free files num=%d \n", s_freefiles_cdf_name.c_str(), free_counter);

	// create freefiles task
	task_attr task;
	task.name = s_freefiles_cdf_name;
	task.input_name = freelist_path;
	task.output_name = s_outputpath + "/" + s_freefiles_cdf_name;

	setup_cdf(&task);
}

void build_cdf(string input_path, string output_path, bool compress, int compress_level, string suffix)
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

	printf("***start cdf builder:\n --- in=%s, out=%s, zlib=%s, zlevel=%d, suffix=%s\n",
		input_path.c_str(), output_path.c_str(), compress ? "true" : "false", compress_level, suffix.c_str());

	fprintf(s_process_log_fp, "***start cdf builder:\n --- in=%s, out=%s, zlib=%s, zlevel=%d, suffix=%s\n",
		input_path.c_str(), output_path.c_str(), compress ? "true" : "false", compress_level, suffix.c_str());

	s_inputpath = input_path;
	s_outputpath = output_path;
	s_compress = compress;
	s_compress_level = compress_level;
	suffix;

	setup_index();
	setup_tasks();
	setup_cdfs();
	setup_freefiles();

	printf("***end cdf builder: \n --- (%d) error occured! \n",  s_error_counter);
	printf("***all done! \n");

	fprintf(s_process_log_fp, "***end cdf builder: \n --- (%d) error occured! \n",  s_error_counter);

	fclose(s_process_log_fp);
}
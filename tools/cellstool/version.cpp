#include "tools.h"

static string s_input_oldpath;
static string s_input_newpath;
static string s_outputpath;

static size_t s_error_counter = 0;

static FILE* s_process_log_fp = NULL;

static idxmap_t s_oldidxmap;
static idxmap_t s_newidxmap;
static idxmap_t* sp_processing_idxmap;

static set<string> s_added_set;
static set<string> s_modified_set;
static set<string> s_deleted_set;
static set<string> s_unchanged_set;

static bool process_folder(string full_path, string rel_path, string name)
{
	return true;
}

static bool process_file(string full_path, string rel_path, string name)
{
	// check is hash file
	string input_file = full_path + name;
	int suffixpos = name.rfind(s_hash_suffix.c_str());
	if ( suffixpos == -1 || suffixpos != name.size() - s_hash_suffix.size() )
	{
		return false;
	}
	string cell_name = rel_path + name.substr(0, suffixpos);

	FILE* input_fp = NULL;
	bool failed = true;

	do {
		input_fp = fopen(input_file.c_str(), "rb");
		if ( input_fp == NULL )
		{
			// error!
			fprintf(s_process_log_fp, "[error]can't access hash file: %s\n", input_file.c_str() );
			s_error_counter++;
			return false;
		}

		cell_attr* attr = new cell_attr;

		vector<string> strs;
		assert_break(parse_line(input_fp, strs));
		assert_break(strs.size() == 1);
		attr->omd5 = strs[0];
		strs.clear();
		assert_break(parse_line(input_fp, strs));
		assert_break(strs.size() == 1);
		attr->osize = CUtils::atoi(strs[0].c_str());
		strs.clear();
		bool zip = false;
		if ( parse_line(input_fp, strs) )
		{
			zip = true;
			assert_break(strs.size() == 1);
			attr->zmd5 = strs[0];
			strs.clear();
			assert_break(parse_line(input_fp, strs));
			assert_break(strs.size() == 1);
			attr->zsize = CUtils::atoi(strs[0].c_str());
			strs.clear();
		}
		
		sp_processing_idxmap->insert(std::make_pair(cell_name, attr));
		
		failed = false;
	} while(0); // do

	fclose(input_fp);

	if ( failed )
	{
		fprintf(s_process_log_fp, "[error]error occured when build version patch, file: %s\n", input_file.c_str() );
		s_error_counter++;
		return false;
	}

	return true;
}


static void visit_load(string path, string relpath = "")
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
				visit_load(path + "/" + file_info.name, relpath + "/" + file_info.name);            
			}
			else if(!(file_info.attrib & _A_SUBDIR))
			{ 
				printf("file:%s", string(relpath + "/"+ file_info.name).c_str());
				process_file(path + "/", relpath + "/", file_info.name) ? 
					printf("[OK]\n") : printf("[IGN]\n");
			} 
		}     
		_findclose(handle);        
	} 
}

static void build_changed()
{
	for ( idxmap_t::iterator nit = s_newidxmap.begin(); nit != s_newidxmap.end(); nit++ )
	{
		idxmap_t::iterator bingo_it = s_oldidxmap.find(nit->first);
		if ( bingo_it == s_oldidxmap.end() )
		{
			// new file
			s_added_set.insert(nit->first);
		}
		else
		{
			if ( bingo_it->second->omd5 == nit->second->omd5 &&
				bingo_it->second->osize == nit->second->osize &&
				bingo_it->second->zmd5 == nit->second->zmd5 &&
				bingo_it->second->zsize == nit->second->zsize)
			{
				// unchanged
				s_unchanged_set.insert(nit->first);
			}
			else
			{
				// modified
				s_modified_set.insert(nit->first);
			}
		}
	}

	for ( idxmap_t::iterator oit = s_oldidxmap.begin(); oit != s_oldidxmap.end(); oit++ )
	{
		idxmap_t::iterator bingo_it = s_newidxmap.find(oit->first);
		if ( bingo_it == s_newidxmap.end() )
		{
			// deleted
			s_deleted_set.insert(oit->first);
		}
	}
}

static bool copy_file(string name)
{
	string new_file = s_input_newpath + name;
	string new_file_hash = new_file + s_hash_suffix;
	string patch_file = s_outputpath + name;
	string patch_file_hash = patch_file + s_hash_suffix;

	// windows only
	BOOL ret = CopyFile(new_file.c_str(), patch_file.c_str(), false);
	if ( !ret )
	{
		ret = CUtils::builddir(patch_file.c_str());
		if ( !ret )
		{
			fprintf(s_process_log_fp, "[error]fatal error! can't copy file: %s\n", patch_file.c_str() );
			s_error_counter++;
			return false;
		}
	}
	ret = CopyFile(new_file_hash.c_str(), patch_file_hash.c_str(), false);
	if ( !ret )
	{
		fprintf(s_process_log_fp, "[error]fatal error! can't copy file: %s\n", patch_file_hash.c_str() );
		s_error_counter++;
		return false;
	}

	return true;
}

static void build_patched()
{
	CUtils::builddir(string(s_outputpath + "/" + s_added_files).c_str());
	FILE* added_fp = fopen(string(s_outputpath + "/" + s_added_files).c_str(), "w+");
	FILE* modified_fp = fopen(string(s_outputpath + "/" + s_modified_files).c_str(), "w+");
	FILE* deleted_fp = fopen(string(s_outputpath + "/" + s_deleted_files).c_str(), "w+");
	FILE* unchanged_fp = fopen(string(s_outputpath + "/" + s_unchanged_files).c_str(), "w+");

	if ( !added_fp )
	{
		error_msg("can't create added file list!");
	}
	if ( !modified_fp )
	{
		error_msg("can't create modified file list!");
	}
	if ( !deleted_fp )
	{
		error_msg("can't create deleted file list!");
	}
	if ( !unchanged_fp )
	{
		error_msg("can't create unchanged file list!");
	}

	for ( set<string>::iterator it = s_added_set.begin(); it != s_added_set.end(); it++ )
	{
		copy_file(*it);
		fprintf(added_fp, "%s\n", (*it).c_str());
	}

	for ( set<string>::iterator it = s_modified_set.begin(); it != s_modified_set.end(); it++ )
	{
		copy_file(*it);
		fprintf(modified_fp, "%s\n", (*it).c_str());
	}

	for ( set<string>::iterator it = s_deleted_set.begin(); it != s_deleted_set.end(); it++ )
	{
		fprintf(deleted_fp, "%s\n", (*it).c_str());
	}

	for ( set<string>::iterator it = s_unchanged_set.begin(); it != s_unchanged_set.end(); it++ )
	{
		fprintf(unchanged_fp, "%s\n", (*it).c_str());
	}

	fclose(added_fp);
	fclose(modified_fp);
	fclose(deleted_fp);
	fclose(unchanged_fp);
}

void build_version_patch(string old_path, string new_path, string output_path)
{
	// setup output path
	if ( access(output_path.c_str(), 0) != 0 && mkdir(output_path.c_str()) != 0 )
	{
		error_msg("can't create output path!");
	}
	// open log file
	s_process_log_fp = fopen(string(s_log_path + "/" + s_process_log).c_str(), "w+");
	if ( !s_process_log_fp )
	{
		error_msg("can't create log file!");
	}

	
	printf("***start version-patch builder:\n --- oldver=%s, newver=%s, patch_output=%s\n",
		old_path.c_str(), new_path.c_str(), output_path.c_str());

	fprintf(s_process_log_fp, "***start version patch builder:\n --- oldver=%s, newver=%s, patch_output=%s\n",
		old_path.c_str(), new_path.c_str(), output_path.c_str());

	s_input_oldpath = old_path;
	s_input_newpath = new_path;
	s_outputpath = output_path;

	// load old path
	sp_processing_idxmap = &s_oldidxmap;
	visit_load(s_input_oldpath);
	
	// load new path
	sp_processing_idxmap = &s_newidxmap;
	visit_load(s_input_newpath);

	// build changed
	build_changed();

	// output patched
	build_patched();

	printf("***end version-patch builder: \n");
	printf(" --- (%d) error occured! \n",  s_error_counter);
	printf(" --- (%d) old version files; (%d) new version files. \n",  s_oldidxmap.size(), s_newidxmap.size());
	printf(" --- (%d) added; (%d) modified; (%d) deleted; (%d) unchanged. \n", s_added_set.size(), s_modified_set.size(), s_deleted_set.size(), s_unchanged_set.size() );
	printf("***all done! \n");

	fprintf(s_process_log_fp, "***end version-patch builder: \n");
	fprintf(s_process_log_fp, " --- (%d) error occured! \n",  s_error_counter);
	fprintf(s_process_log_fp, " --- (%d) old version files; (%d) new version files. \n",  s_oldidxmap.size(), s_newidxmap.size());
	fprintf(s_process_log_fp, " --- (%d) added; (%d) modified; (%d) deleted; (%d) unchanged. \n", s_added_set.size(), s_modified_set.size(), s_deleted_set.size(), s_unchanged_set.size() );
	fprintf(s_process_log_fp, "***all done! \n");

	fclose(s_process_log_fp);
}
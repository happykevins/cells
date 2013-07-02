/*
 * main.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CUtils.h"
#include "cells.h"
#include <stdio.h>
#include <vector>
#include <algorithm>

using namespace std;
using namespace cells;


bool cdf_ok = false;
bool all_done = false;
class Observer
{
public:
	void on_finish(
		estatetype_t type, const std::string& name, eloaderror_t error_no, 
		const props_t* props, const props_list_t* ready_props, const props_list_t* pending_props,
		void* context)
	{
		if ( type == e_state_event_alldone )
		{
			all_done = true;
			printf("*****************************all task done*******************************\n");
			return;
		}
		if ( error_no == 0) return;
		printf("**Observer::on_finish: name=%s; type=%d; error=%d;\n", name.c_str(), type, error_no);

		if ( error_no != 0 )
		{
			return;
		}


		printf("--**Props:");
		for (props_t::const_iterator it = props->begin(); it != props->end(); it++)
		{
			printf("[%s=%s] ", it->first.c_str(), it->second.c_str());
		}
		printf("\n");
	}
};

//
//	run example:
//		- use "cellstool" make your own resource. 1.build files 2.setup CDF
//		- put your resource on http or ftp server
//		- 
//

int main(int argc, char *argv[])
{	
	Observer obs;

	CProgressWatcher watcher;

	CRegulation rule;
	rule.auto_dispatch = true;
	rule.only_local_mode = false;
	rule.worker_thread_num = 1;
	rule.max_download_speed = 1024 * 1024 * 10; 
	rule.enable_ghost_mode = false;
	rule.max_ghost_download_speed = 1024 * 1024;
	rule.local_url = "./downloads/";	// local path
	rule.remote_urls.push_back("ftp://guest:guest@localhost/resource1");	// remote resource url1
	rule.remote_urls.push_back("ftp://guest:guest@localhost/resource2");	// remote resource url2

	CellsHandler* cells = cells_create(rule);
	if ( !cells )
	{
		return 0;
	}

	cells->register_observer(&obs, make_functor_m(&obs, &Observer::on_finish));
	
	// require the "index.xml" CDF, and check/load files in CDF cascade
	cells->post_desire_cdf("_cdf/index.xml", e_priority_exclusive, e_cdf_loadtype_load_cascade, e_zip_none);
		
	while( !all_done )
	{
		CUtils::sleep(20);
		if (!rule.auto_dispatch)
			cells->tick_dispatch(0.02f);
	}

	all_done = false;

	// require a single file
	cells->post_desire_file("a_big_file.bin", e_priority_default, e_zip_zlib, NULL, &watcher);

	// require the "free.xml" CDF, only setup file index in the CDF cascade
	cells->post_desire_cdf("cdf/free.xml", e_priority_exclusive, e_cdf_loadtype_index_cascade, e_zip_none);


	while(true)
	{
		CUtils::sleep(20);
		if (!rule.auto_dispatch)
			cells->tick_dispatch(0.02f);

		static int step_prev = 0;
		static int prog_prev = 0;

		int prog_now = (int)watcher.progress();

		if ( step_prev != watcher.step || prog_now != prog_prev )
		{
			printf("WATCH: STEP=%d, PROG=%d%%\n", watcher.step, prog_now);
			step_prev = watcher.step;
			prog_prev = prog_now;
		}
	}

	cells->remove_observer(&obs);

	cells_destroy(cells);

	return 0;
}


/*
 * main.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "cells.h"
#include "CUtils.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <pthread.h>

using namespace std;
using namespace cells;


bool cdf_ok = false;
bool all_done = false;
class Observer
{
public:
	void on_finish(estatetype_t type, const std::string& name, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
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

void on_finish(estatetype_t type, const std::string& name, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
{
	printf("**Global::on_finish: name=%s; type=%d; error=%d;\n", name.c_str(), type, error_no);
	if ( error_no != 0 )
	{
		return;
	}
	cdf_ok = true;

	printf("--**Props:");
	for (props_t::const_iterator it = props->begin(); it != props->end(); it++)
	{
		printf("[%s=%s] ", it->first.c_str(), it->second.c_str());
	}
	printf("\n");
}

int main(int argc, char *argv[])
{	
	Observer obs;

	CRegulation rule;
	rule.auto_dispatch = true;
	rule.only_local_mode = false;
	//rule.zip_type = e_nozip;
	rule.zip_type = e_zlib;
	rule.zip_cdf = false;
	rule.worker_thread_num = 4;
	rule.max_download_speed = 1024 * 1024 * 5; 
	rule.enable_ghost_mode = true;
	rule.max_ghost_download_speed = 1024 * 1024;
	rule.local_url = "./downloads/";
	rule.remote_urls.push_back("http://localhost/output");
	//rule.remote_urls.push_back("ftp://guest:guest@localhost/uploadz/");
	//rule.remote_urls.push_back("ftp://guest:guest@localhost/cells_test/output/");

	CellsHandler* cells = cells_create(rule);
	if ( !cells )
	{
		return 0;
	}
	//cells.suspend();

	//cells.register_observer(&on_finish, make_functor_g(on_finish));
	cells->register_observer(&obs, make_functor_m(&obs, &Observer::on_finish));

	cells->post_desire_cdf("index.xml", e_priority_exclusive, e_cdf_loadtype_load_cascade);

	//while( !all_done )
	//{
	//	CUtils::sleep(500);
	//}

	cells->post_desire_cdf("cells_cdf_freefiles.xml", e_priority_exclusive, e_cdf_loadtype_index_cascade);


	while(true)
	{
		CUtils::sleep(50);
		if (!rule.auto_dispatch)
			cells->tick_dispatch(0.05f);
	}

	cells->remove_observer(&obs);
	//cells->remove_observer(&on_finish);

	cells_destroy(cells);

	return 0;
}


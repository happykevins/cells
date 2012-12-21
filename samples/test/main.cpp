/*
 * main.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CUtils.h"
#include "CCells.h"
#include <stdio.h>
#include <vector>
#include <algorithm>
#include "zpip.h"

using namespace std;
using namespace cells;

class Observer
{
public:
	void on_finish(const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
	{
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

bool cdf_ok = false;
void on_finish(const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
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

	CCells cells;
	CRegulation rule;
	rule.auto_dispatch = true;
	rule.only_local_mode = false;
	rule.zip_type = e_zlib;
	rule.zip_cdf = false;
	rule.worker_thread_num = 4;
	rule.max_download_speed = 1024 * 1024 * 1; 
	rule.enable_ghost_mode = true;
	rule.max_ghost_download_speed = 1024 * 1024;
	rule.local_url = "./downloads/";
	//rule.remote_urls.push_back("http://192.168.0.1/upload/");
	//rule.remote_urls.push_back("ftp://guest:guest@localhost/uploadz/");
	rule.remote_urls.push_back("ftp://guest:guest@localhost/cells_test/output/");

	cells.initialize(rule);
	//cells.suspend();

	//cells.register_observer(&on_finish, make_functor_g(on_finish));
	cells.register_observer(&obs, make_functor_m(&obs, &Observer::on_finish));

	cells.post_desire_cdf("index.xml", e_priority_exclusive, e_cdf_loadtype_load_cascade);
	cells.post_desire_cdf("cells_cdf_freefiles.xml", e_priority_exclusive, e_cdf_loadtype_load_cascade);

	//while (!cdf_ok)
	//{
	//	CUtils::sleep(1000);
	//}

	//cells.post_desire_file("baby.jpg");
	//cells.post_desire_file("music.ogg");

	//CUtils::sleep(5000);
	//cells.resume();

	CUtils::sleep(10 * 1000);
	cells.set_speedfactor(0.5);

	while(true)
	{
		CUtils::sleep(500);
		if (!rule.auto_dispatch)
			cells.tick_dispatch(0.05f);
	}

	cells.remove_observer(&obs);
	cells.remove_observer(&on_finish);

	cells.destroy();

	return 0;
}


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
	rule.worker_thread_num = 2;
	rule.local_url = "./downloads/";
	//rule.remote_urls.push_back("http://192.168.0.1/upload/");
	rule.remote_urls.push_back("ftp://guest:guest@localhost/uploadz/");

	cells.initialize(rule);
	//cells.suspend();

	cells.register_observer(&on_finish, make_functor_g(on_finish));
	cells.register_observer(&obs, make_functor_m(&obs, &Observer::on_finish));

	cells.post_desire_cdf("cdf.xml");

	//while (!cdf_ok)
	//{
	//	CUtils::sleep(1000);
	//}

	//cells.post_desire_file("baby.jpg");
	//cells.post_desire_file("music.ogg");

	//CUtils::sleep(5000);
	//cells.resume();

	while(true)
	{
		CUtils::sleep(1000);
		if (!rule.auto_dispatch)
			cells.tick_dispatch();
	}

	cells.remove_observer(&obs);
	cells.remove_observer(&on_finish);

	cells.destroy();

	return 0;
}


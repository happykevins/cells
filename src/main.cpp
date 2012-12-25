/*
 * main.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "cells.h"
#include "CUtils.h"
#include "CCells.h"
#include <stdio.h>

using namespace std;
using namespace cells;

bool cdf_ready = false;
class Observer
{
public:
	void on_finish(estatetype_t type, const std::string& name, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
	{
		printf("*notify user: name=%s; type=%d; error=%d;\n", name.c_str(), type, error_no);
		if ( type == cells::e_state_file_cdf && error_no == cells::e_loaderr_ok )
			cdf_ready = true;
	}
};

int main()
{
	Observer obs;

	CCells cells;
	CRegulation rule;
	rule.auto_dispatch = true;
	rule.worker_thread_num = 1;
	rule.zip_type = e_nozip;
	rule.zip_cdf = false;
	rule.local_url = "./downloads/";
	//rule.remote_urls.push_back("http://192.168.0.1/upload/");
	rule.remote_urls.push_back("http://localhost/upload/");
	cells.initialize(rule);

	cells.register_observer(&obs, make_functor_m(&obs, &Observer::on_finish));

	cells.post_desire_cdf("cdf.xml");
	while(!cdf_ready)
	{
		CUtils::sleep(50);
		if (!rule.auto_dispatch)
			cells.tick_dispatch(0.05f);
	}

	cells.post_desire_file("baby.jpg");
	cells.post_desire_file("jpg/baby.jpg");
	//cells.post_desire_file("music.ogg");

	while(true)
	{
		CUtils::sleep(50);
		if (!rule.auto_dispatch)
			cells.tick_dispatch(0.05f);
	}

	cells.remove_observer(&obs);

	cells.destroy();

	return 0;
}


/*
 * main.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CUtils.h"
#include "CCells.h"
#include <stdio.h>

class Observer
{
public:
	void on_finish(const char* name, int type, int error_no)
	{
		printf("*notify user: name=%s; type=%d; error=%d;\n", name, type, error_no);
	}
};

int main()
{
	using namespace std;
	using namespace cells;
	Observer obs;

	CCells cells;
	CRegulation rule;
	rule.auto_dispatch = true;
	rule.worker_thread_num = 2;
	rule.local_url = "./downloads/";
	//rule.remote_urls.push_back("http://192.168.0.1/upload/");
	rule.remote_urls.push_back("http://localhost/upload/");
	cells.initialize(rule);

	cells.register_observer(&obs, make_functor(&obs, &Observer::on_finish));

	cells.post_desire_cdf("cdf.xml");
	cells.post_desire_file("baby.jpg");
	cells.post_desire_file("music.ogg");

	while(true)
	{
		CUtils::sleep(1000);
		if (!rule.auto_dispatch)
			cells.tick_dispatch();
	}

	cells.remove_observer(&obs);

	cells.destroy();

	return 0;
}


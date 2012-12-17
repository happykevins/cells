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
		printf("**Observer::on_finish: name=%s; type=%d; error=%d;\n", name, type, error_no);
	}
};

bool cdf_ok = false;
void on_finish(const char* name, int type, int error_no)
{
	cdf_ok = true;
	printf("**Global::on_finish: name=%s; type=%d; error=%d;\n", name, type, error_no);
}

extern "C" int main_md5(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	//main_md5(argc, argv);
	//return 0;
	using namespace std;
	using namespace cells;
	Observer obs;

	CCells cells;
	CRegulation rule;
	rule.auto_dispatch = true;
	rule.worker_thread_num = 1;
	rule.local_url = "./downloads/";
	//rule.remote_urls.push_back("http://192.168.0.1/upload/");
	rule.remote_urls.push_back("ftp://guest:guest@localhost/upload/");
	cells.initialize(rule);

	cells.register_observer(&on_finish, make_functor_g3(on_finish));
	cells.register_observer(&obs, make_functor_m3(&obs, &Observer::on_finish));

	cells.post_desire_cdf("cdf.xml");

	while (!cdf_ok)
	{
		CUtils::sleep(1000);
	}

	cells.post_desire_file("baby.jpg");
	cells.post_desire_file("music.ogg");

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


/*
 * CDownloader.h
 *
 *  Created on: 2012-12-12
 *      Author: happykevins@gmail.com
 */

#ifndef CDOWNLOADER_H_
#define CDOWNLOADER_H_

#include <cstddef>
#include <stdio.h>

namespace cells
{

class CCell;
class CCreationWorker;

typedef void download_handle_t;

class CDownloader
{
public:
	enum edownloaderr_t
	{
		e_downloaderr_ok = 0,
		e_downloaderr_connect,
		e_downloaderr_timeout,
		e_downloaderr_notfound,
		e_downloaderr_other_nobp,
	};
public:
	CDownloader(CCreationWorker* host);
	~CDownloader();

	edownloaderr_t download(const char* url, FILE* fp, bool bp_resume, size_t bp_range_begin);

private:
	static size_t process_data(void* buffer, size_t size, size_t nmemb, void* context);

	CCreationWorker* m_host;
	download_handle_t* m_handle;
	FILE* m_stream;
};

} /* namespace cells */
#endif /* CDOWNLOADER_H_ */

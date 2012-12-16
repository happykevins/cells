/*
 * CDownloader.h
 *
 *  Created on: 2012-12-12
 *      Author: happykevins@gmail.com
 */

#ifndef CDOWNLOADER_H_
#define CDOWNLOADER_H_

#include <cstddef>

namespace cells
{

class CCell;
class CCreationWorker;

typedef void download_handle_t;

class CDownloader
{
public:
	CDownloader(CCreationWorker* host);
	~CDownloader();

	bool download(CCell* cell, const char* url);

private:
	static size_t process_data(void* buffer, size_t size, size_t nmemb, void* context);

	CCreationWorker* m_host;
	download_handle_t* m_handle;
	CCell* m_cell;
};

} /* namespace cells */
#endif /* CDOWNLOADER_H_ */

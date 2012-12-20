/*
 * CDownloader.cpp
 *
 *  Created on: 2012-12-12
 *      Author: happykevins@gmail.com
 */

#include "CDownloader.h"

#include <curl/curl.h>
#include <stdio.h>
#include <assert.h>

#include "CCreationWorker.h"

namespace cells
{

size_t CDownloader::process_data(void* buffer, size_t size, size_t nmemb,
		void* context)
{
	CDownloader* handle = (CDownloader*) context;
	assert(handle);

	CCell* cell = handle->m_cell;
	assert(cell && cell->m_stream);

	FILE* fp = (FILE*) cell->m_stream;
	size_t cbs = fwrite(buffer, size, nmemb, fp);

	if (handle->m_host->congestion_control())
		fflush(fp);

	return cbs;
}

CDownloader::CDownloader(CCreationWorker* host) :
		m_host(host), m_handle(NULL), m_cell(NULL)
{
	m_handle = curl_easy_init();
	assert(m_handle);
	curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION,
			CDownloader::process_data);
	// TODO: 这个并不是连接超时，而是整个socket重置的时间，下载中也会中断
	curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, 30l);
	curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, 1L);
	// TODO: 监测是否会产生大量CLOSE_WAIT
	//curl_easy_setopt(m_handle, CURLOPT_FORBID_REUSE, 1);
	curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, this);
}

CDownloader::~CDownloader()
{
	curl_easy_cleanup(m_handle);
}

bool CDownloader::download(CCell* cell, const char* url)
{
	assert(cell);
	m_cell = cell;
	curl_easy_setopt(m_handle, CURLOPT_URL, url);
	int result = curl_easy_perform(m_handle);
	m_cell = NULL;
	return result == CURLE_OK;
}

} /* namespace cells */

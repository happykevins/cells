/*
 * CDownloader.cpp
 *
 *  Created on: 2012-12-12
 *      Author: happykevins@gmail.com
 */

#include "CDownloader.h"
#include "CPlatform.h"

#include <curl/curl.h>
#include <stdio.h>
#include <assert.h>
#include <sstream>

#include "CCreationWorker.h"

namespace cells
{

size_t CDownloader::process_data(void* buffer, size_t size, size_t nmemb,
		void* context)
{
	CDownloader* handle = (CDownloader*) context;
	assert(handle && handle->m_stream);
	FILE* fp = (FILE*) handle->m_stream;
	size_t cbs = fwrite(buffer, size, nmemb, fp);

	if (handle->m_host->on_download_bytes(size*nmemb))
		fflush(fp);

	return cbs;
}

CDownloader::CDownloader(CCreationWorker* host) :
		m_host(host), m_handle(NULL), m_stream(NULL)
{
	m_handle = curl_easy_init();
	assert(m_handle);
	curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION,
			CDownloader::process_data);

	curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, 5l);
	curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, 0l);
	curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, 1L);
	// TODO: 监测是否会产生大量CLOSE_WAIT
	//curl_easy_setopt(m_handle, CURLOPT_FORBID_REUSE, 1);
	curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, this);
}

CDownloader::~CDownloader()
{
	curl_easy_cleanup(m_handle);
}

CDownloader::edownloaderr_t CDownloader::download(const char* url, FILE* fp, bool bp_resume, size_t bp_range_begin)
{
	assert(fp);
	m_stream = fp;
	curl_off_t mspeed = m_host->calc_maxspeed();
	curl_easy_setopt(m_handle, CURLOPT_MAX_RECV_SPEED_LARGE, mspeed);
	curl_easy_setopt(m_handle, CURLOPT_URL, url);

	// 设置断点续传
	if ( bp_resume )
	{
		std::stringstream ss;
		ss << bp_range_begin << "-";
		curl_easy_setopt(m_handle, CURLOPT_RANGE, ss.str().c_str()); 
		CLogI("download %s from break point: %d\n", url, bp_range_begin);
	}
	else
	{
		curl_easy_setopt(m_handle, CURLOPT_RANGE, "0-"); 
	}

	int retv = curl_easy_perform(m_handle);
	m_stream = NULL;

	edownloaderr_t result = e_downloaderr_ok;
	switch(retv)
	{
	case CURLE_OK:
		result = e_downloaderr_ok;
		break;
	case CURLE_COULDNT_RESOLVE_PROXY:
	case CURLE_URL_MALFORMAT:
	case CURLE_COULDNT_RESOLVE_HOST:
	case CURLE_COULDNT_CONNECT:
	case CURLE_REMOTE_ACCESS_DENIED:
	case CURLE_FTP_WEIRD_SERVER_REPLY:
	case CURLE_FTP_WEIRD_PASS_REPLY:
	case CURLE_FTP_WEIRD_PASV_REPLY:
	case CURLE_FTP_CANT_GET_HOST:
	case CURLE_FTP_WEIRD_227_FORMAT:
	case CURLE_FTP_USER_PASSWORD_INCORRECT:
	case CURLE_FTP_PRET_FAILED:
	case CURLE_HTTP_POST_ERROR:
	case CURLE_FAILED_INIT:
	case CURLE_SEND_ERROR:
	case CURLE_RECV_ERROR:
	case CURLE_LOGIN_DENIED:
	case CURLE_AGAIN:
		CLogE("curl returned connect error: %d\n", retv);
		result = e_downloaderr_connect;
		break;
	case CURLE_HTTP_RETURNED_ERROR:
	case CURLE_REMOTE_FILE_NOT_FOUND:
		CLogE("curl returned file not exist error: %d\n", retv);
		result = e_downloaderr_notfound;
		break;
	case CURLE_OPERATION_TIMEOUTED:
		CLogE("curl returned timeout error: %d\n", retv);
		result = e_downloaderr_timeout;
		break;
	default:
		CLogE("curl returned fatal error: %d\n", retv);
		result = e_downloaderr_other_nobp;
	}

	return result;
}

} /* namespace cells */

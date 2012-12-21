/*
 * CCreationWorker.h
 *
 *  Created on: 2012-12-12
 *      Author: happykevins@gmail.com
 */

#ifndef CCREATIONWORKER_H_
#define CCREATIONWORKER_H_

#include "CContainer.h"
#include "CCell.h"
#include "CDownloader.h"

namespace cells
{

class CCreationFactory;

#define CWORKER_BUFFER_SIZE 16384

/*
 * CCreationWorker
 * 	创造cell工作线程
 * 	1.验证本地cell是否合法
 * 	2.在指定的url下载cell
 */
class CCreationWorker
{
public:
	CCreationWorker(CCreationFactory* host, size_t no);
	virtual ~CCreationWorker();

public:
	virtual void post_work(CCell* cell);
	virtual void do_work();
	virtual size_t workload();		// 负载情况
	size_t get_downloadbytes();

protected:
	virtual bool work_verify_local(CCell* cell);
	virtual bool work_download_remote(CCell* cell);
	virtual bool work_decompress(const char* tmplocalurl, const char* localurl);
	virtual bool work_patchup_cell(CCell* cell, const char* localurl);
	virtual void work_finished(CCell* cell);

	virtual size_t calc_maxspeed(); 

private:
	static void* working(void* context);

	// downloader callback
	// @return - should flush to disk
	bool on_download_bytes(size_t bytes);

protected:
	CCreationFactory* m_host;
	const size_t m_workno;

private:
	volatile bool m_working;
	pthread_t m_thread;
	sem_t m_sem;
	CQueue<CCell*> m_queue;
	CDownloader m_downloadhandle;
	char m_databuf[CWORKER_BUFFER_SIZE];

	// congestion stat.
	volatile size_t m_downloadbytes;
	size_t m_cachedbytes;			// no flush bytes

	friend class CDownloader;
};

/*
 * CGhostWorker - ghost模式的工作线程
 */
class CGhostWorker : public CCreationWorker
{
public:
	CGhostWorker(CCreationFactory* host, size_t no);
	virtual ~CGhostWorker();

protected:
	virtual size_t calc_maxspeed(); 
};

} /* namespace cells */
#endif /* CCREATIONWORKER_H_ */

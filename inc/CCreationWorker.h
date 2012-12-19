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
	CCreationWorker(CCreationFactory* host);
	virtual ~CCreationWorker();

public:
	virtual void post_work(CCell* cell);
	virtual void do_work();
	bool is_free();
	size_t workload();

protected:
	virtual bool work_verify_local(CCell* cell);
	virtual bool work_download_remote(CCell* cell);
	virtual bool work_decompress(const char* localurl, const char* tmpurl);
	virtual bool work_patchup_cell(CCell* cell, const char* localurl);
	virtual void work_finished(CCell* cell);

	virtual bool congestion_control();

private:
	static void* working(void* context);

	CCreationFactory* m_host;

	volatile bool m_working;
	pthread_t m_thread;
	sem_t m_sem;
	CQueue<CCell*> m_queue;
	CDownloader m_downloadhandle;
	char m_databuf[CWORKER_BUFFER_SIZE];

	friend class CDownloader;
};

} /* namespace cells */
#endif /* CCREATIONWORKER_H_ */

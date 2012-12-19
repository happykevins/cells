/*
 * CCreationFactory.cpp
 *
 *  Created on: 2012-12-13
 *      Author: happykevins@gmail.com
 */

#include "CCreationFactory.h"

#include <assert.h>
#include <list>
#include <stdio.h>

#include "CUtils.h"
#include "CCells.h"
#include "CCreationWorker.h"

namespace cells
{

CCreationFactory::CCreationFactory(CCells* host, size_t worker_num) :
		m_host(host), m_task_counter(0)
{
	for (size_t i = 0; i < worker_num; i++)
	{
		m_workers.push_back(new CCreationWorker(this));
	}
}

CCreationFactory::~CCreationFactory()
{
	for (size_t i = 0; i < m_workers.size(); i++)
	{
		delete m_workers[i];
	}
	m_workers.clear();
}

void CCreationFactory::post_work(CCell* cell)
{
	assert(cell);

	printf(
			"post work: name=%s; type=%d; errorno=%d; times=%d; \n",
			cell->m_name.c_str(), cell->m_celltype, cell->m_errorno,
			(int)cell->m_download_times);

	//
	// 这里要确保同一时刻只有一个cell被加载
	//

	// loading中，不需要投递，等待完成后dispatch即可
	if(cell->m_cellstate == CCell::loading)
	{
		return;
	}
	// 非unknow状态，直接投递finish队列
	else if(cell->m_cellstate != CCell::unknow)
	{
		m_finished.lock();
		m_finished.push(cell);
		m_finished.unlock();
		return;
	}

	// 只在此线程中修改cell状态
	cell->m_cellstate = CCell::loading;

	// TODO: 可实现更优化的根据负载判定选择的worker
	size_t worker_id = m_task_counter % m_workers.size();
	m_workers[worker_id]->post_work(cell);
	m_task_counter++;

	return;
}

CCell* CCreationFactory::pop_result()
{
	CCell* cell = NULL;
	m_finished.lock();
	if (!m_finished.empty())
	{
		cell = m_finished.pop_front();
	}
	m_finished.unlock();
	return cell;
}

void CCreationFactory::notify_work_finished(CCell* cell)
{
	assert(cell);
	m_finished.lock();
	m_finished.push(cell);
	m_finished.unlock();
}

} /* namespace cells */

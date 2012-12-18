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

	// loading中，不需要投递
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
}

size_t CCreationFactory::dispatch_result()
{
	const size_t MAX_DISPATCH_NUM = 20;
	size_t dispatch_num = 0;

	while (dispatch_num < MAX_DISPATCH_NUM)
	{
		m_finished.lock();
		if (m_finished.empty())
		{
			m_finished.unlock();
			break;
		}
		CCell* cell = m_finished.pop_front();
		m_finished.unlock();

		// 成功
		if ( cell->m_errorno == loaderr_ok )
		{
			dispatch_num++;

			printf(
					"work down: name=%s; state=%d; errorno=%d; times=%d; \n",
					cell->m_name.c_str(), cell->m_cellstate, cell->m_errorno,
					(int)cell->m_download_times);

			// 处理cdf
			if ( cell->m_celltype == e_celltype_cdf && cell->get_cdf() )
			{
				setup_cdf(cell);
			}

			// 设置完成校验标记
			cell->m_cellstate = CCell::verified;
		}
		// 下载失败，再做一下尝试
		else if (cell->m_errorno == loaderr_download_failed
				&& cell->m_download_times < m_host->regulation().remote_urls.size())
		{
			cell->m_cellstate = CCell::unknow;
			cell->m_errorno = loaderr_ok;
			post_work(cell);
			continue;
		}
		// 出错了
		else
		{
			// local only mode hack!
			if ( m_host->regulation().only_local_mode && cell->m_errorno == loaderr_verify_failed )
			{
				dispatch_num++;

				printf(
					"local hack done!: name=%s; state=%d; errorno=%d; times=%d; \n",
					cell->m_name.c_str(), cell->m_cellstate, cell->m_errorno,
					(int)cell->m_download_times);

				// 处理cdf
				if ( cell->m_celltype == e_celltype_cdf && cell->get_cdf() )
				{
					setup_cdf(cell);
				}

				cell->m_errorno = loaderr_ok;
				cell->m_cellstate = CCell::verified;
			}
			else
			{
				cell->m_cellstate = CCell::error;
			}
		}

		// 通知加载完成
		m_host->notify_observers(cell);
	}

	return dispatch_num;
}

void CCreationFactory::setup_cdf(CCell* cell)
{
	assert(cell->get_cdf());
	// load all sub-cells?
	bool loadall = false;
	properties_t::iterator cdf_prop_it =
		cell->m_props.find(CDF_LOADALL);

	if (cdf_prop_it != cell->m_props.end()
		&& CUtils::atoi((*cdf_prop_it).second.c_str()) == 1)
	{
		loadall = true;
	}

	for (celllist_t::iterator it =
		cell->get_cdf()->m_subcells.begin();
		it != cell->get_cdf()->m_subcells.end();)
	{
		CCell* subcell = *it;

		m_host->m_cellidx.lock();
		cellidx_t::iterator idxcell_it = m_host->m_cellidx.find(
			subcell->m_name);

		if (idxcell_it != m_host->m_cellidx.end())
		{
			m_host->m_cellidx.unlock();

			// 重名,将cdf中的cell销毁，并换成idx的cell
			CCell* idxcell = (*idxcell_it).second;
			assert(idxcell);

			it = cell->get_cdf()->m_subcells.erase(it);
			delete subcell;
			subcell = idxcell;
			cell->get_cdf()->m_subcells.insert(it, idxcell);
		}
		else
		{
			// 将cdf的cell插入idx
			m_host->m_cellidx.insert(subcell->m_name, subcell);
			m_host->m_cellidx.unlock();

			it++;
		}

		bool postload = loadall;
		if ( !postload )
		{
			properties_t::iterator prop_it = subcell->m_props.find(CDF_CELL_LOAD);
			if (prop_it != subcell->m_props.end()
				&& CUtils::atoi((*prop_it).second.c_str()) == 1)
			{
				postload = true;
			}
		}

		// 级联加载
		if ( postload )
		{
			/** 风险！cdf互相需求会导致无尽循环,所以不对cdf级联
			// TODO: 加入一个CDF索引表，在需求CDF前检查状态，如果已经需求过就不再需求
			subcell->m_celltype == CCell::cdf ?
			m_host->post_desire_cdf(subcell->m_name.c_str()) :
			m_host->post_desire_file(subcell->m_name.c_str());
			*/
			m_host->post_desire_file(subcell->m_name.c_str(), cell->m_priority);
		}
	}
}

void CCreationFactory::on_work_finished(CCell* cell)
{
	assert(cell);
	m_finished.lock();
	m_finished.push(cell);
	m_finished.unlock();
}

} /* namespace cells */

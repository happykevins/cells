/*
 * CCells.cpp
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#include "CCells.h"

#include <assert.h>
#include <stdio.h>
#include "CCell.h"
#include "CUtils.h"
#include "CCreationFactory.h"

namespace cells
{

const double CELLS_WORKING_LOAD = 0.05f;

static pthread_t s_thread;
static volatile bool s_running = false;

void* CCells::cells_working(void* context)
{
	CCells* cells = (CCells*)context;
	assert(cells->regulation().auto_dispatch);

	double last_time = CUtils::gettime_seconds();
	double this_time = .0f;
	double delta_time = .0f;
	while( s_running )
	{
		if ( cells->is_suspend() )
		{
			continue;
		}

		this_time = CUtils::gettime_seconds();

		delta_time = this_time - last_time;
		if ( delta_time < CELLS_WORKING_LOAD )
		{
			CUtils::sleep( (unsigned int) ((CELLS_WORKING_LOAD - delta_time) * 1000) );
			delta_time = CELLS_WORKING_LOAD;
		}

		last_time = this_time;

		cells->tick_dispatch(delta_time);
	}

	return NULL;
}

CCells::CCells() :
		m_factory(NULL), m_suspend(true)
{

}

bool CCells::initialize(const CRegulation& rule)
{
	if ( m_factory )
		return false;

	m_rule = rule;

	// 有效性验证
	if ( m_rule.worker_thread_num < 1 ||  m_rule.worker_thread_num > 32 )
	{
		CLogE("[Cells Error]: invalid worker num %d!\n", m_rule.worker_thread_num );
		return false;
	}

	if ( CUtils::str_trim(m_rule.tempfile_suffix).empty() )
	{
		CLogE("[Cells Error]: invalid suffix name %s!\n", m_rule.tempfile_suffix );
		return false;
	}

	m_factory = new CCreationFactory(this, m_rule.worker_thread_num);

	s_running = true;
	if ( m_rule.auto_dispatch )
		pthread_create(&s_thread, NULL, CCells::cells_working, this);

	resume();

	return true;
}

void CCells::destroy()
{
	s_running = false;
	if ( m_rule.auto_dispatch )
		pthread_join(s_thread, NULL);

	delete m_factory;
	m_factory = NULL;

	// 销毁ghost工作列表
	m_ghosttasks.clear();

	// 销毁请求列表
	m_desires.lock();
	while(!m_desires.empty()) 
	{
		delete m_desires.front();
		m_desires.pop();
	}
	m_desires.unlock();

	// 销毁等待列表
	for ( taskmap_t::iterator it = m_taskloading.begin(); it != m_taskloading.end(); it++ )
	{
		delete it->second;
	}
	m_taskloading.clear();

	// 销毁observers
	m_observers.lock();
	for (observeridx_t::iterator it = m_observers.begin();
			it != m_observers.end(); it++)
	{
		delete it->second;
	}
	m_observers.clear();
	m_observers.unlock();

	// 销毁cdf index
	m_cdfidx.lock();
	m_cdfidx.clear();
	m_cdfidx.unlock();

	// 确保最后销毁m_cellidx
	m_cellidx.lock();
	for (cellidx_t::iterator it = m_cellidx.begin(); it != m_cellidx.end(); it++)
	{
		delete (*it).second;
	}
	m_cellidx.clear();
	m_cellidx.unlock();
}

const CRegulation& CCells::regulation() const
{
	return m_rule;
}

void CCells::resume()
{
	m_suspend = false;
}

void CCells::suspend()
{
	m_suspend = true;
}

bool CCells::is_suspend()
{
	return m_suspend;
}

void CCells::tick_dispatch(double dt)
{
	static double dump_dt = 0;
	static size_t last_bytes = 0;
	dump_dt += dt;
	if ( dump_dt >= 1.0f )
	{
		size_t tb = m_factory->count_downloadbytes();
		size_t wl = m_factory->count_workload();
		size_t sp = size_t( (tb - last_bytes) / dump_dt ); 
		CLog("STAT: WL=%d, KB/s=%d, TB=%d\n", wl, sp / 1024, tb);
		dump_dt = 0;
		last_bytes = tb;
	}
	
	if ( m_suspend )
		return;

	bool i_am_busy = false; 

	// 分发结果
	for ( CCell* cell = m_factory->pop_result(); cell != NULL; cell = m_factory->pop_result())
	{
		on_task_finish(cell);
		i_am_busy = true;
	}

	// 分发请求
	m_desires.lock();
	if ( !m_desires.empty() )
	{
		bool only_ghost = regulation().enable_ghost_mode && m_desires.front()->ghost_task;

		size_t max_workload = only_ghost ? CELLS_WORKER_MAXWORKLOAD : regulation().worker_thread_num * CELLS_WORKER_MAXWORKLOAD;
		size_t now_workload = m_factory->count_workload();
		size_t max_post_num = now_workload < max_workload ? max_workload - now_workload : 0;
		if ( !only_ghost || max_post_num == 0 )
		{
			i_am_busy = true;
		}

		for( size_t i = 0; !m_desires.empty() && i < max_post_num; i++ )
		{
			CCellTask* task = m_desires.front();
			m_factory->post_work(task->cell(), task->ghost_task);
			m_desires.pop();
			m_taskloading.insert(std::make_pair(task->cell(), task));
		}
	}
	m_desires.unlock();

	// tick ghost, only when i am not busy
	if ( !i_am_busy && regulation().enable_ghost_mode )
	{
		ghost_working();
	}
}

bool CCells::post_desire_cdf(const std::string& name, 
							 int priority, /*= priority_exclusive*/
							 ecdf_loadtype_t cdf_load_type, /*= e_cdf_loadtype_config*/
							 void* user_context /*= NULL*/)
{
	if ( name.empty() ) return false;

	return post_desired(name, e_celltype_cdf, priority, user_context, cdf_load_type) != NULL;
}

bool CCells::post_desire_file(const std::string& name, 
							  int priority, /*= priority_default*/
							  void* user_context /*= NULL*/)
{
	if ( name.empty() ) return false;

	return post_desired(name, e_celltype_common, priority, user_context) != NULL;
}

void CCells::register_observer(void* target, CFunctorBase* func)
{
	m_observers.lock();
	m_observers.insert(target, func);
	m_observers.unlock();
}

void CCells::remove_observer(void* target)
{
	m_observers.lock();
	observeridx_t::iterator it = m_observers.find(target);
	if ( it != m_observers.end() )
	{
		delete (*it).second;
		m_observers.erase(target);
	}
	m_observers.unlock();
}

void CCells::set_speedfactor(float f)
{
	f = f < 0.0f ? 0.0f : f;
	f = f > 1.0f ? 1.0f : f;
	m_factory->set_speedfactor(f);
}


CCell* CCells::post_desired(const std::string& _name, ecelltype_t type, int priority, void* user_context, ecdf_loadtype_t cdf_load_type)
{
	assert(priority <= e_priority_exclusive);
	if ( priority > e_priority_exclusive ) priority = e_priority_exclusive;

	// 处理名字
	std::string name = CUtils::str_trim(std::string(_name));
	if ( name.empty() ) return NULL;
	CUtils::str_replace_ch(name, '\\', '/');
	if ( name.find_first_of('/') != 0 )	name = "/" + name;


	CLogD( "post desired: name=%s; type=%d; prio=%d\n",
			name.c_str(), type, priority);

	CCell* cell = NULL;
	m_cellidx.lock();
	cellidx_t::iterator it = m_cellidx.find(name);
	if (it == m_cellidx.end())
	{
		if ( m_rule.enable_free_download || type == e_celltype_cdf )
		{
			// desire a new cell
			cell = new CCell(name, "", type);
			m_cellidx.insert(name, cell);
		}
		else
		{	
			CLogD("post failed: name=%s; name is not in cdf!\n", name);

			// desire a non-exist common cell
			m_cellidx.unlock();
			return NULL;
		}
	}
	else
	{
		cell = it->second;

		// request type mismatch
		if ( cell->m_celltype != type )
		{
			CLogE("post failed: name=%s; o_type=%d; r_type=%d; type mismatch!\n", name, cell->m_celltype, type);

			m_cellidx.unlock();
			return NULL;
		}
	}
	m_cellidx.unlock();

	// create a task
	CCellTask* task = new CCellTask(cell, priority, type, user_context);
	task->cdf_loadtype = cdf_load_type;
	if ( regulation().enable_ghost_mode && priority == e_priority_ghost )
	{
		task->ghost_task = true;
	}

	// auto_dispatch 添加到desire队列
	m_desires.lock();
	m_desires.push(task);
	m_desires.unlock();

	return cell;
}

void CCells::on_task_finish(CCell* cell)
{
	//
	// check cell state
	//

	// success
	if ( cell->m_errorno == e_loaderr_ok )
	{
		CLogD(
			"work down: name=%s; state=%d; errorno=%d; times=%d; \n",
			cell->m_name.c_str(), cell->m_cellstate, cell->m_errorno,
			(int)cell->m_download_times);

		// 处理cdf
		if ( cell->m_celltype == e_celltype_cdf && cell->m_cdf )
		{
			cdf_setupindex(cell);
		}

		// 设置完成校验标记
		cell->m_cellstate = CCell::verified;
	}
	// 下载失败，再做一下尝试
	else if (cell->m_errorno == e_loaderr_download_failed
		&& cell->m_download_times < regulation().remote_urls.size())
	{
		cell->m_cellstate = CCell::unknow;
		cell->m_errorno = e_loaderr_ok;
		m_factory->post_work(cell);
		return;
	}
	// 出错了
	else
	{
		// local only mode hack!
		if ( regulation().only_local_mode && cell->m_errorno == e_loaderr_verify_failed )
		{
			CLogD(
				"local hack done!: name=%s; state=%d; errorno=%d; \n",
				cell->m_name.c_str(), cell->m_cellstate, cell->m_errorno);

			// 处理cdf
			if ( cell->m_celltype == e_celltype_cdf && cell->m_cdf )
			{
				cdf_setupindex(cell);
			}

			cell->m_errorno = e_loaderr_ok;
			cell->m_cellstate = CCell::verified;
		}
		else
		{
			cell->m_cellstate = CCell::error;
		}
	}

	// get tasks
	taskmap_t::_Pairii range = m_taskloading.equal_range(cell);
	for ( taskmap_t::iterator it = range.first; it != range.second; it++ )
	{
		CCellTask* task = it->second;
		CCell* cell = task->cell();

		// 处理是否加载cdf内容
		if ( cell->m_celltype == e_celltype_cdf && cell->m_cdf )
		{
			// check cdf index
			m_cdfidx.lock();
			bool can_post = m_cdfidx.find(cell->m_name) != m_cdfidx.end();
			m_cdfidx.unlock();
			if (can_post)
				cdf_postload(task);
		}

		// ghost 任务不回调
		if ( task->ghost_task )
		{
			continue;
		}

		// notify observers
		m_observers.lock();
		for (observeridx_t::iterator it = m_observers.begin();
			it != m_observers.end(); it++)
		{
			if ( cell->m_celltype == e_celltype_cdf )
			{
				if ( cell->m_cdf )
				{
					props_list_t props_list;

					props_list.insert(std::make_pair(cell->m_name, &(cell->m_props)));

					for ( celllist_t::iterator sub_it = cell->m_cdf->m_subcells.begin(); sub_it != cell->m_cdf->m_subcells.end(); sub_it++ )
					{
						props_list.insert(std::make_pair((*sub_it)->m_name, &((*sub_it)->m_props)));
					}

					(*(it->second))(
						cell->m_name,
						cell->m_celltype,
						cell->m_errorno,
						&(cell->m_cdf->m_props),
						&props_list,
						task->context());
				}
				else
				{
					(*(it->second))(
						cell->m_name,
						cell->m_celltype,
						cell->m_errorno,
						NULL,
						NULL,
						task->context());
				}
			}
			else
			{
				(*(it->second))(
					cell->m_name,
					cell->m_celltype,
					cell->m_errorno,
					&(cell->m_props),
					NULL,
					task->context());
			}
		}
		m_observers.unlock();

		delete task;
	}

	m_taskloading.erase(range.first, range.second);
}


void CCells::cdf_setupindex(CCell* cell)
{
	assert(cell->m_cdf);

	// check if alread setup index
	m_cdfidx.lock();
	if ( m_cdfidx.find(cell->m_name) != m_cdfidx.end() )
	{
		m_cdfidx.unlock();
		return;
	}
	m_cdfidx.unlock();

	for (celllist_t::iterator it =
		cell->m_cdf->m_subcells.begin();
		it != cell->m_cdf->m_subcells.end();)
	{
		CCell* subcell = *it;

		m_cellidx.lock();
		cellidx_t::iterator idxcell_it = m_cellidx.find(
			subcell->m_name);

		if (idxcell_it != m_cellidx.end())
		{
			m_cellidx.unlock();

			// 该名称cell已经存在,以idx中的cell为准
			CCell* idxcell = (*idxcell_it).second;
			assert(idxcell);

			it = cell->m_cdf->m_subcells.erase(it);
			delete subcell;
			subcell = idxcell;
			cell->m_cdf->m_subcells.insert(it, idxcell);
		}
		else
		{
			// 将cdf的cell插入idx
			m_cellidx.insert(subcell->m_name, subcell);
			m_cellidx.unlock();

			// insert to ghost task list
			if ( regulation().enable_ghost_mode )
			{
				m_ghosttasks.push_back(subcell);
			}

			it++;
		}
	}

	m_cdfidx.lock();
	m_cdfidx.insert(cell->m_name, cell);
	m_cdfidx.unlock();
}

void CCells::cdf_postload(CCellTask* task)
{
	// index only
	if ( task->cdf_loadtype == e_cdf_loadtype_index )
	{
		return;
	}

	CCell* cell = task->cell();
	assert(cell && cell->m_cdf);

	bool loadall = false;

	if ( task->cdf_loadtype == e_cdf_loadtype_load || task->cdf_loadtype == e_cdf_loadtype_load_cascade )
	{
		loadall = true;
	}

	// load config 'loadall' mark?
	if ( task->cdf_loadtype == e_cdf_loadtype_config )
	{
		props_t::iterator cdf_prop_it =
			cell->m_cdf->m_props.find(CDF_LOADALL);

		if (cdf_prop_it != cell->m_cdf->m_props.end()
			&& CUtils::atoi((*cdf_prop_it).second.c_str()) == 1)
		{
			loadall = true;
		}
	}

	for (celllist_t::iterator it = cell->m_cdf->m_subcells.begin();
		it != cell->m_cdf->m_subcells.end(); it++)
	{
		CCell* subcell = *it;

		bool postload = loadall;
		// parse config 'load' mark
		if ( !postload && task->cdf_loadtype == e_cdf_loadtype_config )
		{
			props_t::iterator prop_it = subcell->m_props.find(CDF_CELL_LOAD);
			if (prop_it != subcell->m_props.end()
				&& CUtils::atoi((*prop_it).second.c_str()) == 1)
			{
				postload = true;
			}
		}

		if ( subcell->m_celltype == e_celltype_cdf )
		{
			//
			// cdf file
			//

			if ( task->cdf_loadtype == e_cdf_loadtype_index_cascade || task->cdf_loadtype == e_cdf_loadtype_load_cascade )
			{
				m_cdfidx.lock();
				bool alreadyindexed = m_cdfidx.find(subcell->m_name) != m_cdfidx.end();
				m_cdfidx.unlock();
				
				if ( !alreadyindexed )
				{
					// 未建立索引，级联调用
					post_desire_cdf(subcell->m_name, task->priority(), task->cdf_loadtype, task->context());
				}
				else
				{
					// 已经在索引中，不再级联调用
				}
			}
			else if ( postload )
			{
				post_desire_cdf(subcell->m_name, task->priority(), e_cdf_loadtype_index, task->context());
			}
		}
		else 
		{
			//
			// common file
			//

			if ( postload )
				post_desire_file(subcell->m_name, task->priority(), task->context());
		}
	}

}

void CCells::ghost_working()
{
	for ( size_t i = 0; !m_ghosttasks.empty() && i < 100; i++ )
	{
		CCell* cell = m_ghosttasks.front();
		m_ghosttasks.pop_front();
		if ( cell->m_cellstate == CCell::unknow )
		{
			this->post_desired(cell->m_name, cell->m_celltype, e_priority_ghost, NULL, e_cdf_loadtype_index);
			break;
		}
	}
}

} /* namespace cells */

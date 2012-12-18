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

const int CELLS_WORKING_RATE = 50;

static pthread_t s_thread;
static volatile bool s_running = false;

void* CCells::cells_working(void* context)
{
	CCells* cells = (CCells*)context;
	assert(cells->regulation().auto_dispatch);

	while( s_running )
	{
		CUtils::sleep(CELLS_WORKING_RATE);

		if ( cells->is_suspend() )
		{
			continue;
		}

		cells->tick_dispatch();
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

	// 销毁请求列表
	m_desires.lock();
	while(!m_desires.empty()) m_desires.pop_front();
	m_desires.unlock();

	// 销毁observers
	m_observers.lock();
	for (observeridx_t::iterator it = m_observers.begin();
			it != m_observers.end(); it++)
	{
		delete (*it).second;
	}
	m_observers.clear();
	m_observers.unlock();


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

void CCells::tick_dispatch()
{
	if ( m_suspend )
		return;

	// 分发结果
	m_factory->dispatch_result();

	// 分发请求
	m_desires.lock();
	while( !m_desires.empty() )
	{
		m_factory->post_work(m_desires.pop_front());
	}
	m_desires.unlock();
}

bool CCells::post_desire_cdf(const char* name, int priority /*= priority_exclusive*/)
{
	if ( name == NULL ) return false;

	return post_desired(name, e_celltype_cdf, priority) != NULL;
}

bool CCells::post_desire_file(const char* name, int priority /*= priority_default*/)
{
	if ( name == NULL ) return false;

	return post_desired(name, e_celltype_common, priority) != NULL;
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

CCell* CCells::post_desired(const char* name, ecelltype_t type, int priority)
{
	assert(name);
	if ( priority > priority_exclusive ) priority = priority_exclusive;

	printf(
			"post desired: name=%s; type=%d; prio=%d\n",
			name, type, priority);

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
			printf("post failed: name=%s; name is not in cdf!\n", name);

			// desire a non-exist common cell
			m_cellidx.unlock();
			return NULL;
		}
	}
	else
	{
		cell = it->second;
	}
	m_cellidx.unlock();

	// set priority
	cell->m_priority = priority;

	// 非auto_dispatch, 判断是否可以直接返回结果
	if ( !m_rule.auto_dispatch )
	{
		if ( cell->m_cellstate == CCell::loading )
		{
			// 正在加载，不做处理
		}
		if ( cell->m_cellstate == CCell::unknow  )
		{
			// 未加载，直接投递
			m_desires.lock();
			m_desires.push(cell);
			m_desires.unlock();
			//m_factory->post_work(cell);
		}
		else
		{
			// 已经加载过，直接通知结果
			notify_observers(cell);
		}
		return cell;
	}

	// auto_dispatch 添加到desire队列
	m_desires.lock();
	m_desires.push(cell);
	m_desires.unlock();

	return cell;
}

void CCells::notify_observers(CCell* cell)
{
	m_observers.lock();
	for (observeridx_t::iterator it = m_observers.begin();
			it != m_observers.end(); it++)
	{
		(*(it->second))(
				cell->m_name,
				cell->m_celltype,
				cell->m_errorno,
				cell->m_props);
	}
	m_observers.unlock();
}

} /* namespace cells */

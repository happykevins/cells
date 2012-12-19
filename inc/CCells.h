/*
 * CCells.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CCELLS_H_
#define CCELLS_H_

#include "cells.h"
#include "CContainer.h"

//
// 待实现功能列表
// TODO: *候选url多次尝试,curl是否已经支持是否需要支持？
// TODO: 下载限速及拥塞控制, tick_dispatch分发请求时对factory的负载判断
// TODO: 监测系统接口
// TODO: 对url列表的连接进行测速，优先选用最快的url，重试下载的cell是否要调整优先级，以免阻塞后续请求
// TODO: 对加载线程的负载控制，可以启动多线程，但不一定都使用，根据负载来调节，减少libcurl阻塞带来的性能瓶颈
//
namespace cells
{

class CCell;
class CCreationFactory;

typedef CMap<void*, CFunctorBase*> observeridx_t;
typedef CMap<std::string, class CCell*> cellidx_t;

/*
* CCellTask - desire task
* 	1. 记载一次需求任务的信息
* 	2. 每一次post都会产生该对象,task的生存周期如下：
*	  [dispatch线程]cells::post_desire-->[dispatch线程]factory::post_work-->[worker线程]worker::do_work
*	  -->[worker线程]factory::notify_work_finish-->[dispatch线程]cells::dispatch_result->cells::on_task_finish
*/
class CCellTask
{
private:
	CCell*	m_cell;
	int		m_priority;
	ecelltype_t m_type;
	void*	m_context;

public:
	ecdf_loadtype_t cdf_loadtype;

	inline CCell* cell() { return m_cell; }
	inline int priority() { return m_priority; }
	inline ecelltype_t type() { return m_type; }
	inline void* context() { return m_context;}

	CCellTask(CCell* _cell, int _priority, ecelltype_t _type, void* _user_context = NULL) : 
	m_cell(_cell), m_priority(_priority), m_type(_type), m_context(_user_context), cdf_loadtype(e_cdf_loadtype_config) {}

	// 用于按照priority的排序
	struct less_t : public std::binary_function<CCellTask*, CCellTask*, bool>
	{
		bool operator()(CCellTask*& __x, CCellTask*& __y) const
		{ return __x->priority() < __y->priority(); }
	};
};


/*
 * CCells
 * 	cells系统的接口类
 */
class CCells
{
	typedef std::multimap<CCell*, CCellTask*> taskmap_t;
public:
	CCells();

	/*
	 * 初始化cells系统
	 * @param rule - 描述cells系统规则
	 * @return - 是否成功初始化
	 */
	bool initialize(const CRegulation& rule);

	/*
	 * 销毁cells系统
	 */
	void destroy();

	/*
	 * 返回cells系统配置规则
	 */
	const CRegulation& regulation() const;

	/*
	 * 派发结果通告
	 * 	1.auto_dispatch设置为false，用户通过调用该方法来派发执行结果
	 * 		以确保回调函数在用户线程中执行
	 * 	2.auto_dispatch为true,cells系统线程将派
	 * 		发回调，用户需要对回调函数的线程安全性负责
	 */
	void tick_dispatch();

	/*
	 * 恢复执行
	 * 	1.与suspend对应
	 */
	void resume();

	/*
	 * 挂起cells系统：暂停cells系统的所有活动
	 * 	1.挂起状态可以提交需求，但是不会被执行
	 * 	2.在挂起前已经在执行中的任务会继续执行
	 */
	void suspend();

	/*
	 * 判断cells系统是否处于挂起状态
	 */
	bool is_suspend();

	/*
	 * 需求一个cdf文件
	 * 	1.cdf - cells description file
	 * 	2.包含了对用户文件列表的描述
	 * 	@param name - cdf文件名
	 * 	@param priority - 优先级,此值越高，越会优先处理; priority_exclusive代表抢占模式
	 *	@param cdf_load_type - 加载完cdf文件后，如何处理子文件
	 *	@param user_context - 回调时传递给observer的user_context
	 *	@return - 是否成功：name语法问题会导致失败
	 */
	bool post_desire_cdf(const std::string& name, 
		int priority = e_priority_exclusive, 
		ecdf_loadtype_t cdf_load_type = e_cdf_loadtype_config,
		void* user_context = NULL);

	/*
	 * 需求一个用户文件
	 * 	1.需求的文件如果没有包含在此前加载的cdf之中，会因为无法获得文件hash导致每次都需要下载
	 * 	@param name - 文件名
	 * 	@param priority - 优先级,此值越高，越会优先处理; priority_exclusive代表抢占模式
	 *	@param user_context - 回调时传递给observer的user_context
	 *	@return - 是否成功：如果没有开启free_download，如果需求之前加载的cdf表中没有包含的文件，会导致返回失败
	 */
	bool post_desire_file(const std::string& name, 
		int priority = e_priority_default,
		void* user_context = NULL);

	/*
	 * 注册监听器，事件完成会收到通知
	 * 	1.注意在目标target失效前要移除监听器，否则会出现内存访问失败问题
	 * 	@param target - 目标对象
	 * 	@param func - 目标对象的回调方法 - 用make_functor函数获得
	 * 		回调方法原型 - void (T::*mfunc_t)(const char* name, int type, int error_no);
	 */
	void register_observer(void* target, CFunctorBase* func);

	/*
	 * 移除监听器
	 * 	@param target - 注册的目标对象
	 */
	void remove_observer(void* target);

protected:
	CCell* post_desired(const std::string& name, ecelltype_t type, int priority, void* user_context = NULL, ecdf_loadtype_t cdf_load_type = e_cdf_loadtype_config);
	void on_task_finish(CCell* cell);

	static void* cells_working(void* context);

private:
	void setup_cdf(CCell* cell);
	void cdf_postload(CCellTask* task);

protected:
	CRegulation 		m_rule;
	CCreationFactory* 	m_factory;
	volatile bool 		m_suspend;
	cellidx_t 			m_cellidx;
	observeridx_t		m_observers;

	CPriorityQueue<CCellTask*, CCellTask::less_t> m_desires;
	// 正在loading的task表
	taskmap_t m_taskloading;

	friend class CCreationFactory;
};

} /* namespace cells */
#endif /* CCELLS_H_ */

/*
 * CCells.h
 *
 *  Created on: 2012-12-14
 *      Author: happykevins@gmail.com
 */

#ifndef CCELLS_H_
#define CCELLS_H_

#include <string>
#include "CContainer.h"

//
// 待实现功能列表
// TODO: *候选url多次尝试,curl是否已经支持是否需要支持？
// TODO: 压缩，解压部分的实现zlib/7z
// TODO: *实现函数闭包，实现每次投递的相关性
// TODO: 下载限速及拥塞控制
// TODO: tick_dispatch分发请求时对factory的负载判断
// TODO: 监测系统接口
// TODO: 生成cdf文件及相关工具接口
// TODO: 对创建文件夹，文件名其那后"/"等细节处理,更健壮
// TODO: !!curl目前的用法如果远程主机连接不成功，会阻塞住好长时间，如何设置超时时间
// TODO: 对url列表的连接进行测速，优先选用最快的url，重试下载的cell是否要调整优先级，以免阻塞后续请求
// TODO: *退化成可完全使用单线程模型
// TODO: Rule的默认值,libcurl用户可控制是否内部初始化
// TODO: 提供一个批量任务all-done的回调方式
// TODO: 对加载线程的负载控制，可以启动多线程，但不一定都使用，根据负载来调节，减少libcurl阻塞带来的性能瓶颈
// TODO: 对于一些http服务器，如果请求的url无效，则会返回错误html文件，需要通过hash verfiy来辨认文件正确性
// TODO: 支持仅本地验证模式，不会进行下载
//
namespace cells
{

class CCell;
class CCreationFactory;

typedef CMap<std::string, class CCell*> cellidx_t;

/*
 * 定制cells系统规则
 */
struct CRegulation
{
	std::vector<std::string> remote_urls;
	std::string local_url;
	unsigned int worker_thread_num;
	bool auto_dispatch;
	bool only_local_mode;
	bool enable_ghost_mode;
};

/*
 * CFunctorBase-封装函数闭包
 */
class CFunctorBase
{
public:
	virtual ~CFunctorBase(){}
	virtual void operator() (const char* name, int type, int error_no) = 0;
};
typedef CMap<void*, CFunctorBase*> observeridx_t;

class CFunctorG3 : public CFunctorBase
{
public:
	typedef void (*cb_func_g3_t)(const char* name, int type, int error_no);
	CFunctorG3(cb_func_g3_t cb_func) : m_cb_func(cb_func) {}
	CFunctorG3(const CFunctorG3& other) : m_cb_func(other.m_cb_func) {}
	CFunctorG3() : m_cb_func(NULL) {}
	virtual ~CFunctorG3(){ m_cb_func = NULL; }
	virtual void operator() (const char* name, int type, int error_no)
	{
		m_cb_func(name, type, error_no);
	}

protected:
	cb_func_g3_t m_cb_func;
};

template<typename T>
class CFunctorM3 : public CFunctorBase
{
public:
	typedef void (T::*mfunc_t)(const char* name, int type, int error_no);
	CFunctorM3(T* _t, mfunc_t _f) : m_target(_t), m_func(_f) {}
	CFunctorM3(const CFunctorM3<T>& other) : m_target(other.m_target), m_func(other.m_func) {}
	void operator=(const CFunctorM3<T>& other)
	{
		m_target = other.m_target;
		m_func = other.m_func;
	}

	void operator() (const char* name, int type, int error_no)
	{
		(m_target->*m_func)(name, type, error_no);
	}

protected:
	T* m_target;
	mfunc_t m_func;
};

template<typename F>
CFunctorG3* make_functor_g3(F& _f)
{
	return new CFunctorG3(_f);
}
template<typename T>
CFunctorM3<T>* make_functor_m3(T* _t, typename CFunctorM3<T>::mfunc_t _f)
{
	return new CFunctorM3<T>(_t, _f);
}

/*
 * CCells
 * 	cells系统的接口类
 */
class CCells
{
public:
	enum {
		priority_default 		= 0,
		priority_exclusive 	= (unsigned short)-1, // 最大65535
	};

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
	 */
	void post_desire_cdf(const char* name, int priority = priority_exclusive);

	/*
	 * 需求一个用户文件
	 * 	1.需求的文件如果没有包含在此前加载的cdf之中，会因为无法获得文件hash导致每次都需要下载
	 * 	@param name - 文件名
	 * 	@param priority - 优先级,此值越高，越会优先处理; priority_exclusive代表抢占模式
	 */
	void post_desire_file(const char* name, int priority = priority_default);

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
	CCell* post_desired(const char* name, int type, int priority);
	void notify_observers(CCell* cell);
	static void* cells_working(void* context);

protected:
	CRegulation 			m_rule;
	CCreationFactory* 	m_factory;
	volatile bool 		m_suspend;
	cellidx_t 				m_cellidx;
	observeridx_t			m_observers;
	CQueue<class CCell*>	m_desires;

	friend class CCreationFactory;
};

} /* namespace cells */
#endif /* CCELLS_H_ */

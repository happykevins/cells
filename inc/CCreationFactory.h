/*
 * CCreationFactory.h
 *
 *  Created on: 2012-12-13
 *      Author: happykevins@gmail.com
 */

#ifndef CCREATIONFACTORY_H_
#define CCREATIONFACTORY_H_

#include <cstddef>
#include <vector>

#include "CContainer.h"

namespace cells
{

class CCells;
class CCell;
class CCreationWorker;

/*
 * 创建cell的工厂类
 * 	1.多工作线程
 * 	2.任务分派，负载平衡
 * 	3.下载速度控制，拥塞控制
 * 	4.回调及事件通知
 * 	5.*非线程安全
 */
class CCreationFactory
{
public:
	CCreationFactory(CCells* host, size_t worker_num);
	virtual ~CCreationFactory();

	// 投递一个任务
	void post_work(CCell* cell);

	// 分发结果
	size_t dispatch_result();

private:
	// worker thread callback
	void on_work_finished(CCell* cell);

private:
	CCells* m_host;
	std::vector<CCreationWorker*> m_workers;
	size_t m_task_counter;
	// TODO: 是否考虑把这个列表放到CCells类中，这里只管分发任务和返回结果，不应该处理逻辑
	CQueue<class CCell*> m_finished;

	friend class CCreationWorker;
};

} /* namespace cells */
#endif /* CCREATIONFACTORY_H_ */

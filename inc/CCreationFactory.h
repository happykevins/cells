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
class CCellTask;
class CCreationWorker;

/*
 * 创建cell的工厂类
 * 	1.多工作线程
 * 	2.任务分派，负载平衡
 * 	3.下载速度控制，拥塞控制
 * 	4.回调及事件通知
 * 	5.*非线程安全
 *	6.*确保对于每一个cell，从post_work到dispatch_result结束过程中cellstate都处于loading状态,只有loading状态，才允许修改cell内容
 */
class CCreationFactory
{
public:
	CCreationFactory(CCells* host, size_t worker_num);
	virtual ~CCreationFactory();

	// TODO: 根据负载情况，返回投递建议
	bool should_post() { return true; }

	// 投递一个任务
	void post_work(CCell* cell);

	// 获得一个任务结果，如果没有返回NULL
	CCell* pop_result();

private:
	// worker thread callback
	void notify_work_finished(CCell* cell);

private:
	CCells* m_host;
	std::vector<CCreationWorker*> m_workers;
	size_t m_task_counter;

	CQueue<class CCell*> m_finished;
	friend class CCreationWorker;
};

} /* namespace cells */
#endif /* CCREATIONFACTORY_H_ */

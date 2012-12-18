/*
 * CCell.h
 *
 *  Created on: 2012-12-13
 *      Author: happykevins@gmail.com
 */

#ifndef CCELL_H_
#define CCELL_H_

#include <cstddef>
#include <string>
#include <map>
#include <list>
#include "cells.h"

namespace cells
{

class CCDF;

typedef std::list<class CCell*> celllist_t;


/*
 * CCell
 *	cells的最小管理单元
 *	1.单元名字
 *	2.单元hash
 *	3.单元类型
 *	4.单元状态
 */
class CCell
{
public:
	enum cellstate_t
	{
		unknow, loading, verified, error
	};

	// 用于按照priority的排序
	struct less_t : public std::binary_function<CCell*, CCell*, bool>
	{
	  bool operator()(CCell*& __x, CCell*& __y) const
	  { return __x->m_priority < __y->m_priority; }
	};

	CCell(const std::string& _name, const std::string& _hash, ecelltype_t _celltype = e_celltype_common);
	~CCell();

	CCDF* get_cdf() { return m_cdf; }
	void set_cdf(CCDF* cdf) { m_cdf = cdf;}

	const std::string m_name;
	std::string m_hash;
	properties_t m_props;
	volatile int m_cellstate;
	ecelltype_t m_celltype;
	int m_priority;

	void* m_stream;
	size_t m_download_times;
	eloaderror_t m_errorno;

private:
	CCDF* m_cdf;
};

/*
 * CCDF - Cell Description File
 * 	1. 记载描述表内部元素
 * 	2. 可以嵌套CDF类型的cell
 */
class CCDF
{
public:
	CCDF(const CCell* _cell);
	~CCDF();

	bool serialize();
	bool deserialize();

	const CCell* m_hostcell;
	celllist_t   m_subcells;
};

} /* namespace cells */
#endif /* CCELL_H_ */

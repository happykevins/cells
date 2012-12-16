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

namespace cells
{

class CCDF;

typedef std::map<std::string, std::string> properties_t; // 记录属性列表
typedef std::list<class CCell*> celllist_t;

extern const char* CDF_VERSION;			//= "version"	string
extern const char* CDF_LOAD;			//= "load"		boolean:		0 | 1
extern const char* CDF_CELL_TYPE;		//= "type"		celltype_t: 	0 | 1
extern const char* CDF_CELL_NAME;		//= "name"		string
extern const char* CDF_CELL_HASH;		//= "hash"		string
extern const char* CDF_CELL_LOAD;		//= "load"		boolean:		0 | 1

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
	enum celltype_t
	{
		common = 0, cdf = 1
	};

	enum cellstate_t
	{
		unknow, loading, verified, error
	};

	enum cellerror_t
	{
		no_error, openfile_failed, download_failed, bad_cdf
	};

	// 用于按照priority的排序
	struct less_t : public std::binary_function<CCell*, CCell*, bool>
	{
	  bool operator()(CCell*& __x, CCell*& __y) const
	  { return __x->m_priority < __y->m_priority; }
	};

	CCell(const std::string& _name, const std::string& _hash, int _celltype = common);
	~CCell();

	CCDF* get_cdf() { return m_cdf; }
	void set_cdf(CCDF* cdf) { m_cdf = cdf;}

	const std::string m_name;
	std::string m_hash;
	properties_t m_props;
	int m_cellstate;
	int m_celltype;
	int m_priority;

	void* m_stream;
	size_t m_download_times;
	int m_errorno;

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
	properties_t m_props;
	celllist_t   m_subcells;
};

} /* namespace cells */
#endif /* CCELL_H_ */

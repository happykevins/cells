/*
* cells.h
*
*  Created on: 2012-12-18
*      Author: happykevins@gmail.com
*/

#ifndef CELLS_H_
#define CELLS_H_

#include <vector>
#include <list>
#include <map>
#include <string>

#define CELLS_DEFAULT_WORKERNUM			4
#define CELLS_WORKER_MAXWORKLOAD		2
#define CELLS_DOWNLAOD_SPEED_NOLIMIT	(1024 * 1024 * 100) // 100MB
#define CELLS_GHOST_DOWNLAOD_SPEED		(1024 * 32)			// 32KB
#define CELLS_DEFAULT_ZIPTMP_SUFFIX		".tmp"

namespace cells
{

//
// 内建属性名
//
extern const char* CDF_VERSION;			//= "version"	string
extern const char* CDF_LOADALL;			//= "loadall"	boolean:		0 | 1
extern const char* CDF_CELL_CDF;		//= "cdf"		boolean: 		0 | 1 : is 'e_celltype_cdf' type
extern const char* CDF_CELL_NAME;		//= "name"		string
extern const char* CDF_CELL_HASH;		//= "hash"		string
extern const char* CDF_CELL_LOAD;		//= "load"		boolean:		0 | 1

// 属性表
typedef std::map<std::string, std::string> props_t; 
// 属性表列表
typedef std::map<std::string, props_t*> props_list_t;

// 压缩类型
enum eziptype_t
{
	e_nozip = 0,
	e_zlib,
};

// 文件类型
enum ecelltype_t
{
	e_celltype_common = 0, 
	e_celltype_cdf = 1
};

// 优先级
enum epriority_t {
	e_priority_ghost		= -1,
	e_priority_default 		= 0,
	e_priority_exclusive 	= (unsigned short)-1, // 最大65535
};

// CDF文件加载方式
//  *注意：目前对cascade加载方式，由于内部实现用一张cdf索引状态表来终止潜在的无限递归，
//		所以如果cascade路径上遇到已经建立过索引的cdf，会终止递归，其子路径也不会被加载。
enum ecdf_loadtype_t
{
	e_cdf_loadtype_config = 0,		// 建立此cdf索引，并按配置来进行加载操作
	e_cdf_loadtype_index,			// 只建立此cdf的索引，不加载文件
	e_cdf_loadtype_index_cascade,	// 级联建立子cdf索引，不加载common文件
	e_cdf_loadtype_load,			// 建立索引并加载此cdf描述的文件
	e_cdf_loadtype_load_cascade,	// 建立索引并级联加载所有子cdf描述的文件
};

// 加载错误类型
enum eloaderror_t
{
	e_loaderr_ok = 0, 
	e_loaderr_openfile_failed, 
	e_loaderr_download_failed, 
	e_loaderr_decompress_failed, 
	e_loaderr_verify_failed, 
	e_loaderr_patchup_failed
};

//
// cells系统规则定制
//
struct CRegulation
{
	std::vector<std::string> remote_urls;
	std::string local_url;
	size_t worker_thread_num;
	size_t max_download_speed;
	bool auto_dispatch;
	bool only_local_mode;				// 是否开启本地模式：本地文件不匹配也不进行download操作
	bool enable_ghost_mode;				// 是否开启ghost模式
	size_t max_ghost_download_speed;	// ghost的下载速度
	bool enable_free_download;			// 是否开启自由下载模式：(默认关闭)，开启此模式可以自由需求cdf没有描述过的文件
	eziptype_t zip_type;				// 压缩类型：0-未压缩；1-zlib
	bool zip_cdf;						// cdf文件是否为压缩格式
	std::string tempfile_suffix;		// 解压缩临时文件后缀

	// default value
	CRegulation() : worker_thread_num(CELLS_DEFAULT_WORKERNUM), max_download_speed(CELLS_DOWNLAOD_SPEED_NOLIMIT),
		auto_dispatch(false), only_local_mode(false), 
		enable_ghost_mode(false), max_ghost_download_speed(CELLS_GHOST_DOWNLAOD_SPEED),
		enable_free_download(false), zip_type(e_nozip), zip_cdf(false),
		tempfile_suffix(CELLS_DEFAULT_ZIPTMP_SUFFIX)
	{}
};

//
// CFunctorBase-封装函数闭包
//
class CFunctorBase
{
public:
	virtual ~CFunctorBase(){}
	virtual void operator() (const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context) = 0;
};

class CFunctorG : public CFunctorBase
{
public:
	typedef void (*cb_func_g_t)(const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context);
	CFunctorG(cb_func_g_t cb_func) : m_cb_func(cb_func) {}
	CFunctorG(const CFunctorG& other) : m_cb_func(other.m_cb_func) {}
	CFunctorG() : m_cb_func(NULL) {}
	virtual ~CFunctorG(){ m_cb_func = NULL; }
	virtual void operator() (const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
	{
		m_cb_func(name, type, error_no, props, sub_props, context);
	}

protected:
	cb_func_g_t m_cb_func;
};

template<typename T>
class CFunctorM : public CFunctorBase
{
public:
	typedef void (T::*mfunc_t)(const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context);
	CFunctorM(T* _t, mfunc_t _f) : m_target(_t), m_func(_f) {}
	CFunctorM(const CFunctorM<T>& other) : m_target(other.m_target), m_func(other.m_func) {}
	void operator=(const CFunctorM<T>& other)
	{
		m_target = other.m_target;
		m_func = other.m_func;
	}

	void operator() (const std::string& name, ecelltype_t type, eloaderror_t error_no, const props_t* props, const props_list_t* sub_props, void* context)
	{
		(m_target->*m_func)(name, type, error_no, props, sub_props, context);
	}

protected:
	T* m_target;
	mfunc_t m_func;
};

template<typename F>
CFunctorG* make_functor_g(F& _f)
{
	return new CFunctorG(_f);
}
template<typename T>
CFunctorM<T>* make_functor_m(T* _t, typename CFunctorM<T>::mfunc_t _f)
{
	return new CFunctorM<T>(_t, _f);
}

}/* namespace cells */
#endif /* CELLS_H_ */
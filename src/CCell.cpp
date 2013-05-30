/*
 * CCell.cpp
 *
 *  Created on: 2012-12-13
 *      Author: happykevins@gmail.com
 */

#include "CCell.h"

#include <sstream>

namespace cells
{

CCell::CCell(const std::string& _name, const std::string& _hash /*= NULL*/,
		estatetype_t _celltype /*= common*/) :
		m_name(_name), m_hash(_hash), m_cellstate(unknow), m_celltype(_celltype), 
		m_download_times(0), m_errorno(e_loaderr_ok), m_ziptype(e_zip_none), m_cdf(NULL), 
		m_watcher(NULL)
{
}

CCell::~CCell()
{
	if (m_cdf)
	{
		delete m_cdf;
		m_cdf = NULL;
	}
}

CCDF::CCDF(const CCell* _cell) :
		m_hostcell(_cell)
{
}
CCDF::~CCDF()
{
}

} /* namespace cells */

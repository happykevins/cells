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

const char* CDF_VERSION = "version";
const char* CDF_LOAD	= "load";
const char* CDF_CELL_TYPE = "type";
const char* CDF_CELL_NAME = "name";
const char* CDF_CELL_HASH = "hash";
const char* CDF_CELL_LOAD = "load";

CCell::CCell(const std::string& _name, const std::string& _hash /*= NULL*/,
		int _celltype /*= common*/) :
		m_name(_name), m_hash(_hash), m_cellstate(unknow), m_celltype(_celltype), m_priority(
				0), m_stream(NULL), m_download_times(0), m_errorno(no_error), 
				m_cdf(NULL)
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

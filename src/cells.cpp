/*
* cells.cpp
*
*  Created on: 2012-12-10
*      Author: happykevins@gmail.com
*/

#include "cells.h"
#include "CCells.h"


namespace cells
{

const char* CDF_VERSION = "version";
const char* CDF_LOADALL	= "loadall";
const char* CDF_CELL_CDF = "cdf";
const char* CDF_CELL_NAME = "name";
const char* CDF_CELL_LOAD = "load";
const char* CDF_CELL_HASH = "hash";
const char* CDF_CELL_SIZE = "size";
const char* CDF_CELL_ZHASH = "zhash";
const char* CDF_CELL_ZSIZE = "zsize";
const char* CDF_CELL_ZIP = "zip";


// default value
CRegulation::CRegulation() : 
	worker_thread_num(CELLS_DEFAULT_WORKERNUM), max_download_speed(CELLS_DOWNLAOD_SPEED_NOLIMIT),
	auto_dispatch(true), only_local_mode(false), 
	enable_ghost_mode(false), max_ghost_download_speed(CELLS_GHOST_DOWNLAOD_SPEED),
	enable_free_download(false), 
	//zip_type(e_zip_none), zip_cdf(false),
	remote_zipfile_suffix(CELLS_REMOTE_ZIPFILE_SUFFIX),
	tempfile_suffix(CELLS_DEFAULT_TEMP_SUFFIX), temphash_suffix(CELLS_DEFAULT_HASH_SUFFIX)
{
}

CellsHandler* cells_create(const CRegulation& rule)
{
	CCells* handler = new CCells;
	if ( handler->initialize(rule) )
	{
		return handler;
	}
	delete handler;
	return NULL;
}

void cells_destroy(CellsHandler* handler)
{
	if ( !handler )
		return;

	CCells* impl = dynamic_cast<CCells*>(handler);
	if ( impl )
	{
		impl->destroy();
	}

	delete handler;
}

}//namespace cells
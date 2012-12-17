/*
 * CCreationWorker.cpp
 *
 *  Created on: 2012-12-12
 *      Author: happykevins@gmail.com
 */

#include "CCreationWorker.h"

#include <stdio.h>
#include <sstream>
#include <set>
#include <assert.h>
#include <tinyxml.h>

#include "md5.h"
#include "CUtils.h"
#include "CCells.h"
#include "CCreationFactory.h"

namespace cells
{

void* CCreationWorker::working(void* context)
{
	CCreationWorker* worker = (CCreationWorker*) context;

	while (worker->m_working)
	{
		int sem_retv = sem_wait(&(worker->m_sem));
		assert(sem_retv >= 0);

		worker->do_work();
	}

	sem_destroy(&(worker->m_sem));

	return 0;
}

CCreationWorker::CCreationWorker(CCreationFactory* host) :
		m_host(host), m_working(true), m_downloadhandle(this)
{
	assert(host);
	int ret = sem_init(&m_sem, 0, 0);
	assert(ret == 0);
	pthread_create(&m_thread, NULL, CCreationWorker::working, this);
}

CCreationWorker::~CCreationWorker()
{
	m_working = false;
	sem_post(&m_sem);
	pthread_join(m_thread, NULL);
}

void CCreationWorker::post_work(CCell* cell)
{
	assert(cell);
	m_queue.lock();
	m_queue.push(cell);
	m_queue.unlock();
	sem_post(&m_sem);
}

void CCreationWorker::do_work()
{
	// get a task
	m_queue.lock();
	if (m_queue.empty())
	{
		m_queue.unlock();
		return;
	}

	CCell* cell = m_queue.front();
	m_queue.pop();
	m_queue.unlock();

	// make local url
	std::stringstream ss;
	ss << m_host->m_host->regulation().local_url << cell->m_name;
	printf("local path=%s\n", ss.str().c_str());

	// verify & patchup local file
	if ( work_verify_local(cell, ss.str().c_str()) && work_patchup_cell(cell, ss.str().c_str()) )
	{
		work_finished(cell, true);
		return;
	}

	congestion_control();

	// download & patchup cell
	if ( work_download_remote(cell, ss.str().c_str()) && work_patchup_cell(cell, ss.str().c_str()) )
	{
		// download success
		work_finished(cell, true);
	}
	else
	{
		// download or patchup false
		work_finished(cell, false);
	}	
}

bool CCreationWorker::is_free()
{
	CMutexScopeLock(m_queue.mutex());
	return m_queue.empty();
}

size_t CCreationWorker::workload()
{
	CMutexScopeLock(m_queue.mutex());
	return m_queue.size();
}

void CCreationWorker::work_finished(CCell* cell, bool result)
{
	// increase the download times counter
	cell->m_download_times++;

	// notify factory work done!
	m_host->on_work_finished(cell);
}

bool CCreationWorker::congestion_control()
{
	// TODO: 实现拥塞控制算法
	CUtils::sleep(10);

	return true;
}

bool CCreationWorker::work_verify_local(CCell* cell, const char* localurl)
{
	if (cell->m_hash.empty())
	{
		printf("hash code not specified: name=%s;\n", cell->m_name.c_str());
		return false;
	}

	// 验证本地文件hash
	FILE* fp = fopen(localurl, "rb");
	if (!fp)
	{
		return false;
	}

	md5_state_t state;
	md5_byte_t digest[16];
	char hex_output[16*2 + 1];
	size_t file_size = 0;
	md5_init(&state);
	do
	{
		size_t readsize = fread(m_databuf, 1, sizeof(m_databuf), fp);
		file_size += readsize;
		md5_append(&state, (const md5_byte_t *)m_databuf, readsize);
	} while( !feof(fp) && !ferror(fp) );
	md5_finish(&state, digest);
	
	fclose(fp);
	
	for (int di = 0; di < 16; ++di)
		sprintf(hex_output + di * 2, "%02x", digest[di]);

	if ( strcmp(hex_output, cell->m_hash.c_str()) )
	{
		printf("hash verify failed: name=%s; size=%d; cdf_hash=%s, file_hash=%s\n", cell->m_name.c_str(), file_size, cell->m_hash.c_str(), hex_output);
		return false;
	}
	else
	{
		printf("hash verify success: name=%s; size=%d; hash=%s\n", cell->m_name.c_str(), file_size, cell->m_hash.c_str());
	}

	return true;
}

bool CCreationWorker::work_download_remote(CCell* cell, const char* localurl)
{
	// create file
	FILE* fp = fopen(localurl, "wb+");
	if (!fp)
	{
		// create file failed
		cell->m_errorno = CCell::openfile_failed;
		return false;
	}
	cell->m_stream = fp;

	// make remote url
	const std::vector<std::string>& urls = m_host->m_host->regulation().remote_urls;
	int urlidx = cell->m_download_times % urls.size();

	std::stringstream ss;
	ss << urls[urlidx] << cell->m_name;

	bool result = m_downloadhandle.download(cell, ss.str().c_str());

	if ( result )
		printf("download cell success: name=%s\n", cell->m_name.c_str());
	else
		printf("download cell failed: name=%s\n", cell->m_name.c_str());

	// close file
	fflush(fp);
	fclose(fp);
	cell->m_stream = NULL;

	if ( !result )
	{
		// download failed
		cell->m_errorno = CCell::download_failed;
	}

	return result;
}

bool CCreationWorker::work_patchup_cell(CCell* cell, const char* localurl)
{
	bool cdf_result = false;

	// setup cdf
	TiXmlDocument doc;
	if ( cell->m_celltype == CCell::cdf && doc.LoadFile(localurl) )
	{
		std::set<const char*> name_set;
		CCDF* ret_cdf = new CCDF(cell);
		for (TiXmlElement* cells_section = doc.FirstChildElement();
			cells_section; cells_section =
			cells_section->NextSiblingElement())
		{
			for (const TiXmlAttribute* cells_attr =
				cells_section->FirstAttribute(); cells_attr; cells_attr =
				cells_attr->Next())
			{
				//printf("%s=%s\n", cells_attr->Name(), cells_attr->Value());
				ret_cdf->m_props.insert(
					std::make_pair(cells_attr->Name(),
					cells_attr->Value()));
			}

			for (const TiXmlElement* cell_section =
				cells_section->FirstChildElement(); cell_section;
				cell_section = cell_section->NextSiblingElement())
			{
				const char* cell_name = cell_section->Attribute(CDF_CELL_NAME);
				// 健壮性检测：忽略没有名字或重名
				if (!cell_name || name_set.find(cell_name) != name_set.end())
				{
					continue;
				}

				const char* cell_hash = cell_section->Attribute(CDF_CELL_HASH);

				int cell_type = CUtils::atoi(cell_section->Attribute(CDF_CELL_TYPE));
				// 健壮性检测：异常值修正
				if (cell_type != CCell::cdf)
					cell_type = CCell::common;

				CCell* cell = new CCell(cell_name, cell_hash, cell_type);

				for (const TiXmlAttribute* cell_attr =
					cell_section->FirstAttribute(); cell_attr; cell_attr =
					cell_attr->Next())
				{
					//printf("%s=%s\n", cell_attr->Name(), cell_attr->Value());
					cell->m_props.insert(
						std::make_pair(cell_attr->Name(),
						cell_attr->Value()));
				}

				name_set.insert(cell_name);
				ret_cdf->m_subcells.push_back(cell);
			}
		}
		cdf_result = true;
		cell->set_cdf(ret_cdf);
	}
	doc.Clear();

	// cdf file
	if ( !cdf_result && cell->m_celltype == CCell::cdf )
	{
		cell->m_errorno = CCell::verify_failed;
		printf("cdf setup failed!: name=%s\n", cell->m_name.c_str() );
		return false;
	}
	else if ( cell->m_celltype == CCell::cdf )
	{
		printf("cdf setup success: name=%s, child=%d\n", cell->m_name.c_str(), (int)cell->get_cdf()->m_subcells.size());
		return true;
	}

	// common file

	return true;
}

} /* namespace cells */

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
	std::string localurl = ss.str();
	ss << m_host->m_host->regulation().zip_tmp_suffix;
	std::string localtmpurl = ss.str();
	printf("local path=%s\n", localurl.c_str());

	// open local file
	FILE* fp = fopen(localurl.c_str(), "rb");
	if (!fp)
	{
		//
		// can not open local file
		//

		if ( m_host->m_host->regulation().only_local_mode )
		{
			cell->m_errorno = loaderr_openfile_failed;
			work_finished(cell);
			return;
		}
	}
	else
	{
		//
		// check local file
		//

		// only local mode hack, do not verify file!
		if ( m_host->m_host->regulation().only_local_mode )
		{
			fclose(fp);

			if ( !work_patchup_cell(cell, localurl.c_str()) )
			{
				cell->m_errorno = loaderr_patchup_failed;
			}

			work_finished(cell);
			return;
		}

		cell->m_stream = fp;

		// verify local file
		if ( work_verify_local(cell) )
		{
			cell->m_stream = NULL;
			fclose(fp);

			// patchup local file
			if ( !work_patchup_cell(cell, localurl.c_str()) )
			{
				cell->m_errorno = loaderr_patchup_failed;
			}

			work_finished(cell);
			return;
		}
		else
		{
			// 虽然本地验证失败，但这里不用设置错误码，后面还要下载再验证
			//cell->m_errorno == loaderr_verify_failed;
		}

		// close file
		cell->m_stream = NULL;
		fclose(fp);
	}
	
	// before download do congestion control
	congestion_control();

	// check local file
	fp = fopen(localurl.c_str(), "wb+");
	if (!fp)
	{
		cell->m_errorno = loaderr_openfile_failed;
		work_finished(cell);
		return;
	}
	cell->m_stream = fp;

	// download & patchup cell
	if ( work_download_remote(cell) )
	{
		//
		// download success!
		//
		cell->m_stream = NULL;
		fclose(fp);
		
		// decompress
		if ( m_host->m_host->regulation().zip_type != e_nozip )
		{
			if ( cell->m_celltype == e_celltype_common || 
				(cell->m_celltype == e_celltype_cdf && m_host->m_host->regulation().zip_cdf) )
			{
				if ( !work_decompress(localurl.c_str(), localtmpurl.c_str()) )
				{
					printf("file decompress failed: name=%s;\n", cell->m_name.c_str());
					cell->m_errorno = loaderr_decompress_failed;
				}
			}
		}

		// verify downloaded file
		if ( cell->m_errorno == loaderr_ok && !cell->m_hash.empty() )
		{
			fp = fopen(localurl.c_str(), "rb");
			if (fp)
			{
				cell->m_stream = fp;
				if ( !work_verify_local(cell) )
				{
					cell->m_errorno = loaderr_verify_failed;
				}
				cell->m_stream = NULL;
				fclose(fp);
			}
			else
			{
				cell->m_errorno = loaderr_openfile_failed;
			}
		}

		// patchup
		if ( cell->m_errorno == loaderr_ok )
		{
			if ( !work_patchup_cell(cell, localurl.c_str()) )
			{
				cell->m_errorno = loaderr_patchup_failed;
			}
		}
	}
	else
	{
		//
		// download failed
		//
		cell->m_stream = NULL;
		fclose(fp);
		cell->m_errorno = loaderr_download_failed;
	}

	work_finished(cell);	
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

void CCreationWorker::work_finished(CCell* cell)
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

bool CCreationWorker::work_verify_local(CCell* cell)
{
	if (cell->m_hash.empty())
	{
		printf("hash code not specified: name=%s;\n", cell->m_name.c_str());
		return false;
	}

	// 验证本地文件hash
	assert(cell->m_stream);
	FILE* fp = (FILE*)cell->m_stream;

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

bool CCreationWorker::work_download_remote(CCell* cell)
{
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

	return result;
}

bool CCreationWorker::work_decompress(const char* localurl, const char* tmpurl)
{
	if ( CUtils::decompress(localurl, tmpurl) == 0 )
	{
		// success!
		int ret = remove(localurl);
		if ( ret == 0 )
			ret = rename(tmpurl, localurl);
		assert(ret == 0);
		return ret == 0;
	}
	else
	{
		// failed
		remove(tmpurl);
	}

	return false;
}

bool CCreationWorker::work_patchup_cell(CCell* cell, const char* localurl)
{
	bool cdf_result = false;

	// setup cdf
	TiXmlDocument doc;
	if ( cell->m_celltype == e_celltype_cdf && doc.LoadFile(localurl) )
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
				cell->m_props.insert(
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

				ecelltype_t cell_type = (ecelltype_t)CUtils::atoi(cell_section->Attribute(CDF_CELL_TYPE));
				// 健壮性检测：异常值修正
				if (cell_type != e_celltype_cdf)
					cell_type = e_celltype_common;

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
	if ( !cdf_result && cell->m_celltype == e_celltype_cdf )
	{
		printf("cdf setup failed!: name=%s\n", cell->m_name.c_str() );
		return false;
	}
	else if ( cell->m_celltype == e_celltype_cdf )
	{
		printf("cdf setup success: name=%s, child=%d\n", cell->m_name.c_str(), (int)cell->get_cdf()->m_subcells.size());
		return true;
	}

	// common file

	return true;
}

} /* namespace cells */

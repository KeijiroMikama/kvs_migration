#pragma once

#include <string>
#include "rocksdb/db.h"

using namespace rocksdb;

struct WorkloadArg {
	DB* db;
	int src_pmem_rate;
	int dst_pmem_rate;
	int write_rate;
	std::string id;

	WorkloadArg(DB* db_, 
				const int& src_pmem_rate_, 
				const int& dst_pmem_rate_, 
				const int& write_rate_, 
				const std::string& id_)
		: db(db_), 
		  src_pmem_rate(src_pmem_rate_), 
		  dst_pmem_rate(dst_pmem_rate_), 
		  write_rate(write_rate_), 
		  id(id_) {}
};

#ifndef IDLE
void RunWorkload_InMig(WorkloadArg arg, bool& mig_ended);
#endif

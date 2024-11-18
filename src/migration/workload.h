#pragma once

#include "rocksdb/db.h"

using namespace rocksdb;

struct WorkloadArg {
	DB* db;
	int write_rate;
	std::vector<int> wl_fds;

	WorkloadArg(DB* db_, 
				const int& write_rate_, 
				const std::vector<int>& wl_fds_)
		: db(db_), 
		  write_rate(write_rate_), 
		  wl_fds(wl_fds_) {}
};

void RunWorkload_InMig(WorkloadArg arg);

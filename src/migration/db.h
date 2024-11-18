#pragma once

#include "./sst.h"

#define PMEMSIZE 32*1024*1024*1024ULL
#define DISKSIZE 128*1024*1024*1024ULL
// #define KEYMAX   240*1024*1024
#define KEYMAX   16*1024*1024
#define KEYLEN   8
#define VALUELEN 4*1024

using namespace rocksdb;

const std::string pmem_dir = "/mnt/pmem0";
const std::string disk_dir = "/mnt/sdc0";
#ifdef ONLY_PMEM
const std::string pmem_db_dir = pmem_dir + "/only_pmem";
#elif ONLY_DISK
const std::string disk_db_dir = disk_dir + "/only_disk";
#else
const std::string pmem_db_dir = pmem_dir + "/mixing";
const std::string disk_db_dir = disk_dir + "/mixing";
#endif

struct SrcOptions {
	int num_files;
	int num_queues;

	SrcOptions(const int& num_files_, 
			   const int& num_queues_) 
		: num_files(num_files_), 
		  num_queues(num_queues_) {}
};

void AssertStatus(Status status);
void GetSstFileData(DB* db, 
					const Options& options, 
					std::vector<SstFileData>* file_datas, 
					std::vector<std::string>* src_file_paths, 
					int* num_files);


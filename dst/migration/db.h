#pragma once

#include "./sst.h"

// #define PMEMSIZE 32*1024*1024*1024ULL
// #define DISKSIZE 128*1024*1024*1024ULL
// #define KEYMAX 240*1024*1024
#define KEYMAX   16*1024*1024
#define KEYLEN   8
#define VALUELEN 4*1024
#define BUFSIZE  4096

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
};

class MyDB {
private:
	std::vector<std::string> dbname;
	int target_level;
public:
	MyDB(const std::vector<std::string>& dbname_, 
		 const int& target_level_) 
		: dbname(dbname_), 
		  target_level(target_level_) {}
	
	std::string ReturnFilePath(const int& file_level, 
							   bool& on_pmem, 
							   const int& file_num) {
		std::string file_path;
#ifdef ONLY_PMEM
		on_pmem = true;
        file_path = dbname[0]+"/recv"+std::to_string(file_num)+".sst";
#elif ONLY_DISK
		on_pmem = false;
        file_path = dbname[0]+"/recv"+std::to_string(file_num)+".sst";
#else
		if (file_level <= target_level) {
			on_pmem = true;
			file_path = dbname[0]+"/recv"+std::to_string(file_num)+".sst";
		}
		else {
			on_pmem = false;
			file_path = dbname[1]+"/recv"+std::to_string(file_num)+".sst";
		}
#endif
/*
#ifdef RELOCATION
		if (file_level > target_level) {
			on_pmem = false;
			file_path = dbname[1]+"/recv"+std::to_string(file_num)+".sst";
		}
		else {
			on_pmem = true;
			file_path =  dbname[0]+"/recv"+std::to_string(file_num)+".sst";
		}
#elif MIXING
		if (on_pmem == false)
			file_path =  dbname[1]+"/recv"+std::to_string(file_num)+".sst";
		else
			file_path =  dbname[0]+"/recv"+std::to_string(file_num)+".sst";
#elif ONLY_PMEM
		on_pmem = true;
		file_path = dbname[0]+"/recv"+std::to_string(file_num)+".sst";
#elif ONLY_DISK
		on_pmem = false;
		file_path = dbname[0]+"/recv"+std::to_string(file_num)+".sst";
#endif
*/
		return file_path;
	}
};

void AssertStatus(Status status);
void ReturnFilePaths(// const Options& options, 
					 const std::vector<std::string>& dbname, 
					 const int& num_files, 
					 const int& dst_pmem_rate, 
					 std::vector<SstFileData>& file_datas, 
					 std::vector<std::string>* file_paths);


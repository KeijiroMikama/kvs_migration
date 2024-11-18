#pragma once

#include <string>
#include <vector>
#include <utility>
// #include <fstream>
// #include <fcntl.h>
// #include <unistd.h>
// #include <libpmem.h>
#include "./concurrentqueue.h"
#include "rocksdb/db.h"

using namespace rocksdb;

struct SstFileData {
	int level;
	uint64_t num_entries;
	bool on_pmem;
};

struct TransferSstFileArg {
	DB* db;
	Options options;
	std::vector<SstFileData> file_datas;
	std::vector<std::string> file_paths;
	int num_files;
	int num_queues;
	std::vector<int> sst_fds;

	TransferSstFileArg(DB* db_, 
					   const Options& options_, 
					   const std::vector<SstFileData>& file_datas_, 
					   const std::vector<std::string>& file_paths_, 
					   const int& num_files_, 
					   const int& num_queues_, 
					   const std::vector<int>& sst_fds_) 
		: db(db_), 
		  options(options_), 
		  file_datas(file_datas_), 
		  file_paths(file_paths_),
		  num_files(num_files_), 
		  num_queues(num_queues_), 
		  sst_fds(sst_fds_) {}
};

class SstFile {
private:
	std::vector<SstFileData> file_datas;
	std::vector<std::string> file_paths;
	int num_files;
	int num_queues;
	std::vector<std::shared_ptr<moodycamel::ConcurrentQueue<std::string>>> kv_pairs;
	std::vector<bool> write_finished;

public:
	SstFile(const std::vector<SstFileData>& file_datas_, 
			const std::vector<std::string>& file_paths_, 
			const int& num_files_, 
			const int& num_queues_) 
		: file_datas(file_datas_), 
		  file_paths(file_paths_), 
		  num_files(num_files_), 
		  num_queues(num_queues_) {
		kv_pairs.resize(num_queues_);
		write_finished.resize(num_files_);
	}

	void RecvKVPairs(const std::vector<int>& sst_fds);
	void WriteKVPairs(const Options& options);
	void IngestSstFiles(DB* db);
};

void TransferSstFiles(TransferSstFileArg arg, bool& mig_ended);
void RecvSstFileData(const int& sock_fd,
					 const int& num_files,
					 std::vector<SstFileData>* file_datas);


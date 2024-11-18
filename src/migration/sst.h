#pragma once

#include <string>
#include <vector>
#include <utility>
#include "./concurrentqueue.h"
#include "rocksdb/db.h"

using namespace rocksdb;

struct SstFileData {
	int level;
	uint64_t num_entries;
	bool on_pmem;
};

struct TransferSstFileArg {
	Options options;
	std::vector<SstFileData> file_datas;
	std::vector<std::string> src_file_paths;
	int num_files;
	int num_queues;
	std::vector<int> sst_fds;

	TransferSstFileArg(const Options& options_, 
					   std::vector<SstFileData> file_datas_, 
					   std::vector<std::string> src_file_paths_, 
					   const int& num_files_, 
					   const int& num_queues_, 
					   const std::vector<int>& sst_fds_) 
		: options(options_), 
		  file_datas(file_datas_), 
		  src_file_paths(src_file_paths_), 
		  num_files(num_files_), 
		  num_queues(num_queues_), 
		  sst_fds(sst_fds_) {}
};

class SstFile {
private:
	std::vector<SstFileData> file_datas;
	std::vector<std::string> src_file_paths;
	int num_files;
	int num_queues;
	std::vector<std::shared_ptr<moodycamel::ConcurrentQueue<std::string>>> kv_pairs;

public:
	SstFile(std::vector<SstFileData>& file_datas_, 
			const std::vector<std::string>& src_file_paths_, 
			const int& num_files_, 
			const int& num_queues_) 
		: file_datas(file_datas_), 
		  src_file_paths(src_file_paths_), 
		  num_files(num_files_), 
		  num_queues(num_queues_) {
		kv_pairs.resize(num_queues_);
	}

	void ReadKVPairs(const Options& options);
	void SendKVPairs(const std::vector<int>& sst_fds);
};

void TransferSstFiles(TransferSstFileArg arg);
void SendSstFileData(const int& sock_fd,
					 const int& num_files,
					 const std::vector<SstFileData>& file_datas);


#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <sys/socket.h>
#include "./sst.h"
#include "./socket.h"
#include "./debug.h"
#include "rocksdb/sst_file_reader.h"

RunTime read_time, send_time;
std::mutex read_mtx, send_mtx;
static int read_cnt = 0, send_cnt = 0;

void SstFile::ReadKVPairs(const Options& options) {
	while (read_cnt < num_files) {
		int file_num;
		{
			std::lock_guard<std::mutex> lock(read_mtx);
			file_num = read_cnt;
            ++read_cnt;
        }

		// open src sstable
		SstFileReader sst_file_reader(options);
		Status status = sst_file_reader.Open(src_file_paths[file_num]);
		AssertStatus(status);
		Iterator* iter = sst_file_reader.NewIterator(ReadOptions());
		
		// standby when no queue is available
		int queue_num = file_num % num_queues;
		while (kv_pairs[queue_num] != nullptr) {
			std::this_thread::yield();
		}
		
		// allocate kv_pairs queue
		kv_pairs[queue_num] = std::make_shared<moodycamel::ConcurrentQueue<std::string>>();

		auto start = std::chrono::high_resolution_clock::now();

		// enqueue
		for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
			const std::string key   = iter->key().ToString();
			const std::string value = iter->value().ToString();
			kv_pairs[queue_num]->enqueue(key+","+value);
		}
		
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		read_time.elapsed += duration;

		delete iter;
	}
}

void SstFile::SendKVPairs(const std::vector<int>& sst_fds) {
	while (send_cnt < num_files) {
		int file_num;
		{
			std::lock_guard<std::mutex> lock(send_mtx);
			file_num = send_cnt;
			++send_cnt;
		}

		// send file_num
		if (send(sst_fds[0], &file_num, sizeof(int), 0) < 0) {
			std::perror("send(SendKVPairs): ");
			exit(EXIT_FAILURE);
		}
		RecvFlag(sst_fds[0]);

		// wait until queue is allocated
		int queue_num = file_num % num_queues;
		while (kv_pairs[queue_num] == nullptr) {
			std::this_thread::yield();
		}

		for (uint64_t i = 0; i < file_datas[file_num].num_entries; ++i) {
			// retrieve kv_pair from queue
			std::string kv_pair;
			while (!kv_pairs[queue_num]->try_dequeue(kv_pair)) {
				std::this_thread::yield();
			}

			auto start = std::chrono::high_resolution_clock::now();

			const size_t data_size = kv_pair.size();
			if (send(sst_fds[0], &data_size, sizeof(size_t), 0) < 0) {
				std::perror("send(SendKVPairs): ");
				exit(EXIT_FAILURE);
			}

			// send kv-pair
			if (send(sst_fds[1], kv_pair.c_str(), data_size, 0) < 0) {
				std::perror("send(SendKVPairs): ");
				exit(EXIT_FAILURE);
			}

			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			send_time.elapsed += duration;
		}

		// free queues that have been sent
		kv_pairs[queue_num] = nullptr;


		std::cout << "file_number: " << file_num << std::endl;
	}
}

void TransferSstFiles(TransferSstFileArg arg) {
	std::cout << "TransferSstFilesThread started." << std::endl;
	
	
	SstFile sst_file(arg.file_datas, arg.src_file_paths, arg.num_files, arg.num_queues);
	
	std::thread ReadKVPairsThread([&]{ sst_file.ReadKVPairs(arg.options); });
	std::thread SendKVPairsThread([&]{ sst_file.SendKVPairs(arg.sst_fds); });
	ReadKVPairsThread.join();
	SendKVPairsThread.join();
	

	std::cout << "ReadSstFiles : " << read_time.elapsed / 1000000 << " s" << std::endl;
    std::cout << "SendKVPairs  : " << send_time.elapsed / 1000000 << " s" << std::endl;

	std::cout << "TransferSstFilesThread is ended." << std::endl;
}

void SendSstFileData(const int& sock_fd,
					 const int& num_files,
					 const std::vector<SstFileData>& file_datas) {
	for (int file_num = 0; file_num < num_files; ++file_num) {
		SstFileData file_data = file_datas[file_num];

		if (send(sock_fd, &file_data, sizeof(SstFileData), 0) < 0) {
			std::cerr << "send(SendSstFileData)" << std::endl;
			exit(EXIT_FAILURE);
		}

		RecvFlag(sock_fd);
	}
}


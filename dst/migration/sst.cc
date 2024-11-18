#include <iostream>
#include <sstream>
// #include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <sys/socket.h>
#include "./socket.h"
#include "./debug.h"

RunTime recv_time, write_time, ingest_time, mig_time;
std::mutex recv_mtx, write_mtx, ingest_mtx;
static int recv_cnt = 0, write_cnt = 0, ingest_cnt = 0;

void SstFile::RecvKVPairs(const std::vector<int>& sst_fds) {
	// size_t buf_size = KEYLEN + VALUELEN + 1;
	while (recv_cnt < num_files) {
		{
			std::lock_guard<std::mutex> lock(recv_mtx);
			++recv_cnt;
		}

		// recv file_num of sstable
		int file_num;
		if (recv(sst_fds[0], &file_num, sizeof(int), 0) < 0) {
			std::perror("recv(RecvKVPairs): ");
			exit(EXIT_FAILURE);
		}
		SendFlag(sst_fds[0]);

		// standby when no queue is available
		int queue_num = file_num % num_queues;
		while (kv_pairs[queue_num] != nullptr) {
			std::this_thread::yield();
		}

		// allocate kv_pairs queue
		kv_pairs[queue_num] = std::make_shared<moodycamel::ConcurrentQueue<std::string>>();

		for (uint64_t e = 0; e < file_datas[file_num].num_entries; ++e) {
			auto start = std::chrono::high_resolution_clock::now();

			size_t data_size;
			if (recv(sst_fds[0], &data_size, sizeof(size_t), 0) < 0) {
				std::perror("recv(RecvKVPairs): ");
				exit(EXIT_FAILURE);
			}

			size_t recv_size = 0;
			std::string kv_pair;
			// while (recv_size < buf_size) {
			while (recv_size < data_size) {
				// char buf[buf_size-recv_size];
				char buf[data_size-recv_size];
				ssize_t ret = recv(sst_fds[1], buf, sizeof(buf), 0);
				if (ret < 0) {
					std::perror("recv(RecvKVPairs): ");
					exit(EXIT_FAILURE);
				}

				kv_pair.append(buf, ret);
				recv_size += ret;
			}

			kv_pairs[queue_num]->enqueue(kv_pair);

			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			recv_time.elapsed += duration;
		}
	}
}

void SstFile::WriteKVPairs(const Options& options) {
	while (write_cnt < num_files) {
		int file_num;
		{
			std::lock_guard<std::mutex> lock(write_mtx);
			file_num = write_cnt;
			++write_cnt;
		}

		// open dst sstable
		SstFileWriter sst_file_writer(EnvOptions(), options);
		Status status = sst_file_writer.Open(file_paths[file_num]);
		AssertStatus(status);

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
			
			// write kv_pair to file
			std::stringstream ss(kv_pair);
			std::string key, value;
			std::getline(ss, key, ',');
			std::getline(ss, value, ',');
			
			status = sst_file_writer.Put(key, value);
			AssertStatus(status);

			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			write_time.elapsed += duration;
		}

		// close sstable file
		status = sst_file_writer.Finish();
		AssertStatus(status);

		// free queues that have been written
		kv_pairs[queue_num] = nullptr;

		// write sstable is completed
		write_finished[file_num] = true;
	}
}

void SstFile::IngestSstFiles(DB* db) {
	while (ingest_cnt < num_files) {
		int file_num;
		{
			std::lock_guard<std::mutex> lock(ingest_mtx);
			file_num = ingest_cnt;
			++ingest_cnt;
		}

		while (!write_finished[file_num]) {
			std::this_thread::yield();
		}

		auto start = std::chrono::high_resolution_clock::now();

		// specify sstable path to ingest
		std::vector<std::string> ingest_file_paths;
		ingest_file_paths.push_back(file_paths[file_num]);
		
		// setting ingestion options
		IngestExternalFileOptions ifo;
		ifo.move_files = true;
		ifo.external_level = file_datas[file_num].level;

		// ingeste sstable into the instance
		Status status = db->IngestExternalFile(ingest_file_paths, ifo);
		AssertStatus(status);

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		ingest_time.elapsed += duration;


		std::cout << "file_number: " << file_num << std::endl;
	}
}

void TransferSstFiles(TransferSstFileArg arg, bool& mig_ended) {
    std::cout << "TransferSstFilesThread is started." << std::endl;
	
	SstFile sst_file(arg.file_datas, arg.file_paths, arg.num_files, arg.num_queues);

	auto start = std::chrono::high_resolution_clock::now();

	std::thread RecvKVPairsThread([&]{ sst_file.RecvKVPairs(arg.sst_fds); });
	std::thread WriteKVPairsThread([&]{ sst_file.WriteKVPairs(arg.options); });
	std::thread IngestSstFilesThread([&]{ sst_file.IngestSstFiles(arg.db); });
	RecvKVPairsThread.join();
	WriteKVPairsThread.join();
	IngestSstFilesThread.join();
	
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	mig_time.elapsed += duration;

	mig_ended = true;
	
	std::cout << "RecvKVPairs   : " << recv_time.elapsed / 1000000 << " s" << std::endl;
	std::cout << "WriteKVPairs  : " << write_time.elapsed / 1000000 << " s" << std::endl;
	std::cout << "IngestSstFiles: " << ingest_time.elapsed / 1000000 << " s" << std::endl;
	std::cout << "Migration     : " << mig_time.elapsed / 1000000 << " s" << std::endl;

	std::cout << "TransferSstFilesThread is ended." << std::endl;
}

void RecvSstFileData(const int& sock_fd,
					 const int& num_files,
					 std::vector<SstFileData>* file_datas) {
	for (int file_num = 0; file_num < num_files; ++file_num) {
		SstFileData file_data;

		if (recv(sock_fd, &file_data, sizeof(SstFileData), 0) < 0) {
			std::cerr << "recv(RecvSstFileData)" << std::endl;
			exit(EXIT_FAILURE);
		}
		file_datas->push_back(file_data);

		// std::cout << file_num << ": " << file_data.num_entries << std::endl;

		SendFlag(sock_fd);
	}
}


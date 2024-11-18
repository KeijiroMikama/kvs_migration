#include <iostream>
#include <random>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include "./db.h"
#include "./workload.h"
#include "./debug.h"

#ifndef IDLE
static void InitValue(std::string* value) {
	char buf[VALUELEN+1];
	
	for (int i = 0; i < VALUELEN; ++i)
		buf[i] = 'A' + (i % 26);

	buf[VALUELEN] = '\0';
	*value = buf;
}

static inline std::string ReturnRandomKey(const int& max_key_num) {
	std::uniform_int_distribution<> KeyDist(0, max_key_num-1);
	std::mt19937 mt{ std::random_device{}() };

	int n = KeyDist(mt);
	char buf[KEYLEN+1];

	sprintf(buf, "%08d", n);
	
	buf[KEYLEN] = '\0';
	std::string key = buf;

	return key;
}

static inline std::string ReturnSkewKey() {
	std::vector<std::string> keys = {"00000000", "00047194", "00550565", "04782100"};
	std::vector<double> weights = {0.6, 0.2, 0.15, 0.05};
    
    std::mt19937 mt( std::random_device{}() );
    std::discrete_distribution<> dist(weights.begin(), weights.end());

    return keys[dist(mt)];
}

void RunWorkload_InMig(WorkloadArg arg, bool& mig_ended) {
	const std::string file_path = ReturnOutputFilePath(arg.src_pmem_rate, 
													   arg.dst_pmem_rate, 
													   arg.write_rate, 
													   arg.id);
	int file_fd = open(file_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC , 00644);
	if (file_fd < 0) {
		std::cerr << "open(RunWorkload_InMig)" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string put_value;
	InitValue(&put_value);

	std::uniform_int_distribution<> PercentageDist(0, 99);
	std::mt19937 mt{ std::random_device{}() };

	OpCount op_cnt;
    RunTime wl_time;
    auto start = std::chrono::high_resolution_clock::now();
	
	while (!mig_ended) {
		const int rate = PercentageDist(mt);
		
		std::string key;
		if (arg.write_rate == 200) {
			key = ReturnSkewKey();
		}
		else {
			key = ReturnRandomKey(KEYMAX);
		}

		if (rate < arg.write_rate && arg.write_rate != 200) {
			// Put Operation
			Status status = arg.db->Put(WriteOptions(), key, put_value);
			AssertStatus(status);
			
			++op_cnt.put_cnt;
		}
		else {
			// Get Operation
			std::string get_value;
			Status status = arg.db->MyGet(ReadOptions(), key, &get_value);
			AssertStatus(status);
			
			++op_cnt.get_cnt;
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
		wl_time.elapsed += duration;
		if (duration >= 1) {
			PrintExpResult(file_fd, wl_time.elapsed, op_cnt.put_cnt, op_cnt.get_cnt);

			start = end;
		}
	}

	close(file_fd);
	

	std::this_thread::sleep_for(std::chrono::seconds(5));

	std::cout << "Elapsed Time    : " << wl_time.elapsed << " s" << std::endl;
	std::cout << "Total Operations: " << op_cnt.put_cnt+op_cnt.get_cnt << " times" << std::endl;
	std::cout << "Throughput      : " << (op_cnt.put_cnt+op_cnt.get_cnt)/1000/wl_time.elapsed << " kreq/s" << std::endl;
}
#endif


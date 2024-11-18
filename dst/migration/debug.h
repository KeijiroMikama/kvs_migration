#pragma once

#include <string>

struct OpCount {
	int put_cnt = 0;
	int get_cnt = 0;
};

struct RunTime {
	double elapsed = 0.0;
};

void PrintExpResult(const int& file_fd, 
					const double& elapsed, 
					const int& put_cnt, 
					const int& get_cnt);
std::string ReturnOutputFilePath(const int& src_pmem_rate, 
								 const int& dst_pmem_rate, 
								 const int& write_rate, 
								 const std::string& id);


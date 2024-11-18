#include <iostream>
#include <vector>
#include <unistd.h>
#include "./debug.h"

/*
template <typename ... Args>
static std::string format(const std::string& fmt, Args ... args ) {
	size_t len = std::snprintf( nullptr, 0, fmt.c_str(), args ... );
	std::vector<char> buf(len + 1);

	std::snprintf(&buf[0], len + 1, fmt.c_str(), args ... );

	return std::string(&buf[0], &buf[0] + len);
}
*/

void PrintExpResult(const int& file_fd, 
					const double& elapsed, 
					const int& put_cnt, 
					const int& get_cnt) {
	const std::string buf = std::to_string((int)elapsed)+","+std::to_string(put_cnt)+","+std::to_string(get_cnt)+"\n";
	write(file_fd, buf.c_str(), buf.size());
}

std::string ReturnOutputFilePath(const int& src_pmem_rate, 
								 const int& dst_pmem_rate, 
								 const int& write_rate, 
								 const std::string& id) {
	std::string file_path = "./result";

	switch (src_pmem_rate) {
		case 0:
			file_path += "/p0";
			break;
		case 2:
			file_path += "/p12";
			break;
		case 3:
			file_path += "/p123";
			break;
		default:
			file_path += "/p1234";
			break;
	}

	switch (dst_pmem_rate) {
        case 0:
            file_path += "/p0";
            break;
        case 2:
            file_path += "/p12";
            break;
        case 3:
            file_path += "/p123";
            break;
        default:
            file_path += "/p1234";
            break;
    }

	switch (write_rate) {
		case 0:
			file_path += "/w0";
			break;
		case 50:
			file_path += "/w50";
			break;
		case 100:
			file_path += "/w100";
			break;
		default:
			file_path += "/w200";
			break;
	}

	file_path += "/throughput" + id + "_in.csv";

	return file_path;
}


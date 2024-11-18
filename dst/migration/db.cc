#include <iostream>
#include "./db.h"

using namespace rocksdb;

void AssertStatus(Status status) {
	if (!status.ok()) {
		std::cout << status.ToString() << std::endl;
		exit(EXIT_FAILURE);
	}
}

#ifdef RELOCATION
static int ReturnTargetLevel(size_t size, 
							 size_t lv1_total, 
							 size_t lv1_file, 
							 double lv_scale, 
							 double f_scale) {
	size_t v = size, lv_size = lv1_total, fsize = lv1_file;
	int level;

	for (level = 0; lv_size <= v; ++level) {
		v -= lv_size;
		lv_size *= lv_scale;
		fsize *= f_scale;
	}

	return level;
}
#endif

void ReturnFilePaths(// const Options& options, 
					 const std::vector<std::string>& dbname, 
					 const int& num_files, 
					 const int& dst_pmem_rate, 
					 std::vector<SstFileData>& file_datas, 
					 std::vector<std::string>* file_paths) {
	/*
#ifdef RELOCATION
	int target_level = ReturnTargetLevel(options.db_paths[0].target_size, 
										 options.max_bytes_for_level_base, 
										 options.target_file_size_base, 
										 options.max_bytes_for_level_multiplier, 
										 options.target_file_size_multiplier);
#else
	int target_level = -1;
#endif
	*/
	int target_level = dst_pmem_rate;

	MyDB mydb(dbname, target_level);

	for (int file_num = 0; file_num < num_files; ++file_num) {
		const std::string file_path = mydb.ReturnFilePath(file_datas[file_num].level, 
														  file_datas[file_num].on_pmem, 
														  file_num);
		file_paths->push_back(file_path);
	}
}


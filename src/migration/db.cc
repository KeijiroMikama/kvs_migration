#include <iostream>
#include "./db.h"

void AssertStatus(Status status) {
	if (!status.ok()) {
		std::cout << status.ToString() << std::endl;
		exit(EXIT_FAILURE);
	}
}

void GetSstFileData(DB* db, 
					const Options& options, 
					std::vector<SstFileData>* file_datas, 
					std::vector<std::string>* src_file_paths, 
					int* num_files) {
	std::vector<LiveFileMetaData> live_file_metadata;
	db->GetLiveFilesMetaData(&live_file_metadata);

	int n = 0;
	for (const auto& meta : live_file_metadata) {
		SstFileData file_data;
#ifdef ONLY_PMEM
		file_data.on_pmem = true;
#elif ONLY_DISK
		file_data.on_pmem = false;
#else
		if (meta.db_path == options.db_paths[0].path) {
			file_data.on_pmem = true;
		}
		else {
			file_data.on_pmem = false;
		}
#endif
		file_data.level = meta.level;
		file_data.num_entries = meta.num_entries;
		file_datas->push_back(file_data);
		src_file_paths->emplace_back(meta.db_path+meta.name);
		
		++n;
	}
	*num_files = n;
}


#include <iostream>
#include <thread>
#include "./socket.h"
#include "./workload.h"

using namespace rocksdb;

static void StartInstance(const std::string& id, 
						  const int& src_pmem_rate, 
						  const int& dst_pmem_rate, 
						  const int& write_rate) {
	std::cout << "Destination Instance is started." << std::endl;

/*-------------------------------------- Phase 1 --------------------------------------*/


    // open socket
	SocketFD socket_fd;
	OpenSocket(id, &socket_fd);

	// specify db path
	const std::string db_path = "/mig_inst" + id;
#ifdef ONLY_PMEM
	const std::vector<std::string> dbname = {pmem_db_dir + db_path};
#elif ONLY_DISK
	const std::vector<std::string> dbname = {disk_db_dir + db_path};
#else
	const std::vector<std::string> dbname = {pmem_db_dir + db_path, disk_db_dir + db_path};
#endif


/*-------------------------------------- Phase 1 --------------------------------------*/


	// recieve options
	SrcOptions src_options;
	RecvOptions(socket_fd.main_fd, &src_options);

	Options options;
	options.IncreaseParallelism();
	options.create_if_missing = true;
	options.compression = kNoCompression;
#ifdef MIXING
	uint64_t PMEMSIZE;
	if (dst_pmem_rate == 2) {
		PMEMSIZE = 16ULL * 1024 * 1024 * 1024;
	}
	else {
		PMEMSIZE = 32ULL * 1024 * 1024 * 1024;
	}
	uint64_t DISKSIZE = 128ULL * 1024 * 1024 * 1024;
	options.db_paths = {{dbname[0], PMEMSIZE}, {dbname[1], DISKSIZE}};
#endif

	// open db
	DB* db;
	Status status = DB::Open(options, dbname[0], &db);
	AssertStatus(status);
	std::cout << "dbname: " << dbname[0] << std::endl;

#ifndef IDLE
	db->InitSocketFDs(socket_fd.wl_fds);
	db->InitValueSize(VALUELEN);
#endif

	// receive sstables metadata
	std::vector<SstFileData> file_datas;
	RecvSstFileData(socket_fd.main_fd, 
					src_options.num_files, 
					&file_datas);
	
	// generate sstable paths
    std::vector<std::string> file_paths;
	ReturnFilePaths(// options, 
					dbname, 
					src_options.num_files, 
					dst_pmem_rate, 
					file_datas, 
					&file_paths);

	// on auto_compaction
	status = db->SetOptions({{"disable_auto_compactions", "true"},});
	AssertStatus(status);


/*-------------------------------------- Phase 2 --------------------------------------*/

	
	bool mig_ended = false;
#ifndef IDLE
	// initialize argument for workload
	WorkloadArg wl_arg(db, 
					   src_pmem_rate, 
					   dst_pmem_rate, 
					   write_rate, 
					   id);
	
	// run workload
	std::thread RunWorkload_InMigThread([&]{ RunWorkload_InMig(wl_arg, std::ref(mig_ended)); });
#endif

	// initialize argument for transfer sstables
	TransferSstFileArg tsf_arg(db, 
							   options, 
							   file_datas, 
							   file_paths, 
							   src_options.num_files, 
							   src_options.num_queues, 
							   socket_fd.sst_fds);
	
	// run transfer sstables thread
	std::thread TransferSstFilesThread([&]{ TransferSstFiles(tsf_arg, std::ref(mig_ended)); });
	TransferSstFilesThread.join();
	SendFlag(socket_fd.main_fd);

#ifndef IDLE
	RunWorkload_InMigThread.join();
#endif


/*-------------------------------------- Phase 3 --------------------------------------*/


	// off auto_compaction
	status = db->SetOptions({{"disable_auto_compactions", "false"},});
	AssertStatus(status);

	// close socket
	CloseSocket(socket_fd);


	delete db;
}

int main(int argc, char* argv[]) {
	if (argc != 5) {
		std::cerr << "argument error" << std::endl;
		return -1;
	}

	const std::string id = argv[1];
	int src_pmem_rate = atoi(argv[2]);
	int dst_pmem_rate = atoi(argv[3]);
	int write_rate = atoi(argv[4]);
	
	StartInstance(id, src_pmem_rate, dst_pmem_rate, write_rate);

	return 0;
}

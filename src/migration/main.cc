#include <iostream>
#include <thread>
#include "./socket.h"
#include "./workload.h"

static void StartInstance(const std::string& id, const int& write_rate) {
	std::cout << "Source Instance is started." << std::endl;
	
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

	// setting options
	Options options;
	options.IncreaseParallelism();
	options.create_if_missing = true;
	options.compression = kNoCompression;
#ifdef MIXING
	options.db_paths = {{dbname[0], PMEMSIZE}, {dbname[1], DISKSIZE}};
#endif

	// open db
	DB* db;
	Status status = DB::Open(options, dbname[0], &db);
	AssertStatus(status);
	std::cout << "dbname: " << dbname[0] << std::endl;
#ifndef IDLE
    db->InitSocketFDs(socket_fd.wl_fds);
#endif


/*-------------------------------------- Phase 1 --------------------------------------*/


	// get sstables metadata
	std::vector<SstFileData> file_datas;
	std::vector<std::string> src_file_paths;
	int num_files;
	GetSstFileData(db, options, &file_datas, &src_file_paths, &num_files);

	// send options
	int num_queues = 8;
	SrcOptions src_options(num_files, num_queues);
	SendOptions(socket_fd.main_fd, src_options);
	SendSstFileData(socket_fd.main_fd, num_files, file_datas);


/*-------------------------------------- Phase 2 --------------------------------------*/

	
#ifndef IDLE
	//
	WorkloadArg wl_arg(db, write_rate, socket_fd.wl_fds);
	std::thread RunWorkload_InMigThread([&]{ RunWorkload_InMig(wl_arg); });
#endif

	// initialize argument for transfer sstables
	TransferSstFileArg tsf_arg(options, 
							   file_datas, 
							   src_file_paths, 
							   num_files, 
							   num_queues, 
							   socket_fd.sst_fds);

	// run transfer sstables thread
	std::thread TransferSstFilesThread([&]{ TransferSstFiles(tsf_arg); });
	TransferSstFilesThread.join();
	
	RecvFlag(socket_fd.main_fd);

#ifndef IDLE
	RunWorkload_InMigThread.join();
#endif


/*-------------------------------------- Phase 3 --------------------------------------*/


	// close socket
	CloseSocket(socket_fd);

	delete db;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "argument error" << std::endl;
		return -1;
	}

	const std::string id = argv[1];
	int write_rate = atoi(argv[2]);

	StartInstance(id, write_rate);

    return 0;
}


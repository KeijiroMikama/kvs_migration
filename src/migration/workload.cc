#include <iostream>
#include <thread>
#include <sys/socket.h>
#include "./db.h"
#include "./workload.h"

void RunWorkload_InMig(WorkloadArg arg) {
	std::cout << "RunWorkload_InMigThread is started." << std::endl;

	int pull_cnt = 0;	
	while (1) {
		std::string get_value;
		Status status = arg.db->MyGet(ReadOptions(), &get_value);
		if (status.IsTimedOut()) {
			break;
		}
		
		++pull_cnt;
	}

	
	std::this_thread::sleep_for(std::chrono::seconds(5));

	std::cout << "Pull Count: " << pull_cnt << " times" << std::endl;


	std::cout << "RunWorkload_InMigThread is ended." << std::endl;
}

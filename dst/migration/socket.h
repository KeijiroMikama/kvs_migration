#pragma once

#include "./db.h"

struct SocketFD {
	int main_fd;
#ifndef IDLE
	std::vector<int> wl_fds;
#endif
	std::vector<int> sst_fds;
};

void OpenSocket(const std::string& id, SocketFD* socket_fd);
void CloseSocket(const SocketFD& socket_fd);
void SendFlag(const int& sock_fd);
void RecvOptions(const int& sock_fd, SrcOptions* src_options);
void RecvData(const int& sock_fd, const size_t& data_size, std::string* recv_data);


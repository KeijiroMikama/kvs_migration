#pragma once

#include "./db.h"

struct SocketFD {
	int main_fd_;
	int main_fd;
#ifndef IDLE
	std::vector<int> wl_fds_;
	std::vector<int> wl_fds;
#endif
	std::vector<int> sst_fds_;
	std::vector<int> sst_fds;
};

void OpenSocket(const std::string& id, SocketFD* socket_fd);
void CloseSocket(const SocketFD& socket_fd);
void RecvFlag(const int& sock_fd);
void SendOptions(const int& sock_fd, const SrcOptions& src_options);
void SendData(const int& sock_fd, const std::string send_data);


#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "./socket.h"

static int ConnectSocket(const int& port_num, const char* ip) {
	sockaddr_in saddr;
	int         fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		std::cerr << "socket" << std::endl;
		exit(EXIT_FAILURE);
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family      = AF_INET;
	saddr.sin_port        = htons(port_num);
	saddr.sin_addr.s_addr = inet_addr(ip);

	while (true) {
		int ret = connect(fd, (sockaddr*)&saddr, sizeof(saddr));
		if (ret == 0) {
			break;
		}
        else {
			std::cerr << "connect()" << std::endl;
			// close(fd);
			// exit(EXIT_FAILURE);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	return fd;
}

void OpenSocket(const std::string& id, SocketFD* socket_fd) {
	int port_num = 20000 + (atoi(id.c_str()) * 100);
	const char* ip = "192.168.202.204";

	socket_fd->main_fd = ConnectSocket(port_num++, ip);
#ifndef IDLE
	for (int i = 0; i < 2; ++i) {
		socket_fd->wl_fds.emplace_back(ConnectSocket(port_num++, ip));
	}
#endif
	for (int i = 0; i < 2; ++i) {
		socket_fd->sst_fds.emplace_back(ConnectSocket(port_num++, ip));
	}
}

void CloseSocket(const SocketFD& socket_fd) {
	close(socket_fd.main_fd);
#ifndef IDLE
	for (int i = 0; i < 2; ++i) {
		close(socket_fd.wl_fds[i]);
	}
#endif
	for (int i = 0; i < 2; ++i) {
		close(socket_fd.sst_fds[i]);
	}
}

void SendFlag(const int& sock_fd) {
	int flag = 1;
	ssize_t send_size = send(sock_fd, &flag, sizeof(int), 0);
	if (send_size < 0) {
		std::cerr << "send(SendFlag)" << std::endl;
		exit(EXIT_FAILURE);
	}
}

/*
short RecvFlag(const int& fd, const std::string& flag_name) {
	short flag = 0;

	if (recv(fd, &flag, sizeof(short), 0) < 0) {
		std::perror(("recv(" + flag_name + ")").c_str());
		exit(EXIT_FAILURE);
	}

	return flag;
}
*/

void RecvOptions(const int& sock_fd, SrcOptions* src_options) {
	ssize_t recv_size = recv(sock_fd, src_options, sizeof(SrcOptions), 0);
	if (recv_size < 0) {
		std::cerr << "recv(RecvOptions)" << std::endl;
		exit(EXIT_FAILURE);
	}

	SendFlag(sock_fd);
}

void RecvData(const int& sock_fd, const size_t& data_size, std::string* recv_data) {
	size_t recv_size = 0;
	while (recv_size < data_size) {
		char buf[data_size+1];
		
		ssize_t ret = recv(sock_fd, buf, sizeof(buf), 0);
		if (ret < 0) {
			std::cerr << "recv(RecvData)" << std::endl;
			exit(EXIT_FAILURE);
		}

		recv_data->append(buf, ret);
		recv_size += ret;
	}
	SendFlag(sock_fd);
}

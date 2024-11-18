#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "./socket.h"

static int ListenSocket(const int& port_num) {
	sockaddr_in saddr;
	int         sock_fd;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		std::cerr << "socket()" << std::endl;
		exit(EXIT_FAILURE);
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family      = AF_INET;
	saddr.sin_port        = htons(port_num);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock_fd, (sockaddr*)&saddr, sizeof(saddr)) < 0) {
		std::cerr << "bind()" << std::endl;
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(sock_fd, SOMAXCONN) < 0) {
		std::cerr << "listen()" << std::endl;;
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	return sock_fd;
}

static int AcceptSocket(const int& sock_fd) {
	sockaddr_in caddr;
	int         fd;

	socklen_t len = sizeof(caddr);
	if ((fd = accept(sock_fd, (sockaddr*)&caddr, &len)) < 0) {
		std::cerr << "accept()" << std::endl;
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	return fd;
}

void OpenSocket(const std::string& id, SocketFD* socket_fd) {
	int port_num = 20000 + (atoi(id.c_str()) * 100);

	socket_fd->main_fd_ = ListenSocket(port_num++);
#ifndef IDLE
	for (int i = 0; i < 2; ++i) {
		socket_fd->wl_fds_.emplace_back(ListenSocket(port_num++));
	}
#endif
	for (int i = 0; i < 2; ++i) {
		socket_fd->sst_fds_.emplace_back(ListenSocket(port_num++));
	}


	socket_fd->main_fd = AcceptSocket(socket_fd->main_fd_);
#ifndef IDLE
	for (int i = 0; i < 2; ++i) {
		socket_fd->wl_fds.emplace_back(AcceptSocket(socket_fd->wl_fds_[i]));
	}
#endif
	for (int i = 0; i < 2; ++i) {
		socket_fd->sst_fds.emplace_back(AcceptSocket(socket_fd->sst_fds_[i]));
	}
}

void CloseSocket(const SocketFD& socket_fd) {
	close(socket_fd.main_fd_);
	close(socket_fd.main_fd);
#ifndef IDLE
	for (int i = 0; i < 2; ++i) {
		close(socket_fd.wl_fds_[i]);
		close(socket_fd.wl_fds[i]);
	}
#endif
	for (int i = 0; i < 2; ++i) {
		close(socket_fd.sst_fds_[i]);
		close(socket_fd.sst_fds[i]);
	}

}

void RecvFlag(const int& sock_fd) {
	int flag;
	ssize_t recv_size = recv(sock_fd, &flag, sizeof(int), 0);
	if (recv_size < 0 || flag != 1) {
		std::cerr << "recv(RecvFlag)" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void SendOptions(const int& sock_fd, const SrcOptions& src_options) {
	ssize_t send_size = send(sock_fd, &src_options, sizeof(SrcOptions), 0);
	if (send_size < 0) {
		std::cerr << "send(SendOptions)" << std::endl;
		exit(EXIT_FAILURE);
	}

	RecvFlag(sock_fd);
}

void SendData(const int& sock_fd, const std::string send_data) {
	if (send(sock_fd, send_data.c_str(), send_data.size(), 0) < 0) {
		std::cerr << "send(SendData)" << std::endl;
		exit(EXIT_FAILURE);
	}
	RecvFlag(sock_fd);
}

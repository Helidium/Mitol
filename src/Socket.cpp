//
// Created by helidium on 3/27/17.
//

#include "Socket.h"

MNS::SocketData::SocketData(uv_poll_t *poll_h, int fd, SOCKET_TYPE type, MNS::Server *server) {
	this->poll_h = poll_h;
	this->fd = fd;
	this->type = type;
	this->server = server;
	this->request = NULL;
	this->response = NULL;
	this->nodeRequestPlaceholder = nullptr;
	this->nodeResponsePlaceholder = nullptr;
	if (type != SOCKET_TYPE::LISTENING) {
		this->request = new MNS::Request(this);
		this->response = new MNS::Response(this);
	}
}

MNS::SocketData::~SocketData() {
	if (this->request) delete (this->request);
	if (this->response) delete (this->response);
	this->fd = -1;
	this->request = NULL;
	this->response = NULL;
	this->server = NULL;
	this->nodeRequestPlaceholder = nullptr;
	this->nodeResponsePlaceholder = nullptr;
}

int MNS::Socket::createListening(int port) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		int y_int = 1;
		//setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &y_int, sizeof(int));
		setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &y_int, sizeof(int));

		s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0) {
			/* We managed to bind successfully! */
			break;
		}

		close(sfd);
	}

	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	freeaddrinfo(result);

	int y_int = 1;
	if ((setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &y_int, sizeof(int)) == -1) ||
	    (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &y_int, sizeof(int)) == -1) ||
	    (setsockopt(sfd, IPPROTO_TCP, TCP_QUICKACK, &y_int, sizeof(int)) == -1)) {
		// TODO: Return an error
		return -1;
	}
	Socket::makeNonBlocking(sfd);

	if (::listen(sfd, 4096) == -1) {
		// TODO: Return an error
		return -1;
	}

	return sfd;
}

int MNS::Socket::makeNonBlocking(int sfd) {
	int flags, s;

	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s == -1) {
		perror("fcntl");
		return -1;
	}

	return 0;
}
//
// Created by helidium on 3/22/17.
//

#include <ctime>
#include "Server.h"

static uv_loop_t *loop;
static std::unordered_map<int, uv_poll_t *> polls;
std::string MNS::Server::currTime = "";
std::map<int, std::string> MNS::Server::response_msgs;

void MNS::Server::onConnect(uv_poll_t *handle, int status, int events) {
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	if (data && status==0) {
		const MNS::Server *server = data->server;
		socklen_t addr_size = 0;
		sockaddr_in sadr;
#ifdef __linux__
		int csock = accept4(data->fd, (sockaddr *) &sadr, &addr_size, SOCK_NONBLOCK);
#elif __APPLE__
		int csock = accept(data->fd, (sockaddr *) &sadr, &addr_size);
		if(csock >= 0) MNS::Socket::makeNonBlocking(csock);
#endif

		if(csock >= 0) { // If valid socket
			int y_int = 1;
			setsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &y_int, sizeof(int));
#ifdef __linux__
			setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, &y_int, sizeof(int));
#endif
			//setsockopt(csock, IPPROTO_TCP, TCP_QUICKACK, &y_int, sizeof(int));

			uv_poll_t *socket_poll_h = (uv_poll_t *) malloc(sizeof(uv_poll_t));
			socket_poll_h->data = new MNS::SocketData(socket_poll_h, csock, MNS::SOCKET_TYPE::PEER, data->server);

			uv_poll_init(loop, socket_poll_h, csock);
			uv_poll_start(socket_poll_h, UV_READABLE, MNS::Server::onReadData);

			polls[csock] = socket_poll_h;

			if (server && server->onHttpConnectionHandler) {
				server->onHttpConnectionHandler(static_cast<MNS::SocketData *>(socket_poll_h->data));
			}
		}
	}
}

void MNS::Server::onClose(uv_handle_t *handle) {
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	// Deregister NodeJS Persistent<Object>
	MNS::Server *server = data->server;
	if(server && server->onHttpCancelHandler) server->onHttpCancelHandler(data);

	polls.erase(data->fd);
	close(data->fd);
	delete (data);
	free(handle);
}

void MNS::Server::onCloseTimer(uv_handle_t *handle) {
	uv_timer_stop((uv_timer_t *) handle);
	delete (handle);
}

void MNS::Server::onReadDataPipelined(uv_poll_t *handle, int status, int events) {
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	MNS::Server *server = data->server;

	if (!data->request->Parse(-1)) {
		data->response->startResponse();

		if (server->onHttpRequestHandler) {
			server->onHttpRequestHandler(data);
		} else {
			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);
		}
	} else {
		printf("Parse ERROR! Closing socket!\n");

		uv_poll_stop(handle);
		uv_close((uv_handle_t *) handle, MNS::Server::onClose);
	}
}

void MNS::Server::onReadData(uv_poll_t *handle, int status, int events) {
	if (status < 0 && errno != EAGAIN) {
		uv_poll_stop(handle);
		uv_close((uv_handle_t *) handle, MNS::Server::onClose);

		return;
	}

	ssize_t requestLen = 0;
	ssize_t bytesRead = 0;
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	MNS::Server *server = data->server;

	// Todo: Offset and grow buffer in case of larger requests
	// Todo: Mark request state as reading socket
	while ((bytesRead = recv(data->fd, data->request->getBuffer() + requestLen, 1024, 0)) > 0) { // WHILE DATA READ:: POTENTIAL ERROR SIZE OF READ DATA
		requestLen += bytesRead;

		// If buffer too small, resize
		if(requestLen + 1024 >= data->request->getBufferSize()) {
			data->request->resizeBuffer(data->request->getBufferSize() + 4096);
		}
	}

	if (requestLen <= 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
		} else {
			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);

			return;
		}
	} else {
		if (!data->request->Parse(requestLen)) {
			data->response->startResponse();

			if (server->onHttpRequestHandler) {
				server->onHttpRequestHandler(data);
			} else {
				fflush(stdout);

				uv_poll_stop(handle);
				uv_close((uv_handle_t *) handle, MNS::Server::onClose);
			}
		} else {
			//printf("Parse ERROR! Closing socket!\n");

			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);
		}
	}
}

void MNS::Server::onWriteData(uv_poll_t *handle, int status, int events) {
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);

	if (status < 0) {
		if (errno != EAGAIN) {
			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);
			return;
		}
	}

	if (data) {
		MNS::Response *response = data->response;

		ssize_t numSent = send(data->fd, response->getBuffer(), response->getBufferLen(), 0);
		if (numSent == (int) response->getBufferLen()) {
			response->clear();

			if (data->request->isFinished()) {
				if(!uv_is_closing((uv_handle_t*)handle))
					uv_poll_start(handle, UV_READABLE, onReadData);
			} else {
				// Request not finished, pipelining, continue serving data
				if(!uv_is_closing((uv_handle_t*)handle))
					uv_poll_start(handle, UV_WRITABLE, onReadDataPipelined);
				//onReadDataPipelined(handle, 0, 0);
			}
		} else if (numSent < 0) {
			int err = errno;

			if (err != EAGAIN) {
				uv_poll_stop(handle);
				uv_close((uv_handle_t *) handle, MNS::Server::onClose);
				return;
			}
		}

	}
}

MNS::Server::Server() {
	MNS::Server::response_msgs[200] = "200 OK\r\n";
	MNS::Server::response_msgs[301] = "301 Redirect\r\n";
	MNS::Server::response_msgs[404] = "404 Not Found\r\n";
	MNS::Server::response_msgs[500] = "500 Internal Server Error\r\n";

	loop = uv_default_loop();

	this->onHttpCancelHandler = NULL;
	this->onHttpConnectionHandler = NULL;
	this->onHttpRequestHandler = NULL;
	this->listeningSocket = 0;
}

void MNS::Server::listen(int port) {
	listeningSocket = Socket::createListening(port);

	if (listeningSocket != -1) {
		// Start the timer
		this->timer_h = new uv_timer_t;//(uv_timer_t *)malloc(sizeof(uv_timer_t));
		uv_timer_init(loop, timer_h);
		uv_timer_start(timer_h, MNS::Server::onSecondTimer, 0, 1000);

		// Register the listening socket
		uv_poll_t *listening_poll_h = (uv_poll_t *) malloc(sizeof(uv_poll_t));
		listening_poll_h->data = new SocketData(listening_poll_h, listeningSocket, SOCKET_TYPE::LISTENING, this);

		uv_poll_init(loop, listening_poll_h, listeningSocket);

		uv_poll_start(listening_poll_h, UV_READABLE, onConnect);
		polls[listeningSocket] = listening_poll_h;
	}
}

void MNS::Server::run() {
	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);
}

void MNS::Server::stop() {
	uv_close((uv_handle_t *) this->timer_h, MNS::Server::onCloseTimer);

	for (auto it = polls.begin(); it != polls.end(); ++it) {
		uv_poll_stop(it->second);
		uv_close((uv_handle_t *) it->second, MNS::Server::onClose);
	}

	polls.clear();
}

void MNS::Server::onSecondTimer(uv_timer_t *handle) {
	char strt[100];
	std::time_t t = std::time(NULL);
	std::strftime(strt, 100, "%a, %e %b %Y %H:%M:%S GMT\0", std::gmtime(&t));

	MNS::Server::currTime = strt;
}

void MNS::Server::onHttpConnection(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpConnectionHandler = callback;
}

void MNS::Server::onHttpRequest(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpRequestHandler = callback;
}

void MNS::Server::onHttpCancel(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpCancelHandler = callback;
}


MNS::Server::~Server() {
}
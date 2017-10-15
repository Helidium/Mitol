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
			if(setsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &y_int, sizeof(int)) == -1) {
				// NOOP
			}
#ifdef __linux__
			if(setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, &y_int, sizeof(int)) == -1) {
				// NOOP
			}
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

	int parseResult = data->request->Parse(-1);
	if (!parseResult) { // PARSING OK
		data->response->startResponse();

		if (server->onHttpRequestHandler) {
			server->onHttpRequestHandler(data);
		} else {
			fflush(stdout);

			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);
		}
	} else { // ERROR PARSING
		if(data->request->state == MNS::Request::REQUEST_STATE::NEED_MORE_DATA) { // MORE DATA REQUIRED
			if(!uv_is_closing((uv_handle_t*)handle))
				uv_poll_start(handle, UV_READABLE, onReadData);
		} else {
			//printf("Parse ERROR! Closing socket!\n");

			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);
		}
	}
}

void MNS::Server::onReadData(uv_poll_t *handle, int status, int events) {
	if (status < 0 && errno != EAGAIN) {
		uv_poll_stop(handle);
		uv_close((uv_handle_t *) handle, MNS::Server::onClose);

		return;
	}

	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	MNS::Server *server = data->server;

	ssize_t requestLen = data->request->isFinished()?0:data->request->getBufferLen();
	ssize_t bytesRead = 0;
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
		int parseResult = data->request->Parse(requestLen);
		if (!parseResult) { // PARSING OK
			data->response->startResponse();

			if (server->onHttpRequestHandler) {
				server->onHttpRequestHandler(data);
			} else {
				fflush(stdout);

				uv_poll_stop(handle);
				uv_close((uv_handle_t *) handle, MNS::Server::onClose);
			}
		} else { // ERROR PARSING
			if(data->request->state == MNS::Request::REQUEST_STATE::NEED_MORE_DATA) { // MORE DATA REQUIRED
				if(!uv_is_closing((uv_handle_t*)handle))
					uv_poll_start(handle, UV_READABLE, onReadData);
			} else {
				//printf("Parse ERROR! Closing socket!\n");

				uv_poll_stop(handle);
				uv_close((uv_handle_t *) handle, MNS::Server::onClose);
			}
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
				data->request->clear();
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
	MNS::Server::response_msgs[100] = "100 Continue\r\n";
	MNS::Server::response_msgs[101] = "101 Switching Protocols\r\n";
	MNS::Server::response_msgs[102] = "102 Checkpoint\r\n";

	MNS::Server::response_msgs[200] = "200 OK\r\n";
	MNS::Server::response_msgs[201] = "201 Created\r\n";
	MNS::Server::response_msgs[202] = "202 Accepted\r\n";
	MNS::Server::response_msgs[203] = "203 Non-Authoritative Information\r\n";
	MNS::Server::response_msgs[204] = "204 No Content\r\n";
	MNS::Server::response_msgs[205] = "205 Reset Content\r\n";
	MNS::Server::response_msgs[206] = "206 Partial Content\r\n";

	MNS::Server::response_msgs[300] = "300 Multiple Choices\r\n";
	MNS::Server::response_msgs[301] = "301 Moved Permanently\r\n";
	MNS::Server::response_msgs[302] = "302 Found\r\n";
	MNS::Server::response_msgs[303] = "303 See Other\r\n";
	MNS::Server::response_msgs[304] = "304 Not Modified\r\n";
	MNS::Server::response_msgs[306] = "306 Switch Proxy\r\n";
	MNS::Server::response_msgs[307] = "307 Temporary Redirect\r\n";
	MNS::Server::response_msgs[308] = "308 Resume Incomplete\r\n";


	MNS::Server::response_msgs[400] = "400 Bad Request\r\n";
	MNS::Server::response_msgs[401] = "401 Unauthorized\r\n";
	MNS::Server::response_msgs[402] = "402 Payment Required\r\n";
	MNS::Server::response_msgs[403] = "403 Forbidden\r\n";
	MNS::Server::response_msgs[404] = "404 Not Found\r\n";
	MNS::Server::response_msgs[405] = "405 Method Not Allowed\r\n";
	MNS::Server::response_msgs[406] = "406 Not Acceptable\r\n";
	MNS::Server::response_msgs[407] = "407 Proxy Authentication Required\r\n";
	MNS::Server::response_msgs[408] = "408 Request Timeout\r\n";
	MNS::Server::response_msgs[409] = "409 Conflict\r\n";
	MNS::Server::response_msgs[410] = "410 Gone\r\n";
	MNS::Server::response_msgs[411] = "411 Length Required\r\n";
	MNS::Server::response_msgs[412] = "412 Precondition Failed\r\n";
	MNS::Server::response_msgs[413] = "413 Request Entity Too Large\r\n";
	MNS::Server::response_msgs[414] = "414 Request-URI Too Long\r\n";
	MNS::Server::response_msgs[415] = "415 Unsupported Media Type\r\n";
	MNS::Server::response_msgs[416] = "416 Requested Range Not Satisfiable\r\n";
	MNS::Server::response_msgs[417] = "417 Expectation Failed\r\n";

	MNS::Server::response_msgs[500] = "500 Internal Server Error\r\n";
	MNS::Server::response_msgs[501] = "501 Not Implemented\r\n";
	MNS::Server::response_msgs[502] = "502 Bad Gateway\r\n";
	MNS::Server::response_msgs[503] = "503 Service Unavailable\r\n";
	MNS::Server::response_msgs[504] = "504 Gateway Timeout\r\n";
	MNS::Server::response_msgs[505] = "505 HTTP Version Not Supported\r\n";
	MNS::Server::response_msgs[511] = "511 Network Authentication Required\r\n";

	loop = uv_default_loop();

	this->onHttpCancelHandler = NULL;
	this->onHttpConnectionHandler = NULL;
	this->onHttpRequestHandler = NULL;
	this->listeningSocket = 0;
	this->timer_h = NULL;
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

		if(this->onHttpListeningHandler) {
			this->onHttpListeningHandler();
		}
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

void MNS::Server::onHttpListening(const std::function<void()> &callback) {
	this->onHttpListeningHandler = callback;
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
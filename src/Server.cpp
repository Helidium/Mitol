//
// Created by helidium on 3/22/17.
//

#include <ctime>
#include "Server.h"

static uv_loop_t *loop;
static std::unordered_map<int, uv_poll_t*> polls;
std::string MNS::Server::currTime = "";
std::map<int, std::string> MNS::Server::response_msgs;

void MNS::Server::onConnect(uv_poll_t *handle, int status, int events) {
	//printf("Acception socket"); fflush(stdout);
	MNS::SocketData* data = static_cast<MNS::SocketData *>(handle->data);
	if(data) {
		const MNS::Server* server = data->server;
		socklen_t addr_size = 0;
		sockaddr_in sadr;
		int csock = accept4(data->fd, (sockaddr *) &sadr, &addr_size, SOCK_NONBLOCK);

		int y_int = 1;
		//setsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &y_int, sizeof(int));
		setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, &y_int, sizeof(int));
		//setsockopt(csock, IPPROTO_TCP, TCP_QUICKACK, &y_int, sizeof(int));

		uv_poll_t *socket_poll_h = (uv_poll_t *) malloc(sizeof(uv_poll_t));
		socket_poll_h->data = new MNS::SocketData(socket_poll_h, csock, MNS::SOCKET_TYPE::PEER, data->server);

		uv_poll_init(loop, socket_poll_h, csock);
		uv_poll_start(socket_poll_h, UV_READABLE, MNS::Server::onReadData);

		polls[csock] = socket_poll_h;

		if(server && server->onHttpConnectionHandler) {
			server->onHttpConnectionHandler(static_cast<MNS::SocketData *>(socket_poll_h->data));
		}
	}
}

void MNS::Server::onClose(uv_handle_t *handle) {
	printf("Closing socket\n");
	MNS::SocketData* data = static_cast<MNS::SocketData *>(handle->data);

	polls.erase(data->fd);
	close(data->fd);
	delete (data);
	free(handle);
}

void MNS::Server::onReadData(uv_poll_t *handle, int status, int events) {
	ssize_t requestLen = 0;
	ssize_t bytesRead = 0;
	MNS::SocketData* data = static_cast<MNS::SocketData*>(handle->data);
	MNS::Server *server = data->server;

	//if(data) {
		// Todo: Offset and grow buffer in case of larger requests
		// Todo: Mark request state as reading socket
		while ((bytesRead = recv(data->fd, data->request->getBuffer(), 1024, 0)) >
		       0) { // WHILE DATA READ:: POTENTIAL ERROR SIZE OF READ DATA
			requestLen += bytesRead;
		}

		if (requestLen <= 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
			} else {
				//printf("Closing socket, err %s\n", uv_err_name(errno));

				uv_poll_stop(handle);
				uv_close((uv_handle_t *)handle, MNS::Server::onClose);
			}
		} else {
			if (!data->request->Parse(requestLen)) {
				data->response->startResponse();

				if(server->onHttpRequestHandler) {
					server->onHttpRequestHandler(data);
				} else {
					printf("No request handler defined\n");
					fflush(stdout);

					uv_poll_stop(handle);
					uv_close((uv_handle_t*)handle, MNS::Server::onClose);
				}

				//uv_poll_start(handle, UV_WRITABLE, onWriteData);
			} else {
				printf("Parse ERROR! Closing socket!\n");
			}
		}
	//}
}

void MNS::Server::onWriteData(uv_poll_t *handle, int status, int events) {
	MNS::SocketData* data = static_cast<MNS::SocketData *>(handle->data);

	if(data) {
		MNS::Response *response = data->response;

		ssize_t numSent = send(data->fd, response->getBuffer(), response->getBufferLen(), 0);
		if(numSent == (int)response->getBufferLen()) {
			response->clear();
			uv_poll_start(handle, UV_READABLE, onReadData);
		}

		//uv_poll_stop(handle);
		//uv_close((uv_handle_t *)handle, MNS::Server::onClose);

//		const char *rBuffer = "HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: MNS\r\nContent-Length: 12\r\nContent-Type: text\r\nConnection: Keep-Alive\r\n\r\nHello World!\0";
//
//		send(data->fd, rBuffer, strlen(rBuffer), 0);
//
//		uv_poll_start(handle, UV_READABLE, onReadData);
	}
}

MNS::Server::Server() {
	MNS::Server::response_msgs[200] = "200 OK\r\n";
	MNS::Server::response_msgs[301] = "301 Redirect\r\n";
	MNS::Server::response_msgs[404] = "404 Not Found\r\n";
	MNS::Server::response_msgs[500] = "500 Internal Server Error\r\n";

	loop = uv_default_loop();
}

void MNS::Server::listen(int port) {
	listeningSocket = Socket::createListening(port);

	if(listeningSocket != -1) {
		// Start the timer
		uv_timer_t *timer_h = new uv_timer_t;
		uv_timer_init(loop, timer_h);
		uv_timer_start(timer_h, MNS::Server::onSecondTimer, 0, 1000);

		// Register the listening socket
		uv_poll_t *listening_poll_h = new uv_poll_t;
		listening_poll_h->data = new SocketData(listening_poll_h, listeningSocket, SOCKET_TYPE::LISTENING, this);

		uv_poll_init(loop, listening_poll_h, listeningSocket);

		uv_poll_start(listening_poll_h, UV_READABLE, onConnect);
		polls[listeningSocket] = listening_poll_h;
	}

	//struct epoll_event event;
	//event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP;
	//event.data.ptr = new SocketData(listeningSocket, SOCKET_TYPE::LISTENING);
	//epoll_ctl(epollFd, EPOLL_CTL_ADD, listeningSocket, &event);

	/*socklen_t addr_size = 0;
	sockaddr_in sadr;
	struct epoll_event events[1024];
	int allevents = 1000000;
	for (;;) {
		int numEvents = epoll_wait(epollFd, events, 1024, 500);
		if (numEvents <= 0) continue;


		if (allevents <= 0) break;

		for (struct epoll_event *cevent = events; numEvents--; cevent++) {
			SocketData *eventData = (SocketData *) cevent->data.ptr;

			if (cevent->events & EPOLLERR || cevent->events & EPOLLHUP ||
			    cevent->events & EPOLLRDHUP) { // ERROR OCCURED OR SOCKET CLOSED
				// Todo: Handle Socket termination

				close(eventData->fd);
				delete (eventData);
				continue;
			} else if (eventData->type == SOCKET_TYPE::LISTENING) { // ACCEPT CONNECTION
				while (1) {
					// ACCEPT CONNECTION
					int csock = accept4(eventData->fd, (sockaddr *) &sadr, &addr_size, SOCK_NONBLOCK);
					if (csock == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							//printf("%s\n", explain_accept4( cevent->data.fd, (sockaddr*)&sadr, &addr_size, SOCK_NONBLOCK) );
							// We have processed all incoming connections.
							break;
						} else {
							printf("%s\n", "Error accepting connection");
							perror("accept");
							break;
						}
					} else {
						int y_int = 1;
						int n_int = 0;
						setsockopt(csock, SOL_SOCKET, SO_REUSEADDR, &y_int, sizeof(int));

						setsockopt(csock, SOL_TCP, TCP_QUICKACK, &n_int, sizeof(int));
						setsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &y_int, sizeof(int));
						//setsockopt( csock, SOL_TCP, TCP_DEFER_ACCEPT, &y_int, sizeof(int) );


						// ADD SOCKET TO EPOLL QUEUE
						epoll_event tevent;
						tevent.data.ptr = new SocketData(csock, SOCKET_TYPE::PEER);

						tevent.events =
							EPOLLIN | EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLRDHUP; // HANDLE WITHOUT EPOLLONESHOT
						if (epoll_ctl(epollFd, EPOLL_CTL_ADD, csock, &tevent) == -1) {
							printf("Epoll add error");
							perror("Socket accept epoll_ctl error");

							delete ((SocketData *) tevent.data.ptr);
							close(csock);
							continue;
						}
					}
				}
			} else { // RESPOND
				if (cevent->events & EPOLLIN) {
					ssize_t requestLen = 0;
					ssize_t bytesRead = 0;

					// Todo: Offset and grow buffer in case of larger requests
					// Todo: Mark request state as reading socket
					while ((bytesRead = recv(eventData->fd, eventData->request->getBuffer(), 1024, 0)) >
					       0) { // WHILE DATA READ:: POTENTIAL ERROR SIZE OF READ DATA
						requestLen += bytesRead;
					}

					if (requestLen <= 0) {
						printf("Read zero length");
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
						} else {
							printf("Closing socket");

							close(eventData->fd);
							delete (eventData);
						}
					} else {
						if (!eventData->request->Parse(requestLen)) {
							cevent->events = EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLRDHUP;
							epoll_ctl(epollFd, EPOLL_CTL_MOD, eventData->fd, cevent);
						} else {
							printf("Parse ERROR! Closing socket!");

							close(eventData->fd);
							delete (eventData);
						}
					}
				} else if (cevent->events & EPOLLOUT) {
					const char *rBuffer = "HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: MNS\r\nContent-Length: 12\r\nContent-Type: text\r\nConnection: Keep-Alive\r\n\r\nHello World!\0";

					send(eventData->fd, rBuffer, strlen(rBuffer), 0);

					cevent->events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP;
					epoll_ctl(epollFd, EPOLL_CTL_MOD, eventData->fd, cevent);
				}
			}
		}
	}

	close(listeningSocket);
	close(epollFd);*/
}

void MNS::Server::run() {
	uv_run(loop, UV_RUN_DEFAULT);
}

void MNS::Server::stop() {
	for(auto it=polls.begin(); it!=polls.end(); it++) {
		MNS::SocketData *data = (MNS::SocketData *)it->second->data;
		it->second->data = NULL;

		uv_poll_stop(it->second);
		free(it->second);
		close(it->first);
		delete (data);
	}

	polls.clear();
}

void MNS::Server::onSecondTimer(uv_timer_t *handle) {
	char strt[100];
	std::time_t t = std::time( NULL );
	std::strftime( strt, 100, "%a, %e %b %Y %H:%M:%S GMT\0", std::gmtime( &t ) );

	MNS::Server::currTime = strt;
}

void MNS::Server::onHttpConnection(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpConnectionHandler = callback;
}

void MNS::Server::onHttpRequest(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpRequestHandler = callback;
}

MNS::Server::~Server() {
}
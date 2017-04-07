//
// Created by helidium on 3/22/17.
//

#ifndef MNS_SOCKET_H
#define MNS_SOCKET_H

#include <fcntl.h>
#include <netinet/tcp.h>
#include <cstdio>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include "Variables.h"
#include "Request.h"
#include "Response.h"
#include "Server.h"

namespace MNS {
	// Forward declaration
	class Server;
	// Forward declaration
	class Request;
	// Forward declaration
	class Response;

	/// Type of the socket (LISTENING - Listening socket, PEER - Client socket)
	enum SOCKET_TYPE {
		LISTENING,
		PEER
	};

	/// SocketData class. Used as a glue of server - request - response
	class SocketData {
	public:
		/// Fd of the socket
		int fd;

		/// Type of the socket
		SOCKET_TYPE type;

		/// Request object of the socket
		MNS::Request *request;

		/// Response object of the socket
		MNS::Response *response;

		/// Reference to Server
		MNS::Server *server;

		/// Poll handle of the socket
		uv_poll_t *poll_h;

		/// Request Persistent<Function> placeholder
		void *nodeRequestPlaceholder;

		/// Response Persistent<Function> placeholder
		void *nodeResponsePlaceholder;

		/**
		 * Default contructor
		 * @param poll_h - Poll handle
		 * @param fd - File descriptor of the socket
		 * @param type - Type of the socket
		 * @param server - Server reference
		 */
		SocketData(uv_poll_t *poll_h, int fd, SOCKET_TYPE type, MNS::Server *server = NULL);

		/// Default destructor
		~SocketData();
	};

	/**
	 * Helper socket class
	 */
	class Socket {
	public:
		/**
		 * Create a listening socket
		 * @param port - Port number to listen on
		 * @return Fd of the listening socket. -1 On error
		 */
		static int createListening(int port);

		/**
		 * Flags socket as NON_BLOCKING
		 * @param sfd - File descriptor of socket
		 * @return 0 on sucess, -1 on error
		 */
		static int makeNonBlocking(int sfd);
	};
}
#endif //MNS_SOCKET_H

//
// Created by helidium on 3/22/17.
//

#ifndef MNS_SERVER_H
#define MNS_SERVER_H

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <uv.h>
#include <stdio.h>
#include <unordered_map>
#include <map>
#include <functional>
#include <string>

#include "Process.h"
#include "Socket.h"
#include "Request.h"
#include "Response.h"

#ifndef SOCK_NONBLOCK
	#define SOCK_NONBLOCK O_NONBLOCK
#endif

namespace MNS {
	// Forward declaration
	class SocketData;

	/**
	 * Server class
	 */
    class Server {
    public:
	    /**
	     * Connection callback
	     * @param handle - Poll handle
	     * @param status - Status of the call, 0 OK
	     * @param events - Poll events
	     */
	    static void onConnect(uv_poll_t *handle, int status, int events);

	    /**
	     * Close callback
	     * @param handle - Poll handle
	     */
	    static void onClose(uv_handle_t *handle);

	    /**
	     * Close timer callback
	     * @param handle - Poll handle
	     */
	    static void onCloseTimer(uv_handle_t *handle);

	    /**
	     * Data available callback
	     * @param handle - Poll handle
	     * @param status - Status of the call, 0 OK
	     * @param events - Poll events
	     */
	    static void onReadData(uv_poll_t *handle, int status, int events);

	    /**
	     * Pipeline data callback
	     * @param handle - Poll handle
	     * @param status - Status of the call, 0 OK
	     * @param events - Poll events
	     */
	    static void onReadDataPipelined(uv_poll_t *handle, int status, int events);
	    /**
	     * Callback for writing data on the socket
	     * @param handle - Poll handle
	     * @param status - Status of the call, 0 OK
	     * @param events - Poll events
	     */
	    static void onWriteData(uv_poll_t *handle, int status, int events);

	    /**
	     * Timer callback. Returns every second.
	     * @param handle - Timer handle
	     */
	    static void onSecondTimer(uv_timer_t *handle);

	    /// Current GMT time. Formatted for responses
	    static std::string currTime;

	    /// Prepopulated defaut response messages
	    static std::map<int, std::string> response_msgs;

	    /// Default constructor
        Server();

	    /**
	     * Listens on specified port
	     * @param port - Port number to listen to
	     */
        void listen(int port);

	    /// Callback holder
	    std::function<void()> onHttpListeningHandler;

	    ///Registers a callback function, when a server starts listening
	    void onHttpListening(const std::function<void()> &);

	    /// Callback holder
	    std::function<void(MNS::SocketData *)> onHttpConnectionHandler;

	    ///Registers a callback function, when a connection is received
	    void onHttpConnection(const std::function<void(MNS::SocketData *)> &);

	    /// Callback holder
	    std::function<void(MNS::SocketData *)> onHttpRequestHandler;

	    ///Registers a callback function, when a request is received
	    void onHttpRequest(const std::function<void(MNS::SocketData *)> &);

	    /// Callback holder
	    std::function<void(MNS::SocketData *)> onHttpCancelHandler;

	    ///Registers a callback function, when a request/response is canceled
	    void onHttpCancel(const std::function<void(MNS::SocketData *)> &);

	    /// Runs the poll
	    void run();

	    /// Stops the poll and releases all the handles
	    void stop();

	    /// Default destructor
        ~Server();
    private:
	    /// Fd of the listening socket
        int listeningSocket;

	    /// Handle for second timer
	    uv_timer_t *timer_h;
    protected:
    };
}


#endif //MNS_SERVER_H

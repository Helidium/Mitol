//
// Created by helidium on 3/24/17.
//

#ifndef MNS_RESPONSE_H
#define MNS_RESPONSE_H

#include <set>
#include <map>
#include <string>
#include <uv.h>
#include <cstring>
#include <stddef.h>
#include "Variables.h"
#include "Request.h"
#include "Socket.h"

#ifdef __APPLE__   /* Fails for Macs, need to define mempcpy
                      explicitly */
/* From Gnulib */
void *mempcpy(void *dest, const void *src, size_t n) {
  return (char *) memcpy(dest, src, n) + n;
}

#endif

namespace MNS {
	// Forward declaration
	class SocketData;

	/**
	 * Response class. Implements the methods for response manipulation.
	 */
	class Response {
	public:
		/**
		 * Default constructor
		 * @param socketData - SocketData needed for the response object
		 */
		Response(const SocketData *socketData);

		/**
		 * Default destructor
		 */
		~Response();

		/**
		 * Clears the response to be reused in case of persistent connections
		 * @return 0 on success, non zero on error
		 */
		int clear();

		/**
		 * Returns the response buffer that is to be sent by the server
		 * @return Buffer
		 */
		const char *getBuffer();

		/**
		 * Returns the length of the repsonse buffer
		 * @return
		 */
		unsigned int getBufferLen();

		/**
		 * !NOT IMPLEMENTED
		 * Add the trailers to the response
		 * @param headers
		 * @return
		 */
		int addTrailers(void *headers);

		/**
		 * Ends the response
		 * @param data - Buffer to finally send. Can be NULL.
		 * @param dataLen - The length of the buffer to send
		 * @return 0 on success. Non zero on error.
		 */
		int end(const char *data, unsigned int dataLen);

		/**
		 * Is the response finished
		 */
		bool finished;

		/**
		 * Returns the value of the header
		 * @param name - Name of the header to be returned
		 * @return Header value; NULL if not found
		 */
		const char *getHeader(const char *name);

		/**
		 * Returns all the header names used in the response
		 * @return std::set holding header names
		 */
		std::set<const char *> getHeaderNames();

		/**
		 * Returns all the headers in the response
		 * @return std::map holding all the headers
		 */
		std::map<std::string, std::string> getHeaders();

		/**
		 * Checks if the response has the requested header
		 * @param name - Name of the header to check
		 * @return TRUE if header existing, FALSE otherwise
		 */
		bool hasHeader(const char *name);

		/**
		 * Were headers sent
		 */
		bool headersSent;

		/**
		 * Removes the header from the map
		 * @param name - Name of the header to remove
		 * @return 0 on success, non zero on error
		 */
		int removeHeader(const char *name);

		/**
		 * Send the date header with the response
		 */
		bool sendDate;

		/**
		 * Adds or changes the header to be sent with the response
		 * @param name - Name of the header
		 * @param value - Value of the header
		 * @return 0 on success, non zero on error
		 */
		int setHeader(const std::string &name, const std::string &value);

		/**
		 * Sets the timeout for the response object
		 * @param msecs - Duration before the timeout kicks in
		 * @param callback - Callback to call in case of timeout
		 * @return 0 on success, non zero on error
		 */
		int setTimeout(int msecs, void *callback);

		/**
		 * Status code of the response<br>
		 * 200 - OK<br>
		 * 404 - Not Found<br>
		 * 500 - Internal Server Error
		 */
		int statusCode;

		/**
		 * Custom status message
		 */
		char *statusMessage;

		/**
		 * Write the data to the response buffer
		 * @param data - Data to write to responseBuffer
		 * @param dataLen - Length of the data to write
		 * @return 0 on success, non zero on error
		 */
		int write(const char *data, unsigned int dataLen);

		/**
		 * Return the CONTINUE 101 response for body data
		 * @return 0 on success, non zero on error
		 */
		int writeContinue();

		/**
		 * Method for writing the head of the response
		 * @param statusCode - Status code of the response
		 * @param statusMessage - Status message of the response
		 * @param headers - Headers for the response
		 * @return 0 on success, non zero on error
		 */
		int writeHead(int statusCode, const char *statusMessage, const std::map<const char *, const char *> &headers);

		/**
		 * Starts the response by filling the default headers
		 */
		void startResponse();

	private:
		/// Length of the internal buffer
		unsigned int bufferLen;

		/// Size of the internal buffer
		unsigned int bufferSize;

		/// Internal buffer
		char *buffer;

		/// Internal response buffer
		char *responseBuffer;

		/// Map of the headers to send with the response
		std::map<std::string, std::string> headers;

		/// SocketData
		const SocketData *socketData;
	protected:
	};
}


#endif //MNS_RESPONSE_H

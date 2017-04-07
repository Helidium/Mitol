//
// Created by helidium on 3/24/17.
//

#include "Response.h"

MNS::Response::Response(const MNS::SocketData *socketData) {
	this->finished = false;
	this->socketData = socketData;

	this->statusCode = 200;

	this->bufferLen = 0;
	this->buffer = (char *)malloc(1024);
	this->responseBuffer = (char *)malloc(1024);
	this->bufferSize = 1024;
	//this->response.reserve(1024);
}

int MNS::Response::clear() {
	this->finished = false;
	//this->response.clear();

	this->bufferLen = 0;
	if(this->bufferSize != 1024) {
		this->buffer = (char *)realloc(this->buffer, 1024);
		this->responseBuffer = (char *)realloc(this->responseBuffer, 1024);
		this->bufferSize = 1024;
	}

	return 0;
}

void MNS::Response::startResponse() {
	MNS::Request *request = this->socketData->request;
	if(request->httpVersion == HTTP_VERSION::HTTP_1_0 && request->headers["connection"] == "close") {
		this->headers = {
			{std::string("Server", 6),        std::string("Mitol", 5)},
			{std::string("Content-Type", 12), std::string("text/html", 9)},
			{std::string("Connection", 10),   std::string("close", 5)}
		};
	} else {
		this->headers = {
			{std::string("Server", 6),        std::string("Mitol", 5)},
			{std::string("Content-Type", 12), std::string("text/html", 9)},
			{std::string("Connection", 10),   std::string("keep-alive", 10)}
		};
	}
}

const char* MNS::Response::getBuffer() {
	return this->responseBuffer;
	//return this->response.c_str();
}

unsigned int MNS::Response::getBufferLen() {
	return this->bufferLen;
	//return this->response.length();
}

int MNS::Response::addTrailers(void *headers) {return 0;}

int MNS::Response::write(const char *data, unsigned int dataLen) {
	if(data) {
		//unsigned int dataLen = strlen(data);
		if(bufferSize - bufferLen < dataLen) {
			this->buffer = (char *) realloc(this->buffer, (dataLen > 1024) ? bufferSize + dataLen : bufferSize + 1024);
		}

		memcpy(this->buffer + this->bufferLen, data, dataLen);
		bufferLen += dataLen;

		return 0;
	}

	return -1;
}

int MNS::Response::end(const char *data, unsigned int dataLen) {
//	int offset = std::sprintf(this->buffer, "HTTP/1.1 200 OK\r\nContent-Length: %u\r\n\r\n", (unsigned int) dataLen);
//	memcpy(this->buffer+offset, data, dataLen);
//
//	this->bufferLen = offset + dataLen;
//
//	uv_poll_start(this->socketData->poll_h, UV_WRITABLE, MNS::Server::onWriteData);
//	return 0;

	int offset = 0;
	this->setHeader(std::string("Content-Length", 14), std::to_string(this->bufferLen + dataLen));

	MNS::Request *request = this->socketData->request;
	char *rb = this->responseBuffer;
	// First print the status code
	rb = (char *)mempcpy(rb, (request->httpVersion==HTTP_VERSION::HTTP_1_1)?"HTTP/1.1 ":"HTTP/1.0 ", 9);
	// Copy the response MSG
	std::string msg = MNS::Server::response_msgs[this->statusCode];
	rb = (char *)mempcpy(rb, msg.c_str(), msg.length());
	offset += msg.length() + 9;

	// Copy the headers
	for(std::map<std::string, std::string>::const_iterator header = this->headers.begin(); header != this->headers.end(); header++) {
		rb = (char *)mempcpy(rb, header->first.c_str(), header->first.length());
		rb = (char *)mempcpy(rb, ": ", 2);
		rb = (char *)mempcpy(rb, header->second.c_str(), header->second.length());
		rb = (char *)mempcpy(rb, "\r\n", 2);
		offset += header->first.length() + header->second.length() + 4;
	}

	// Copy trailing line end
	rb = (char *)mempcpy(rb, "\r\n", 2);
	offset += 2;

	// Finally copy the buffer
	if(this->bufferLen) {
		rb = (char *)mempcpy(rb, this->buffer, this->bufferLen);
		offset += this->bufferLen;
	}

	// Copy the sent data
	if(data) {
		rb = (char *)mempcpy(rb, data, dataLen);
		this->bufferLen += dataLen;
	}

	this->bufferLen += offset;

	uv_poll_start(this->socketData->poll_h, UV_WRITABLE, MNS::Server::onWriteData);
	this->finished = true;

	return 0;

	/*
	MNS::Request *request = this->socketData->request;

	if(data) {
		//unsigned int dataLen = strlen(data);
		if(this->bufferSize - bufferLen < dataLen) {
			this->bufferSize = (dataLen>1024)?this->bufferSize + dataLen:this->bufferSize + 1024;
			this->buffer = (char *)realloc(this->buffer, this->bufferSize);
		}

		memcpy(this->buffer + this->bufferLen, data, dataLen);
		this->bufferLen += dataLen;
	}


	this->setHeader(std::string("Content-Length", 14), std::to_string(this->bufferLen));
	this->setHeader("Date", MNS::Server::currTime);

	response.append((request->httpVersion==HTTP_VERSION::HTTP_1_1)?"HTTP/1.1 ":"HTTP/1.0 ", 9);
	response.append(MNS::Server::response_msgs[this->statusCode]);

	for(std::map<std::string, std::string>::const_iterator header = this->headers.begin(); header != this->headers.end(); header++) {
		response.append(header->first);
		response.append(": ", 2);
		response.append(header->second);
		response.append("\r\n", 2);
	}

	response.append("\r\n", 2);
	response.append(this->buffer, this->bufferLen);

//	ssize_t numSent = send(this->socketData->fd, response.c_str(), response.length(), 0);
//	if(numSent == (int)response.length()) {
//		this->bufferLen = 0;
//		response.clear();
//		//uv_poll_start(handle, UV_READABLE, onReadData);
//	}
//	uv_poll_start(this->socketData->poll_h, UV_READABLE, MNS::Server::onReadData);
	uv_poll_start(this->socketData->poll_h, UV_WRITABLE, MNS::Server::onWriteData);
	this->finished = true;

	return 0;*/
}

const char* MNS::Response::getHeader(const char *name) {
	auto headerItem = this->headers.find(name);
	if(headerItem != this->headers.end()) {
		return headerItem->second.c_str();
	}

	return NULL;
}

std::set<const char *> MNS::Response::getHeaderNames() {
	std::set<const char*> headerNames;
	for(auto headerItem = this->headers.begin(); headerItem != this->headers.end(); headerItem++) {
		headerNames.insert(headerItem->first.c_str());
	}

	return headerNames;
}

std::map<std::string, std::string> MNS::Response::getHeaders() {
	return this->headers;
}

bool MNS::Response::hasHeader(const char *name) {
	return this->headers.find(name) != this->headers.end();
}

int MNS::Response::removeHeader(const char *name) {
	this->headers.erase(name);

	return 0;
}

int MNS::Response::setHeader(const std::string &name, const std::string &value) {
	this->headers[name] = value;

	return 0;
}

int MNS::Response::setTimeout(int msecs, void *callback) {
	return 0;
}

int MNS::Response::writeContinue() {
	return 0;
}

int MNS::Response::writeHead(int statusCode, const char *statusMessage, const std::map<const char *, const char *> &headers) {
	return 0;
}

MNS::Response::~Response() {
	this->socketData = NULL;

	free(this->buffer);
	this->buffer = NULL;
	this->bufferLen = 0;
}
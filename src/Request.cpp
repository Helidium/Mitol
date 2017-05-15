//
// Created by helidium on 3/24/17.
//
// Todo: Parse Request BODY; Chunked encoding

#include "Request.h"

enum PARSER_STATE {
	METHOD,
	URL,
	HTTP_VERSION,
	HEADER_NAME,
	BODY,
	FINISHED
};

MNS::Request::Request(const MNS::SocketData *socketData) {
	this->socketData = socketData;

	this->finished = false;
	this->lastParsePos = 0;
	this->buffer = (char *) malloc(4096);
	this->bufferSize = 4096;
	this->bufferLen = 0;
	this->state = REQUEST_STATE::CONNECTING;

	this->httpVersion = MNS::HTTP_VERSION::UNKNOWN_VERSION;
	this->method = MNS::HTTP_METHOD::UNKNOWN_METHOD;
	this->socket = -1;
	this->url = NULL;
}

int MNS::Request::clear() {
	this->finished = false;
	this->lastParsePos = 0;
	this->state = REQUEST_STATE::CONNECTING;
	this->bufferLen = 0;
	if(this->bufferSize != 4096) {
		this->buffer = (char *)realloc(this->buffer, 4096); // TODO: Sanity check, realloc might run out of memory
		this->bufferSize = 4096;
	}

	this->httpVersion = MNS::HTTP_VERSION::UNKNOWN_VERSION;
	this->method = MNS::HTTP_METHOD::UNKNOWN_METHOD;
	this->url = NULL;

	return 0;
}

bool MNS::Request::isFinished() {
	return this->finished;
}

char *MNS::Request::getBuffer() {
	return this->buffer;
}

ssize_t MNS::Request::getBufferSize() {
	return this->bufferSize;
}

void MNS::Request::resizeBuffer(int ns) {
	this->buffer = (char *)realloc(this->buffer, ns);
	this->bufferSize = ns;
}

ssize_t MNS::Request::getBufferLen() {
	return this->bufferLen;
}

int MNS::Request::Parse(ssize_t requestLen) {
	if(requestLen >= 0) {
		this->finished = false;
		this->lastParsePos = 0;
		this->bufferLen = requestLen;
	}

	char *tailPos = NULL;
	this->state = REQUEST_STATE::PARSING_HEADERS;
	
	PARSER_STATE parserState = PARSER_STATE::METHOD;

	for (int i = this->lastParsePos; i < this->bufferLen; i++) {
		char c = this->buffer[i];

		switch (parserState) {
			case PARSER_STATE::METHOD:
				if (c == 'G' || c == 'g') {
					this->method = HTTP_METHOD::GET;
					i += 3;
				} else if (c == 'O' || c == 'o') {
					this->method = HTTP_METHOD::OPTIONS;
					i += 7;
				} else if (c == 'P' || c == 'p') {
					char c1 = this->buffer[i + 1];
					if (c1 == 'O' || c1 == 'o') {
						this->method = HTTP_METHOD::POST;
						i += 4;
					} else if (c1 == 'U' || c1 == 'u') {
						this->method = HTTP_METHOD::PUT;
						i += 3;
					} else {
						this->method = HTTP_METHOD::PATCH;
						i += 5;
						// Todo: This may be unsafe and offset should be found
					}
				} else if (c == 'H' || c == 'h') {
					this->method = HTTP_METHOD::HEAD;
					i += 4;
				} else if (c == 'D' || c == 'd') {
					this->method = HTTP_METHOD::DELETE;
					i += 6;
				} else if (c == 'C' || c == 'c') {
					this->method = HTTP_METHOD::CONNECT;
					i += 7;
				} else if (c == 'T' || c == 't') {
					this->method = HTTP_METHOD::TRACE;
					i += 5;
				}

				parserState = PARSER_STATE::URL;
				break;
			case PARSER_STATE::URL:
				this->url = this->buffer + i;
				tailPos = (char*)memchr(this->url, ' ', this->bufferLen - i);
				if(tailPos == NULL) { // ERROR, TRY AND GET MROE DATA
					this->state = REQUEST_STATE::NEED_MORE_DATA;

					return 1;
				}
				i = (int)(tailPos - this->buffer);
				buffer[i] = '\0';

				i += 7;
				parserState = PARSER_STATE::HTTP_VERSION;
				break;
			case PARSER_STATE::HTTP_VERSION:
				if (c == '1') this->httpVersion = HTTP_VERSION::HTTP_1_1;
				else if (c == '0') this->httpVersion = HTTP_VERSION::HTTP_1_0;
				parserState = PARSER_STATE::HEADER_NAME;
				i += 2;
				break;
			case PARSER_STATE::HEADER_NAME: {
				char *name = buffer + i;
				while (c != ':') {
					if (c >= 'A') buffer[i] |= 0x20;//-= 'A' - 'a';

					c = buffer[++i];
				}

				buffer[i] = '\0';
				i += (buffer[i + 1] == ' '?2:1);

				char *value = buffer + i;
				tailPos = (char *)memchr(buffer+i, '\r', this->bufferLen - i);
				if(tailPos == NULL) { // ERROR, TRY AND GET MORE DATA
					this->state = REQUEST_STATE::NEED_MORE_DATA;

					return 1;
				} else {
					i = (int) (tailPos - buffer);
					buffer[i++] = '\0';
				}

				//this->headers[std::string(name, nameLen)] = std::string(value, valueLen);
				this->headers[name] = value;

				// Finished parsing headers
				if (buffer[i + 1] == '\r' && buffer[i + 2] == '\n') {
					// Possibility of pipelining
					if(this->method == HTTP_METHOD::GET || this->method == HTTP_METHOD::HEAD) {
						// More pipelined requests
						if(i + 3 < this->bufferLen) {
							this->lastParsePos = i+3;

							// Return back to handler
							return 0;
						} else {
							parserState = PARSER_STATE::FINISHED;
							this->finished = true;

							return 0;
						}
					} else {
						// Is request not drained
						if(i + 3 < this->bufferLen) {
							parserState = PARSER_STATE::BODY;
						} else {
							parserState = PARSER_STATE::FINISHED;
							this->finished = true;

							return 0;
						}
					}
				}

				break;
			}
			case PARSER_STATE::BODY:
				parserState = PARSER_STATE::FINISHED;
				break;
			default:
				i = this->bufferLen;
				this->finished = true;
				break;
		}
	}

	return 0;
}

bool MNS::Request::hasHeader(const std::string &name) {
	//return false;
	return this->headers.find(name) != this->headers.end();
}

std::string MNS::Request::getHeader(const std::string &name) {
	//return "";
	return std::string(this->headers[name]);
}

MNS::Request::~Request() {
	free(this->buffer);

	this->socketData = NULL;
	this->buffer = NULL;
	this->state = REQUEST_STATE::FINISHED;
}
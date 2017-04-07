#include "Http.h"

MNS::Http::Http() {}

MNS::Server *MNS::Http::createServer() {
	if(this->server) return this->server;

	this->server = new MNS::Server();

	return this->server;
}

MNS::Http::~Http() {
	if(this->server) {
		delete(this->server);
		this->server = NULL;
	}
}
#include "src/Socket.h"
#include "src/Http.h"
#include "src/Response.h"
#include <stdio.h>

MNS::Server *server = NULL;

void onSignalReceived(int signum) {
	if (server)
		server->stop();
}

int main() {
	signal(SIGTERM, onSignalReceived);
	signal(SIGINT, onSignalReceived);
	signal(SIGKILL, onSignalReceived);
	signal(SIGALRM, onSignalReceived);

	MNS::Http *http = new MNS::Http();
	server = http->createServer();
	server->onHttpConnection([](MNS::SocketData *data) {
	});

	server->onHttpRequest([](MNS::SocketData *data) {
		MNS::Response *response = data->response;

		response->end("Hello world!", 12);
	});
	server->listen(8080);
	server->run();

	delete (http);
	return 0;
}
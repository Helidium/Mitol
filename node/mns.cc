#include "http.h"
#include <node.h>
#include <v8.h>

using namespace v8;

void Init(Handle<Object> exports, Handle<Object> module) {
	NODE_SET_METHOD(exports, "createServer", Http::createServer);
}

NODE_MODULE(mns, Init)
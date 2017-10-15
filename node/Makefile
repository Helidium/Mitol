CPP_SHARED := -std=c++11 -O3 -I ../src -shared -fPIC ../src/Http.cpp ../src/Request.cpp ../src/Response.cpp ../src/Server.cpp ../src/Socket.cpp mns.cc
CPP_DARWIN := -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup

default:
	make targets
	NODE=targets/node-v5.12.0 ABI=47 make `(uname -s)`
	NODE=targets/node-v6.10.2 ABI=48 make `(uname -s)`
	NODE=targets/node-v7.8.0 ABI=51 make `(uname -s)`
	NODE=targets/node-v8.0.0 ABI=57 make `(uname -s)`
	cp ../README.md dist/README.md
	cp ../LICENSE dist/LICENSE
	cp package.json dist/package.json
	cp index.js dist/index.js
	for f in dist/bin/*.node; do chmod +x $$f; done
targets:
	mkdir dist
	mkdir dist/bin
	mkdir targets
	curl https://nodejs.org/dist/v4.8.2/node-v4.8.2-headers.tar.gz | tar xz -C targets
	curl https://nodejs.org/dist/v5.12.0/node-v5.12.0-headers.tar.gz | tar xz -C targets
	curl https://nodejs.org/dist/v6.10.2/node-v6.10.2-headers.tar.gz | tar xz -C targets
	curl https://nodejs.org/dist/v7.8.0/node-v7.8.0-headers.tar.gz | tar xz -C targets
	curl https://nodejs.org/dist/v8.0.0/node-v8.0.0-headers.tar.gz | tar xz -C targets
Linux:
	$(CXX) $(CPP_SHARED) -static-libstdc++ -static-libgcc -I $$NODE/include/node -s -o dist/bin/mns_linux_$$ABI.node
Darwin:
	$(CXX) $(CPP_SHARED) $(CPP_DARWIN) -I $$NODE/include/node -o dist/bin/mns_darwin_$$ABI.node
.PHONY: clean
clean:
	rm -rf dist
	rm -rf targets

# ![Mitol](https://rawgit.com/Helidium/Mitol/blob/master/docs/images/mitol.svg) Mitol
Aim of the project is to offer a drop in replacement for NodeJS http server

[![Build Status](https://travis-ci.org/Helidium/Mitol.svg?branch=master)](https://travis-ci.org/Helidium/Mitol)

## Benchmarks
![Benchmark](https://github.com/Helidium/Mitol/raw/master/misc/Mitol_Bench.jpg)

## How to use
Currently only Linux has been tested. To install the project make sure you have build-tools installed.
To build the node packages do the following:
```bash
cd node
make
```
The packages will be built in the dist folder, containing compiled packages as well as index.js module file.
In your project copy the dist folder to:
```bash
node_modules/mns
```

To test the project you can use the following script 

(Single process):
```javascript
const http = require('mns');
 
let server = http.createServer((req, res) => {
    res.end('Hello World!');
});
 
server.listen(8080, function () {
    console.log('Example app listening on port 8080!')
});
```

(Multi process):
```javascript
const cluster = require('cluster');
const numCPUs = require('os').cpus().length;
const http = require('mns');
 
if (cluster.isMaster) {
    // Fork workers.
    for (let i = 0; i < numCPUs; i++) {
        cluster.fork();
    }
 
    cluster.on('exit', (worker, code, signal) => {
        console.log(`worker ${worker.process.pid} died`);
    });
} else {
    let server = http.createServer((req, res) => {
        res.end('Hello World!');
    });
 
    server.listen(8080, function () {
        console.log('Example app listening on port 8080!')
    });
}
```
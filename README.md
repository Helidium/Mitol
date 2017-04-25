<a href="https://github.com/Helidium/Mitol/raw/master/docs/images/mitol.png">
<img src="https://github.com/Helidium/Mitol/raw/master/docs/images/mitol.png" alt="Mitol" width="190">
</a>

Lightweight, high performance NodeJS Server.

[![Build Status](https://travis-ci.org/Helidium/Mitol.svg?branch=master)](https://travis-ci.org/Helidium/Mitol)
[![Coverity Status](https://img.shields.io/coverity/scan/12489.svg)](https://scan.coverity.com/projects/helidium-mitol)

## Project description
Project was born out of the need for **faster** performing server using NodeJS.
Current implementation lacks focus on **performance**, which can be achieved by moving parts of code to native C++ bindings.

**Aim of the project** is to offer an alternative solution, which is using less **memory** and **CPU** power, giving you available resources for your code or handling higher number of requests.

### Advantages
+ **No changes required**: Just replace require('http') with require('mns')
+ **Top Speed**: Roughly 3x better performance than integrated server
+ **Additional features**: Work in progress (Static file server, Router, ...)

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

## Thanks
My Family and Friends for supporting me!<br/>
All the fans for believing in the project!<br/>
[Leandro A. F. Pereira](https://github.com/lpereira) For his wonderful Lwan project.<br/>
[Alex Hultman](https://github.com/alexhultman) For his wonderful uWebSockets project.<br/>
[TechEmpower](https://www.techempower.com/benchmarks/) For Server benchmarks.<br/>
[GitHub](https://github.com) For code hosting.<br/>
[Travis](https://travis-ci.org) For code testing.<br/>
[Coverity](https://scan.coverity.com) For code defects check<br/>

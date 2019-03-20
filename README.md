**unmaintained** It was great developing the high performance HTTP server alternative for NodeJS, but I don't have the time to continue development of the project. If anyone wants to continue development, feel free to contact me...

<a href="https://github.com/Helidium/Mitol/raw/master/docs/images/mitol.png">
<img src="https://github.com/Helidium/Mitol/raw/master/docs/images/mitol.png" alt="Mitol" width="190">
</a>

Lightweight, high performance NodeJS Server.

[![Build Status](https://travis-ci.org/Helidium/Mitol.svg?branch=master)](https://travis-ci.org/Helidium/Mitol)
[![Coverity Status](https://img.shields.io/coverity/scan/12489.svg)](https://scan.coverity.com/projects/helidium-mitol)

***

## Project description
Project was born out of the need for **faster** performing server using NodeJS.
Current implementation lacks focus on **performance**, which can be achieved by moving parts of code to native C++ bindings.

**Aim of the project** is to offer an alternative solution, which is using less **memory** and **CPU** power, giving you available resources for your code or handling higher number of requests.

***

### Advantages
+ **No changes required**: Just replace require('http') with require('mitol')
+ **Top Speed**: Roughly 3x better performance than integrated server
+ **Additional features**: Work in progress (Static file server, Router, ...)

***

## Benchmarks
![Benchmark](https://github.com/Helidium/Mitol/raw/master/misc/Mitol_Bench.jpg)

***

## How to use
Currently only Linux has been tested. To install the project make sure you have node-gyp, python and build-tools (GCC, etc.) installed.
To build the node package do the following:
```bash
npm i mitol
```

To test the project you can use the following script 

(Single process):
```javascript
const http = require('mitol');
 
let server = http.createServer((req, res) => {
    res.end('Hello World!');
});
 
server.listen(8080, () => {
    console.log('Example app listening on port 8080!')
});
```

(Multi process):
```javascript
const cluster = require('cluster');
const numCPUs = require('os').cpus().length;
const http = require('mitol');
 
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
 
    server.listen(8080, () => {
        console.log('Example app listening on port 8080!')
    });
}
```

***
## Like the project?

You can support me by donating: https://www.paypal.com/paypalme/helidium
***

## Thanks
My Family and Friends for supporting me!<br/>
All the fans for believing in the project!<br/>
[Leandro A. F. Pereira](https://github.com/lpereira) For his wonderful Lwan project.<br/>
[Alex Hultman](https://github.com/alexhultman) For his wonderful uWebSockets project.<br/>
[TechEmpower](https://www.techempower.com/benchmarks/) For Server benchmarks.<br/>
[GitHub](https://github.com) For code hosting.<br/>
[Travis](https://travis-ci.org) For code testing.<br/>
[Coverity](https://scan.coverity.com) For code defects check<br/>

***
Copyright (c) 2017 Mitol Project - Released under the Zlib license.

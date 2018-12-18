# LEAR - Linux Engine for Asset Retrieval
## Description
LEAR is a simple HTTP server designed to be as simple and fast as possible to achieve one task:
serve static resources with amazing efficiency. Currently the project is in its early stage,
but is gaining momentum and features.
## Architecture
While being small and lightweight, LEAR is as concurrent and non-blocking as possible.
It also uses state-of-the-art technologies and solutions to achieve its task: serve your assets
rapidly. It features:
* Multiprocess architecture of traffic handler, using Linux's >3.9 SO_REUSEPORT
* Epoll queue for each worker
* Fully non-blocking architecture of network IO with dynamically allocated read and write buffers
* Lots of customization through user-friendly YAML properties file
## Features
* LEAR accomplishes its task by implementing GET, HEAD, and OPTIONS methods of HTTP/1.1 
* Its non-blocking nature and concurrence-by-design makes responses incredibly fast and processing very efficient
* Server implements the most common response status codes and offers response body customization (eg. custom 404 error pages for error verbosity and SPA routers)
* Server parses headers properly and returns Content-Length with any request
* Requested resources are stored as forever-lasting cache in fast, mmap, shared memory.
So, LEAR processes sharing same kernel will have fast access to these resources without unneeded memory reallocations
* Custom string format and lack of standard C null-terminated string makes server safer and prone to memory retrieval attacks

## TODO
If you like this project, feel free to contribute, fork and send PRs! Current, non-finished
list of feature requests is available below. Remember to keep straight KISS rule - LEAR is
never going to be RFC-complete as it's designed just to serve GET responses as fast
as possible.

- [ ] Finish OPTIONS implementation
- [ ] Add TLS (OpenSSL, possible to disable at compile-time)
- [ ] Implement gzip response packaging
- [ ] Support HTTP/2.0 requests
- [ ] Implement thread pools for long tasks (see nginx ones)
- [ ] Make it able to munmap cache unused for long time
- [ ] Finish socket dropping implementation
- [ ] Document getting started
- [ ] Add automated tests
- [ ] Setup CI (e.g. TravisCI)

## License
LEAR is distributed for free as a source code, under permissive MIT license

## Benchmark
As development is in early stage, this benchmark is just a performance profiling tool for me, and maybe significant information for people who like this project. It will be updated recently when any performance-related changes happen. Logging is currently disabled in LEAR while beanchmarking. Also, please, do not believe these benchmarks. It's just the result of some code run on my laptop, running default, non-tuned nginx, you know. If you'll have any results to share, please, PR to this readme!

Concurrent performance for small files (4 threads)

ab -k -c 100 -n 100000\
Nginx 1.15.5: Requests per second: 62269.41\
LEAR master: Requests per second: 80211.05

Single-threaded performance for small files

ab -k -c 1 -n 100000\
Nginx 1.15.5: Requests per second: 20922.25\
LEAR master: Requests per second: 22499.05

Concurrent performance for large files (~2MB) (4 threads)

ab -k -c 100 -n 10000\
Nginx 1.15.5: Requests per second: 2033.09\
LEAR master: Requests per second: 809.56

Single-threaded performance for large files (~2MB)

ab -k -c 1 -n 10000\
Nginx 1.15.5: Requests per second: 1602.04\
LEAR master: Requests per second: 699.79

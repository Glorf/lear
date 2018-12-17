# LEAR - Linux Engine for Asset Retrieval
## Description
LEAR is a simple http server designed to be as simple and fast as possible to achieve one task:
serve static resources with amazing efficiency. Currently the project is in it's early stage,
but gaining momentum and features.
## Architecture
While being small and lightweight, LEAR is as concurrent and non-blocking as possible.
It also uses state-of-the-art technologies and solutions to achieve it's task: serve your assets
rapidly. It features:
* Multiprocess architecture of traffic handler, using Linux's >3.9 SO_REUSEPORT
* Epoll queue for each worker
* Fully non-blocking architecture with dynamically allocated read and write buffers
* Lots of customization through user-friendly YAML properties file
## Features
* LEAR accomplish it's task by implementing GET, HEAD, and OPTIONS methods of HTTP/1.1 
* It's non-blocking nature and concurrence-by-design makes it's responses incredibly fast and
 it's processing very efficient
* Server implements most common response status codes and offers response body 
customization (eg. custom 404 error pages for error verbosity and SPA routers)
* Server parses headers properly and returns Content-Length with any request
* Requested resources are as stored forever-lasting cache in fast, mmap, shared memory.
So, LEAR processes sharing same kernel will have fast access to these resources

## TODO
If you like this project, feel free to contribute, fork and send PRs! Current, non-finished
list of feature requests is available below. Remember, we keep straight KISS rule - LEAR is
never going to be RFC-complete as it's designed just to serve GET responses as fast
as possible.

- [ ] Finish OPTIONS implementation
- [ ] Add TLS (OpenSSL, possible to disable at compile-time)
- [ ] Implement gzip response packaging
- [ ] Support HTTP/2.0 requests
- [ ] Implement thread pools for long tasks (see nginx ones)
- [ ] Make it able to munmap cache unused for long time
- [ ] Finish socket dropping implementation

## License
LEAR is distributed for free as a source code, under permissive MIT license

# LEAR - Linux Engine for Asset Retrieval

[![Build Status](https://travis-ci.org/Glorf/lear.svg?branch=master)](https://travis-ci.org/Glorf/lear)

## Description
LEAR is a simple HTTP server designed to be as simple and fast as possible to achieve one task:
serve static resources with amazing efficiency. Currently the project is in its early stage,
but is gaining momentum and features.
## Architecture
While being small and lightweight, LEAR is as concurrent and non-blocking as possible.
It also uses state-of-the-art technologies and solutions to achieve its task: serve your assets
rapidly. It features:
* Multiprocess architecture of traffic handler, using Linux >3.9's `SO_REUSEPORT`
* Epoll queue for each worker
* Fully non-blocking architecture of network IO with dynamically allocated read and write buffers
* Lots of customization through user-friendly YAML properties file
## Features
* LEAR accomplishes its task by implementing GET, HEAD, and OPTIONS methods of HTTP/1.1
* Its non-blocking nature and concurrence-by-design makes responses incredibly fast and processing very efficient
* Server implements the most common response status codes and offers response body customization (eg. custom 404 error pages for error verbosity and SPA routers)
* Server parses headers properly and returns Content-Length with any request
* Requested resources are mmapes, so LEAR has fast direct access to them
* Custom string format and lack of standard C null-terminated string makes server safe from memory retrieval attacks

## Installation
* Prerequisites: CMake, GCC, libyaml
* Installation


    ```bash
    $ git clone https://github.com/Glorf/lear.git
    $ cd lear
    $ cmake .
    $ make install
    $ cd bin
    # now modify httpd.yaml to suit your needs
    $ ./lear

## Customization
httpd.yaml offers you all the options currently available - there are no console switches.
I believe that at this moment these settings are self-explanatory. We'll do full configs
rewrite soon, config documentation is planned to appear afterwards.

## FAQ
#### Why is LEAR faster than nginx/apache...
Because LEAR is small and simple. LEAR does one task - serve static pages - and does it well.
It also implements only a small subset of the HTTP, which makes it incomplete in the sense
of being standard-compilant, but also very fast in the sense of real life use.
#### You said it's fast but actually it's slow on big files
LEAR caching support is WIP. When it's ready, we hope it'll outperform most common
HTTP engines
#### Why is X unsupported?
Because LEAR started in October 2018 - so it's quite a young project isn't it? If you like C,
please help us in development by accomplishing some task from the
[Github Issues page](https://github.com/Glorf/lear/issues). If you prefer not to
- just be patient.
#### Should I use it in my production environment?
**No.** In its current status, LEAR is extremely incomplete, even for simplest deployments.
 Its security has also not yet been checked by any means. Please, keep us in mind and return
 in few months - we're sure LEAR will be your next production server then.

## Benchmark
As development is in early stage, this benchmark is just a performance profiling tool
for me, and maybe significant information for people who like this project. It will
be updated recently when any performance-related changes happen. Logging is currently
disabled in LEAR while benchmarking. Also, please, do not believe these benchmarks.
It's just the result of some code run on my laptop, running default, non-tuned nginx,
you know. If you have any results to share, please, submit a PR to this readme!

#### ApacheBench 2.3 results

| Method | Keep-alive? | File size [B] | Number of requests | Concurrency level | LEAR master [rps] | NGINX 1.5.15 [rps] |
|--------|-------------|---------------|--------------------|-------------------|-------------------|--------------------|
| GET    | Yes         | 865           | 1000000            | 100               | **98885.12**      | 62859.55           |
| GET    | Yes         | 865           | 1000000            | 1                 | **27786.05**      | 21591.06           |
| GET    | Yes         | 2229306       | 10000              | 100               | 760.80            | **1821.60**        |
| GET    | Yes         | 2229306       | 10000              | 1                 | 781.60            | **1508.20**        |

## TODO
If you like this project, feel free to contribute, fork and send PRs! Current, non-finished
list of feature requests is available on Github Issues page. Remember to keep straight KISS rule - LEAR is
never going to be RFC-complete as it's designed just to serve GET responses as fast
as possible.

Please, open an issue if you find any bugs or consider any feature that fits the spirit of
this project.

## License
LEAR is distributed for free as a source code, under the permissive MIT license.

# Dazibao

A project by Baptiste Fontaine, David Galichet and Julien Sagot.

## Development

This project was versionned with Git and we used a light bug-tracker to track
issues during the development time. We followed a slightly modified version of
Torvald's coding style to have a consistent code in the whole project. Utility
functions are tested using a minimalist, home-made, unit tests framework (`make
tests`). Functions are documented using a Javadoc-like syntax to be parsed by
Doxygen.

## General notes

### Portability

This project aims at being compatible with any UNIX-like system, conforming the
norms C99 and POSIX. We did not use Linux specific functions, or provided an
alternative if so.

## Extensions

The following extensions were implemented:

* Additional TLV types: a few media types were added, such as MP3, MP4, PDF,
  BMP and GIF.
* Special TLV type called "long TLV" which is used to store files largers than
  the maximum size of a TLV. These long TLVs are implemented using a TLV 
  header which contains a type and a length (defined on 4 bytes), followed 
  by some "body" TLVs containing their value. We just have to "join" them to 
  get original data.
* Notifications: Our implementation is able to watch some dazibaos and notify a
  set of clients when they change.
* Web server: We implemented a command-line interface _and_ a Web server. The
  server is able to display a Dazibao, send notifications to clients, and be
  used to add or delete TLVs.

## API

The code is divided in modules. The project's core is implemented in
`mdazibao.{c,h}` and `tlv.{c,h}`. These files define an API that is then used
by all user interfaces to avoid code duplication.

### Dazibao and TLV APIs

We designed these APIs as an object-oriented structure for the purpose of being
idependant of their implementation. A programm using the Dazibao (or TLV) API
does not have to know about the structure used, and the Dazibao API does not
have to know how a TLV is actually represented.

The Dazibao API was originally implemented using `read(2)` and `write(2)`
calls, but we eventually switched to using `mmap(3)`. Our first implementation
was working, but some actions were painful to implement since `read` and
`write` calls move the cursor in the file and we needed to save and restore
this cursor every time. Moving to `mmap` resolved this issue.

## User Interfaces

### Command-Line Interface: dazicli

Command line interface provides a more powerfull tool to handle dazibaos. For 
instance, long TLVs support is only available in dazicli. We like to believe 
that dazicli is a complete tool

#### Features

- Compact a dazibao
- Add tlv from files and strings with auto-detection of TLV type
  (recording a date if wanted, using a compound if need)
- Remove TLVs using offsets. You can also remove a part of a compound only.
- Extract TLVs (write value into a file)
- dazicli is, for now, the only user interface able to handle long TLVs.

#### Limitations

Long TLVs handling is not as well implemented as regular TLVs, and it is for 
instance impossible to remove a tlv inside a long tlv. You only can remove 
the whole TLV. Nevertheless, any other command is supported.

### Web Server

We implemented a Web server to have a more user-friendly interface to a
Dazibao. It serves it as an HTML page, and allows the user to interact with it
using AJAX requests.

TLVs are represented in a reversed order to have the most recent ones at the
top of the page.

#### Features

- Compact a Dazibao, add and remove TLVs
- Get notifications when the Dazibao changes
- Static files are served from a `public_html` directory
- All user interactions are done without reloading the page, using AJAX with an
  HTTP API.

#### Limitations

- Only a subset of HTTP 1.0 is supported. We only implemented features we
  needed for this project.
- The server is synchronous and mono-threaded, because serving a Dazibao
  doesn't need a lot of requests and processing, and it's easier to work with
  this implementation.
- Notifications are really simple, because connecting the Web server to the
  notifications one would have been complicated. Also, the client need to
  regularly poll the server in order to get notifications, implementing a push
  feature would need to add a support for Web sockets, and we didn't want to
  spend too much time on such tangential features.
- Long TLVs are not supported

#### Implementation

Like the rest of the project, the Web server is modular, which makes it easy to
modify without having to change a lot of files. Want to add an HTTP status? You
only need to modify `http.{c,h}`. Want to add a route? You only need to modify
`routes.c`. All modules below are implemented with a `.c` and a `.h` files.

* `webutils`: various utilities used by other modules
* `mime`: MIME types handling
* `http`: Contains all HTTP constants and handle HTTP headers
* `request`: Utilities to parse an HTTP request and generate an `http_request`
  struct
* `html`: Utilities to generate HTML for HTTP responses
* `response`: Utilities to generate an HTTP response and send it
* `routing`: This module is used to dispatch requests to the corresponding
  routes
* `routes`: Defines all routes
* `daziweb`: Main module

When the server (`daziweb`) receives a request, it parses it (`request`) and
dispatch it to the good route (`routing`). If necessary, the route (`routes`)
generate some HTML (`html`) to fill an HTTP response (`response`). This
response is passed back to the routing module which sends it with the
appropriate headers and handle possible errors.

Some special routes are used by AJAX calls only and don't use HTML.

## Notifications

### Notification server

Until [4e5562e](4e5562e28d15ed8013407136ed62125a16d6686d), we used signals to
notify change on file. The plan was:
* one process per file
* one process per client connection
* Once a process watching a file see a modification, it sends a SIGUSR1 to all
  processes in groupid. Then:
  * Processes watching files ignore this signal
  * Processes managing clients find the file name correponding to the process
    who sent the signal (`si_pid`), and write notification on its socket.

The good side:
* Different processes means that if one fail, others still work and do not
  care.
* It is easy to broadcast a signal to a whole group id.

The bad side:
* If a signal is sent more than one time before entering handler, only one
  signal will be delivered.
* This issue could have been resolved with real-time signals, but we would
  loose the broadcasting ability of "standard" signals.

We finally moved to pthread to avoid complication in process communication, 
simplify memory sharing, and save ressources.

#### Known issues / Limitations

* As default file watching was based on `ctime` of files, it could notify
  false modification, or miss some of them. To prevent these problem, we 
  added another watching method, hashing file to be reliable. Since the 
  first method save time and ressources we kept it and could be turned on 
  with `--reliable 0` option.
* Notification server use several threads to watch file and notify clients. 
  As signal SIGPIPE is supposed to be received often (every time a client 
  disconnects), it is ignored, but it is the only signal caught. It means 
  that receiving a signal asking for termination (SIGINT is caugth to free 
  ressources before exiting), the server should crash.
  We should set a signal handler up to prevent it from happening.
* Number of client able to connect simultaneously has to be determined by user 
  when starting the server.

### Notification client

Notification client is based on `notification-stupide.c`, which has been 
slightly improved.

#### Improvement over `notification-stupide`

* UNIX compatible (`notification-stupide` does not compile on mac OS X)
* Can receive multiple notifications in one read (`notification-stupide` does 
  not loop, and does not notify all files but only once per read).
* Can use a desktop notification service (as `notify-send` on ubuntu, but 
  compatible with any application, see manual for configuration)
* Handle error message received from server (message starting with E)

#### Known issues

* Notifications longer than 1024 will not be read
* System call (notifier command length + message length + title length) to
  notify user longer than 2048 will not work properly.
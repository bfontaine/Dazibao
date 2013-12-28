# Dazibao - 2013

A project by Baptiste Fontaine, David Galichet and Julien Sagot.

TODO:
- Mention how we did the project -> git, bug tracker, coding style, ...
- Detail some choices, e.g. why do we have 2 CLIs or why is the http server
  mono-threaded, and some abandonned ideas
- Add an 'Install' section

## General notes

### Portability

This project aims at being compatible with any UNIX-like system, conforming the
norms C99 and POSIX. We did not use Linux specific functions, or provided an
alternative if so.

## Organisation

The code is divided in modules. The project's core is implemented in
`mdazibao.[ch]` and `tlv.[ch]`. These files define an API that is then used by
all user interfaces to avoid code duplication.

### Dazibao and TLV APIs

We designed these APIs as an object-oriented structure for the purpose of being
idependant of their implementation. A programm using the Dazibao (or TLV) API
does not have to know about the structure used, and the Dazibao API does not
have to know how a TLV is actually represented.

### User Interfaces

#### Command-Line Interface

TODO

#### Web Server

We implemented a Web server to have a more user-friendly interface to a
Dazibao. It serves it as an HTML page, and allows the user to interact with it
using AJAX requests.

TLVs are represented in a reversed order to have the most recent ones at the
top of the page.

##### Features

- Compact a Dazibao, add and remove TLVs
- Get notifications when the Dazibao changes
- Static files are served from a `public_html` directory
- All user interactions are done without reloading the page, using AJAX with an
  HTTP API.

##### Limitations

- The server is synchronous and mono-threaded
- Only a subset of HTTP 1.0 is supported. We only implemented features we
  needed for this project.

## Notifications

### Notification server

TODO: nobody cares about 4e556-something, we should move this in /dev/nu^W an
'History' part

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
  loose the broadcasting ability of "normal" signals.

#### Known issues
* As default file watching is based on `ctime` of files, it can notify false
  modification, or miss some of them.  This choice have been made to save
  ressources avoiding file parsing.  Not a real issue since you can enable the
  *reliable mode* with `--reliable` option.

### Notification client

#### Known issues

* Notifications longer than `BUFFER_SIZE` will not be read

* System call (notifier command length + message length + title length) to
  notify user longer than `BUFFER_SIZE * 2` will not work properly.

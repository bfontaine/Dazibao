# Dazibao - 2013

A project by Baptiste Fontaine, David Galichet and Julien Sagot.

## General notes

### Portability

This project aim at being compatible with any UNIX system, conforming the norm POSIX-???. We did not use Linux specific functions, or provided an alternative if so.

## Dazibao and TLV APIs

We designed these APIs as an object-oriented structure for the purpose of being idependant of the implementation. A programm using the Dazibao (or TLV) API does not have to know about the structure used, and the Dazibao API does not have to know how a TLV actually is represented.

NB: For now, there still is some lack in the API. For instance, we work with file descriptors only, and can not use `mmap`. We planned to add functions to handle allocated memory instead of files.

### Dazibao API

### TLV API

## Notifications

### Notification server

#### Usage
```
notification-server [FILE]
```
If you want to change the file used by the socket, use the `--path` option
```
notification-server --path /path/to/the/file/you/want/to/use
```
NB: You can put the `--path` option both before or after the file list, but the file list must not be splitted by it.

#### History

Until [4e5562e](4e5562e28d15ed8013407136ed62125a16d6686d), we used signals to notify change on file. The plan was:
* one process per file
* one process per client connection
* Once a process watching a file see a modification, it sends a SIGUSR1 to all processes in groupid. Then:
  * Processes watching files ignore this signal
  * Processes managing clients find the file name correponding to the process who sent the signal (`si_pid`), and write notification on its socket.

The good side:
* Different processes means that if one fail, others still work and do not care.
* It is easy to broadcast a signal to a whole group id.

The bad side:
* If a signal is sent more than one time before entering handler, only one signal will be delivered.
* This issue could have been resolved with real-time signals, but we would loose the broadcasting ability of "normal" signals.

#### Known issues
* As file watching is based on `ctime` of files, it can notify false modification, or miss some of them.
  This choice have been made to save ressources avoiding file parsing.

### Notification client

#### Usage
```
notification-client [OPTION]
```
Available options:
```
--path </path/to/server>
    Define the path used to connect to the server

--notifier "notifier-command -title \"%s\" -message \"%s\""
    Define the command used to notify user.
    First %s is the place where message title will be include.
    Second %s is the place where message body will be include.
```
NB: If no `--notifier` option is provided, default is `"notify-send \"%s\" \"%s\""`

#### Known issues

* Notifications longer than `BUFFER_SIZE` will not be read

* System call (notifier command length + message length + title length) to notify user longer than `BUFFER_SIZE * 2` will not work properly.

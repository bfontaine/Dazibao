# ![Dazibao Logo](https://raw.github.com/bfontaine/Dazibao/master/docs/img/dazibao_logo.png?token=1826552__eyJzY29wZSI6IlJhd0Jsb2I6YmZvbnRhaW5lL0RhemliYW8vbWFzdGVyL2RvY3MvaW1nL2RhemliYW9fbG9nby5wbmciLCJleHBpcmVzIjoxMzg3NDA1NjcyfQ%3D%3D--bedd766f31b11b064da158a2976f517639529038) Dazibao - 2013

A project by Baptiste Fontaine, David Galichet and Julien Sagot.

## General notes

### Portability

This project aim at being compatible with any UNIX system, conforming the norm POSIX-???. We did not use Linux specific functions, or provided an alternative if so.

## Dazibao and TLV APIs

We designed these APIs as an object-oriented structure for the purpose of being idependant of the implementation. A programm using the Dazibao (or TLV) API does not have to know about the structure used, and the Dazibao API does not have to know how a TLV actually is represented.

### Dazibao API

### TLV API

## Notifications

### Notification server

#### Usage
```
notification-server [OPTION] [FILE]
```
Available options:
```
--path <path>  use <path> as connection point for clients
       	       default is "$HOME/.dazibao-notification-socket"
--max <n>      allow <n> client maximum on server
      	       default is MAX_CLIENTS (from notification-server.h)
--reliable <n> if n = 0, disable the reliable mode, else, enable it
	       default is 1
--wtimemin <n> modify minimum waiting time between 2 file checks
	       default is WATCH_SLEEP_MIN
--wtimedef <n> modify default waiting time between 2 file checks
	       default is WATCH_SLEEP_DEFAULT
--wtimemax <n> modify maximum waiting time between 2 file checks
	       default is WATCH_SLEEP_MAX
```
NB: Order between options does not matter, but all options have to be written before the file list.

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
* As default file watching is based on `ctime` of files, it can notify false modification, or miss some of them.
  This choice have been made to save ressources avoiding file parsing.
  Not a real issue since you can enable the *reliable mode* with `--reliable` option.

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

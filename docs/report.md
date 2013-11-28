# report

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

#### Known issues

* As file watching is based on `ctime` of files, it can notify false modification, or miss some of them.
  This choice have been made to save ressources avoiding file parsing.

* As we use signals to tell a notification has to be sent to clients, if many files are modified and checked at the same time, some of them will actually not be notified. We do not use real-time signals because it can not be broadcast.


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
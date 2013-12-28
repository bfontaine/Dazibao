# Dazibao - User manual

## Notifications

### Notifications server

#### Usage

```
notification-server [OPTIONS] FILE
```

Available options:

* `--path <path>`:  use `<path>` as connection point for clients. default is
  `"$HOME/.dazibao-notification-socket"`
* `--max <n>`: allow max `<n>` client maximum on the server. default is 10.
* `--reliable <n>`: enable or disable the reliable mode (`0` or `1`, default is
  `1`)
* `--wtimemin <n>`: set the minimum waiting time between two file checks, in
  seconds. Default is 2.
* `--wtimedef <n>`: set the default waiting time between two file checks, in
  seconds. Default is 10.
* `--wtimemax <n>`: set the maximum waiting time between two file checks, in
  seconds. Default is 30.

### Notifications client

#### Usage

```
notification-client [OPTIONS]
```

Available options:

* `--path <path>`: define the path used to connect to the server
* `--notifier <command>` : define the command used to notify user. It must
  contain two `%s` placeholders for the message's title and body. On Ubuntu,
  the command `'notify-send "%s" "%s"'` can be used. On OS X, if
  `terminal-notifier` is installed, one can use the following command:
  `"terminal-notifier -title '%s' -message '%s'"`.

## Command line interface

### Dazicli

#### Usage

```
dazicli [COMMAND] [OPTIONS] [FILE] <dazibao path>
```

Command can be one of these:

##### `add`

```
dazicli add [OPTIONS] [FILES] <dazibao path>
```

Produce a tlv with a list of TLV and add it to a dazibao.  
If more than one TLV is provided, they will be merged into a compound TLV.

Available Options:

* `--type <type>`: force tlvs types (`text`, `jpg`, `compound`, etc). They must
  be comma-separated if there are multiple TLVs. If the number of specified
  types doesn't match the number of sources or if the option is not present,
  the program will infer their type based on their content.
* `--date`: if present, the TLV will be included in a dated one, using the
  current time.

### `compact`

```
dazicli compact <dazibao path>
```

Compact a dazibao.

#### `dump`

```
dazicli dump [OPTIONS] <dazibao path>
```

Dump a whole dazibao on the standard output.

Available Options:

* `--depth <n>`: set `<n>` as the maximum depth of the output. Default is -1 (no limit).
* `--debug`: print `PAD1`s and `PADN`s

#### `extract`

```
dazicli extract [<offset> ...] <dazibao path>
```
Extract one or more TLV in file (one file per tlv).
If no offset is provided, dazicli will extract all TLVs in the dazibao.

#### `rm`

```
dazicli rm <offset> [<offset> ...] <dazibao path>
```

Remove a list of TLVs from a Dazibao. You need to provide an offset for each
TLV you want to remove.

## Web Server

### Usage

```
daziweb [OPTIONS]
```

Launch a Web server (HTTP 1.0) which serves a dazibao at `localhost:3437`.

Available options:

* `-d <path>`: set the path to the dazibao (mandatory)
* `-p <port>`: set the port used by the server (default: 3437)
* `-v`: increase the verbosity level
* `-h`: print an usage message and quit
* `-D`: debug mode. Add PAD1s and PADNs to the output

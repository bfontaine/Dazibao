# ![Dazibao Logo](https://raw.github.com/bfontaine/Dazibao/master/docs/img/dazibao_logo.png?token=1826552__eyJzY29wZSI6IlJhd0Jsb2I6YmZvbnRhaW5lL0RhemliYW8vbWFzdGVyL2RvY3MvaW1nL2RhemliYW9fbG9nby5wbmciLCJleHBpcmVzIjoxMzg3NDA1NjcyfQ%3D%3D--bedd766f31b11b064da158a2976f517639529038) Dazibao - User manual

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

## Command line interface

### Dazicli

#### `dazicli add [OPTION] [FILE]`

Produce a tlv with a list of TLV and add it to a dazibao.  
If more than one TLV is provided, they will be merge into a compound TLV.

Options:

```
--dazibao <file> Specify path of dazibao to use.
--type <type>    Force types of tlvs (text, jpg, compound, ...), separated by comma.
--date           If present, will include the TLV in a DATED TLV (using current time)
```

NB:
* Since `--dazibao` *MUST* be provided by user, it is not really an option.
* If `--type` is provided, it **MUST** provide type for each input.
  If it is not, program will guess all the types.

#### `dazicli dump_tlv [OPTION] DAZIBAO`

Dump a tlv on stdout.

Options:
```
--offset <n> Define the offset of the wanted tlv
--value      Flag to dump the value only (default also dump header)
```

#### `dazicli dump [OPTION] DAZIBAO`

Dump a whole dazibao on stdout.

Options:
```
--depth <n> Set n as the maximum depth that will be printed.
            Default is 0.
--debug     Flag to force PAD1 and PADN printing.
```

### `dazicli rm [OPTION] DAZIBAO`

Remove a TLV from DAZIBAO.

Options:
```
--offset <n> Specify the offset of the TLV to remove.
```

NB: `--offset` is not a real option, it *MUST* be provided by user.

### `dazicli compact DAZIBAO`

Compact a dazibao.
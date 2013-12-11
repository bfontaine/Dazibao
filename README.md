# ![Dazibao Logo](https://raw.github.com/bfontaine/Dazibao/master/docs/img/dazibao_logo.png?token=1826552__eyJzY29wZSI6IlJhd0Jsb2I6YmZvbnRhaW5lL0RhemliYW8vbWFzdGVyL2RvY3MvaW1nL2RhemliYW9fbG9nby5wbmciLCJleHBpcmVzIjoxMzg3NDA1NjcyfQ%3D%3D--bedd766f31b11b064da158a2976f517639529038) Dazibao

This is a school project for the [System Programming class][jch-ens] at Paris
Diderot University.

[jch-ens]: http://www.pps.univ-paris-diderot.fr/~jch/enseignement/systeme/

## Compilation

Run `make` in root directory.

## Usage

In root directory run `./dazibao <path> <command>`

`<path>` represent the path to dazibao to use.

`<command>` is one of these:
* `add <type>`: add a tlv of type `<type>` (which is an integer between 0 and 255). Read tlv value from standard input.
* `rm <offset>`: remove tlv at offset `<offset>`.
* `dump`: dump tlv on standard output.
* `create`: create a dazibao file at `<path>`

# ![Dazibao Logo](docs/img/dazibao_log.png) Dazibao

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

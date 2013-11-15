#! /bin/bash

T=$(mktemp dzbXXX)

function _check() {
    # usage:
    #   _check <regex> <text>
    egrep -n --include=*.[ch] "$1" src/* > $T
    if [ "$?" -eq "0" ]; then
            echo "$2"
            cut -f1,2 -d: < $T
    fi
    rm -f $T
}

function _warncheck() {
    _check "$1" "!! There are $2:"
}

function _warnfncheck() {
    _check "$1\\(" "!! Use $2 instead of $1 here:"
}

_warncheck '.{80,}' '80+ chars lines'
_warncheck ' +$'    'trailing spaces'
_warncheck '//'     'C99-style comments'

_warnfncheck 'sprintf'    'snprintf'
_warnfncheck 'atoi'       'strtol'


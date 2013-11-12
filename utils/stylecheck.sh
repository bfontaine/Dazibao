#! /bin/bash

T=$(mktemp dzbXXX)

function _check() {
    # usage:
    #   _check <regex> <text>
    egrep -n --include=.*\.[ch]$ "$1" src/* > $T
    if [ "$?" -eq "0" ]; then
            echo "$2"
            cut -f1,2 -d: < $T
    fi
}

function _warncheck() {
    _check "$1" "!! There are $2:"
}

_warncheck '.{80,}' '80+ chars lines'
_warncheck ' +$'    'trailing spaces'
_warncheck '//'     'C99-style comments'

#rm -f $T;

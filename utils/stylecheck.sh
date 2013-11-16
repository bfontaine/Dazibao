#! /bin/bash

T=$(mktemp dzbXXX)

function _check() {
    # usage:
    #   _check <regex> <text>
    grep -EIn "$1" \
            src/*.c src/*.h src/web/*.c src/web/*.h > $T
    if [ "$?" -eq "0" ]; then
            echo "$2"
            cut -f1,2 -d: < $T
    fi
    rm -f $T
}

function warncheck() {
    _check "$1" "!! There are $2:"
}

function warnfncheck() {
    _check "$1\\(" "!! Use $2 instead of $1 here:"
}

warncheck '.{80,}'  '80+ chars lines'
warncheck ' +$'     'trailing spaces'
warncheck '//'      'C99-style comments'
warncheck '^ {4}\w' '4-spaces indentation'

warnfncheck 'sprintf' 'snprintf'
warnfncheck 'atoi'    'strtol'

# from http://stackoverflow.com/a/167182/735926
warnfncheck 'strcpy' 'strncpy'
warnfncheck 'strcat' 'strncat'
warnfncheck 'gets'   'fgets'

# from http://www.ibm.com/developerworks/library/s-buffer-defend.html
warnfncheck 'vsprintf' 'vsnprintf'

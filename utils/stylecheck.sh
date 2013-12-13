#! /bin/bash

T=$(mktemp dzbXXX)

function _check() {
    # usage:
    #   _check <regex> <text>
    grep -EIn "$1" \
            src/*.c src/*.h \
            src/web/*.c src/web/*.h \
            src/notifier/*.c src/notifier/*.h > $T
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
warncheck '	'       'hard tabs (instead of 8 spaces)'
warncheck ' +$'     'trailing spaces'
warncheck '//'      'C99-style comments'
warncheck '^ {4}\w' '4-spaces indentation'
warncheck '\w\* \w+[),]' '"*" adjacent to types instead of variables'
warncheck ' +,\w'   'spaces before a comma'
warncheck '\){'     'no spaces between ")" and "{"'
warncheck '}else'   'no spaces between "}" and "else"'
warncheck 'if\('    'no spaces between "if" and "("'
warncheck 'while\(' 'no spaces between "while" and "("'
warncheck 'switch\(' 'no spaces between "which" and "("'
warncheck ' --;'    'spaces before a "--" operator'

warnfncheck 'sprintf' 'snprintf'
warnfncheck 'atoi'    'strtol'

# from http://stackoverflow.com/a/167182/735926
warnfncheck 'strcpy' 'strncpy'
warnfncheck 'strcat' 'strncat'
warnfncheck 'gets'   'fgets'

# from http://www.ibm.com/developerworks/library/s-buffer-defend.html
warnfncheck 'vsprintf' 'vsnprintf'

# from https://github.com/leafsr/gcc-poison/blob/master/poison.h
warnfncheck 'wcscpy' 'wcsncpy'
warnfncheck 'stpcpy' 'stpncpy'
warnfncheck 'wcpcpy' 'wcpncpy'

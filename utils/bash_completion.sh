# Bash completion for 'dazibao' and 'daziweb'.
# Source it somewhere in your .bashrc,
# or copy it in /etc/bash_completion.d/

_daziweb() {

    local cur

    cur=${COMP_WORDS[COMP_CWORD]}

    case "$cur" in
        -*)
        COMPREPLY=( $( compgen -W '-v -l -p -d' -- $cur )) ;;

        *)
        COMPREPLY=( $( compgen -fX '!*.@(dzb|dazibao)' -o plusdirs -- $cur ) ) ;;
    esac

    return 0

}

# TODO dazibao

complete -F _daziweb -o filenames daziweb

# Bash completion for 'dazibao' and 'daziweb'.
# Source it somewhere in your .bashrc,
# or copy it in /etc/bash_completion.d/

DZ_EXTS=${DZ_EXTS:-dz|dzb|dbz|daz|dazibao|}

_daziweb() {

    local cur prev

    COMPREPLY=()
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD-1]}

    OPTS='-d -D -h -p -v'

    case "$prev" in
        -d)
            COMPREPLY=( $( compgen -fX "!*.@($DZ_EXTS)" -o plusdirs -- $cur ) ) ;;
        *)
            COMPREPLY=( $( compgen -W "$OPTS" -- $cur )) ;;
    esac

    return 0

}

_dazibao() {

    local cmd cur prev

    COMPREPLY=()
    cmd=${COMP_WORDS[1]}
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD-1]}

    COMMANDS='add compact create dump rm'

    case "$cmd" in
        # TODO add subcommands-specific options here
        add)     completions='' ;;
        compact) completions='' ;;
        create)  completions='' ;;
        dump)    completions='' ;;
        rm)      completions='' ;;
        *)
            COMPREPLY=( $( compgen -W "$COMMANDS" -- $cur ))
            return 0 ;;
    esac

    COMPREPLY=( $( compgen -W "$completions" -fX "!*.@($DZ_EXTS)" -o plusdirs -- $cur ));
    return 0
}

complete -F _daziweb -o filenames daziweb
complete -F _dazibao -o filenames dazibao

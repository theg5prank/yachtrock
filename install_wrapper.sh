#!/bin/sh

usage ()
{
    echo "usage: $0 [-m mode] [-v] {-d directory | -c file directory }" >&2
}

script=$0
invoke_install ()
{
    true
    if [ $verbose != 0 ]; then
        echo "$script: invoking:" install "$@" >&2
    fi
    install "$@"
    exit $?
}

main_sunos ()
{
    if [ $op = directory ]; then
        set -- "-d" "$1"
    elif [ $op = file ]; then
        set -- "-f" "$2" "$1"
    else
        echo "mode was somehow $mode !?" >&2;
        exit 1
    fi
    if [ ! -z "$mode" ]; then
        set -- "-m" "$mode" "$@"
    fi
    if [ `id -u` = 0 ]; then
        # install sets both group AND owner to 'bin' by default? even though everything is set
        # root:bin? guuuhhh?
        set -- "-u" "root" "$@"
    fi
    invoke_install "$@"
    exit $?
}

main_sane ()
{
    if [ $op = directory ]; then
        set -- "-d" "$1"
    elif [ $op = file ]; then
        set -- "-c" "$1" "$2"
    else
        echo "mode was somehow $mode !?" >&2;
        exit 1
    fi
    if [ ! -z "$mode" ]; then
       set -- "-m" "$mode" "$@"
    fi
    if [ $verbose != 0 ]; then
        set -- "-v" "$@"
    fi
    invoke_install "$@"
    exit $?
}

args=`getopt m:dcv $*`

if [ $? != 0 ]
then
    usage
    exit 1
fi

set -- $args

mode=""
verbose=0
op=unset
done=false

while true
do
    case "$1" in
        -m)
            mode="$2"; shift
            ;;
        -v)
            verbose=1
            ;;
        -d)
            if [ $op = unset ] ; then
                op=directory
            else
                usage
                exit 1
            fi
            ;;
        -c)
            if [ $op = unset ] ; then
                op=file
            else
                usage
                exit 1
            fi
            ;;
        --)
            done=true
            ;;
    esac
    shift
    if $done ; then
        break
    fi
done

if [ $op = unset ] ; then
    usage
    exit 1
fi

if [ $op = directory ]; then
    if [ $# != 1 ]; then
        echo "wrong number of args for -d, got $#" >&2
        usage
        exit 1
    fi
elif [ $op = file ]; then
    if [ $# !=  2 ]; then
        echo "need 2 args for -c, got $#" >&2
        usage
        exit 1
    fi
    if [ ! -d $2 ]; then
        echo "$2 is not a directory" >&2
        usage
        exit 1
    fi
fi

if uname | grep SunOS > /dev/null ; then
    main_sunos "$@"
else
    main_sane "$@"
fi

exit $?

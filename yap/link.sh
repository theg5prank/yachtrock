#!/bin/sh

uname_s=`uname -s`

usage()
{
    echo "usage: $0 -o output_filename -d driver [driveropt…]" >&2
}

invalid_usage()
{
    usage
    exit 1
}

# http://www.etalabs.net/sh_tricks.html
save ()
{
    for i; do
        printf %s\\n "$i" | sed "s/'/'\\\\''/g;1s/^/'/;\$s/\$/' \\\\/" ;
    done
    echo " "
}

quote ()
{
    printf %s\\n "$1" | sed "s/'/'\\\\''/g;1s/^/'/;\$s/\$/'/"
}

append_element_to_quoted_array()
{
    var=$1
    saved_args=$(save "$@")
    eval "set -- $2 $(quote "$3")"
    retv=$(save "$@")
    eval "$var=\${retv}"
    eval "set -- $saved_args"
    unset retv
}

add_linkage_rpath_to_array()
{
    if [ $uname_s = Darwin ]; then
        eval "$1=\$2"
    else
        append_element_to_quoted_array "$1" "$2" "-Wl,-rpath,${3}"
    fi
}

add_install_name_arguments_to_array()
{
    if [ $uname_s = Darwin ]; then
        temp="$2"
        append_element_to_quoted_array temp "$temp" "-dynamiclib"
        append_element_to_quoted_array temp "$temp" "-install_name"
        append_element_to_quoted_array "$1" "$temp" "${3}"
        unset temp
    else
        append_element_to_quoted_array "$1" "$2" "-shared"
    fi
}

add_link_prefix_to_array()
{
    if [ $uname_s = Darwin ]; then
        eval "$1=\$2"
    else
        add_linkage_rpath_to_array "$1" "$2" "$3"
    fi
}

add_stdthreads_links()
{
    if [ $uname_s = FreeBSD ]; then
        append_element_to_quoted_array "$1" "$2" "-lstdthreads"
    else
        eval "$1=\$2"
    fi
}

add_sockets_links()
{
    if [ $uname_s = SunOS ]; then
        temp="$2"
        append_element_to_quoted_array temp "$temp" -lsocket
        append_element_to_quoted_array "$1" "$temp" -lnsl
        unset temp
    else
        eval "$1=\$2"
    fi
}

add_dl_links()
{
    if [ $uname_s = Linux ]; then
        append_element_to_quoted_array "$1" "$2" "-ldl"
    else
        eval "$1=\$2"
    fi
}

add_curses_links()
{
    if [ $uname_s = Linux ]; then
        append_element_to_quoted_array "$1" "$2" "-lncursesw"
    else
        append_element_to_quoted_array "$1" "$2" "-lncurses"
    fi
}

driver=cc
dynamic_install_name=""
link_prefix=""
uses_stdthreads=false
uses_sockets=false
uses_dl=false
uses_curses=false

#parse script options.
while true; do
    case "$1" in
        --driver)
            driver="$2"; shift
            ;;
        --dynamic_install_name)
            dynamic_install_name="$2"; shift
            ;;
        --links_self_using_prefix)
            link_prefix="$2"; shift
            ;;
        --stdthreads)
            uses_stdthreads=true
            ;;
        --sockets)
            uses_sockets=true
            ;;
        --dl)
            uses_dl=true
            ;;
        --curses)
            uses_curses=true
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "$0: unknown argument $1" >&2
            invalid_usage
            ;;
    esac
    shift
done

# remaining argument list is the base of linker arglist.
linker_arglist=$(save "$@")

# process script options that add links.
if [ ${dynamic_install_name}x != x ]; then
    # linking a shared library.
    add_install_name_arguments_to_array linker_arglist "${linker_arglist}" "${dynamic_install_name}"
fi

if [ ${link_prefix}x != x ]; then
    # asked to add the prefix to the rpath and lib search paths.
    append_element_to_quoted_array linker_arglist "${linker_arglist}" "-L."
    add_link_prefix_to_array linker_arglist "${linker_arglist}" "${link_prefix}/lib"
fi

if $uses_stdthreads; then
    add_stdthreads_links linker_arglist "${linker_arglist}"
fi

if $uses_sockets; then
    add_sockets_links linker_arglist "${linker_arglist}"
fi

if $uses_dl; then
    add_dl_links linker_arglist "${linker_arglist}"
fi

if $uses_curses; then
    add_curses_links linker_arglist "${linker_arglist}"
fi

# iterate argument list looking for -L flags.
while true; do
    link=""
    case "$1" in
        -L)
            link=$2; shift
            ;;
        -L*)
            link=${1##-L}
            ;;
        *)
            ;;
    esac

    if [ ${link}x != x ] ; then
        # do a shuffle to add -Wl,-rpath,<libdir>
        add_linkage_rpath_to_array linker_arglist "${linker_arglist}" "${link}"
    fi

    shift
    if [ $# -eq 0 ] ; then
        break
    fi
done

eval "set -- $linker_arglist"

set -x
$driver "$@"

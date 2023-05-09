#!/bin/sh

cc=cc

if which gcc > /dev/null; then
    cc=gcc # default to gcc. we're using its options after all
fi

# remove all -arch <whatever> directives save the first.
#
# We should not be doing this. Instead, we should collect the dependencies for every arch
# independently and then union the results. However, the prospect of doing that in POSIX sh fills me
# with despair.
found_arch=false
nargs=$#
i=0
while [ $i -ne $nargs ]; do
    i=$((i + 1))
    save="$1"
    shift
    if [ "$save" = "-arch" ] ; then
        if $found_arch ; then
            # found -arch already. Snarf this and the next arg.
            i=$((i + 1))
            shift
        else
            # let the first -arch slide. Next iteration will handle the arch option itself.
            set -- "$@" "$save"
            found_arch=true
        fi
    else
        set -- "$@" "$save"
    fi
done

DIR="$1"
shift 1
case "$DIR" in
    "" | ".")
        $cc -MM -MG -DDEPEND_ANALYSIS "$@" | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'
        ;;
    *)
        ESCAPEDDIR=$(echo $DIR | sed -e 's/\//\\\//g' 2>&1)
        $cc -MM -MG -DDEPEND_ANALYSIS "$@" | sed -e "s/^\(.*\)\.o:/$ESCAPEDDIR\/\1.d $ESCAPEDDIR\/\1.o:/"
        ;;
esac

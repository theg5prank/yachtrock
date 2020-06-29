#!/bin/sh

cc=cc

if which gcc > /dev/null; then
    cc=gcc # default to gcc. we're using its options after all
fi

DIR="$1"
shift 1
case "$DIR" in
    "" | ".")
        $cc -MM -MG -DDEPEND_ANALYSIS "$@" | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'
        ;;
    *)
        ESCAPEDDIR=$(echo $DIR | sed -e 's/\//\\\//' 2>&1)
        $cc -MM -MG -DDEPEND_ANALYSIS "$@" | sed -e "s/^\(.*\)\.o:/$ESCAPEDDIR\/\1.d $ESCAPEDDIR\/\1.o:/"
        ;;
esac

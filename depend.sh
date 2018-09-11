#!/bin/sh
DIR="$1"
shift 1
case "$DIR" in
    "" | ".")
        gcc -MM -MG -DDEPEND_ANALYSIS "$@" | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'
        ;;
    *)
        ESCAPEDDIR=$(echo $DIR | sed -e 's/\//\\\//' 2>&1)
        gcc -MM -MG -DDEPEND_ANALYSIS "$@" | sed -e "s/^\(.*\)\.o:/$ESCAPEDDIR\/\1.d $ESCAPEDDIR\/\1.o:/"
        ;;
esac

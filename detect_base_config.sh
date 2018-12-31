#!/bin/sh

dir=`dirname $0`

command=print_platform_config

while [ $# -ne 0 ] ; do
    case "$1" in
        --clean)
            command=clean
            ;;
        *)
            echo "bad argument $1" >&2
            exit 1
            ;;
    esac
    shift
done

if [ ! -n "$YACHTROCK_UNIXY" ]; then
    unset YACHTROCK_UNIXY
fi
if [ ! -n "$YACHTROCK_POSIXY" ]; then
    unset YACHTROCK_POSIXY
fi
if [ ! -n "$YACHTROCK_DLOPEN" ]; then
    unset YACHTROCK_DLOPEN
fi
if [ ! -n "$YACHTROCK_MULTIPROCESS" ]; then
    unset YACHTROCK_MULTIPROCESS
fi

if uname | grep SunOS > /dev/null; then
    COMPILER_OVERRIDE_SNIPPET="CC=gcc"
else
    COMPILER_OVERRIDE_SNIPPET=""
fi

make -s -f - $command <<EOF
DIR=$dir/
$COMPILER_OVERRIDE_SNIPPET
print_platform_config: \$(DIR)detect_base_config
	\$(DIR)detect_base_config

clean:
	rm -f \$(DIR)detect_base_config

EOF


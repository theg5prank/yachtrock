#!/bin/sh

uname_s=`uname -s`

usage()
{
    echo "usage: $0 dylib_canonical_name" >&2
}

invalid_usage()
{
    usage
    exit 1
}

if [ $# -ne 1 ]; then
    invalid_usage
fi

if [ $uname_s = Darwin ]; then
    echo "lib${1}.dylib"
else
    echo "lib${1}.so"
fi

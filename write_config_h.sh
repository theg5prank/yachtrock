#!/bin/sh

set -ex
cat <<EOF

#ifndef YACHTROCK_CONFIG_H
#define YACHTROCK_CONFIG_H

/* This file is generated automatically when libyachtrock is built. */

#define YACHTROCK_UNIXY $YACHTROCK_UNIXY
#define YACHTROCK_POSIXY $YACHTROCK_POSIXY
#define YACHTROCK_DLOPEN $YACHTROCK_DLOPEN
#define YACHTROCK_MULTIPROCESS $YACHTROCK_MULTIPROCESS

#endif

EOF
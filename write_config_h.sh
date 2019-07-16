#!/bin/sh

set -ex
cat <<EOF

#ifndef YACHTROCK_CONFIG_H
#define YACHTROCK_CONFIG_H

/* This file is generated automatically when libyachtrock is built. */

/**
 * Whether this libyachtrock is targeting a "UNIX-y" environment.
 */
#define YACHTROCK_UNIXY $YACHTROCK_UNIXY

/**
 * Whether this libyachtrock is targeting a "POSIX-y" environment.
 */
#define YACHTROCK_POSIXY $YACHTROCK_POSIXY

/**
 * Whether this libyachtrock supports dynamic testcase discovery through the use of dlopen(3).
 */
#define YACHTROCK_DLOPEN $YACHTROCK_DLOPEN

/**
 * Whether this libyachtrock supports multi-process test execution.
 */
#define YACHTROCK_MULTIPROCESS $YACHTROCK_MULTIPROCESS

#endif

EOF

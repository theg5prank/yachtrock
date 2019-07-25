#include <yachtrock/version.h>

#include "version_internal.h"

const unsigned yr_major = YR_MAJOR;
const unsigned yr_minor = YR_MINOR;
const unsigned yr_patch = YR_PATCH;

const char *yr_version = YR_VERSION_LITERAL;
extern const char *yr_what;
const char *yr_what = "@(#) PROGRAM:libyachtrock  PROJECT:libyachtrock " YR_VERSION_LITERAL;

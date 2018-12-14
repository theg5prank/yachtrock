#ifndef YR_MULTIPROCESS_INFERIOR_H
#define YR_MULTIPROCESS_INFERIOR_H

#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include "multiprocess.h"

extern void yr_inferior_loop(yr_test_suite_collection_t collection,
                             struct yr_runtime_callbacks runtime_callbacks);

#endif // YACHTROCK_POSIXY

#endif // YR_MULTIPROCESS_INFERIOR_H

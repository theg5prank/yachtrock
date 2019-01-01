#ifndef YR_MULTIPROCESS_SUPERIOR_H
#define YR_MULTIPROCESS_SUPERIOR_H

#include <yachtrock/yachtrock.h>

#if YACHTROCK_MULTIPROCESS

#include "multiprocess.h"

extern void yr_handle_run_multiprocess(char *path, char **argv, char **environ,
                                       yr_test_suite_collection_t collection,
                                       yr_result_store_t store,
                                       struct yr_runtime_callbacks runtime_callbacks);

#endif // YACHTROCK_MULTIPROCESS

#endif // YR_MULTIPROCESS_SUPERIOR_H

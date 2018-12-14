#ifndef YR_MULTIPROCESS_SUPERIOR_H
#define YR_MULTIPROCESS_SUPERIOR_H

#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include "multiprocess.h"

extern void yr_handle_run_multiprocess(struct inferior_handle inferior,
                                       yr_test_suite_collection_t collection,
                                       yr_result_store_t store,
                                       struct yr_runtime_callbacks runtime_callbacks);

#endif // YACHTROCK_POSIXY

#endif // YR_MULTIPROCESS_SUPERIOR_H

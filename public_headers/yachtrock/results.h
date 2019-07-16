#ifndef YACHTROCK_RESULTS_H
#define YACHTROCK_RESULTS_H

#include <yachtrock/base.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * A testing result.
 *
 * YR_RESULT_UNSET: The result has not been set.
 * YR_RESULT_PASSED: A passing result.
 * YR_RESULT_FAILED: A failing result.
 * YR_RESULT_SKIPPED: The item was skipped.
 */
typedef enum yr_result {
  YR_RESULT_UNSET,
  YR_RESULT_PASSED,
  YR_RESULT_FAILED,
  YR_RESULT_SKIPPED
} yr_result_t;

/**
 * Apply merging logic to a result, updating it with new information and returning the new result.
 */
YACHTROCK_EXTERN yr_result_t yr_merge_result(yr_result_t old_result, yr_result_t new_result);

typedef struct yr_result_store yr_result_store_s;

/**
 * Result store.
 *
 * A result store is a tree structure that organizes test results.
 *
 * Each node can record some test result, but the result state they actually contain is constrained
 * by the results of their children. For instance, a store that has a passing result will implicitly
 * and irrevocably move to a failing state if one of its children records a failing result. Thus
 * test results percolate up the store tree to yield an overall result at the root.
 *
 * Each node has a state (initially YR_RESULT_UNSET), a name, and zero or more children.
 */
typedef yr_result_store_s *yr_result_store_t;

/**
 * Hooks to invoke when interesting things happen to a result store hierarchy.
 *
 * context: an arbitrary pointer passed through to the hook functions.
 * store_opened: invoked when a store in the hierarchy is opened (including the root store).
 * store_closed: invoked when a store in the hierarchy is closed (including the root store).
 * store_result_changed: invoked when a store in the hierarchy has its result changed.
 *
 * A given hook may be set to NULL, in which case it will not be invoked.
 */
struct yr_result_hooks {
  void *context;
  void (*store_opened)(yr_result_store_t new_store, void *refcon);
  void (*store_closed)(yr_result_store_t closed_store, void *refcon);
  void (*store_result_changed)(yr_result_store_t store, void *refcon);
};

/**
 * Create a new root store with the given name and no hooks.
 */

YACHTROCK_EXTERN yr_result_store_t yr_result_store_create(const char *name);

/**
 * Create a new root store with the given name and the given hooks.
 */
YACHTROCK_EXTERN yr_result_store_t yr_result_store_create_with_hooks(const char *name,
                                                                     struct yr_result_hooks hooks);

/**
 * Destroy a root store and all of its subresults.
 *
 * The provided store *must* be a root store. You cannot remove a subresult once added.
 */
YACHTROCK_EXTERN void yr_result_store_destroy(yr_result_store_t store);


/**
 * Close a store.
 *
 * Any remaining open subresult stores are closed. If the store's result is still unset, if any
 * subresult stores have failed, the store's result is set to failed; otherwise, no subresult stores
 * have failed, and the result is set to passed.
 */
YACHTROCK_EXTERN void yr_result_store_close(yr_result_store_t store);

/**
 * Test whether a store is closed.
 */
YACHTROCK_EXTERN bool yr_result_store_is_closed(yr_result_store_t store);

/**
 * Open a subresult of a store with the given name, returning the opened subresult store.
 */
YACHTROCK_EXTERN yr_result_store_t yr_result_store_open_subresult(yr_result_store_t store, const char *name);

/**
 * Record a result.
 *
 * The effective result is computed via merge_result with the old and provided results.
 */
YACHTROCK_EXTERN void yr_result_store_record_result(yr_result_store_t store, yr_result_t result);

/**
 * Extract the result from a store.
 */
YACHTROCK_EXTERN yr_result_t yr_result_store_get_result(yr_result_store_t store);

/**
 * Extract the name from a store.
 *
 * The lifetime of the string is the same as the lifetime of the store.
 */
YACHTROCK_EXTERN const char *yr_result_store_get_name(yr_result_store_t store);

/**
 * Look up the parent store for a given store.
 *
 * Returns NULL for a root store.
 */
YACHTROCK_EXTERN yr_result_store_t yr_result_store_get_parent(yr_result_store_t store);

typedef void (*yr_result_store_enumerator_t)(yr_result_store_t subresult, void *refcon);

/**
 * Enumerate the store's children with the given callback.
 */
YACHTROCK_EXTERN void yr_result_store_enumerate(yr_result_store_t store,
                                                yr_result_store_enumerator_t enumerator, void *refcon);

/**
 * Get the number of direct subresult stores of this store.
 */
YACHTROCK_EXTERN size_t yr_result_store_count_subresults(yr_result_store_t store);

/**
 * Get the subresult of this store with the given index.
 *
 * Indices are assigned sequentially in the obvious way.
 */
YACHTROCK_EXTERN yr_result_store_t yr_result_store_get_subresult(yr_result_store_t store,
                                                                 size_t i);

/**
 * Construct a string that describes the given store hierarchy.
 *
 * This fuction writes at most buf_size bytes into buf. The result is always NUL-terminated, unless
 * buf_size is zero.
 *
 * The return value is how many bytes would be written if given infinite space, even if the function
 * was actually forced to truncate the result written into buf.
 */
YACHTROCK_EXTERN size_t yr_result_store_get_description(yr_result_store_t store, char *buf, size_t buf_size);

/**
 * Construct a string on the heap that describes the given store hierarchy.
 *
 * The result must be passed to free by the caller.
 */
YACHTROCK_EXTERN char *yr_result_store_copy_description(yr_result_store_t store);

#endif

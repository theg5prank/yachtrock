#include <yachtrock/results.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

struct yr_result_store
{
  bool open;
  yr_result_t result;
  const char *name;
  size_t subresult_count;
  size_t subresult_size;
  yr_result_store_t *subresults;
  yr_result_store_t parent;
  struct yr_result_hook_context *hook_context;
};

struct yr_result_hook_context
{
  struct yr_result_hooks hooks;
  void *refcon;
};

static inline yr_result_t merge_result(yr_result_t old, yr_result_t new)
{
  switch ( old ) {
  case YR_RESULT_UNSET:
    return new;
  case YR_RESULT_PASSED:
    if ( new == YR_RESULT_FAILED ) {
      return new;
    } else {
      return old;
    }
  case YR_RESULT_FAILED:
    return old;
  case YR_RESULT_SKIPPED:
    return new;
  }
  abort();
}

static inline void store_mut_check(yr_result_store_t store)
{
  if ( !store->open ) {
    fprintf(stderr, "mutation of closed result store requested\n");
    abort();
  }
}

static struct yr_result_hook_context *store_hook_context(yr_result_store_t store)
{
  while ( store ) {
    if ( store->hook_context ) {
      return store->hook_context;
    }
    store = store->parent;
  }
  return NULL;
}

#define CALL_HOOK(store, hook_name) do {                                \
    struct yr_result_hook_context *hook_context = store_hook_context((store)); \
    if ( hook_context &&                                                \
         hook_context->hooks.hook_name ) {                              \
      hook_context->hooks.hook_name((store),                            \
                                    hook_context->refcon);              \
    }                                                                   \
  } while (0)

static void yr_result_store_init(const char *name, yr_result_store_t store)
{
  if ( !name ) {
    name = "(unnamed)";
  }
  store->open = true;
  char *name_cpy = malloc(strlen(name) + 1);
  strcpy(name_cpy, name);
  store->name = name_cpy;
  store->result = YR_RESULT_UNSET;
  store->subresult_count = 0;
  store->subresult_size = 0;
  store->subresults = NULL;
  store->parent = NULL;
  store->hook_context = NULL;
}

yr_result_store_t yr_result_store_create_with_hooks(const char *name,
                                                    struct yr_result_hooks hooks,
                                                    void *refcon)
{
  yr_result_store_t retval = malloc(sizeof(struct yr_result_store));
  yr_result_store_init(name, retval);
  retval->hook_context = malloc(sizeof(struct yr_result_hook_context));
  retval->hook_context->hooks = hooks;
  retval->hook_context->refcon = refcon;
  if ( hooks.store_opened ) {
    hooks.store_opened(retval, refcon);
  }
  return retval;
}

yr_result_store_t yr_result_store_create(const char *name)
{
  yr_result_store_t retval = malloc(sizeof(struct yr_result_store));
  yr_result_store_init(name, retval);
  return retval;
}

void yr_result_store_destroy(yr_result_store_t store)
{
  for ( size_t i = 0; i < store->subresult_count; i++ ) {
    yr_result_store_destroy(store->subresults[i]);
  }
  free(store->subresults);
  free(/* const cast */(void *)store->name);
  free(store->hook_context);
  free(store);
}

void yr_result_store_close(yr_result_store_t store)
{
  if ( !store->open ) {
    return; // not an error since we recursively try to close things
  }
  bool any_failed = false;
  for ( size_t i = 0; i < store->subresult_count; i++ ) {
    yr_result_store_close(store->subresults[i]);
    any_failed = any_failed || store->subresults[i]->result == YR_RESULT_FAILED;
  }
  if ( any_failed ) {
    yr_result_store_record_result(store, YR_RESULT_FAILED);
  } else if ( store->result == YR_RESULT_UNSET ) {
    yr_result_store_record_result(store, YR_RESULT_PASSED);
  }
  store->open = false;
  CALL_HOOK(store, store_closed);
}

yr_result_store_t yr_result_store_open_subresult(yr_result_store_t store, const char *name)
{
  store_mut_check(store);
  if ( store->subresult_count == store->subresult_size ) {
    size_t new_size = store->subresult_size == 0 ? 10 : 1.625 * store->subresult_size;
    store->subresults = realloc(store->subresults, new_size * sizeof(yr_result_store_t));
    store->subresult_size = new_size;
  }
  yr_result_store_t subresult = yr_result_store_create(name);
  subresult->parent = store;
  store->subresults[store->subresult_count++] = subresult;
  CALL_HOOK(subresult, store_opened);
  return subresult;
}

void yr_result_store_record_result(yr_result_store_t store, yr_result_t result)
{
  store_mut_check(store);
  yr_result_t new_result = merge_result(store->result, result);
  if ( store->result != new_result ) {
    store->result = new_result;
    CALL_HOOK(store, store_result_changed);
    if ( new_result == YR_RESULT_FAILED && store->parent ) {
      yr_result_store_record_result(store->parent, new_result);
    }
  }
}

yr_result_t yr_result_store_get_result(yr_result_store_t store)
{
  return store->result;
}

const char *yr_result_store_get_name(yr_result_store_t store)
{
  return store->name;
}

yr_result_store_t yr_result_store_get_parent(yr_result_store_t store)
{
  return store->parent;
}

void yr_result_store_enumerate(yr_result_store_t store, yr_result_store_enumerator_t enumerator, void *refcon)
{
  for ( size_t i = 0; i < store->subresult_count; i++ ) {
    enumerator(store->subresults[i], refcon);
  }
}

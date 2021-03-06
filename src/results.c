#include <yachtrock/results.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "yrutil.h"

struct yr_result_store
{
  bool open;
  yr_result_t result;
  const char *name;
  size_t subresult_count;
  size_t subresult_size;
  yr_result_store_t *subresults;
  yr_result_store_t parent;
  struct yr_result_hooks *hooks;
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
    if ( new == YR_RESULT_FAILED ) {
      return new;
    } else {
      return old;
    }
  }
  YR_RUNTIME_ASSERT(false, "unhandled result case in %s", __FUNCTION__);
}

yr_result_t yr_merge_result(yr_result_t old, yr_result_t new)
{
  return merge_result(old, new);
}

static inline void store_mut_check(yr_result_store_t store)
{
  YR_RUNTIME_ASSERT(store->open, "mutation of closed result store requested");
}

static struct yr_result_hooks *store_hook_context(yr_result_store_t store)
{
  while ( store ) {
    if ( store->hooks ) {
      return store->hooks;
    }
    store = store->parent;
  }
  return NULL;
}

#define CALL_HOOK(store, hook_name) do {                                \
    struct yr_result_hooks *hook_context = store_hook_context((store)); \
    if ( hook_context &&                                                \
         hook_context->hook_name ) {                                    \
      hook_context->hook_name((store),                                  \
                              hook_context->context);                   \
    }                                                                   \
  } while (0)

static void yr_result_store_init(const char *name, yr_result_store_t store)
{
  if ( !name ) {
    name = "(unnamed)";
  }
  store->open = true;
  store->name = yr_strdup(name);
  store->result = YR_RESULT_UNSET;
  store->subresult_count = 0;
  store->subresult_size = 0;
  store->subresults = NULL;
  store->parent = NULL;
  store->hooks = NULL;
}

yr_result_store_t yr_result_store_create_with_hooks(const char *name,
                                                    struct yr_result_hooks hooks)
{
  yr_result_store_t retval = yr_malloc(sizeof(struct yr_result_store));
  yr_result_store_init(name, retval);
  retval->hooks = yr_malloc(sizeof(struct yr_result_hooks));
  *(retval->hooks) = hooks;
  if ( hooks.store_opened ) {
    hooks.store_opened(retval, hooks.context);
  }
  return retval;
}

yr_result_store_t yr_result_store_create(const char *name)
{
  yr_result_store_t retval = yr_malloc(sizeof(struct yr_result_store));
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
  free(store->hooks);
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

bool yr_result_store_is_closed(yr_result_store_t store)
{
  return !store->open;
}

yr_result_store_t yr_result_store_open_subresult(yr_result_store_t store, const char *name)
{
  store_mut_check(store);
  if ( store->subresult_count == store->subresult_size ) {
    size_t new_size = store->subresult_size == 0 ? 10 : 1.625 * store->subresult_size;
    store->subresults = yr_realloc(store->subresults, new_size * sizeof(yr_result_store_t));
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

size_t yr_result_store_count_subresults(yr_result_store_t store)
{
  return store->subresult_count;
}

yr_result_store_t yr_result_store_get_subresult(yr_result_store_t store, size_t i)
{
  YR_RUNTIME_ASSERT(store->subresult_count > i, "index %zu too large for number of subresults %zu",
                    i, store->subresult_count);
  return store->subresults[i];
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static size_t _yr_result_store_get_description_depth(yr_result_store_t store, char *buf,
                                                     size_t buf_size, bool colored,
                                                     unsigned depth);
struct _get_description_enumeration_context
{
  char *buf;
  size_t buf_size;
  unsigned depth;
  size_t amount;
  bool colored;
};

static void _get_description_child_enumerator(yr_result_store_t subresult, void *refcon)
{
  struct _get_description_enumeration_context *context = refcon;
  size_t used = _yr_result_store_get_description_depth(subresult, context->buf, context->buf_size,
                                                       context->colored, context->depth + 1);
  size_t advance = MIN(context->buf_size, used);
  context->buf += advance;
  context->buf_size -= advance;
  context->amount += used;
}

// This function returns the number of characters NOT including the terminating NUL
static size_t _yr_result_store_get_description_depth(yr_result_store_t store, char *buf,
                                                     size_t buf_size, bool colored,
                                                     unsigned depth)
{
  char _;
  if ( buf == NULL ) {
    // don't hide a bug by mistake
    YR_RUNTIME_ASSERT(buf_size == 0, "don't try to write store description into a NULL pointer");
    // don't pass NULL to snprintf
    buf = &_;
  }

  char *result_addendum = NULL;
  char *coloron = "\033[0m", *coloroff = "\033[0m";
  switch ( yr_result_store_get_result(store) ) {
  case YR_RESULT_UNSET:
    coloron = "\033[36m";
    result_addendum = " [UNSET]";
    break;
  case YR_RESULT_PASSED:
    coloron = "\033[32m";
    result_addendum = " [PASSED]";
    break;
  case YR_RESULT_FAILED:
    coloron = "\033[31m";
    result_addendum = " [FAILED]";
    break;
  case YR_RESULT_SKIPPED:
    coloron = "\033[33m";
    result_addendum = " [SKIPPED]";
    break;
  }

  if ( !colored ) {
    coloron = "";
    coloroff = "";
  }

  int num_spaces = depth * 4;
  const char *name = yr_result_store_get_name(store);
  bool newline_required = depth != 0;
  size_t amount = ((newline_required ? 1 : 0) + num_spaces + strlen(name) +
                   strlen(result_addendum) + strlen(coloron) + strlen(coloroff));

  /* newline (or empty), spaces, name, coloron, addendum, coloroff */
  int written = snprintf(buf, buf_size, "%s%*s%s%s%s%s", newline_required ? "\n" : "", num_spaces,
                         "", name, coloron, result_addendum, coloroff);
  assert(written > 0);
  assert((size_t)written == amount);
  size_t advance = MIN((size_t)written, buf_size);
  buf += advance;
  buf_size -= advance;

  struct _get_description_enumeration_context enumeration_context = {0};
  enumeration_context.buf = buf;
  enumeration_context.buf_size = buf_size;
  enumeration_context.depth = depth;
  enumeration_context.amount = 0;
  enumeration_context.colored = colored;

  yr_result_store_enumerate(store, _get_description_child_enumerator, &enumeration_context);

  return amount + enumeration_context.amount;
}

size_t yr_result_store_get_description_ansi(yr_result_store_t store, char *buf,
                                            size_t buf_size, bool colored)
{
  return _yr_result_store_get_description_depth(store, buf, buf_size, colored, 0) + 1;
}

char *yr_result_store_copy_description_ansi(yr_result_store_t store, bool colored)
{
  size_t necessary = yr_result_store_get_description_ansi(store, NULL, 0, colored);
  char *buf = yr_malloc(necessary);
  yr_result_store_get_description_ansi(store, buf, necessary, colored);
  return buf;
}


size_t yr_result_store_get_description(yr_result_store_t store, char *buf, size_t buf_size)
{
  return yr_result_store_get_description_ansi(store, buf, buf_size, false);
}

char *yr_result_store_copy_description(yr_result_store_t store)
{
  return yr_result_store_copy_description_ansi(store, false);
}


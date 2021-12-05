#ifndef YACHTROCK_SCRIPT_HELPERS_H
#define YACHTROCK_SCRIPT_HELPERS_H

#include <yachtrock/base.h>
#include <stdbool.h>

#define YACHTROCK_HAS_SCRIPT_HELPERS (YACHTROCK_UNIXY && YACHTROCK_POSIXY)

#if YACHTROCK_HAS_SCRIPT_HELPERS

/**
 * Invoke a subprocess and wait for it to complete.
 *
 * The subprocess is invoked with the given arguments, environment, and file descriptors.
 *
 * The program to be executed is taken from the first element in the argument array, which is
 * NULL-terminated and may be relative. The argument array must be provided and it's length must be
 * at least 1. The environment array is optional; if not provided, the environment of the caller is
 * copied. The given file descriptors are optional; if they are -1, the corresponding stream is
 * closed in the subprocess.
 *
 * If the subprocess was invoked successfully its exit status is written to the int pointed to by
 * stat_loc, if it is not a NULL pointer.
 *
 * If the subprocess was created and waited for successfully, the function returns true. Otherwise,
 * it returns false and sets errno.
 */
YACHTROCK_EXTERN bool yr_invoke_subprocess(const char * const * YACHTROCK_RESTRICT argv,
                                           const char * const * YACHTROCK_RESTRICT envp,
                                           int stdin_fd, int stdout_fd, int stderr_fd,
                                           int *stat_loc);

/**
 * yr_invoke_subprocess, but it automatically asserts that the invoked process exits with a code of
 * 0.
 */
YACHTROCK_EXTERN bool yr_invoke_subprocess_with_assert(const char * const * YACHTROCK_RESTRICT argv,
                                                       const char * const * YACHTROCK_RESTRICT envp,
                                                       int stdin_fd, int stdout_fd, int stderr_fd,
                                                       int *stat_loc);
#endif // YACHTROCK_HAS_SCRIPT_HELPERS

#endif // ndef YACHTROCK_SCRIPT_HELPERS_H

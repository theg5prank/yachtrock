#include <yachtrock/script_helpers.h>
#include <yachtrock/assert.h>

#if YACHTROCK_HAS_SCRIPT_HELPERS

#include <spawn.h>

#include "yrutil.h"

extern char **environ;

static int add_stream_actions(posix_spawn_file_actions_t *actions, int fd, int nominal_fd)
{
  int result = 0;
  if ( fd == -1 ) {
    result = posix_spawn_file_actions_addclose(actions, nominal_fd);
    if ( result != 0 ) {
      yr_warnc(result, "posix_spawn_file_actions_addclose failed");
    }
  } else {
    result = posix_spawn_file_actions_adddup2(actions, fd, nominal_fd);
    if ( result != 0 ) {
      yr_warnc(result, "posix_spawn_file_actions_adddup2 failed");
    }
  }
  return result;
}

bool yr_invoke_subprocess(const char * const * YACHTROCK_RESTRICT argv,
                          const char * const * YACHTROCK_RESTRICT envp,
                          int stdin_fd, int stdout_fd, int stderr_fd,
                          int *stat_loc)
{
  bool ok = false;
  int scratch_err = 0;
  YR_RUNTIME_ASSERT(argv && argv[0], "bad argv in %s!", __FUNCTION__);
  const char *prog = argv[0];
  if ( envp == NULL ) {
    envp = (const char **)environ; // we solemnly swear not to perform Shenanigans
  }

  posix_spawn_file_actions_t file_actions;
  if ( (scratch_err = posix_spawn_file_actions_init(&file_actions)) != 0 ) {
    errno = scratch_err;
    goto out_noactions;
  }

  if ( (scratch_err = add_stream_actions(&file_actions, stdin_fd, 0)) != 0 ) {
    goto out;
  }
  if ( (scratch_err = add_stream_actions(&file_actions, stdout_fd, 1)) != 0 ) {
    goto out;
  }
  if ( (scratch_err = add_stream_actions(&file_actions, stderr_fd, 2)) != 0 ) {
    goto out;
  }

  pid_t pid = 0;
  if ( (scratch_err =
        posix_spawn(&pid, prog, &file_actions, NULL, (char **)argv, (char **)envp)) != 0 ) {
    yr_warnc(scratch_err, "posix_spawn failed");
    goto out;
  }

  pid_t awaited;
  int wait_stat_loc;
  do {
    awaited = waitpid(pid, &wait_stat_loc, 0);
  } while ( awaited >= 0 && !(WIFEXITED(wait_stat_loc) || WIFSIGNALED(wait_stat_loc)) );

  if ( awaited < 0 ) {
    yr_warn("waitpid failed");
    goto out;
  }

  YR_RUNTIME_ASSERT(awaited == pid, "waitpid() on %d returned bogus pid %d?",
                    (int)pid, (int)awaited);

  if ( stat_loc ) {
    *stat_loc = wait_stat_loc;
  }

  /* All done and it feels so good! */
  ok = true;

out:
  posix_spawn_file_actions_destroy(&file_actions);

out_noactions:
  return ok;
}

YACHTROCK_EXTERN bool yr_invoke_subprocess_with_assert(const char * const * YACHTROCK_RESTRICT argv,
                                                       const char * const * YACHTROCK_RESTRICT envp,
                                                       int stdin_fd, int stdout_fd, int stderr_fd,
                                                       int *stat_loc)
{
  int my_stat_loc;
  bool invoke_ok = yr_invoke_subprocess(argv, envp, stdin_fd, stdout_fd, stderr_fd, &my_stat_loc);
  YR_ASSERT(invoke_ok, "failed to start subprocess!");
  if ( invoke_ok ) {
    YR_ASSERT(WIFEXITED(my_stat_loc) && WEXITSTATUS(my_stat_loc) == 0,
              "subprocess exited uncleanly!");
  }
  if ( invoke_ok && stat_loc ) {
    *stat_loc = my_stat_loc;
  }
  return invoke_ok;
}

#endif // YACHTROCK_HAS_SCRIPT_HELPERS

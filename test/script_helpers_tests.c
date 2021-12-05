#include <yachtrock/script_helpers.h>

#if YACHTROCK_HAS_SCRIPT_HELPERS

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>

#include "yrtests.h"

static const char basic_invoke_script[] =
  "#!/bin/sh\n"
  "read line\n"
  "if [ \"$line\" = \"cool script\" ]; then\n"
  "  echo invoke success woo\n"
  "  exit 0\n"
  "else\n"
  "  echo invoke failure boo >&2\n"
  "  exit 12\n"
  "fi\n";
static bool write_basic_invoke_script(char *out_path_buf)
{
  char temp_file_name[] = "/tmp/tempscript.XXXX";
  int tmpfd = mkstemp(temp_file_name);
  YR_ASSERT(tmpfd >= 0);
  if ( tmpfd < 0 ) { return false; /* bail */ }
  YR_ASSERT_EQUAL(fchmod(tmpfd, 0700), 0);
  FILE *f = fdopen(tmpfd, "w");
  size_t nwritten = 0;
  while ( nwritten < sizeof(basic_invoke_script) && !ferror(f) ) {
    size_t this_write = sizeof(basic_invoke_script) - nwritten;
    nwritten += fwrite(basic_invoke_script + nwritten, 1, this_write, f);
  }
  YR_ASSERT_EQUAL(nwritten, sizeof(basic_invoke_script));
  fclose(f);
  strcpy(out_path_buf, temp_file_name);
  return nwritten == sizeof(basic_invoke_script);
}

static YR_TESTCASE(test_script_basic_invoke)
{
  char execpath[PATH_MAX];
  if ( !write_basic_invoke_script(execpath) ) {
    return;
  }
  fprintf(stderr, "gonna exec %s\n", execpath);
  int p[2];
  YR_ASSERT_EQUAL(pipe(p), 0);
  const char *data = "cool script\n";
  YR_ASSERT_EQUAL(write(p[1], data, strlen(data)), strlen(data));
  const char *script_argv[] = { execpath, NULL };
  YR_ASSERT(yr_invoke_subprocess_with_assert(script_argv, NULL, p[0], 1, 2, NULL));
  close(p[0]);
  close(p[1]);
}

static YR_TESTCASE(test_script_test_pipe)
{
  char execpath[PATH_MAX];
  if ( !write_basic_invoke_script(execpath) ) {
    return;
  }
  fprintf(stderr, "gonna exec %s\n", execpath);
  int sub_stdin_pipe[2];
  int sub_stderr_pipe[2];
  YR_ASSERT_EQUAL(pipe(sub_stdin_pipe), 0);
  YR_ASSERT_EQUAL(pipe(sub_stderr_pipe), 0);
  const char *script_argv[] = { execpath, NULL };
  int stat_loc;
  close(sub_stdin_pipe[1]);
  YR_ASSERT(yr_invoke_subprocess(script_argv, NULL, sub_stdin_pipe[0], 1, sub_stderr_pipe[1], &stat_loc));
  char read_buf[128];
  memset(read_buf, 0, sizeof(read_buf));
  char *expected_read = "invoke failure boo\n";
  size_t nread = read(sub_stderr_pipe[0], read_buf, sizeof(read_buf) - 1);
  YR_ASSERT_EQUAL(nread, strlen(expected_read), "read %zu bytes instead of %zu",
                  nread, strlen(expected_read));
  fwrite(read_buf, 1, nread, stderr);
  close(sub_stderr_pipe[0]);
  close(sub_stderr_pipe[1]);
  YR_ASSERT(WIFEXITED(stat_loc));
  YR_ASSERT_EQUAL(WEXITSTATUS(stat_loc), 12);
}

static YR_TESTCASE(test_test_fails_if_script_fails_invoke_expected_fail)
{
  char execpath[PATH_MAX];
  if ( !write_basic_invoke_script(execpath) ) {
    return;
  }
  fprintf(stderr, "gonna exec %s\n", execpath);
  const char *script_argv[] = { execpath, NULL };
  yr_invoke_subprocess_with_assert(script_argv, NULL, -1, 1, 2, NULL);
}

static YR_TESTCASE(test_test_fails_if_script_cant_spawn_invoke_expected_fail)
{
  char execpath[PATH_MAX];
  if ( !write_basic_invoke_script(execpath) ) {
    return;
  }
  const char *script_argv[] = { "/tmp/nonexistent", NULL };
  yr_invoke_subprocess_with_assert(script_argv, NULL, -1, 1, 2, NULL);
}

static YR_TESTCASE(test_test_fails_if_script_fails)
{
  // Script is going to die a flaming death without any stdin at all, but it should still fail.
  yr_test_suite_t suite =
    yr_create_suite_from_functions("testing test fails if script fails",
                                   NULL, YR_NO_CALLBACKS,
                                   test_test_fails_if_script_fails_invoke_expected_fail);
  bool ok = yr_basic_run_suite(suite);
  free(suite);
  YR_ASSERT_FALSE(ok);

  suite =
    yr_create_suite_from_functions("testing test fails if script can't spawn",
                                   NULL, YR_NO_CALLBACKS,
                                   test_test_fails_if_script_cant_spawn_invoke_expected_fail);
  ok = yr_basic_run_suite(suite);
  free(suite);
  YR_ASSERT_FALSE(ok);
}

yr_test_suite_t yr_create_script_helpers_suite(void)
{
  return yr_create_suite_from_functions("script helpers tests", NULL, YR_NO_CALLBACKS,
                                        test_script_basic_invoke, test_script_test_pipe,
                                        test_test_fails_if_script_fails);
}

#endif // YACHTROCK_HAS_SCRIPT_HELPERS

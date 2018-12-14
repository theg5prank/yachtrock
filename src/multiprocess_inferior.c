#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <err.h>
#include <stdint.h>
#include <sysexits.h>

#include "multiprocess.h"

static bool inferior_handle_command(int sock, uint32_t command,
                                    yr_test_suite_collection_t collection,
                                    struct yr_runtime_callbacks runtime_callbacks)
{
  switch ( command ) {
  case MESSAGE_TERMINATE:
    warnx("inferior terminating cleanly per superior instruction");
    exit(0);
  default:
    warnx("unknown command %ld", (long)command);
    return false;
  }
}

void yr_inferior_loop(yr_test_suite_collection_t collection,
                      struct yr_runtime_callbacks runtime_callbacks)
{
  int sock = yr_inferior_socket();
  while ( 1 ) {
    uint32_t command;
    bool ok = yr_recv_uint32(sock, &command, NULL);
    if ( !ok ) {
      errx(EX_SOFTWARE, "inferior exiting due to error");
    }
    ok = inferior_handle_command(sock, command, collection, runtime_callbacks);
    if ( !ok ) {
      errx(EX_SOFTWARE, "failed to handle command");
    }
  }
}

#endif // YACHTROCK_POSIXY

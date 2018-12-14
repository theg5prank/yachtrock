#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <err.h>
#include <stdint.h>
#include <sysexits.h>

#include "multiprocess.h"

static bool inferior_handle_message(int sock, struct yr_message *command_message,
                                    yr_test_suite_collection_t collection,
                                    struct yr_runtime_callbacks runtime_callbacks)
{
  switch ( command_message->message_code ) {
  case MESSAGE_TERMINATE:
    warnx("inferior terminating cleanly per superior instruction");
    exit(0);
  default:
    warnx("unknown command %ld", (long)command_message->message_code);
    return false;
  }
}

void yr_inferior_loop(yr_test_suite_collection_t collection,
                      struct yr_runtime_callbacks runtime_callbacks)
{
  int sock = yr_inferior_socket();
  while ( 1 ) {
    struct yr_message *command_message = NULL;
    bool ok = yr_recv_message(sock, &command_message, NULL);
    if ( !ok ) {
      errx(EX_SOFTWARE, "inferior exiting due to error");
    }
    ok = inferior_handle_message(sock, command_message, collection, runtime_callbacks);
    if ( !ok ) {
      errx(EX_SOFTWARE, "failed to handle command");
    }
    free(command_message);
  }
}

#endif // YACHTROCK_POSIXY

#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <err.h>
#include <stdint.h>
#include <sysexits.h>
#include <assert.h>

#include "multiprocess.h"

static bool send_collection_desc_message(int sock, yr_test_suite_collection_t collection)
{
  size_t collection_desc_size = yr_multiprocess_collection_desc(NULL, 0, collection);
  struct yr_message *message = malloc(sizeof(struct yr_message) + collection_desc_size);
  size_t filled_out_size = yr_multiprocess_collection_desc(message->payload,
                                                           collection_desc_size,
                                                           collection);
  assert(filled_out_size == collection_desc_size);
  message->message_code = MESSAGE_PROVIDE_COLLECTION_DESC;
  message->payload_length = collection_desc_size;
  bool ok = yr_send_message(sock, message, NULL);
  free(message);
  return ok;
}

static bool handle_invoke_case(int sock, struct yr_message *invoke_case_message,
                               yr_test_suite_collection_t collection,
                               struct yr_runtime_callbacks runtime_callbacks)
{
  size_t suiteid = 0, caseid = 0;
  if ( !yr_extract_ids_from_invoke_case_message(invoke_case_message, &suiteid, &caseid) ) {
    errx(EX_SOFTWARE, "bogus message payload length from superior");
  }

  size_t payload_len = yr_case_finished_payload(NULL, 0, suiteid, caseid);
  char payload_buf[payload_len];
  yr_case_finished_payload(payload_buf, payload_len, suiteid, caseid);
  struct yr_message *case_finished_message = yr_message_create_with_payload(MESSAGE_CASE_FINISHED,
                                                                            payload_buf,
                                                                            payload_len);
  bool responded_ok = yr_send_message(sock, case_finished_message, NULL);

  return responded_ok;
}

static bool inferior_handle_message(int sock, struct yr_message *command_message,
                                    yr_test_suite_collection_t collection,
                                    struct yr_runtime_callbacks runtime_callbacks)
{
  switch ( command_message->message_code ) {
  case MESSAGE_REQUEST_COLLECTION_DESC:
    return send_collection_desc_message(sock, collection);
    break;
  case MESSAGE_TERMINATE:
    warnx("inferior terminating cleanly per superior instruction");
    exit(0);
  case MESSAGE_INVOKE_CASE:
    return handle_invoke_case(sock, command_message, collection, runtime_callbacks);
    break;
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

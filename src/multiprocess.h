#ifndef YR_MULTIPROCESS_H
#define YR_MULTIPROCESS_H

#include <yachtrock/yachtrock.h>

#if YACHTROCK_MULTIPROCESS

#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#define DEBUG_INFERIOR_VAR "YR_DEBUG_INFERIOR"

struct inferior_handle {
  pid_t pid;
  int socket;
};

extern const struct inferior_handle YR_INFERIOR_HANDLE_NULL;
extern bool yr_inferior_handle_is_null(struct inferior_handle handle);

/*
 * Message format: msg_code[4] payload_len[4] payload[payload_len]
 */
enum yr_inferior_message {
  /* Payload format: {} */
  MESSAGE_REQUEST_COLLECTION_DESC,
  /* Payload format: num_suites[4] (suite * num_suites)
   * suite: suite_name_len[4], suite_name[suite_name_len], num_cases[4], (testcase * num_cases)
   * testcase: testcase_name_len[4], testcase_name[testcase_name_len]
   */
  MESSAGE_PROVIDE_COLLECTION_DESC,
  /* Payload format: suiteid[4], caseid[4] */
  MESSAGE_INVOKE_CASE,
  /* Payload format: suiteid[4], caseid[4], result[1] */
  MESSAGE_CASE_RESULT,
  /* Payload format: suiteid[4], caseid[4] */
  MESSAGE_CASE_FINISHED,
  /* Payload format: {} */
  MESSAGE_TERMINATE,
};

struct yr_message {
  uint32_t message_code;
  uint32_t payload_length;
  char payload[];
};

extern int yr_inferior_socket(void);

extern bool yr_spawn_inferior(char *path, char **argv, char **environ,
                              struct inferior_handle *out_result);

enum recv_length_flags {
  RECV_LENGTH_DRAIN = 1 << 0
};
extern bool yr_recv_length(int sock, void *buf, size_t len, struct timeval *timeout, int flags);
extern bool yr_recv_uint32(int sock, uint32_t out[static 1], struct timeval *timeout);
extern bool yr_send_length(int sock, const void *buf, size_t len, struct timeval *timeout);
extern bool yr_send_uint32(int sock, uint32_t in, struct timeval *timeout);
extern bool yr_send_message(int sock, struct yr_message *in, struct timeval *timeout);
extern bool yr_recv_message(int sock, struct yr_message **out, struct timeval *timeout);

extern struct yr_message *yr_message_create_with_payload(enum yr_inferior_message message,
                                                         void *payload, size_t payload_length);

extern size_t yr_multiprocess_collection_desc(char *buf, size_t buflen,
                                              yr_test_suite_collection_t collection);
extern size_t yr_invoke_case_payload(char *buf, size_t buflen, size_t suiteid, size_t caseid);
extern bool yr_extract_ids_from_invoke_case_message(struct yr_message *message, size_t *suiteid,
                                                    size_t *caseid);
extern size_t yr_case_finished_payload(char *buf, size_t buflen, size_t suiteid, size_t caseid);
bool yr_extract_ids_from_case_finished_message(struct yr_message *message, size_t *suiteid,
                                               size_t *caseid);
extern size_t yr_case_result_payload(char *buf, size_t buflen, size_t suiteid, size_t caseid,
                                     yr_result_t result);
extern bool yr_extract_info_from_case_result_message(struct yr_message *message, size_t *suiteid,
                                                     size_t *caseid, yr_result_t *result);

#endif // YACHTROCK_MULTIPROCESS

#endif

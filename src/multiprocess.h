#ifndef YR_MULTIPROCESS_H
#define YR_MULTIPROCESS_H

#include <yachtrock/yachtrock.h>

#if YACHTROCK_POSIXY

#include <time.h>
#include <stdint.h>

struct inferior_handle {
  pid_t pid;
  int socket;
};

/*
 * Message format: msg_code[4] payload_len[4] payload[payload_len]
 */
enum yr_inferior_message {
  /* Payload format: {} */
  MESSAGE_REQUEST_COLLECTION_DESC,
  /* Payload format: num_suites[4] (suite * num_suites)
   * suite: suite_name_len[4], suite_name[suite_len], num_cases[4], (testcase * num_cases)
   * testcase: testcase_name_len[4], testcase_name[testcase_name_len]
   */
  MESSAGE_PROVIDE_COLLECTION_DESC,
  /* Payload format: suiteid[4], caseid[4] */
  MESSAGE_INVOKE_CASE,
  /* Payload format: suiteid[4], caseid[4], result[1] */
  MESSAGE_CASE_RESULT,
  /* Payload format: {} */
  MESSAGE_TERMINATE,
};

struct yr_message {
  uint32_t message_code;
  uint32_t payload_length;
  char payload[];
};

extern int yr_inferior_socket(void);

enum recv_length_flags {
  RECV_LENGTH_DRAIN = 1 << 0
};
extern bool yr_recv_length(int sock, void *buf, size_t len, struct timeval *timeout, int flags);
extern bool yr_recv_uint32(int sock, uint32_t out[static 1], struct timeval *timeout);
extern bool yr_send_length(int sock, const void *buf, size_t len, struct timeval *timeout);
extern bool yr_send_uint32(int sock, uint32_t in, struct timeval *timeout);
extern bool yr_send_message(int sock, struct yr_message *in, struct timeval *timeout);
extern bool yr_recv_message(int sock, struct yr_message **out, struct timeval *timeout);

#endif // YACHTROCK_POSIXY

#endif

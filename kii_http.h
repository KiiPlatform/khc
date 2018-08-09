#ifndef __kii_http
#define __kii_http

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "kii_socket_callback.h"

typedef size_t (*WRITE_CALLBACK)(char *ptr, size_t size, size_t nmemb, void *userdata);
typedef size_t (*READ_CALLBACK)(char *buffer, size_t size, size_t nitems, void *instream);
typedef size_t (*HEADER_CALLBACK)(char *buffer, size_t size, size_t nitems, void *userdata);

typedef struct kii_slist {
  char* data;
  struct kii_slist* next;
} kii_slist;

kii_slist* kii_slist_append(kii_slist* slist, const char* string, size_t length);

void kii_slist_free_all(kii_slist* slist);

#define READ_REQ_BUFFER_SIZE 1024
#define READ_RESP_HEADER_SIZE 1024
#define READ_BODY_SIZE 1024
#define REQ_LINE_BUFFER_SIZE 256

typedef enum kii_http_state {
  IDLE,
  CONNECT,
  REQUEST_LINE,
  REQUEST_HEADER,
  REQUEST_HEADER_SEND,
  REQUEST_HEADER_SEND_CRLF,
  REQUEST_HEADER_END,
  REQUEST_BODY_READ,
  REQUEST_BODY_SEND,
  RESPONSE_HEADERS_ALLOC,
  RESPONSE_HEADERS_REALLOC,
  RESPONSE_HEADERS_READ,
  RESPONSE_HEADERS_CALLBACK,
  /** Process flagment of body obtaind when trying to find body boundary. */
  RESPONSE_BODY_FLAGMENT,
  RESPONSE_BODY_READ,
  RESPONSE_BODY_CALLBACK,
  CLOSE,
  FINISHED,
} kii_http_state;

typedef enum kii_http_code {
  // TODO: Add codes.
  KII_OK,
  KIIE_CONNECT,
  KIIE_CLOSE,
  KIIE_SEND_REQUEST_LINE,
  KIIE_SEND_REQUEST_HEADER,
  KIIE_SEND_REQUEST_BODY,
  KIIE_READ_HEADER,
  KIIE_READ_BODY,
  KIIE_HEADER_CALLBACK,
  KIIE_WRITE_CALLBACK,
  KIIE_ALLOCATION,
  KIIE_INSUFFICIENT_BUFFER,
  KII_NG
} kii_http_code;

typedef struct kii_http {
  WRITE_CALLBACK write_callback;
  void* write_data;
  READ_CALLBACK read_callback;
  void* read_data;
  HEADER_CALLBACK header_callback;
  void* header_data;

  /** Request header list */
  kii_slist* reaquest_headers;

  char* host;
  char* path;
  char* method;

  /** State machine */
  kii_http_state state;

  /** Socket functions. */
  KII_SOCKET_CONNECT_CB sc_connect_cb;
  KII_SOCKET_SEND_CB sc_send_cb;
  KII_SOCKET_RECV_CB sc_recv_cb;
  KII_SOCKET_CLOSE_CB sc_close_cb;
  /** Socket context. */
  void* socket_context;

  kii_slist* current_request_header;

  /** Request body buffer stream */
  char read_buffer[READ_REQ_BUFFER_SIZE];
  size_t read_size;
  int read_request_end;

  /** Response header buffer (Dynamic allocation) */
  char* resp_header_buffer;
  char* resp_header_buffer_current_pos;
  size_t resp_header_buffer_size;

  /** Pointer to the double CRLF boundary in the resp_header_buffer */
  char* body_boundary;

  /** Header callback */
  char* cb_header_pos;
  /** Used to seek for CRFL effectively. */
  size_t cb_header_remaining_size;

  char* body_flagment;
  size_t body_flagment_size;
  int read_end;

  char body_buffer[READ_BODY_SIZE];
  size_t body_read_size;

  kii_http_code result;
} kii_http;

kii_http_code kii_http_perform(kii_http* kii_http);

#ifdef __cplusplus
}
#endif

#endif //__kii_http
#ifndef __kii_http
#define __kii_http

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "kii_socket_callback.h"

typedef size_t (*KII_CB_WRITE)(char *ptr, size_t size, size_t count, void *userdata);
typedef size_t (*KII_CB_READ)(char *buffer, size_t size, size_t count, void *userdata);
typedef size_t (*KII_CB_HEADER)(char *buffer, size_t size, size_t count, void *userdata);

typedef struct kii_slist {
  char* data;
  struct kii_slist* next;
} kii_slist;

kii_slist* kii_slist_append(kii_slist* slist, const char* string, size_t length);

void kii_slist_free_all(kii_slist* slist);

#define READ_REQ_BUFFER_SIZE 1024
#define READ_RESP_HEADER_SIZE 1024
#define READ_BODY_SIZE 1024

typedef enum kii_http_param {
  KIIPARAM_HOST,
  KIIPARAM_PATH,
  KIIPARAM_METHOD,
  KIIPARAM_REQ_HEADERS
} kii_http_param;

typedef enum kii_http_state {
  KIIST_IDLE,
  KIIST_CONNECT,
  KIIST_REQ_LINE,
  KIIST_REQ_HEADER,
  KIIST_REQ_HEADER_SEND,
  KIIST_REQ_HEADER_SEND_CRLF,
  KIIST_REQ_HEADER_END,
  KIIST_REQ_BODY_READ,
  KIIST_REQ_BODY_SEND,
  KIIST_RESP_HEADERS_ALLOC,
  KIIST_RESP_HEADERS_REALLOC,
  KIIST_RESP_HEADERS_READ,
  KIIST_RESP_HEADERS_CALLBACK,
  /** Process flagment of body obtaind when trying to find body boundary. */
  KIIST_RESP_BODY_FLAGMENT,
  KIIST_RESP_BODY_READ,
  KIIST_RESP_BODY_CALLBACK,
  KIIST_CLOSE,
  KIIST_FINISHED,
} kii_http_state;

typedef enum kii_http_code {
  KIIE_OK,
  KIIE_SC_CONNECT,
  KIIE_SC_CLOSE,
  KIIE_SC_SEND,
  KIIE_SC_RECV,
  KIIE_HEADER_CALLBACK,
  KIIE_WRITE_CALLBACK,
  KIIE_ALLOCATION,
  KIIE_FAIL,
} kii_http_code;

typedef struct kii_http {
  KII_CB_WRITE _cb_write;
  void* _write_data;
  KII_CB_READ _cb_read;
  void* _read_data;
  KII_CB_HEADER _cb_header;
  void* _header_data;

  /** Request header list */
  kii_slist* _req_headers;

  char* _host;
  char* _path;
  char* _method;

  /** State machine */
  kii_http_state _state;

  /** Socket functions. */
  KII_CB_SOCK_CONNECT _cb_sock_connect;
  KII_CB_SOCK_SEND _cb_sock_send;
  KII_CB_SOCK_RECV _cb_sock_recv;
  KII_CB_SOCK_CLOSE _cb_sock_close;
  /**   Socket context. */
  void* _sock_ctx_connect;
  void* _sock_ctx_send;
  void* _sock_ctx_recv;
  void* _sock_ctx_close;

  kii_slist* _current_req_header;

  /** Request body buffer stream */
  char _read_buffer[READ_REQ_BUFFER_SIZE];
  size_t _read_size;
  int _read_req_end;

  /** Response header buffer (Dynamic allocation) */
  char* _resp_header_buffer;
  char* _resp_header_buffer_current_pos;
  size_t _resp_header_buffer_size;
  size_t _resp_header_read_size;

  /** Pointer to the double CRLF boundary in the resp_header_buffer */
  char* _body_boundary;

  /** Header callback */
  char* _cb_header_pos;
  /** Used to seek for CRFL effectively. */
  size_t _cb_header_remaining_size;

  char* _body_flagment;
  size_t _body_flagment_size;
  int _read_end;

  char _body_buffer[READ_BODY_SIZE];
  size_t _body_read_size;

  kii_http_code _result;
} kii_http;

kii_http_code kii_http_perform(kii_http* kii_http);

kii_http_code kii_http_set_param(kii_http* kii_http, kii_http_param param_type, void* data);

kii_http_code kii_http_set_cb_sock_connect(
  kii_http* kii_http,
  KII_CB_SOCK_CONNECT cb,
  void* userdata);

kii_http_code kii_http_set_cb_sock_send(
  kii_http* kii_http,
  KII_CB_SOCK_SEND cb,
  void* userdata);

kii_http_code kii_http_set_cb_sock_recv(
  kii_http* kii_http,
  KII_CB_SOCK_RECV cb,
  void* userdata);

kii_http_code kii_http_set_cb_sock_close(
  kii_http* kii_http,
  KII_CB_SOCK_CLOSE cb,
  void* userdata);

kii_http_code kii_http_set_cb_read(
  kii_http* kii_http,
  KII_CB_READ cb,
  void* userdata);

kii_http_code kii_http_set_cb_write(
  kii_http* kii_http,
  KII_CB_WRITE cb,
  void* userdata);

kii_http_code kii_http_set_cb_header(
  kii_http* kii_http,
  KII_CB_HEADER cb,
  void* userdata);

#ifdef __cplusplus
}
#endif

#endif //__kii_http
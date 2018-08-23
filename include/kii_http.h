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
  KII_PARAM_HOST,
  KII_PARAM_PATH,
  KII_PARAM_METHOD,
  KII_PARAM_REQ_HEADERS
} kii_http_param;

typedef enum kii_http_state {
  KII_STATE_IDLE,
  KII_STATE_CONNECT,
  KII_STATE_REQ_LINE,
  KII_STATE_REQ_HEADER,
  KII_STATE_REQ_HEADER_SEND,
  KII_STATE_REQ_HEADER_SEND_CRLF,
  KII_STATE_REQ_HEADER_END,
  KII_STATE_REQ_BODY_READ,
  KII_STATE_REQ_BODY_SEND,
  KII_STATE_RESP_HEADERS_ALLOC,
  KII_STATE_RESP_HEADERS_REALLOC,
  KII_STATE_RESP_HEADERS_READ,
  KII_STATE_RESP_HEADERS_CALLBACK,
  /** Process flagment of body obtaind when trying to find body boundary. */
  KII_STATE_RESP_BODY_FLAGMENT,
  KII_STATE_RESP_BODY_READ,
  KII_STATE_RESP_BODY_CALLBACK,
  KII_STATE_CLOSE,
  KII_STATE_FINISHED,
} kii_http_state;

typedef enum kii_http_code {
  KII_ERR_OK,
  KII_ERR_SC_CONNECT,
  KII_ERR_SC_CLOSE,
  KII_ERR_SC_SEND,
  KII_ERR_SC_RECV,
  KII_ERR_HEADER_CALLBACK,
  KII_ERR_WRITE_CALLBACK,
  KII_ERR_ALLOCATION,
  KII_ERR_FAIL,
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
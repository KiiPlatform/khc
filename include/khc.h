#ifndef __khc
#define __khc

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include "khc_socket_callback.h"

typedef size_t (*KHC_CB_WRITE)(char *ptr, size_t size, size_t count, void *userdata);
typedef size_t (*KHC_CB_READ)(char *buffer, size_t size, size_t count, void *userdata);
typedef size_t (*KHC_CB_HEADER)(char *buffer, size_t size, size_t count, void *userdata);

typedef struct khc_slist {
  char* data;
  struct khc_slist* next;
} khc_slist;

khc_slist* khc_slist_append(khc_slist* slist, const char* string, size_t length);

void khc_slist_free_all(khc_slist* slist);

#define READ_REQ_BUFFER_SIZE 1024
#define READ_RESP_HEADER_SIZE 1024
#define READ_BODY_SIZE 1024

typedef enum khc_param {
  KHC_PARAM_HOST,
  KHC_PARAM_PATH,
  KHC_PARAM_METHOD,
  KHC_PARAM_REQ_HEADERS
} khc_param;

typedef enum khc_state {
  KHC_STATE_IDLE,
  KHC_STATE_CONNECT,
  KHC_STATE_REQ_LINE,
  KHC_STATE_REQ_HEADER,
  KHC_STATE_REQ_HEADER_SEND,
  KHC_STATE_REQ_HEADER_SEND_CRLF,
  KHC_STATE_REQ_HEADER_END,
  KHC_STATE_REQ_BODY_READ,
  KHC_STATE_REQ_BODY_SEND,
  KHC_STATE_RESP_HEADERS_ALLOC,
  KHC_STATE_RESP_HEADERS_REALLOC,
  KHC_STATE_RESP_HEADERS_READ,
  KHC_STATE_RESP_HEADERS_CALLBACK,
  /** Process flagment of body obtaind when trying to find body boundary. */
  KHC_STATE_RESP_BODY_FLAGMENT,
  KHC_STATE_RESP_BODY_READ,
  KHC_STATE_RESP_BODY_CALLBACK,
  KHC_STATE_CLOSE,
  KHC_STATE_FINISHED,
} khc_state;

typedef enum khc_code {
  KHC_ERR_OK,
  KHC_ERR_SOCK_CONNECT,
  KHC_ERR_SOCK_CLOSE,
  KHC_ERR_SOCK_SEND,
  KHC_ERR_SOCK_RECV,
  KHC_ERR_HEADER_CALLBACK,
  KHC_ERR_WRITE_CALLBACK,
  KHC_ERR_ALLOCATION,
  KHC_ERR_FAIL,
} khc_code;

typedef struct khc {
  KHC_CB_WRITE _cb_write;
  void* _write_data;
  KHC_CB_READ _cb_read;
  void* _read_data;
  KHC_CB_HEADER _cb_header;
  void* _header_data;

  /** Request header list */
  khc_slist* _req_headers;

  char* _host;
  char* _path;
  char* _method;

  /** State machine */
  khc_state _state;

  /** Socket functions. */
  KHC_CB_SOCK_CONNECT _cb_sock_connect;
  KHC_CB_SOCK_SEND _cb_sock_send;
  KHC_CB_SOCK_RECV _cb_sock_recv;
  KHC_CB_SOCK_CLOSE _cb_sock_close;
  /**   Socket context. */
  void* _sock_ctx_connect;
  void* _sock_ctx_send;
  void* _sock_ctx_recv;
  void* _sock_ctx_close;

  khc_slist* _current_req_header;

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

  khc_code _result;
} khc;

khc_code khc_set_zero(khc* khc);

khc_code khc_perform(khc* khc);

khc_code khc_set_param(khc* khc, khc_param param_type, void* data);

khc_code khc_set_cb_sock_connect(
  khc* khc,
  KHC_CB_SOCK_CONNECT cb,
  void* userdata);

khc_code khc_set_cb_sock_send(
  khc* khc,
  KHC_CB_SOCK_SEND cb,
  void* userdata);

khc_code khc_set_cb_sock_recv(
  khc* khc,
  KHC_CB_SOCK_RECV cb,
  void* userdata);

khc_code khc_set_cb_sock_close(
  khc* khc,
  KHC_CB_SOCK_CLOSE cb,
  void* userdata);

khc_code khc_set_cb_read(
  khc* khc,
  KHC_CB_READ cb,
  void* userdata);

khc_code khc_set_cb_write(
  khc* khc,
  KHC_CB_WRITE cb,
  void* userdata);

khc_code khc_set_cb_header(
  khc* khc,
  KHC_CB_HEADER cb,
  void* userdata);

#ifdef __cplusplus
}
#endif

#endif //__khc
#include <stdio.h>
#include "kii_socket_callback.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef size_t (*WRITE_CALLBACK)(char *ptr, size_t size, size_t nmemb, void *userdata);
typedef size_t (*READ_CALLBACK)(char *buffer, size_t size, size_t nitems, void *instream);
typedef size_t (*HEADER_CALLBACK)(char *buffer, size_t size, size_t nitems, void *userdata);

typedef struct kii_slist {
  char* data;
  struct kii_slist* next;
} kii_slist;

kii_slist* kii_slist_append(kii_slist* slist, const char* string, size_t length);

void kii_slist_free_all(kii_slist* slist);

#define READ_BUFFER_SIZE 1024

typedef struct kii_http {
  WRITE_CALLBACK write_callback;
  void* write_data;
  READ_CALLBACK read_callback;
  void* read_data;
  HEADER_CALLBACK header_callback;
  void* header_data;
  kii_slist* reaquest_headers;
  char* host;
  char* path;
  char* method;
  kii_http_state state;
  KII_SOCKET_CONNECT_CB sc_connect_cb;
  KII_SOCKET_SEND_CB sc_send_cb;
  KII_SOCKET_RECV_CB sc_recv_cb;
  KII_SOCKET_CLOSE_CB sc_close_cb;
  char read_buffer[READ_BUFFER_SIZE];
  size_t read_size;
  int read_buffer_need_resend;
} kii_http;

typedef enum kii_http_state {
  IDLE,
  CONNECT,
  REQUEST_LINE,
  REQUEST_HEADERS,
  REQUEST_BODY,
  RESPONSE_HEADERS,
  RESPONSE_BODY,
  CLOSE,
  CLOSE_AFTER_FAILURE,
} kii_http_state;

typedef enum kii_http_code {
  // TODO: Add codes.
  KII_OK,
  KII_NG
} kii_http_code;

kii_http_code kii_http_perform(kii_http* kii_http);

#ifdef __cplusplus
}
#endif
#include <stdio.h>

typedef size_t (*WRITE_CALLBACK)(char *ptr, size_t size, size_t nmemb, void *userdata);
typedef size_t (*READ_CALLBACK)(char *buffer, size_t size, size_t nitems, void *instream);
typedef size_t (*HEADER_CALLBACK)(char *buffer, size_t size, size_t nitems, void *userdata);

typedef struct kii_slist {

} kii_slist;

kii_slist* kii_slist_append(kii_slist* slist, const char* string);

void kii_slist_free_all(kii_slist* slist);

typedef struct kii_http {
  WRITE_CALLBACK* write_callback;
  void* write_data;
  READ_CALLBACK* read_callback;
  void* read_data;
  HEADER_CALLBACK* header_callback;
  void* header_data;
  kii_slist* reaquest_headers;
  char* url;
  char* method;
} kii_http;

typedef enum kii_http_state {
  IDLE,
  CONNECT,
  REQUEST_HEADERS,
  REQUEST_BODY,
  RESPONSE_HEADERS,
  RESPONSE_BODY,
  CLOSE
} kii_http_state;

typedef enum kii_http_code {
  KII_OK,
  KII_NG
} kii_http_code;

kii_http_code kii_http_perform(kii_http* kii_http);
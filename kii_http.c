#include <stdio.h>
#include "kii_http.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {

}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream) {

}

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {

}

kii_slist* kii_slist_append(kii_slist* slist, const char* string) {

}

void kii_slist_free_all(kii_slist* slist) {

}

kii_http_code kii_http_perform(kii_http* kii_http) {
  
}
#include <stdio.h>
#include <string.h>
#include "kii_http.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {

}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream) {

}

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {

}

kii_slist* kii_slist_append(kii_slist* slist, const char* string, size_t length) {
  kii_slist* next;
  next = malloc(sizeof(kii_slist));
  next->next = NULL;
  next->data = malloc(length+1);
  strncpy(next->data, string, length);
  next->data[length] = '\0';
  if (slist = NULL) {
    return next;
  }
  slist->next = next;
  return slist;
}

void kii_slist_free_all(kii_slist* slist) {
  kii_slist *curr;
  curr = slist;
  while (curr != NULL) {
    kii_slist *next = curr->next;
    free(curr->data);
    free(curr);
    curr = next;
  }
}

kii_http_code kii_http_perform(kii_http* kii_http) {
  
}
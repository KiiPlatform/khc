#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kii_http.h"
#include "kii_http_impl.h"
#include "kii_socket_callback.h"

kii_slist* kii_slist_append(kii_slist* slist, const char* string, size_t length) {
  kii_slist* next;
  next = (kii_slist*)malloc(sizeof(kii_slist));
  next->next = NULL;
  next->data = (char*)malloc(length+1);
  strncpy(next->data, string, length);
  next->data[length] = '\0';
  if (slist == NULL) {
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
    curr->data = NULL;
    free(curr);
    curr = next;
  }
}

kii_http_code kii_http_perform(kii_http* kii_http) {
  while(kii_http->_state != KII_STATE_FINISHED) {
    state_handlers[kii_http->_state](kii_http);
  }
  kii_http_code res = kii_http->_result;
  kii_http->_state = KII_STATE_IDLE;
  kii_http->_result = KII_ERR_OK;
  
  return res;
}
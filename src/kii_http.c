#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kch.h"
#include "kch_impl.h"
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

kch_code kch_perform(kch* kch) {
  kch->_state = KII_STATE_IDLE;
  while(kch->_state != KII_STATE_FINISHED) {
    state_handlers[kch->_state](kch);
  }
  kch_code res = kch->_result;
  kch->_state = KII_STATE_IDLE;
  kch->_result = KII_ERR_OK;
  
  return res;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kch.h"
#include "kch_impl.h"
#include "kch_socket_callback.h"

kch_slist* kch_slist_append(kch_slist* slist, const char* string, size_t length) {
  kch_slist* next;
  next = (kch_slist*)malloc(sizeof(kch_slist));
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

void kch_slist_free_all(kch_slist* slist) {
  kch_slist *curr;
  curr = slist;
  while (curr != NULL) {
    kch_slist *next = curr->next;
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
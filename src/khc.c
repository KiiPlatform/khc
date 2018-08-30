#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "khc.h"
#include "khc_impl.h"
#include "khc_socket_callback.h"

khc_slist* khc_slist_append(khc_slist* slist, const char* string, size_t length) {
  khc_slist* next;
  next = (khc_slist*)malloc(sizeof(khc_slist));
  next->next = NULL;
  next->data = (char*)malloc(length+1);
  strncpy(next->data, string, length);
  next->data[length] = '\0';
  if (slist == NULL) {
    return next;
  }
  khc_slist* end = slist;
  while (end->next != NULL) {
    end = end->next;
  }
  end->next = next;
  return slist;
}

void khc_slist_free_all(khc_slist* slist) {
  khc_slist *curr;
  curr = slist;
  while (curr != NULL) {
    khc_slist *next = curr->next;
    free(curr->data);
    curr->data = NULL;
    free(curr);
    curr = next;
  }
}

khc_code khc_perform(khc* khc) {
  khc->_state = KHC_STATE_IDLE;
  while(khc->_state != KHC_STATE_FINISHED) {
    state_handlers[khc->_state](khc);
  }
  khc_code res = khc->_result;
  khc->_state = KHC_STATE_IDLE;
  khc->_result = KHC_ERR_OK;
  
  return res;
}
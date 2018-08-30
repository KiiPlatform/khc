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

khc_code khc_set_stream_buff(khc* khc, char* buffer, size_t buff_size) {
  khc->_stream_buff = buffer;
  khc->_stream_buff_size = buff_size;
  return KHC_ERR_OK;
}

khc_code khc_set_large_buff(khc* khc, char* buff, size_t buff_size) {
  khc->_large_buff = buff;
  khc->_large_buff_size = buff_size;
  khc->_large_buff_req_size = strlen(buff);

  // Reset stream buff. So that SDK allocates it later.
  khc->_stream_buff = NULL;
  khc->_stream_buff_size = 0;
  khc->_stream_buff_allocated = 0;

  // Replace callback functions.
  khc->_cb_write = khc_write_large_buff;
  khc->_write_data = khc;
  khc->_cb_read = khc_read_large_buff;
  khc->_read_data = khc;
  return KHC_ERR_OK;
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

khc_code khc_set_zero(khc* khc) {
  // Callbacks.
  khc->_cb_write = NULL;
  khc->_write_data = NULL;

  khc->_cb_header = NULL;
  khc->_header_data = NULL;

  khc->_cb_read = NULL;
  khc->_read_data = NULL;

  khc->_cb_sock_connect = NULL;
  khc->_sock_ctx_connect = NULL;

  khc->_cb_sock_send = NULL;
  khc->_sock_ctx_send = NULL;

  khc->_cb_sock_recv = NULL;
  khc->_sock_ctx_recv = NULL;

  khc->_cb_sock_close = NULL;
  khc->_sock_ctx_close = NULL;

  // Elements consist request header.
  khc->_req_headers = NULL;
  khc->_host = NULL;
  khc->_path = NULL;
  khc->_method = NULL;

  // Internal states.
  khc->_state = KHC_STATE_IDLE;
  khc->_current_req_header = NULL;
  khc->_read_size = 0;
  khc->_read_req_end = 0;
  khc->_resp_header_buffer = NULL;
  khc->_resp_header_buffer_current_pos = NULL;
  khc->_resp_header_buffer_size = 0;
  khc->_resp_header_read_size = 0;
  khc->_body_boundary = NULL;
  khc->_cb_header_pos = NULL;
  khc->_cb_header_remaining_size = 0;
  khc->_body_flagment = NULL;
  khc->_body_flagment_size = 0;
  khc->_read_end = 0;
  khc->_body_read_size = 0;
  khc->_result = KHC_ERR_OK;

  // Stream Buffer
  khc->_stream_buff = NULL;
  khc->_stream_buff_size = 0;
  khc->_stream_buff_allocated = 0;

  // Large  Buffer.
  khc->_large_buff = NULL;
  khc->_large_buff_size = 0;
  khc->_large_buff_req_size = 0;
  khc->_large_buff_written = 0;
  khc->_large_buff_read = 0;
  return KHC_ERR_OK;
}
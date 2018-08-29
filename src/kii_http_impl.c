#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kch.h"
#include "kch_impl.h"

kch_code kch_set_cb_sock_connect(
  kch* kch,
  KII_CB_SOCK_CONNECT cb,
  void* userdata)
{
  kch->_cb_sock_connect = cb;
  kch->_sock_ctx_connect = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_cb_sock_send(
  kch* kch,
  KII_CB_SOCK_SEND cb,
  void* userdata)
{
  kch->_cb_sock_send = cb;
  kch->_sock_ctx_send = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_cb_sock_recv(
  kch* kch,
  KII_CB_SOCK_RECV cb,
  void* userdata)
{
  kch->_cb_sock_recv = cb;
  kch->_sock_ctx_recv = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_cb_sock_close(
  kch* kch,
  KII_CB_SOCK_CLOSE cb,
  void* userdata)
{
  kch->_cb_sock_close = cb;
  kch->_sock_ctx_close = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_cb_read(
  kch* kch,
  KII_CB_READ cb,
  void* userdata)
{
  kch->_cb_read = cb;
  kch->_read_data = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_cb_write(
  kch* kch,
  KII_CB_WRITE cb,
  void* userdata)
{
  kch->_cb_write = cb;
  kch->_write_data = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_cb_header(
  kch* kch,
  KII_CB_HEADER cb,
  void* userdata)
{
  kch->_cb_header = cb;
  kch->_header_data = userdata;
  return KII_ERR_OK;
}

kch_code kch_set_param(kch* kch, kch_param param_type, void* data) {
  switch(param_type) {
    case KII_PARAM_HOST:
      kch->_host = (char*)data;
      break;
    case KII_PARAM_PATH:
      kch->_path = (char*)data;
      break;
    case KII_PARAM_METHOD:
      kch->_method = (char*)data;
      break;
    case KII_PARAM_REQ_HEADERS:
      kch->_req_headers = (kch_slist*)data;
      break;
    default:
      return KII_ERR_FAIL;
  }
  return KII_ERR_OK;
}

void kch_state_idle(kch* kch) {
  if (kch->_host == NULL) {
    // Fallback to localhost
    kch->_host = "localhost";
  }
  if (kch->_method == NULL) {
    // Fallback to GET.
    kch->_method = "GET";
  }
  memset(kch->_read_buffer, '\0', READ_REQ_BUFFER_SIZE);
  kch->_state = KII_STATE_CONNECT;
  kch->_resp_header_buffer_size = 0;
  kch->_read_end = 0;
  kch->_read_size = 0;
  kch->_resp_header_read_size = 0;
  kch->_result = KII_ERR_OK;
  return;
}

void kch_state_connect(kch* kch) {
  kch_sock_code_t con_res = kch->_cb_sock_connect(kch->_sock_ctx_connect, kch->_host, 443);
  if (con_res == KIISOCK_OK) {
    kch->_state = KII_STATE_REQ_LINE;
    return;
  }
  if (con_res == KIISOCK_AGAIN) {
    return;
  }
  if (con_res == KIISOCK_FAIL) {
    kch->_result = KII_ERR_SOCK_CONNECT;
    kch->_state = KII_STATE_FINISHED;
    return;
  }
}

static const char schema[] = "https://";
static const char http_version[] = "HTTP/1.0\r\n";

static size_t request_line_len(kch* kch) {
  char* method = kch->_method;
  char* host = kch->_host;
  // Path must be started with '/'
  char* path = kch->_path;

  return ( // example)GET https://api.kch.com/v1/users HTTP1.0\r\n
    strlen(method) + 1
    + strlen(schema) + strlen(host) + strlen(path) + 1
    + strlen(http_version)
  );
}

void kch_state_req_line(kch* kch) {
  size_t len = request_line_len(kch);
  char request_line[len+1];
  char* host = kch->_host;
  char* path = kch->_path;

  request_line[0] = '\0';
  strcat(request_line, kch->_method);
  strcat(request_line, " ");
  strcat(request_line, schema);
  strcat(request_line, host);
  strcat(request_line, path);
  strcat(request_line, " ");
  strcat(request_line, http_version);
  kch_sock_code_t send_res = kch->_cb_sock_send(kch->_sock_ctx_send, request_line, strlen(request_line));
  if (send_res == KIISOCK_OK) {
    kch->_state = KII_STATE_REQ_HEADER;
    kch->_current_req_header = kch->_req_headers;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_SEND;
    return;
  } 
}

void kch_state_req_header(kch* kch) {
  if (kch->_current_req_header == NULL) {
    kch->_state = KII_STATE_REQ_HEADER_END;
    return;
  }
  char* data = kch->_current_req_header->data;
  if (data == NULL) {
    // Skip if data is NULL.
    kch->_current_req_header = kch->_current_req_header->next;
    return;
  }
  size_t len = strlen(data);
  if (len == 0) {
    // Skip if nothing to send.
    kch->_current_req_header = kch->_current_req_header->next;
    return;
  }
  kch->_state = KII_STATE_REQ_HEADER_SEND;
}

void kch_state_req_header_send(kch* kch) {
  char* line = kch->_current_req_header->data;
  kch_sock_code_t send_res = kch->_cb_sock_send(kch->_sock_ctx_send, line, strlen(line));
  if (send_res == KIISOCK_OK) {
    kch->_state = KII_STATE_REQ_HEADER_SEND_CRLF;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_SEND;
    return;
  } 
}

void kch_state_req_header_send_crlf(kch* kch) {
  kch_sock_code_t send_res = kch->_cb_sock_send(kch->_sock_ctx_send, "\r\n", 2);
  if (send_res == KIISOCK_OK) {
    kch->_current_req_header = kch->_current_req_header->next;
    kch->_state = KII_STATE_REQ_HEADER;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_SEND;
    return;
  } 
}

void kch_state_req_header_end(kch* kch) {
  kch_sock_code_t send_res = kch->_cb_sock_send(kch->_sock_ctx_send, "\r\n", 2);
  if (send_res == KIISOCK_OK) {
    kch->_state = KII_STATE_REQ_BODY_READ;
    kch->_read_req_end = 0;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_SEND;
    return;
  }
}

void kch_state_req_body_read(kch* kch) {
  kch->_read_size = kch->_cb_read(kch->_read_buffer, 1, READ_BODY_SIZE, kch->_read_data);
  // TODO: handle read failure. let return signed value?
  if (kch->_read_size > 0) {
    kch->_state = KII_STATE_REQ_BODY_SEND;
  } else {
    kch->_state = KII_STATE_RESP_HEADERS_ALLOC;
  }
  if (kch->_read_size < READ_BODY_SIZE) {
    kch->_read_req_end = 1;
  }
  return;
}

void kch_state_req_body_send(kch* kch) {
  kch_sock_code_t send_res = kch->_cb_sock_send(kch->_sock_ctx_send, kch->_read_buffer, kch->_read_size);
  if (send_res == KIISOCK_OK) {
    if (kch->_read_req_end == 1) {
      kch->_state = KII_STATE_RESP_HEADERS_ALLOC;
    } else {
      kch->_state = KII_STATE_REQ_BODY_READ;
    }
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_SEND;
    return;
  }
}

void kch_state_resp_headers_alloc(kch* kch) {
  kch->_resp_header_buffer = malloc(READ_RESP_HEADER_SIZE);
  if (kch->_resp_header_buffer == NULL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_ALLOCATION;
    return;
  }
  memset(kch->_resp_header_buffer, '\0', READ_RESP_HEADER_SIZE);
  kch->_resp_header_buffer_size = READ_RESP_HEADER_SIZE;
  kch->_resp_header_buffer_current_pos = kch->_resp_header_buffer;
  kch->_state = KII_STATE_RESP_HEADERS_READ;
  return;
}

void kch_state_resp_headers_realloc(kch* kch) {
  void* newBuff = realloc(kch->_resp_header_buffer, kch->_resp_header_buffer_size + READ_RESP_HEADER_SIZE);
  if (newBuff == NULL) {
    free(kch->_resp_header_buffer);
    kch->_resp_header_buffer = NULL;
    kch->_resp_header_buffer_size = 0;
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_ALLOCATION;
    return;
  }
  // Pointer: last part newly allocated.
  kch->_resp_header_buffer = newBuff;
  kch->_resp_header_buffer_current_pos = newBuff + kch->_resp_header_buffer_size;
  memset(kch->_resp_header_buffer_current_pos, '\0', READ_RESP_HEADER_SIZE);
  kch->_resp_header_buffer_size = kch->_resp_header_buffer_size + READ_RESP_HEADER_SIZE;
  kch->_state = KII_STATE_RESP_HEADERS_READ;
  return;
}

void kch_state_resp_headers_read(kch* kch) {
  size_t read_size = 0;
  size_t read_req_size = READ_RESP_HEADER_SIZE - 1;
  kch_sock_code_t read_res = 
    kch->_cb_sock_recv(kch->_sock_ctx_recv, kch->_resp_header_buffer_current_pos, read_req_size, &read_size);
  if (read_res == KIISOCK_OK) {
    kch->_resp_header_read_size += read_size;
    if (read_size < READ_RESP_HEADER_SIZE) {
      kch->_read_end = 1;
    }
    // Search boundary for whole buffer.
    char* boundary = strstr(kch->_resp_header_buffer, "\r\n\r\n");
    if (boundary == NULL) {
      // Not reached to end of headers.
      kch->_state = KII_STATE_RESP_HEADERS_REALLOC;
      return;
    } else {
      kch->_body_boundary = boundary;
      kch->_state = KII_STATE_RESP_HEADERS_CALLBACK;
      kch->_cb_header_remaining_size = kch->_resp_header_buffer_size;
      kch->_cb_header_pos = kch->_resp_header_buffer;
      return;
    }
  }
  if (read_res == KIISOCK_AGAIN) {
    return;
  }
  if (read_res == KIISOCK_FAIL) {
    char* start = kch->_resp_header_buffer + READ_RESP_HEADER_SIZE - kch->_resp_header_buffer_size;
    free(start);
    kch->_resp_header_buffer = NULL;
    kch->_resp_header_buffer_size = 0;
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_RECV;
    return;
  }
}

void kch_state_resp_headers_callback(kch* kch) {
  char* header_boundary = strstr(kch->_cb_header_pos, "\r\n");
  size_t header_size = header_boundary - kch->_cb_header_pos;
  size_t header_written = 
    kch->_cb_header(kch->_cb_header_pos, 1, header_size, kch->_header_data);
  if (header_written != header_size) { // Error in callback function.
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_HEADER_CALLBACK;
    free(kch->_resp_header_buffer);
    kch->_resp_header_buffer = NULL;
    kch->_resp_header_buffer_size = 0;
    return;
  }
  if (header_boundary < kch->_body_boundary) {
    kch->_cb_header_pos = header_boundary + 2; // +2 : Skip CRLF
    kch->_cb_header_remaining_size = kch->_cb_header_remaining_size - header_size - 2;
    return;
  } else { // Callback called for all headers.
    // Check if body is included in the buffer.
    size_t header_size = kch->_body_boundary - kch->_resp_header_buffer;
    size_t body_size = kch->_resp_header_read_size - header_size - 4;
    if (body_size > 0) {
      kch->_body_flagment = kch->_body_boundary + 4;
      kch->_body_flagment_size = body_size;
      kch->_state = KII_STATE_RESP_BODY_FLAGMENT;
      return;
    } else {
      free(kch->_resp_header_buffer);
      kch->_resp_header_buffer = NULL;
      kch->_resp_header_buffer_size = 0;
      if (kch->_read_end == 1) {
        kch->_state = KII_STATE_CLOSE;
        return;
      } else {
        kch->_state = KII_STATE_RESP_BODY_READ;
        return;
      }
    }
  }
}

void kch_state_resp_body_flagment(kch* kch) {
  size_t written = 
    kch->_cb_write(kch->_body_flagment, 1, kch->_body_flagment_size, kch->_write_data);
  free(kch->_resp_header_buffer);
  kch->_resp_header_buffer = NULL;
  kch->_resp_header_buffer_size = 0;
  if (written != kch->_body_flagment_size) { // Error in write callback.
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_WRITE_CALLBACK;
    return;
  }
  if (kch->_read_end == 1) {
    kch->_state = KII_STATE_CLOSE;
    return;
  } else {
    kch->_state = KII_STATE_RESP_BODY_READ;
    return;
  }
}

void kch_state_resp_body_read(kch* kch) {
  size_t read_size = 0;
  kch_sock_code_t read_res = 
    kch->_cb_sock_recv(kch->_sock_ctx_recv, kch->_body_buffer, READ_BODY_SIZE, &read_size);
  if (read_res == KIISOCK_OK) {
    if (read_size < READ_BODY_SIZE) {
      kch->_read_end = 1;
    }
    kch->_body_read_size = read_size;
    kch->_state = KII_STATE_RESP_BODY_CALLBACK;
    return;
  }
  if (read_res == KIISOCK_AGAIN) {
    return;
  }
  if (read_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_SOCK_RECV;
    return;
  }
}

void kch_state_resp_body_callback(kch* kch) {
  size_t written = kch->_cb_write(kch->_body_buffer, 1, kch->_body_read_size, kch->_write_data);
  if (written < kch->_body_read_size) { // Error in write callback.
    kch->_state = KII_STATE_CLOSE;
    kch->_result = KII_ERR_WRITE_CALLBACK;
    return;
  }
  if (kch->_read_end == 1) {
    kch->_state = KII_STATE_CLOSE;
  } else {
    kch->_state = KII_STATE_RESP_BODY_READ;
  }
  return;
}

void kch_state_close(kch* kch) {
  kch_sock_code_t close_res = kch->_cb_sock_close(kch->_sock_ctx_close);
  if (close_res == KIISOCK_OK) {
    kch->_state = KII_STATE_FINISHED;
    kch->_result = KII_ERR_OK;
    return;
  }
  if (close_res == KIISOCK_AGAIN) {
    return;
  }
  if (close_res == KIISOCK_FAIL) {
    kch->_state = KII_STATE_FINISHED;
    kch->_result = KII_ERR_SOCK_CLOSE;
    return;
  }
}

const KII_STATE_HANDLER state_handlers[] = {
  kch_state_idle,
  kch_state_connect,
  kch_state_req_line,
  kch_state_req_header,
  kch_state_req_header_send,
  kch_state_req_header_send_crlf,
  kch_state_req_header_end,
  kch_state_req_body_read,
  kch_state_req_body_send,
  kch_state_resp_headers_alloc,
  kch_state_resp_headers_realloc,
  kch_state_resp_headers_read,
  kch_state_resp_headers_callback,
  kch_state_resp_body_flagment,
  kch_state_resp_body_read,
  kch_state_resp_body_callback,
  kch_state_close
};
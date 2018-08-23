#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kii_http.h"
#include "kii_http_impl.h"

kii_http_code kii_http_set_cb_sock_connect(
  kii_http* kii_http,
  KII_CB_SOCK_CONNECT cb,
  void* userdata)
{
  kii_http->_cb_sock_connect = cb;
  kii_http->_sock_ctx_connect = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_cb_sock_send(
  kii_http* kii_http,
  KII_CB_SOCK_SEND cb,
  void* userdata)
{
  kii_http->_cb_sock_send = cb;
  kii_http->_sock_ctx_send = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_cb_sock_recv(
  kii_http* kii_http,
  KII_CB_SOCK_RECV cb,
  void* userdata)
{
  kii_http->_cb_sock_recv = cb;
  kii_http->_sock_ctx_recv = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_cb_sock_close(
  kii_http* kii_http,
  KII_CB_SOCK_CLOSE cb,
  void* userdata)
{
  kii_http->_cb_sock_close = cb;
  kii_http->_sock_ctx_close = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_cb_read(
  kii_http* kii_http,
  KII_CB_READ cb,
  void* userdata)
{
  kii_http->_cb_read = cb;
  kii_http->_read_data = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_cb_write(
  kii_http* kii_http,
  KII_CB_WRITE cb,
  void* userdata)
{
  kii_http->_cb_write = cb;
  kii_http->_write_data = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_cb_header(
  kii_http* kii_http,
  KII_CB_HEADER cb,
  void* userdata)
{
  kii_http->_cb_header = cb;
  kii_http->_header_data = userdata;
  return KII_ERR_OK;
}

kii_http_code kii_http_set_param(kii_http* kii_http, kii_http_param param_type, void* data) {
  switch(param_type) {
    case KII_PARAM_HOST:
      kii_http->_host = (char*)data;
      break;
    case KII_PARAM_PATH:
      kii_http->_path = (char*)data;
      break;
    case KII_PARAM_METHOD:
      kii_http->_method = (char*)data;
      break;
    case KII_PARAM_REQ_HEADERS:
      kii_http->_req_headers = (kii_slist*)data;
      break;
    default:
      return KII_ERR_FAIL;
  }
  return KII_ERR_OK;
}

void kii_state_idle(kii_http* kii_http) {
  if (kii_http->_host == NULL) {
    // Fallback to localhost
    kii_http->_host = "localhost";
  }
  if (kii_http->_method == NULL) {
    // Fallback to GET.
    kii_http->_method = "GET";
  }
  memset(kii_http->_read_buffer, '\0', READ_REQ_BUFFER_SIZE);
  kii_http->_state = KII_STATE_CONNECT;
  kii_http->_resp_header_buffer_size = 0;
  kii_http->_read_end = 0;
  kii_http->_read_size = 0;
  kii_http->_resp_header_read_size = 0;
  kii_http->_result = KII_ERR_OK;
  return;
}

void kii_state_connect(kii_http* kii_http) {
  kii_sock_code_t con_res = kii_http->_cb_sock_connect(kii_http->_sock_ctx_connect, kii_http->_host, 8080);
  if (con_res == KIISOCK_OK) {
    kii_http->_state = KII_STATE_REQ_LINE;
    return;
  }
  if (con_res == KIISOCK_AGAIN) {
    return;
  }
  if (con_res == KIISOCK_FAIL) {
    kii_http->_result = KII_ERR_SOCK_CONNECT;
    kii_http->_state = KII_STATE_FINISHED;
    return;
  }
}

static const char schema[] = "https://";
static const char http_version[] = "HTTP 1.0\r\n";

static size_t request_line_len(kii_http* kii_http) {
  char* method = kii_http->_method;
  char* host = kii_http->_host;
  // Path must be started with '/'
  char* path = kii_http->_path;

  return ( // example)GET https://api.kii.com/v1/users HTTP1.0\r\n
    strlen(method) + 1
    + strlen(schema) + strlen(host) + strlen(path) + 1
    + strlen(http_version)
  );
}

void kii_state_req_line(kii_http* kii_http) {
  size_t len = request_line_len(kii_http);
  char request_line[len+1];
  char* host = kii_http->_host;
  char* path = kii_http->_path;

  request_line[0] = '\0';
  strcat(request_line, kii_http->_method);
  strcat(request_line, " ");
  strcat(request_line, schema);
  strcat(request_line, host);
  strcat(request_line, path);
  strcat(request_line, " ");
  strcat(request_line, http_version);
  kii_sock_code_t send_res = kii_http->_cb_sock_send(kii_http->_sock_ctx_send, request_line, strlen(request_line));
  if (send_res == KIISOCK_OK) {
    kii_http->_state = KII_STATE_REQ_HEADER;
    kii_http->_current_req_header = kii_http->_req_headers;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_SEND;
    return;
  } 
}

void kii_state_req_header(kii_http* kii_http) {
  if (kii_http->_current_req_header == NULL) {
    kii_http->_state = KII_STATE_REQ_HEADER_END;
    return;
  }
  char* data = kii_http->_current_req_header->data;
  if (data == NULL) {
    // Skip if data is NULL.
    kii_http->_current_req_header = kii_http->_current_req_header->next;
    return;
  }
  size_t len = strlen(data);
  if (len == 0) {
    // Skip if nothing to send.
    kii_http->_current_req_header = kii_http->_current_req_header->next;
    return;
  }
  kii_http->_state = KII_STATE_REQ_HEADER_SEND;
}

void kii_state_req_header_send(kii_http* kii_http) {
  char* line = kii_http->_current_req_header->data;
  kii_sock_code_t send_res = kii_http->_cb_sock_send(kii_http->_sock_ctx_send, line, strlen(line));
  if (send_res == KIISOCK_OK) {
    kii_http->_state = KII_STATE_REQ_HEADER_SEND_CRLF;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_SEND;
    return;
  } 
}

void kii_state_req_header_send_crlf(kii_http* kii_http) {
  kii_sock_code_t send_res = kii_http->_cb_sock_send(kii_http->_sock_ctx_send, "\r\n", 2);
  if (send_res == KIISOCK_OK) {
    kii_http->_current_req_header = kii_http->_current_req_header->next;
    kii_http->_state = KII_STATE_REQ_HEADER;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_SEND;
    return;
  } 
}

void kii_state_req_header_end(kii_http* kii_http) {
  kii_sock_code_t send_res = kii_http->_cb_sock_send(kii_http->_sock_ctx_send, "\r\n", 2);
  if (send_res == KIISOCK_OK) {
    kii_http->_state = KII_STATE_REQ_BODY_READ;
    kii_http->_read_req_end = 0;
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_SEND;
    return;
  }
}

void kii_state_req_body_read(kii_http* kii_http) {
  kii_http->_read_size = kii_http->_cb_read(kii_http->_read_buffer, 1, READ_BODY_SIZE, kii_http->_read_data);
  // TODO: handle read failure. let return signed value?
  kii_http->_state = KII_STATE_REQ_BODY_SEND;
  if (kii_http->_read_size < READ_BODY_SIZE) {
    kii_http->_read_req_end = 1;
  }
  return;
}

void kii_state_req_body_send(kii_http* kii_http) {
  kii_sock_code_t send_res = kii_http->_cb_sock_send(kii_http->_sock_ctx_send, kii_http->_read_buffer, kii_http->_read_size);
  if (send_res == KIISOCK_OK) {
    if (kii_http->_read_req_end == 1) {
      kii_http->_state = KII_STATE_RESP_HEADERS_ALLOC;
    } else {
      kii_http->_state = KII_STATE_REQ_BODY_READ;
    }
    return;
  }
  if (send_res == KIISOCK_AGAIN) {
    return;
  }
  if (send_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_SEND;
    return;
  }
}

void kii_state_resp_headers_alloc(kii_http* kii_http) {
  kii_http->_resp_header_buffer = malloc(READ_RESP_HEADER_SIZE);
  if (kii_http->_resp_header_buffer == NULL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_ALLOCATION;
    return;
  }
  memset(kii_http->_resp_header_buffer, '\0', READ_RESP_HEADER_SIZE);
  kii_http->_resp_header_buffer_size = READ_RESP_HEADER_SIZE;
  kii_http->_resp_header_buffer_current_pos = kii_http->_resp_header_buffer;
  kii_http->_state = KII_STATE_RESP_HEADERS_READ;
  return;
}

void kii_state_resp_headers_realloc(kii_http* kii_http) {
  void* newBuff = realloc(kii_http->_resp_header_buffer, kii_http->_resp_header_buffer_size + READ_RESP_HEADER_SIZE);
  if (newBuff == NULL) {
    free(kii_http->_resp_header_buffer);
    kii_http->_resp_header_buffer = NULL;
    kii_http->_resp_header_buffer_size = 0;
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_ALLOCATION;
    return;
  }
  // Pointer: last part newly allocated.
  kii_http->_resp_header_buffer = newBuff;
  kii_http->_resp_header_buffer_current_pos = newBuff + kii_http->_resp_header_buffer_size;
  memset(kii_http->_resp_header_buffer_current_pos, '\0', READ_RESP_HEADER_SIZE);
  kii_http->_resp_header_buffer_size = kii_http->_resp_header_buffer_size + READ_RESP_HEADER_SIZE;
  kii_http->_state = KII_STATE_RESP_HEADERS_READ;
  return;
}

void kii_state_resp_headers_read(kii_http* kii_http) {
  size_t read_size = 0;
  size_t read_req_size = READ_RESP_HEADER_SIZE - 1;
  kii_sock_code_t read_res = 
    kii_http->_cb_sock_recv(kii_http->_sock_ctx_recv, kii_http->_resp_header_buffer_current_pos, read_req_size, &read_size);
  if (read_res == KIISOCK_OK) {
    kii_http->_resp_header_read_size += read_size;
    if (read_size < READ_RESP_HEADER_SIZE) {
      kii_http->_read_end = 1;
    }
    // Search boundary for whole buffer.
    char* boundary = strstr(kii_http->_resp_header_buffer, "\r\n\r\n");
    if (boundary == NULL) {
      // Not reached to end of headers.
      kii_http->_state = KII_STATE_RESP_HEADERS_REALLOC;
      return;
    } else {
      kii_http->_body_boundary = boundary;
      kii_http->_state = KII_STATE_RESP_HEADERS_CALLBACK;
      kii_http->_cb_header_remaining_size = kii_http->_resp_header_buffer_size;
      kii_http->_cb_header_pos = kii_http->_resp_header_buffer;
      return;
    }
  }
  if (read_res == KIISOCK_AGAIN) {
    return;
  }
  if (read_res == KIISOCK_FAIL) {
    char* start = kii_http->_resp_header_buffer + READ_RESP_HEADER_SIZE - kii_http->_resp_header_buffer_size;
    free(start);
    kii_http->_resp_header_buffer = NULL;
    kii_http->_resp_header_buffer_size = 0;
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_RECV;
    return;
  }
}

void kii_state_resp_headers_callback(kii_http* kii_http) {
  char* header_boundary = strstr(kii_http->_cb_header_pos, "\r\n");
  size_t header_size = header_boundary - kii_http->_cb_header_pos;
  size_t header_written = 
    kii_http->_cb_header(kii_http->_cb_header_pos, 1, header_size, kii_http->_header_data);
  if (header_written != header_size) { // Error in callback function.
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_HEADER_CALLBACK;
    free(kii_http->_resp_header_buffer);
    kii_http->_resp_header_buffer = NULL;
    kii_http->_resp_header_buffer_size = 0;
    return;
  }
  if (header_boundary < kii_http->_body_boundary) {
    kii_http->_cb_header_pos = header_boundary + 2; // +2 : Skip CRLF
    kii_http->_cb_header_remaining_size = kii_http->_cb_header_remaining_size - header_size - 2;
    return;
  } else { // Callback called for all headers.
    // Check if body is included in the buffer.
    size_t header_size = kii_http->_body_boundary - kii_http->_resp_header_buffer;
    size_t body_size = kii_http->_resp_header_read_size - header_size - 4;
    if (body_size > 0) {
      kii_http->_body_flagment = kii_http->_body_boundary + 4;
      kii_http->_body_flagment_size = body_size;
      kii_http->_state = KII_STATE_RESP_BODY_FLAGMENT;
      return;
    } else {
      free(kii_http->_resp_header_buffer);
      kii_http->_resp_header_buffer = NULL;
      kii_http->_resp_header_buffer_size = 0;
      if (kii_http->_read_end == 1) {
        kii_http->_state = KII_STATE_CLOSE;
        return;
      } else {
        kii_http->_state = KII_STATE_RESP_BODY_READ;
        return;
      }
    }
  }
}

void kii_state_resp_body_flagment(kii_http* kii_http) {
  size_t written = 
    kii_http->_cb_write(kii_http->_body_flagment, 1, kii_http->_body_flagment_size, kii_http->_write_data);
  free(kii_http->_resp_header_buffer);
  kii_http->_resp_header_buffer = NULL;
  kii_http->_resp_header_buffer_size = 0;
  if (written != kii_http->_body_flagment_size) { // Error in write callback.
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_WRITE_CALLBACK;
    return;
  }
  if (kii_http->_read_end == 1) {
    kii_http->_state = KII_STATE_CLOSE;
    return;
  } else {
    kii_http->_state = KII_STATE_RESP_BODY_READ;
    return;
  }
}

void kii_state_resp_body_read(kii_http* kii_http) {
  size_t read_size = 0;
  kii_sock_code_t read_res = 
    kii_http->_cb_sock_recv(kii_http->_sock_ctx_recv, kii_http->_body_buffer, READ_BODY_SIZE, &read_size);
  if (read_res == KIISOCK_OK) {
    if (read_size < READ_BODY_SIZE) {
      kii_http->_read_end = 1;
    }
    kii_http->_body_read_size = read_size;
    kii_http->_state = KII_STATE_RESP_BODY_CALLBACK;
    return;
  }
  if (read_res == KIISOCK_AGAIN) {
    return;
  }
  if (read_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_SOCK_RECV;
    return;
  }
}

void kii_state_resp_body_callback(kii_http* kii_http) {
  size_t written = kii_http->_cb_write(kii_http->_body_buffer, 1, kii_http->_body_read_size, kii_http->_write_data);
  if (written < kii_http->_body_read_size) { // Error in write callback.
    kii_http->_state = KII_STATE_CLOSE;
    kii_http->_result = KII_ERR_WRITE_CALLBACK;
    return;
  }
  if (kii_http->_read_end == 1) {
    kii_http->_state = KII_STATE_CLOSE;
  } else {
    kii_http->_state = KII_STATE_RESP_BODY_READ;
  }
  return;
}

void kii_state_close(kii_http* kii_http) {
  kii_sock_code_t close_res = kii_http->_cb_sock_close(kii_http->_sock_ctx_close);
  if (close_res == KIISOCK_OK) {
    kii_http->_state = KII_STATE_FINISHED;
    kii_http->_result = KII_ERR_OK;
    return;
  }
  if (close_res == KIISOCK_AGAIN) {
    return;
  }
  if (close_res == KIISOCK_FAIL) {
    kii_http->_state = KII_STATE_FINISHED;
    kii_http->_result = KII_ERR_SOCK_CLOSE;
    return;
  }
}

const KII_STATE_HANDLER state_handlers[] = {
  kii_state_idle,
  kii_state_connect,
  kii_state_req_line,
  kii_state_req_header,
  kii_state_req_header_send,
  kii_state_req_header_send_crlf,
  kii_state_req_header_end,
  kii_state_req_body_read,
  kii_state_req_body_send,
  kii_state_resp_headers_alloc,
  kii_state_resp_headers_realloc,
  kii_state_resp_headers_read,
  kii_state_resp_headers_callback,
  kii_state_resp_body_flagment,
  kii_state_resp_body_read,
  kii_state_resp_body_callback,
  kii_state_close
};
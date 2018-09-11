#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "khc.h"
#include "khc_impl.h"

khc_code khc_set_cb_sock_connect(
  khc* khc,
  KHC_CB_SOCK_CONNECT cb,
  void* userdata)
{
  khc->_cb_sock_connect = cb;
  khc->_sock_ctx_connect = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_cb_sock_send(
  khc* khc,
  KHC_CB_SOCK_SEND cb,
  void* userdata)
{
  khc->_cb_sock_send = cb;
  khc->_sock_ctx_send = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_cb_sock_recv(
  khc* khc,
  KHC_CB_SOCK_RECV cb,
  void* userdata)
{
  khc->_cb_sock_recv = cb;
  khc->_sock_ctx_recv = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_cb_sock_close(
  khc* khc,
  KHC_CB_SOCK_CLOSE cb,
  void* userdata)
{
  khc->_cb_sock_close = cb;
  khc->_sock_ctx_close = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_cb_read(
  khc* khc,
  KHC_CB_READ cb,
  void* userdata)
{
  khc->_cb_read = cb;
  khc->_read_data = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_cb_write(
  khc* khc,
  KHC_CB_WRITE cb,
  void* userdata)
{
  khc->_cb_write = cb;
  khc->_write_data = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_cb_header(
  khc* khc,
  KHC_CB_HEADER cb,
  void* userdata)
{
  khc->_cb_header = cb;
  khc->_header_data = userdata;
  return KHC_ERR_OK;
}

khc_code khc_set_host(khc* khc, const char* host) {
  size_t len = strlen(host);
  size_t buff_len = sizeof(khc->_host);
  if (buff_len < len + 1) {
    return KHC_ERR_TOO_LARGE_DATA;
  }
  memcpy(khc->_host, host, len + 1);
  return KHC_ERR_OK;
}

khc_code khc_set_path(khc* khc, const char* path) {
  size_t len = strlen(path);
  size_t buff_len = sizeof(khc->_path);
  if (buff_len < len + 1) {
    return KHC_ERR_TOO_LARGE_DATA;
  }
  memcpy(khc->_path, path, len + 1);
  return KHC_ERR_OK;
}

khc_code khc_set_method(khc* khc, const char* method) {
  size_t len = strlen(method);
  size_t buff_len = sizeof(khc->_method);
  if (buff_len < len + 1) {
    return KHC_ERR_TOO_LARGE_DATA;
  }
  memcpy(khc->_method, method, len + 1);
  return KHC_ERR_OK;
}

khc_code khc_set_req_headers(khc* khc, khc_slist* headers) {
  khc->_req_headers = headers;
  return KHC_ERR_OK;
}

void khc_state_idle(khc* khc) {
  if (strlen(khc->_host) == 0) {
    // Fallback to localhost
    strncpy(khc->_host, "localhost", sizeof(khc->_host));
  }
  if (strlen(khc->_method) == 0) {
    // Fallback to GET.
    strncpy(khc->_method, "GET", sizeof(khc->_method));
  }
  if (khc->_stream_buff == NULL) {
    char* buff = malloc(DEFAULT_STREAM_BUFF_SIZE);
    if (buff == NULL) {
      khc->_state = KHC_STATE_FINISHED;
      khc->_result = KHC_ERR_ALLOCATION;
      return;
    }
    khc->_stream_buff = buff;
    khc->_stream_buff_allocated = 1;
    khc->_stream_buff_size = DEFAULT_STREAM_BUFF_SIZE;
  }
  khc->_state = KHC_STATE_CONNECT;
  return;
}

void khc_state_connect(khc* khc) {
  khc_sock_code_t con_res = khc->_cb_sock_connect(khc->_sock_ctx_connect, khc->_host, 443);
  if (con_res == KHC_SOCK_OK) {
    khc->_state = KHC_STATE_REQ_LINE;
    return;
  }
  if (con_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (con_res == KHC_SOCK_FAIL) {
    khc->_result = KHC_ERR_SOCK_CONNECT;
    khc->_state = KHC_STATE_FINISHED;
    return;
  }
}

static const char schema[] = "https://";
static const char http_version[] = "HTTP/1.0\r\n";

static size_t request_line_len(khc* khc) {
  char* method = khc->_method;
  char* host = khc->_host;
  // Path must be started with '/'
  char* path = khc->_path;

  return ( // example)GET https://api.khc.com/v1/users HTTP1.0\r\n
    strlen(method) + 1
    + strlen(schema) + strlen(host) + strlen(path) + 1
    + strlen(http_version)
  );
}

void khc_state_req_line(khc* khc) {
  size_t len = request_line_len(khc);
  char request_line[len+1];
  char* host = khc->_host;
  char* path = khc->_path;

  request_line[0] = '\0';
  strcat(request_line, khc->_method);
  strcat(request_line, " ");
  strcat(request_line, schema);
  strcat(request_line, host);
  strcat(request_line, path);
  strcat(request_line, " ");
  strcat(request_line, http_version);
  khc_sock_code_t send_res = khc->_cb_sock_send(khc->_sock_ctx_send, request_line, strlen(request_line));
  if (send_res == KHC_SOCK_OK) {
    khc->_state = KHC_STATE_REQ_HEADER;
    khc->_current_req_header = khc->_req_headers;
    return;
  }
  if (send_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (send_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_SEND;
    return;
  } 
}

void khc_state_req_header(khc* khc) {
  if (khc->_current_req_header == NULL) {
    khc->_state = KHC_STATE_REQ_HEADER_END;
    return;
  }
  char* data = khc->_current_req_header->data;
  if (data == NULL) {
    // Skip if data is NULL.
    khc->_current_req_header = khc->_current_req_header->next;
    return;
  }
  size_t len = strlen(data);
  if (len == 0) {
    // Skip if nothing to send.
    khc->_current_req_header = khc->_current_req_header->next;
    return;
  }
  khc->_state = KHC_STATE_REQ_HEADER_SEND;
}

void khc_state_req_header_send(khc* khc) {
  char* line = khc->_current_req_header->data;
  khc_sock_code_t send_res = khc->_cb_sock_send(khc->_sock_ctx_send, line, strlen(line));
  if (send_res == KHC_SOCK_OK) {
    khc->_state = KHC_STATE_REQ_HEADER_SEND_CRLF;
    return;
  }
  if (send_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (send_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_SEND;
    return;
  } 
}

void khc_state_req_header_send_crlf(khc* khc) {
  khc_sock_code_t send_res = khc->_cb_sock_send(khc->_sock_ctx_send, "\r\n", 2);
  if (send_res == KHC_SOCK_OK) {
    khc->_current_req_header = khc->_current_req_header->next;
    khc->_state = KHC_STATE_REQ_HEADER;
    return;
  }
  if (send_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (send_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_SEND;
    return;
  } 
}

void khc_state_req_header_end(khc* khc) {
  khc_sock_code_t send_res = khc->_cb_sock_send(khc->_sock_ctx_send, "\r\n", 2);
  if (send_res == KHC_SOCK_OK) {
    khc->_state = KHC_STATE_REQ_BODY_READ;
    khc->_read_req_end = 0;
    return;
  }
  if (send_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (send_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_SEND;
    return;
  }
}

void khc_state_req_body_read(khc* khc) {
  khc->_read_size = khc->_cb_read(khc->_stream_buff, 1, khc->_stream_buff_size, khc->_read_data);
  // TODO: handle read failure. let return signed value?
  if (khc->_read_size > 0) {
    khc->_state = KHC_STATE_REQ_BODY_SEND;
  } else {
    khc->_state = KHC_STATE_RESP_HEADERS_ALLOC;
  }
  if (khc->_read_size < khc->_stream_buff_size) {
    khc->_read_req_end = 1;
  }
  return;
}

void khc_state_req_body_send(khc* khc) {
  khc_sock_code_t send_res = khc->_cb_sock_send(khc->_sock_ctx_send, khc->_stream_buff, khc->_read_size);
  if (send_res == KHC_SOCK_OK) {
    if (khc->_read_req_end == 1) {
      khc->_state = KHC_STATE_RESP_HEADERS_ALLOC;
    } else {
      khc->_state = KHC_STATE_REQ_BODY_READ;
    }
    return;
  }
  if (send_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (send_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_SEND;
    return;
  }
}

void khc_state_resp_headers_alloc(khc* khc) {
  khc->_resp_header_buffer = malloc(RESP_HEADER_BUFF_SIZE);
  if (khc->_resp_header_buffer == NULL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_ALLOCATION;
    return;
  }
  memset(khc->_resp_header_buffer, '\0', RESP_HEADER_BUFF_SIZE);
  khc->_resp_header_buffer_size = RESP_HEADER_BUFF_SIZE;
  khc->_resp_header_buffer_current_pos = khc->_resp_header_buffer;
  khc->_state = KHC_STATE_RESP_HEADERS_READ;
  return;
}

void khc_state_resp_headers_realloc(khc* khc) {
  void* newBuff = realloc(khc->_resp_header_buffer, khc->_resp_header_buffer_size + RESP_HEADER_BUFF_SIZE);
  if (newBuff == NULL) {
    free(khc->_resp_header_buffer);
    khc->_resp_header_buffer = NULL;
    khc->_resp_header_buffer_size = 0;
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_ALLOCATION;
    return;
  }
  // Pointer: last part newly allocated.
  khc->_resp_header_buffer = newBuff;
  khc->_resp_header_buffer_current_pos = newBuff + khc->_resp_header_read_size;
  khc->_resp_header_buffer_size += RESP_HEADER_BUFF_SIZE;
  size_t remain = khc->_resp_header_buffer_size - khc->_resp_header_read_size;
  memset(khc->_resp_header_buffer_current_pos, '\0', remain);
  khc->_state = KHC_STATE_RESP_HEADERS_READ;
  return;
}

void khc_state_resp_headers_read(khc* khc) {
  size_t read_size = 0;
  size_t read_req_size = RESP_HEADER_BUFF_SIZE - 1;
  khc_sock_code_t read_res = 
    khc->_cb_sock_recv(khc->_sock_ctx_recv, khc->_resp_header_buffer_current_pos, read_req_size, &read_size);
  if (read_res == KHC_SOCK_OK) {
    khc->_resp_header_read_size += read_size;
    if (read_size == 0) {
      khc->_read_end = 1;
    }
    // Search boundary for whole buffer.
    char* boundary = strstr(khc->_resp_header_buffer, "\r\n\r\n");
    if (boundary == NULL) {
      // Not reached to end of headers.
      khc->_state = KHC_STATE_RESP_HEADERS_REALLOC;
      return;
    } else {
      khc->_body_boundary = boundary;
      khc->_state = KHC_STATE_RESP_STATUS_PARSE;
      khc->_cb_header_remaining_size = khc->_resp_header_buffer_size;
      khc->_cb_header_pos = khc->_resp_header_buffer;
      return;
    }
  }
  if (read_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (read_res == KHC_SOCK_FAIL) {
    free(khc->_resp_header_buffer);
    khc->_resp_header_buffer = NULL;
    khc->_resp_header_buffer_size = 0;
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_RECV;
    return;
  }
}

void khc_state_resp_status_parse(khc* khc) {
  const char http_version[] = "HTTP/d.d ";
  char* ptr = khc->_resp_header_buffer + strlen(http_version);

  int status_code = 0;
  for (int i = 0; i < 3; ++i) {
    char d = ptr[i];
    if (isdigit((int)d) == 0){
      khc->_state = KHC_STATE_CLOSE;
      khc->_result = KHC_ERR_FAIL;  
      return;
    }
    status_code = status_code * 10 + (d - '0');
  }
  khc->_status_code = status_code;
  khc->_state = KHC_STATE_RESP_HEADERS_CALLBACK;
  return;
}

void khc_state_resp_headers_callback(khc* khc) {
  char* header_boundary = strstr(khc->_cb_header_pos, "\r\n");
  size_t header_size = header_boundary - khc->_cb_header_pos;
  size_t header_written = 
    khc->_cb_header(khc->_cb_header_pos, 1, header_size, khc->_header_data);
  if (header_written != header_size) { // Error in callback function.
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_HEADER_CALLBACK;
    free(khc->_resp_header_buffer);
    khc->_resp_header_buffer = NULL;
    khc->_resp_header_buffer_size = 0;
    return;
  }
  if (header_boundary < khc->_body_boundary) {
    khc->_cb_header_pos = header_boundary + 2; // +2 : Skip CRLF
    khc->_cb_header_remaining_size = khc->_cb_header_remaining_size - header_size - 2;
    return;
  } else { // Callback called for all headers.
    // Check if body is included in the buffer.
    size_t header_size = khc->_body_boundary - khc->_resp_header_buffer;
    size_t body_size = khc->_resp_header_read_size - header_size - 4;
    if (body_size > 0) {
      khc->_body_flagment = khc->_body_boundary + 4;
      khc->_body_flagment_size = body_size;
      khc->_state = KHC_STATE_RESP_BODY_FLAGMENT;
      return;
    } else {
      free(khc->_resp_header_buffer);
      khc->_resp_header_buffer = NULL;
      khc->_resp_header_buffer_size = 0;
      if (khc->_read_end == 1) {
        khc->_state = KHC_STATE_CLOSE;
        return;
      } else {
        khc->_state = KHC_STATE_RESP_BODY_READ;
        return;
      }
    }
  }
}

void khc_state_resp_body_flagment(khc* khc) {
  size_t written = 
    khc->_cb_write(khc->_body_flagment, 1, khc->_body_flagment_size, khc->_write_data);
  free(khc->_resp_header_buffer);
  khc->_resp_header_buffer = NULL;
  khc->_resp_header_buffer_size = 0;
  if (written != khc->_body_flagment_size) { // Error in write callback.
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_WRITE_CALLBACK;
    return;
  }
  if (khc->_read_end == 1) {
    khc->_state = KHC_STATE_CLOSE;
    return;
  } else {
    khc->_state = KHC_STATE_RESP_BODY_READ;
    return;
  }
}

void khc_state_resp_body_read(khc* khc) {
  size_t read_size = 0;
  khc_sock_code_t read_res = 
    khc->_cb_sock_recv(khc->_sock_ctx_recv, khc->_stream_buff, khc->_stream_buff_size, &read_size);
  if (read_res == KHC_SOCK_OK) {
    khc->_body_read_size = read_size;
    if (read_size == 0) {
      khc->_read_end = 1;
      khc->_state = KHC_STATE_CLOSE;
      return;
    }
    khc->_state = KHC_STATE_RESP_BODY_CALLBACK;
    return;
  }
  if (read_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (read_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_SOCK_RECV;
    return;
  }
}

void khc_state_resp_body_callback(khc* khc) {
  size_t written = khc->_cb_write(khc->_stream_buff, 1, khc->_body_read_size, khc->_write_data);
  if (written < khc->_body_read_size) { // Error in write callback.
    khc->_state = KHC_STATE_CLOSE;
    khc->_result = KHC_ERR_WRITE_CALLBACK;
    return;
  }
  if (khc->_read_end == 1) {
    khc->_state = KHC_STATE_CLOSE;
  } else {
    khc->_state = KHC_STATE_RESP_BODY_READ;
  }
  return;
}

void khc_state_close(khc* khc) {
  if (khc->_stream_buff_allocated == 1) {
    free(khc->_stream_buff);
    khc->_stream_buff = NULL;
    khc->_stream_buff_size = 0;
    khc->_stream_buff_allocated = 0;
  }
  khc_sock_code_t close_res = khc->_cb_sock_close(khc->_sock_ctx_close);
  if (close_res == KHC_SOCK_OK) {
    khc->_state = KHC_STATE_FINISHED;
    khc->_result = KHC_ERR_OK;
    return;
  }
  if (close_res == KHC_SOCK_AGAIN) {
    return;
  }
  if (close_res == KHC_SOCK_FAIL) {
    khc->_state = KHC_STATE_FINISHED;
    khc->_result = KHC_ERR_SOCK_CLOSE;
    return;
  }
}

const KHC_STATE_HANDLER state_handlers[] = {
  khc_state_idle,
  khc_state_connect,
  khc_state_req_line,
  khc_state_req_header,
  khc_state_req_header_send,
  khc_state_req_header_send_crlf,
  khc_state_req_header_end,
  khc_state_req_body_read,
  khc_state_req_body_send,
  khc_state_resp_headers_alloc,
  khc_state_resp_headers_realloc,
  khc_state_resp_headers_read,
  khc_state_resp_status_parse,
  khc_state_resp_headers_callback,
  khc_state_resp_body_flagment,
  khc_state_resp_body_read,
  khc_state_resp_body_callback,
  khc_state_close
};
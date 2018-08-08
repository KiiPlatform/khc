#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kii_http.h"
#include "kii_socket_callback.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  return 0;
}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream) {
  return 0;
}

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
  return 0;
}

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
  kii_http->state = IDLE;
  if (kii_http->host == NULL) {
    return KII_NG;
  }
  if (kii_http->method == NULL) {
    return KII_NG;
  }
  memset(kii_http->read_buffer, '\0', READ_REQ_BUFFER_SIZE);
  kii_http->state = CONNECT;
  kii_slist* curr = kii_http->reaquest_headers;
  kii_http->resp_header_buffer = NULL;
  kii_http->resp_header_buffer_size = 0;
  kii_http->read_end = 0;
  while(1) {
    if (kii_http->state == CONNECT) {
      kii_socket_code_t con_res = kii_http->sc_connect_cb(kii_http->socket_context, kii_http->host, 8080);
      if (con_res == KII_SOCKETC_OK) {
        kii_http->state = REQUEST_LINE;
        continue;
      }
      if (con_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (con_res == KII_SOCKETC_FAIL) {
        return KII_NG;
      }
    }
    if (kii_http->state == REQUEST_LINE) {
      char request_line[64];
      request_line[0] = '\0';
      strcat(request_line, kii_http->method);
      strcat(request_line, " ");
      char http_version[] = "HTTP 1.0\r\n";
      strcat(request_line, http_version);
      kii_socket_code_t send_res = kii_http->sc_send_cb(kii_http->socket_context, request_line, strlen(request_line));
      if (send_res == KII_SOCKETC_OK) {
        kii_http->state = REQUEST_HEADERS;
        continue;
      }
      if (send_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (send_res == KII_SOCKETC_FAIL) {
        kii_http->state = CLOSE_AFTER_FAILURE;
        continue;
      }
    }
    if (kii_http->state == REQUEST_HEADERS) {
      int aborted = 0;
      while(curr != NULL) {
        size_t len = strlen(curr->data);
        char line[len+5];
        if (line == NULL) {
          kii_http->state = CLOSE_AFTER_FAILURE;
        }
        line[0] = '\0';
        strcat(line, curr->data);
        strcat(line, "\r\n");
        len = len + 2;
        if (curr->next == NULL) {
          strcat(line, "\r\n");
          len = len + 2;
        }
        kii_socket_code_t send_res = kii_http->sc_send_cb(kii_http->socket_context, line, len);
        if (send_res == KII_SOCKETC_OK) {
          curr = curr->next;
          continue;
        }
        if (send_res == KII_SOCKETC_AGAIN) {
          continue;
        }
        if (send_res == KII_SOCKETC_FAIL) {
          aborted = 1;
          break;
        }
      }
      if (aborted == 1) {
        kii_http->state = CLOSE_AFTER_FAILURE;
        continue;
      }
      kii_http->state = REQUEST_BODY_READ;
      kii_http->read_request_end = 0;
      continue;
    }
    if (kii_http->state == REQUEST_BODY_READ) {
      kii_http->read_size = kii_http->read_callback(kii_http->read_buffer, 1, READ_BODY_SIZE, kii_http->read_data);
      // TODO: handle read failure. let return signed value?
      kii_http->state = REQUEST_BODY_SEND;
      if (kii_http->read_size == 0) {
        kii_http->read_request_end = 1;
      }
      continue;
    }
    if (kii_http->state == REQUEST_BODY_SEND) {
      kii_socket_code_t send_res = kii_http->sc_send_cb(kii_http->socket_context, kii_http->read_buffer, kii_http->read_size);
      if (send_res == KII_SOCKETC_OK) {
        if (kii_http->read_request_end == 1) {
          kii_http->state = RESPONSE_HEADERS_ALLOC;
        } else {
          kii_http->state = REQUEST_BODY_READ;
        }
        continue;
      }
      if (send_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (send_res == KII_SOCKETC_FAIL) {
        kii_http->state = CLOSE_AFTER_FAILURE;
        continue;
      }
    }
    if (kii_http->state == RESPONSE_HEADERS_ALLOC) {
      if (kii_http->resp_header_buffer == NULL) {
        kii_http->resp_header_buffer = malloc(READ_RESP_HEADER_SIZE);
        if (kii_http->resp_header_buffer == NULL) {
          kii_http->state = CLOSE_AFTER_FAILURE;
          continue;
        }
        kii_http->resp_header_buffer_size = READ_RESP_HEADER_SIZE;
        kii_http->state = RESPONSE_HEADERS_READ;
        continue;
      } else {
        void* newBuff = realloc(kii_http->resp_header_buffer, kii_http->resp_header_buffer_size + READ_RESP_HEADER_SIZE);
        if (newBuff == NULL) {
          free(kii_http->resp_header_buffer);
          kii_http->resp_header_buffer = NULL;
          kii_http->resp_header_buffer_size = 0;
          kii_http->state = CLOSE_AFTER_FAILURE;
          continue;
        }
        // Pointer: last part newly allocated.
        kii_http->resp_header_buffer = newBuff + kii_http->resp_header_buffer_size;
        kii_http->resp_header_buffer_size = kii_http->resp_header_buffer_size + READ_RESP_HEADER_SIZE;
        kii_http->state = RESPONSE_HEADERS_READ;
        continue;
      }
    }
    if (kii_http->state == RESPONSE_HEADERS_READ) {
      size_t read_size = 0;
      kii_socket_code_t read_res = 
        kii_http->sc_recv_cb(kii_http->socket_context, kii_http->read_buffer, READ_RESP_HEADER_SIZE, &read_size);
      if (read_res == KII_SOCKETC_OK) {
        if (read_size == 0) {
          kii_http->read_end = 1;
        }
        char* start = kii_http->resp_header_buffer + READ_RESP_HEADER_SIZE - kii_http->resp_header_buffer_size;
        // Search boundary for whole buffer.
        char* boundary = strnstr(start, "\r\n\r\n", kii_http->resp_header_buffer_size);
        if (boundary == NULL) {
          // Not reached to end of headers.
          kii_http->state = RESPONSE_HEADERS_ALLOC;
          continue;
        } else {
          kii_http->body_boundary = boundary;
          kii_http->state = RESPONSE_HEADERS_CALLBACK;
          kii_http->resp_header_buffer = start;
          kii_http->current_header = start;
          kii_http->remaining_header_buffer_size = kii_http->resp_header_buffer_size;
          continue;
        }
      }
    }
    if (kii_http->state == RESPONSE_HEADERS_CALLBACK) {
      char* header_boundary = strnstr(kii_http->current_header, "\r\n", kii_http->remaining_header_buffer_size);
      size_t header_size = header_boundary - kii_http->current_header;
      size_t header_written = 
        kii_http->header_callback(kii_http->current_header, 1, header_size, kii_http->header_data);
      if (header_written != header_size) { // Error in callback function.
        kii_http->state = CLOSE_AFTER_FAILURE;
        free(kii_http->resp_header_buffer);
        kii_http->resp_header_buffer = NULL;
        kii_http->resp_header_buffer_size = 0;
        continue;
      }
      if (header_boundary < kii_http->body_boundary) {
        kii_http->current_header = header_boundary + 2;
        kii_http->remaining_header_buffer_size = kii_http->remaining_header_buffer_size - header_size - 2;
        continue;
      } else { // Callback called for all headers.
        // Check if body is included in the buffer.
        size_t body_size = kii_http->remaining_header_buffer_size - (kii_http->body_boundary + 4 - kii_http->current_header);
        if (body_size > 0) {
          kii_http->body_flagment = kii_http->body_boundary + 4;
          kii_http->body_flagment_size = body_size;
          kii_http->state = RESPONSE_BODY_FLAGMENT;
          continue;
        } else {
          free(kii_http->resp_header_buffer);
          kii_http->resp_header_buffer = NULL;
          kii_http->resp_header_buffer_size = 0;
          if (kii_http->read_end == 1) {
            kii_http->state = CLOSE;
            continue;
          } else {
            kii_http->state = RESPONSE_BODY_READ;
            continue;
          }
        }
      }
    }
    if (kii_http->state == RESPONSE_BODY_FLAGMENT) {
      size_t written = 
        kii_http->write_callback(kii_http->body_flagment, 1, kii_http->body_flagment_size, kii_http->read_data);
      if (written != kii_http->body_flagment_size) { // Error in write callback.
        free(kii_http->resp_header_buffer);
        kii_http->resp_header_buffer = NULL;
        kii_http->resp_header_buffer_size = 0;
        kii_http->state = CLOSE_AFTER_FAILURE;
        continue;
      }
      free(kii_http->resp_header_buffer);
      kii_http->resp_header_buffer = NULL;
      kii_http->resp_header_buffer_size = 0;
      if (kii_http->read_end == 1) {
        kii_http->state = CLOSE;
        continue;
      } else {
        kii_http->state = RESPONSE_BODY_READ;
        continue;
      }
    }
    if (kii_http->state == RESPONSE_BODY_READ) {
      size_t read_size = 0;
      kii_socket_code_t read_res = 
        kii_http->sc_recv_cb(kii_http->socket_context, kii_http->body_buffer, READ_BODY_SIZE, &read_size);
      if (read_res == KII_SOCKETC_OK) {
        if (read_size < READ_BODY_SIZE) {
          kii_http->read_end = 1;
        }
        kii_http->body_read_size = read_size;
        kii_http->state = RESPONSE_BODY_CALLBACK;
        continue;
      }
      if (read_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (read_res == KII_SOCKETC_FAIL) {
        kii_http->state = CLOSE_AFTER_FAILURE;
        continue;
      }
    }
    if (kii_http->state == RESPONSE_BODY_CALLBACK) {
      size_t written = kii_http->write_callback(kii_http->body_buffer, 1, kii_http->body_read_size, kii_http->write_data);
      if (written < kii_http->body_read_size) { // Error in write callback.
        kii_http->state = CLOSE_AFTER_FAILURE;
        continue;
      }
      if (kii_http->read_end == 1) {
        kii_http->state = CLOSE;
      } else {
        kii_http->state = RESPONSE_BODY_READ;
        continue;
      }
    }
    if (kii_http->state == CLOSE) {
      kii_socket_code_t close_res = kii_http->sc_close_cb(kii_http->socket_context);
      if (close_res == KII_SOCKETC_OK) {
        kii_http->state = IDLE;
        return KII_OK;
      }
      if (close_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (close_res == KII_SOCKETC_FAIL) {
        kii_http->state = IDLE;
        return KII_NG;
      }
    }
    if (kii_http->state == CLOSE_AFTER_FAILURE) {
      kii_socket_code_t close_res = kii_http->sc_close_cb(kii_http->socket_context);
      if (close_res == KII_SOCKETC_OK) {
        kii_http->state = IDLE;
        return KII_OK;
      }
      if (close_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (close_res == KII_SOCKETC_FAIL) {
        kii_http->state = IDLE;
        return KII_NG;
      }
    }
  }

  return KII_NG;
}
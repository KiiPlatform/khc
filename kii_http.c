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
  memset(kii_http->read_buffer, '\0', READ_BUFFER_SIZE);
  kii_http->state = CONNECT;
  kii_socket_context_t* s_ctx = NULL;
  kii_slist* curr = kii_http->reaquest_headers;
  while(1) {
    if (kii_http->state == CONNECT) {
      s_ctx = malloc(sizeof(kii_socket_context_t));
      if (s_ctx == NULL) {
        return KII_NG;
      }
      kii_socket_code_t con_res = kii_http->sc_connect_cb(s_ctx, kii_http->host, 8080);
      if (con_res == KII_SOCKETC_OK) {
        kii_http->state = REQUEST_LINE;
        continue;
      }
      if (con_res == KII_SOCKETC_AGAIN) {
        continue;
      }
      if (con_res == KII_SOCKETC_FAIL) {
        free(s_ctx);
        return KII_NG;
      }
    }
    if (kii_http->state =- REQUEST_LINE) {
      char* request_line[64];
      request_line[0] = '\0';
      strcat(request_line, kii_http->method);
      strcat(request_line, " ");
      char http_version[] = "HTTP 1.0\r\n";
      strcat(request_line, http_version);
      kii_socket_code_t send_res = kii_http->sc_send_cb(s_ctx, request_line, strlen(request_line));
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
      while(curr != NULL) {
        size_t len = strlen(curr->data);
        char* line = malloc(len+5);
        if (line == NULL) {
          kii_http->state = CLOSE;
          return KII_NG;
        }
        line = '\0';
        line = strcat(line, curr->data);
        line = strcat(line, "\r\n");
        len = len + 2;
        line[len-1] = '\0';
        if (curr->next == NULL) {
          line = strcat(line, "\r\n");
          len = len + 2;
          line[len-1] = '\0';
        }
        kii_socket_code_t send_res = kii_http->sc_send_cb(s_ctx, line, len);
        free(line);
        if (send_res == KII_SOCKETC_OK) {
          curr = curr->next;
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
      kii_http->state = REQUEST_BODY;
      kii_http->read_buffer_need_resend = 0;
      continue;
    }
    if (kii_http->state == REQUEST_BODY) {
      if (kii_http->read_buffer_need_resend == 0) {
        size_t read_size = kii_http->read_callback(kii_http->read_buffer, 1, 1024, kii_http->read_data);
        if (read_size > 0) {
          kii_socket_code_t send_res = kii_http->sc_send_cb(s_ctx, kii_http->read_buffer, read_size);
          if (send_res == KII_SOCKETC_OK) {
            kii_http->read_buffer_need_resend = 0;
            continue;
          }
          if (send_res == KII_SOCKETC_AGAIN) {
            kii_http->read_buffer_need_resend = 1;
            kii_http->read_size = read_size;
            continue;
          }
          if (send_res == KII_SOCKETC_FAIL) {
            kii_http->state = CLOSE_AFTER_FAILURE;
            kii_http->read_buffer_need_resend = 0;
            continue;
          }
        }
        if (read_size == 0) {
          // Have READ the whole request.
          kii_http->state = RESPONSE_HEADERS;
          continue;
        }
        if (read_size < 0) {
          // Failed to read.
          kii_http->state = CLOSE_AFTER_FAILURE;
          continue;
        }
      } else { // In case Non-Blocking socket requires resend the buffer.
        kii_socket_code_t send_res = kii_http->sc_send_cb(s_ctx, kii_http->read_buffer, kii_http->read_size);
        if (send_res == KII_SOCKETC_OK) {
          kii_http->read_buffer_need_resend = 0;
          continue;
        }
        if (send_res == KII_SOCKETC_AGAIN) {
          kii_http->read_buffer_need_resend = 1;
          continue;
        }
        if (send_res == KII_SOCKETC_FAIL) {
          kii_http->state = CLOSE_AFTER_FAILURE;
          kii_http->read_buffer_need_resend = 0;
          continue;
        }
      }
    }
  }


  return KII_NG;
}
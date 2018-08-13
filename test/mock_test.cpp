#define CATCH_CONFIG_MAIN

#include <string.h>
#include "catch.hpp"
#include "kii_http.h"


typedef struct sock_ctx {
  std::function<void(void* socket_context, const char* host, unsigned int port)> on_connect;
} sock_ctx;

kii_socket_code_t cb_connect(void* socket_context, const char* host, unsigned int port) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  ctx->on_connect(socket_context, host, port);
  return KII_SOCKETC_OK;
}

kii_socket_code_t cb_send (void* socket_context, const char* buffer, size_t length) {
  return KII_SOCKETC_OK;
}

kii_socket_code_t cb_recv(void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length) {
  return KII_SOCKETC_OK;
}

kii_socket_code_t cb_close(void* socket_context) {
  return KII_SOCKETC_OK;
}

size_t cb_write(char *ptr, size_t size, size_t count, void *userdata) {
  return 0;
}

size_t cb_read(char *buffer, size_t size, size_t count, void *userdata) {
  return 0;
}
size_t cb_header (char *buffer, size_t size, size_t count, void *userdata) {
  return 0;
}

TEST_CASE( "slist append (1)" ) {
  kii_http http;
  http.host = (char*)"api.kii.com";
  http.method = (char*)"GET";
  http.path = (char*)"/api/apps";
  http.sc_connect_cb = cb_connect;
  http.sc_send_cb = cb_send;
  http.sc_recv_cb = cb_recv;
  http.sc_close_cb = cb_close;
  
  sock_ctx s_ctx;
  http.socket_context = &s_ctx;


  s_ctx.on_connect = [=](void* socket_context, const char* host, unsigned int port) {
    REQUIRE( strncmp(host, "api.kii.com", strlen("api.kii.com")) == 0 );
  };
  
  http.read_callback = cb_read;
  http.write_callback = cb_write;
  http.header_callback = cb_header;  
  
  kii_http_perform(&http);
}

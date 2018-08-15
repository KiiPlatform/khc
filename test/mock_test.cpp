#define CATCH_CONFIG_MAIN

#include <string.h>
#include "catch.hpp"
#include "kii_http.h"
#include "../src/kii_http_impl.h"


typedef struct sock_ctx {
  std::function<kii_socket_code_t(void* socket_context, const char* host, unsigned int port)> on_connect;
  std::function<kii_socket_code_t(void* socket_context, const char* buffer, size_t length)> on_send;
} sock_ctx;

kii_socket_code_t cb_connect(void* socket_context, const char* host, unsigned int port) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_connect(socket_context, host, port);
}

kii_socket_code_t cb_send (void* socket_context, const char* buffer, size_t length) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_send(socket_context, buffer, length);
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

TEST_CASE( "Http Test" ) {
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
    REQUIRE( strlen(host) == strlen("api.kii.com") );
    REQUIRE( port == 8080 );
    return KII_SOCKETC_OK;
  };
  
  http.read_callback = cb_read;
  http.write_callback = cb_write;
  http.header_callback = cb_header;  
  
  kii_state_idle(&http);
  REQUIRE( http.state == CONNECT );
  REQUIRE( http.result == KIIE_OK );

  kii_state_connect(&http);
  REQUIRE( http.state == REQUEST_LINE );
  REQUIRE( http.result == KIIE_OK );

  s_ctx.on_send = [=](void* socket_context, const char* buffer, size_t length) {
    const char req_line[] = "GET https://api.kii.com/api/apps HTTP 1.0\r\n";
    REQUIRE( length == strlen(req_line) );
    REQUIRE( strncmp(buffer, req_line, length) == 0 );
    return KII_SOCKETC_OK;
  };

  kii_state_request_line(&http);
  REQUIRE( http.state == REQUEST_HEADER );
  REQUIRE( http.result == KIIE_OK );

  kii_state_request_header(&http);
  REQUIRE( http.state == REQUEST_HEADER_END );
  REQUIRE( http.result == KIIE_OK );
}

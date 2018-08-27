#define CATCH_CONFIG_MAIN

#include <string.h>
#include "catch.hpp"
#include <kii_http.h>
#include "kii_http_impl.h"
#include "http_test.h"
#include <sstream>

typedef struct sock_ctx {
  std::function<kii_sock_code_t(void* socket_context, const char* host, unsigned int port)> on_connect;
  std::function<kii_sock_code_t(void* socket_context, const char* buffer, size_t length)> on_send;
  std::function<kii_sock_code_t(void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length)> on_recv;
  std::function<kii_sock_code_t(void* socket_context)> on_close;
} sock_ctx;

typedef struct io_ctx {
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_read;
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_header;
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_write;
} io_ctx;

kii_sock_code_t cb_connect(void* socket_context, const char* host, unsigned int port) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_connect(socket_context, host, port);
}

kii_sock_code_t cb_send (void* socket_context, const char* buffer, size_t length) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_send(socket_context, buffer, length);
}

kii_sock_code_t cb_recv(void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_recv(socket_context, buffer, length_to_read, out_actual_length);
}

kii_sock_code_t cb_close(void* socket_context) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_close(socket_context);
}

size_t cb_write(char *buffer, size_t size, size_t count, void *userdata) {
  io_ctx* ctx = (io_ctx*)(userdata);
  return ctx->on_write(buffer, size, count, userdata);
}

size_t cb_read(char *buffer, size_t size, size_t count, void *userdata) {
  io_ctx* ctx = (io_ctx*)(userdata);
  return ctx->on_read(buffer, size, count, userdata);
}

size_t cb_header(char *buffer, size_t size, size_t count, void *userdata) {
  io_ctx* ctx = (io_ctx*)(userdata);
  return ctx->on_header(buffer, size, count, userdata);
}

TEST_CASE( "HTTP minimal" ) {
  kii_http http;

  http_test::Resp resp;
  resp.headers = { "HTTP/1.0 200 OK" };

  kii_http_set_param(&http, KII_PARAM_HOST, (char*)"api.kii.com");
  kii_http_set_param(&http, KII_PARAM_METHOD, (char*)"GET");
  kii_http_set_param(&http, KII_PARAM_PATH, (char*)"/api/apps");
  kii_http_set_param(&http, KII_PARAM_REQ_HEADERS, NULL);

  sock_ctx s_ctx;
  kii_http_set_cb_sock_connect(&http, cb_connect, &s_ctx);
  kii_http_set_cb_sock_send(&http, cb_send, &s_ctx);
  kii_http_set_cb_sock_recv(&http, cb_recv, &s_ctx);
  kii_http_set_cb_sock_close(&http, cb_close, &s_ctx);

  io_ctx io_ctx;
  kii_http_set_cb_read(&http, cb_read, &io_ctx);
  kii_http_set_cb_write(&http, cb_write, &io_ctx);
  kii_http_set_cb_header(&http, cb_header, &io_ctx);

  kii_state_idle(&http);
  REQUIRE( http._state == KII_STATE_CONNECT );
  REQUIRE( http._result == KII_ERR_OK );

  bool called = false;
  s_ctx.on_connect = [=, &called](void* socket_context, const char* host, unsigned int port) {
    called = true;
    REQUIRE( strncmp(host, "api.kii.com", strlen("api.kii.com")) == 0 );
    REQUIRE( strlen(host) == strlen("api.kii.com") );
    REQUIRE( port == 443 );
    return KIISOCK_OK;
  };

  kii_state_connect(&http);
  REQUIRE( http._state == KII_STATE_REQ_LINE );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( called );

  called = false;
  s_ctx.on_send = [=, &called](void* socket_context, const char* buffer, size_t length) {
    called = true;
    const char req_line[] = "GET https://api.kii.com/api/apps HTTP/1.0\r\n";
    REQUIRE( length == strlen(req_line) );
    REQUIRE( strncmp(buffer, req_line, length) == 0 );
    return KIISOCK_OK;
  };

  kii_state_req_line(&http);
  REQUIRE( http._state == KII_STATE_REQ_HEADER );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( called );

  kii_state_req_header(&http);
  REQUIRE( http._state == KII_STATE_REQ_HEADER_END );
  REQUIRE( http._result == KII_ERR_OK );

  called = false;
  s_ctx.on_send = [=, &called](void* socket_context, const char* buffer, size_t length) {
    called = true;
    REQUIRE( length == 2 );
    REQUIRE( strncmp(buffer, "\r\n", 2) == 0 );
    return KIISOCK_OK;
  };

  kii_state_req_header_end(&http);
  REQUIRE( http._state == KII_STATE_REQ_BODY_READ );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( called );

  called = false;
  io_ctx.on_read = [=, &called](char *buffer, size_t size, size_t count, void *userdata) {
    called = true;
    REQUIRE( size == 1);
    REQUIRE( count == 1024);
    const char body[] = "http body";
    strncpy(buffer, body, strlen(body));
    return strlen(body);
  };
  kii_state_req_body_read(&http);
  REQUIRE( http._state == KII_STATE_REQ_BODY_SEND );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( http._read_req_end == 1 );
  REQUIRE( called );

  called = false;
  s_ctx.on_send = [=, &called](void* socket_context, const char* buffer, size_t length) {
    called = true;
    const char body[] = "http body";
    REQUIRE( length == strlen(body) );
    REQUIRE( strncmp(buffer, body, length) == 0 );
    return KIISOCK_OK;
  };
  kii_state_req_body_send(&http);
  REQUIRE( http._state == KII_STATE_RESP_HEADERS_ALLOC );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( called );

  kii_state_resp_headers_alloc(&http);
  REQUIRE( http._state == KII_STATE_RESP_HEADERS_READ );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( *http._resp_header_buffer == '\0' );
  REQUIRE( http._resp_header_buffer == http._resp_header_buffer_current_pos );
  REQUIRE (http._resp_header_buffer_size == 1024 );

  called = false;
  s_ctx.on_recv = [=, &called, &resp](void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length) {
    called = true;
    REQUIRE( length_to_read == 1023 );
    *out_actual_length = resp.to_istream().readsome(buffer, length_to_read);
    return KIISOCK_OK;
  };

  kii_state_resp_headers_read(&http);
  REQUIRE( http._state == KII_STATE_RESP_HEADERS_CALLBACK );
  REQUIRE( http._read_end == 1 );
  REQUIRE( http._result == KII_ERR_OK );
  char buffer[1024];
  size_t len = resp.to_istream().readsome((char*)&buffer, 1023);
  REQUIRE( http._resp_header_read_size == len );
  REQUIRE( called );

  called = false;
  io_ctx.on_header = [=, &called, &resp](char *buffer, size_t size, size_t count, void *userdata) {
    called = true;
    const char* status_line = resp.headers[0].c_str();
    size_t len = strlen(status_line);
    REQUIRE( size == 1);
    REQUIRE( count == len );
    REQUIRE( strncmp(buffer, status_line, len) == 0 );
    return size * count;
  };

  kii_state_resp_headers_callback(&http);
  REQUIRE( http._state == KII_STATE_CLOSE );
  REQUIRE( http._read_end == 1 );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( called );

  called = false;
  s_ctx.on_close = [=, &called](void* socket_ctx) {
    called = true;
    return KIISOCK_OK;
  };
  kii_state_close(&http);
  REQUIRE( http._state == KII_STATE_FINISHED );
  REQUIRE( http._result == KII_ERR_OK );
  REQUIRE( called );
}

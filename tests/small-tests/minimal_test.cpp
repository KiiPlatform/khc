#define CATCH_CONFIG_MAIN

#include <string.h>
#include "catch.hpp"
#include <kch.h>
#include "kch_impl.h"
#include "http_test.h"
#include "test_callbacks.h"
#include <sstream>

TEST_CASE( "HTTP minimal" ) {
  kch http;

  http_test::Resp resp;
  resp.headers = { "HTTP/1.0 200 OK" };

  kch_set_param(&http, KII_PARAM_HOST, (char*)"api.kii.com");
  kch_set_param(&http, KII_PARAM_METHOD, (char*)"GET");
  kch_set_param(&http, KII_PARAM_PATH, (char*)"/api/apps");
  kch_set_param(&http, KII_PARAM_REQ_HEADERS, NULL);

  sock_ctx s_ctx;
  kch_set_cb_sock_connect(&http, cb_connect, &s_ctx);
  kch_set_cb_sock_send(&http, cb_send, &s_ctx);
  kch_set_cb_sock_recv(&http, cb_recv, &s_ctx);
  kch_set_cb_sock_close(&http, cb_close, &s_ctx);

  io_ctx io_ctx;
  kch_set_cb_read(&http, cb_read, &io_ctx);
  kch_set_cb_write(&http, cb_write, &io_ctx);
  kch_set_cb_header(&http, cb_header, &io_ctx);

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
    *out_actual_length = resp.to_istringstream().read(buffer, length_to_read).gcount();
    return KIISOCK_OK;
  };

  kii_state_resp_headers_read(&http);
  REQUIRE( http._state == KII_STATE_RESP_HEADERS_CALLBACK );
  REQUIRE( http._read_end == 1 );
  REQUIRE( http._result == KII_ERR_OK );
  char buffer[1024];
  size_t len = resp.to_istringstream().read((char*)&buffer, 1023).gcount();
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

#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <kii_http.h>
#include "secure_socket_impl.h"
#include "kii_http_impl.h"

typedef struct io_ctx {
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_read;
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_header;
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_write;
} io_ctx;

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

TEST_CASE( "HTTP Get" ) {
  kii_http http;

  kii_http_set_param(&http, KII_PARAM_HOST, (char*)"api-jp.kii.com");
  kii_http_set_param(&http, KII_PARAM_METHOD, (char*)"GET");
  kii_http_set_param(&http, KII_PARAM_PATH, (char*)"/api/apps");
  kii_http_set_param(&http, KII_PARAM_REQ_HEADERS, NULL);

  ssl_context_t s_ctx;
  kii_http_set_cb_sock_connect(&http, s_connect_cb, &s_ctx);
  kii_http_set_cb_sock_send(&http, s_send_cb, &s_ctx);
  kii_http_set_cb_sock_recv(&http, s_recv_cb, &s_ctx);
  kii_http_set_cb_sock_close(&http, s_close_cb, &s_ctx);

  io_ctx io_ctx;
  kii_http_set_cb_read(&http, cb_read, &io_ctx);
  kii_http_set_cb_write(&http, cb_write, &io_ctx);
  kii_http_set_cb_header(&http, cb_header, &io_ctx);

  io_ctx.on_read = [=](char *buffer, size_t size, size_t count, void *userdata) {
    // No req body.
    return 0;
  };

  io_ctx.on_header = [=](char *buffer, size_t size, size_t count, void *userdata) {
    // Ignore resp headers.
    return size * count;
  };

  io_ctx.on_write = [=](char *buffer, size_t size, size_t count, void *userdata) {
    REQUIRE ( strncmp(buffer, "{}", 2) );
    REQUIRE ( size == 1);
    REQUIRE ( count == 2 );
    return size * count;
  };

  kii_state_idle(&http);
  REQUIRE( http._state == KII_STATE_CONNECT );
  REQUIRE( http._result == KII_ERR_OK );

  kii_state_connect(&http);
  REQUIRE( http._state == KII_STATE_REQ_LINE );
  REQUIRE( http._result == KII_ERR_OK );

  kii_state_req_line(&http);
  REQUIRE( http._state == KII_STATE_REQ_HEADER );
  REQUIRE( http._result == KII_ERR_OK );

  kii_state_req_header(&http);
  REQUIRE( http._state == KII_STATE_REQ_HEADER_END );
  REQUIRE( http._result == KII_ERR_OK );

  kii_state_req_header_end(&http);
  REQUIRE( http._state == KII_STATE_REQ_BODY_READ );
  REQUIRE( http._result == KII_ERR_OK );

  kii_state_req_body_read(&http);
  REQUIRE( http._state == KII_STATE_RESP_HEADERS_ALLOC );
  REQUIRE( http._result == KII_ERR_OK );

  kii_state_resp_headers_alloc(&http);
  REQUIRE( http._state == KII_STATE_RESP_HEADERS_READ );
  REQUIRE( http._result == KII_ERR_OK );
  // kii_http_code ret = kii_http_perform(&http);
  // REQUIRE( ret == KII_ERR_OK );

}
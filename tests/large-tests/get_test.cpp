#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <kch.h>
#include "secure_socket_impl.h"
#include "kch_impl.h"

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
  kch http;

  kch_set_param(&http, KII_PARAM_HOST, (char*)"api-jp.kii.com");
  kch_set_param(&http, KII_PARAM_METHOD, (char*)"GET");
  kch_set_param(&http, KII_PARAM_PATH, (char*)"");
  kch_set_param(&http, KII_PARAM_REQ_HEADERS, NULL);

  ssl_context_t s_ctx;
  kch_set_cb_sock_connect(&http, s_connect_cb, &s_ctx);
  kch_set_cb_sock_send(&http, s_send_cb, &s_ctx);
  kch_set_cb_sock_recv(&http, s_recv_cb, &s_ctx);
  kch_set_cb_sock_close(&http, s_close_cb, &s_ctx);

  io_ctx io_ctx;
  kch_set_cb_read(&http, cb_read, &io_ctx);
  kch_set_cb_write(&http, cb_write, &io_ctx);
  kch_set_cb_header(&http, cb_header, &io_ctx);

  int on_read_called = 0;
  io_ctx.on_read = [=, &on_read_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_read_called;
    // No req body.
    return 0;
  };

  int on_header_called = 0;
  io_ctx.on_header = [=, &on_header_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_header_called;
    // Ignore resp headers.
    char str[size*count + 1];
    strncpy(str, buffer, size*count);
    str[size*count] = '\0';
    printf("%s\n", str);
    return size * count;
  };

  int on_write_called = 0;
  io_ctx.on_write = [=, &on_write_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_write_called;
    REQUIRE ( size == 1);
    REQUIRE ( count == 2 );
    REQUIRE ( strncmp(buffer, "{}", 2) == 0 );
    return size * count;
  };

  kch_code res = kch_perform(&http);
  REQUIRE( res == KII_ERR_OK );
  REQUIRE( on_read_called == 1 );
  REQUIRE( on_header_called > 1 );
  REQUIRE( on_write_called == 1 );
}
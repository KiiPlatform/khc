#include "catch.hpp"
#include <khc.h>
#include "http_test.h"
#include "test_callbacks.h"
#include <fstream>
#include <string.h>

TEST_CASE( "HTTP response test" ) {
  khc http;
  khc_set_zero(&http);
  const size_t buff_size = DEFAULT_STREAM_BUFF_SIZE;

  ifstream ifs;
  ifs.open("./data/resp-login.txt");

  khct::http::Resp resp(ifs);

  ifs.close();

  khc_set_host(&http, "api.kii.com");
  khc_set_method(&http, "GET");
  khc_set_path(&http, "/api/apps");
  khc_set_req_headers(&http, NULL);

  khct::cb::SockCtx s_ctx;
  khc_set_cb_sock_connect(&http, khct::cb::mock_connect, &s_ctx);
  khc_set_cb_sock_send(&http, khct::cb::mock_send, &s_ctx);
  khc_set_cb_sock_recv(&http, khct::cb::mock_recv, &s_ctx);
  khc_set_cb_sock_close(&http, khct::cb::mock_close, &s_ctx);

  khct::cb::IOCtx io_ctx;
  khc_set_cb_read(&http, khct::cb::cb_read, &io_ctx);
  khc_set_cb_write(&http, khct::cb::cb_write, &io_ctx);
  khc_set_cb_header(&http, khct::cb::cb_header, &io_ctx);

  int on_connect_called = 0;
  s_ctx.on_connect = [=, &on_connect_called](void* socket_context, const char* host, unsigned int port) {
    ++on_connect_called;
    REQUIRE( strncmp(host, "api.kii.com", strlen("api.kii.com")) == 0 );
    REQUIRE( strlen(host) == strlen("api.kii.com") );
    REQUIRE( port == 443 );
    return KHC_SOCK_OK;
  };

  int on_send_called = 0;
  s_ctx.on_send = [=, &on_send_called](void* socket_context, const char* buffer, size_t length) {
    ++on_send_called;
    return KHC_SOCK_OK;
  };

  int on_read_called = 0;
  io_ctx.on_read = [=, &on_read_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_read_called;
    REQUIRE( size == 1);
    REQUIRE( count == buff_size);
    return 0;
  };

  int on_recv_called = 0;
  auto is = resp.to_istringstream();
  s_ctx.on_recv = [=, &on_recv_called, &resp, &is](void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length) {
    ++on_recv_called;
    if (on_recv_called == 1)
      REQUIRE( length_to_read == 1023 );
    if (on_recv_called == 2)
      REQUIRE( length_to_read == 1024 );
    *out_actual_length = is.read(buffer, length_to_read).gcount();
    return KHC_SOCK_OK;
  };

  int on_header_called = 0;
  io_ctx.on_header = [=, &on_header_called, &resp](char *buffer, size_t size, size_t count, void *userdata) {
    const char* header = resp.headers[on_header_called].c_str();
    size_t len = strlen(header);
    REQUIRE( size == 1);
    REQUIRE( count == len );
    REQUIRE( strncmp(buffer, header, len) == 0 );
    ++on_header_called;
    return size * count;
  };

  istringstream iss = istringstream(resp.body);
  int on_write_called = 0;
  io_ctx.on_write = [=, &iss, &on_write_called](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_write_called;
    char body_buffer[size * count];
    memset(body_buffer, '\0', size * count);
    iss.read((char*)body_buffer, size * count);
    REQUIRE ( strncmp(buffer, body_buffer, size * count) == 0 );
    return size * count;
  };

  int on_close_called = 0;
  s_ctx.on_close = [=, &on_close_called](void* socket_ctx) {
    ++on_close_called;
    return KHC_SOCK_OK;
  };

  khc_code res = khc_perform(&http);
  REQUIRE( res == KHC_ERR_OK );
  REQUIRE( khc_get_status_code(&http) == 200 );
  REQUIRE( on_connect_called == 1 );
  REQUIRE( on_send_called == 2 );
  REQUIRE( on_read_called == 1 );
  REQUIRE( on_recv_called == 2 );
  REQUIRE( on_header_called == 12 );
  REQUIRE( on_write_called == 1 );
  REQUIRE( on_close_called == 1 );
}
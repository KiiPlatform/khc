#include "catch.hpp"
#include <khc.h>
#include "secure_socket_impl.h"
#include "khc_impl.h"
#include "test_callbacks.h"
#include "picojson.h"
#include <sstream>

TEST_CASE( "HTTP Post" ) {
  khc http;
  khc_set_zero(&http);

  std::string app_id = "1ud4iv020xa8";
  std::string username = "pass-1234";
  std::string password = "1234";
  std::string path = "/api/apps/" + app_id + "/oauth2/token";
  std::string host = "api-jp.kii.com";
  std::string x_kii_appid = "X-Kii-Appid: " + app_id;
  std::string x_kii_appkey = "X-Kii-Appkey: dummy";
  std::string content_type = "Content-Type: application/vnd.kii.OauthTokenRequest+json";

  khc_set_host(&http, host.c_str());
  khc_set_method(&http, "POST");
  khc_set_path(&http, path.c_str());

  // Prepare Req Body.
  picojson::object o;
  o.insert(std::make_pair("username", username));
  o.insert(std::make_pair("password", password));
  o.insert(std::make_pair("grant_type", "password"));
  picojson::value v(o);
  std::string req_body = v.serialize();
  size_t body_len = req_body.length();

  // Prepare Req Headers.
  khc_slist* headers = NULL;
  headers = khc_slist_append(headers, x_kii_appid.c_str(), x_kii_appid.length());
  std::string content_length = "Content-Length: " + std::to_string(body_len);
  headers = khc_slist_append(headers, content_length.c_str(), content_length.length());
  headers = khc_slist_append(headers, content_type.c_str(), content_type.length());
  headers = khc_slist_append(headers, x_kii_appkey.c_str(), x_kii_appkey.length());

  khc_set_req_headers(&http, headers);

  khct::ssl::SSLData s_ctx;
  khc_set_cb_sock_connect(&http, khct::ssl::cb_connect, &s_ctx);
  khc_set_cb_sock_send(&http, khct::ssl::cb_send, &s_ctx);
  khc_set_cb_sock_recv(&http, khct::ssl::cb_recv, &s_ctx);
  khc_set_cb_sock_close(&http, khct::ssl::cb_close, &s_ctx);

  khct::cb::IOCtx io_ctx;
  khc_set_cb_read(&http, khct::cb::cb_read, &io_ctx);
  khc_set_cb_write(&http, khct::cb::cb_write, &io_ctx);
  khc_set_cb_header(&http, khct::cb::cb_header, &io_ctx);

  int on_read_called = 0;
  io_ctx.on_read = [=, &on_read_called, &req_body](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_read_called;
    memcpy(buffer, req_body.c_str(), body_len);
    return body_len;
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
  std::ostringstream oss;
  io_ctx.on_write = [=, &on_write_called, &oss](char *buffer, size_t size, size_t count, void *userdata) {
    ++on_write_called;
    oss.write(buffer, size * count);
    return size * count;
  };

  khc_code res = khc_perform(&http);
  khc_slist_free_all(headers);
  REQUIRE( khc_get_status_code(&http) == 200 );

  // Parse response body.
  auto resp_body = oss.str();
  auto err_str = picojson::parse(v, resp_body);
  REQUIRE ( err_str.empty() );
  REQUIRE ( v.is<picojson::object>() );
  picojson::object obj = v.get<picojson::object>();
  auto access_token = obj.at("access_token");
  REQUIRE ( access_token.is<std::string>() );
  REQUIRE ( !access_token.get<std::string>().empty() );
  
  REQUIRE( res == KHC_ERR_OK );
  REQUIRE( on_read_called == 1 );
  REQUIRE( on_header_called > 1 );
  REQUIRE( on_write_called == 1 );
}
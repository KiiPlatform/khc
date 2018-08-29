#include "catch.hpp"
#include "http_test.h"
#include <string>
#include <sstream>

TEST_CASE( "temp" ) {
  std::string respStr = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nbody";
  std::istringstream iss(respStr);
  std::istream& is(iss);
  khct::http::Resp resp = khct::http::Resp(is);
  REQUIRE( resp.headers[0] == "HTTP/1.1 200 OK" );
  REQUIRE( resp.headers[1] == "Content-Type: text/plain" );
  REQUIRE( resp.body == "body" );
}

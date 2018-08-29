#ifndef __http_test__
#define __http_test__

using namespace std;

#include <vector>
#include <string>
#include <istream>
#include <sstream>

namespace khct {
namespace http {

std::istream &read_header(std::istream &in, std::string &out);
class Resp;

}

struct khct::http::Resp {
  std::vector<std::string> headers;
  std::string body;
  std::string to_string();
  std::istringstream to_istringstream();
  Resp();
  Resp(std::istream& is);
};
}


#endif
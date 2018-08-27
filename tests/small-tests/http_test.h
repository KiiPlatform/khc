
using namespace std;

#include <vector>
#include <string>
#include <sstream>

using namespace std;

namespace http_test {

struct Resp {
  vector<string> headers;
  string body;
  string to_string();
  istringstream to_istream();
};

string Resp::to_string() {
  ostringstream o;
  for (string h : headers) {
    o << h;
    o << "\r\n";
  }
  o << "\r\n";
  o << body;
  return o.str();
}

istringstream Resp::to_istream() {
  return istringstream(this->to_string());
}

}

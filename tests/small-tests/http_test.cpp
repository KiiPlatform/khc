#include <vector>
#include <string>
#include <istream>
#include <iostream>
#include "http_test.h"

std::istream& http_test::read_header(std::istream &in, std::string &out)
{
  std::cout << "read_header" << std::endl;
  char c;
  while (in.get(c).good())
  {
    if (c == '\r')
    {
      c = in.peek();
      if (in.good())
      {
        if (c == '\n')
        {
          in.ignore();
          break;
        }
      }
    }
    out.append(1, c);
  }
  return in;
}

std::string http_test::Resp::to_string() {
  ostringstream o;
  for (string h : headers) {
    o << h;
    o << "\r\n";
  }
  o << "\r\n";
  o << body;
  return o.str();
}

std::istringstream http_test::Resp::to_istringstream() {
  return std::istringstream(this->to_string());
}

http_test::Resp::Resp() {}

http_test::Resp::Resp(std::istream& is) {
  is.seekg(0, std::ios::end);
  std::streampos length = is.tellg();
  is.seekg(0, std::ios::beg);

  while(is.tellg() < length) {
    std::string header = "";
    read_header(is, header);
    if (header == "") {
      break;
    }
    this->headers.push_back(header);
  }
  // Read body
  char* buffer = new char[1024];
  std::ostringstream os;
  while (is.tellg() < length) {
    size_t len = is.readsome(buffer, 1024);
    os.write(buffer, len);
  }
  delete[] buffer;
  this->body = os.str();
}
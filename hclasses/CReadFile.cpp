#include "CReadFile.h"
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <streambuf>

std::string readFile(const std::string &fileName) {
  std::ifstream t(fileName);
  std::string str;
  if (!t.seekg(0, std::ios::end)) {
    throw(CREADFILE_FILENOTFOUND);
  }
  str.reserve(t.tellg());
  if (!t.seekg(0, std::ios::beg)) {
    throw(CREADFILE_FILENOTFOUND);
  }
  str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  return str;
}

void writeFile(const std::string &fileName, const std::string &buffer) {
  std::ofstream out(fileName);
  if (!out.is_open()) {
    throw CREADFILE_FILENOTWRITE;
  }
  out << buffer;
  out.close();
}
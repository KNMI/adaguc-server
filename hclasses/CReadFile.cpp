#include "CReadFile.h"
#include <filesystem>
#include <fstream>
#include <string>

std::string readFile(const std::string &fileName) {
  std::error_code ec;
  if (std::filesystem::is_directory(fileName, ec)) {
    throw(CREADFILE_FILENOTFOUND);
  }
  std::ifstream file(fileName, std::ios::binary);
  if (!file || !file.seekg(0, std::ios::end)) {
    throw(CREADFILE_FILENOTFOUND);
  }
  std::streamoff size = file.tellg();
  if (size < 0 || !file.seekg(0, std::ios::beg)) {
    throw(CREADFILE_FILENOTFOUND);
  }
  // Read the whole file in one go instead of character by character
  std::string str(size, '\0');
  file.read(str.data(), size);
  str.resize(file.gcount());
  // Pick up anything beyond the reported size, in case the file grew while reading
  char buffer[65536];
  while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
    str.append(buffer, file.gcount());
  }
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
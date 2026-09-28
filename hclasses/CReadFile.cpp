#include "CReadFile.h"
#include <filesystem>
#include <fstream>
#include <string>

std::string readFile(const std::string &fileName) {
  std::error_code ec;
  // Only read regular files: directories, FIFOs and device files open fine but report a wrong size
  if (!std::filesystem::is_regular_file(fileName, ec)) {
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
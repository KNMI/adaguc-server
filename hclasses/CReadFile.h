#ifndef CREADFILE_H
#define CREADFILE_H
#include <string>

#define CREADFILE_OK 0
#define CREADFILE_FILENOTFOUND 1
#define CREADFILE_FILENOTWRITE 3

std::string readFile(const std::string &fileName);

void writeFile(const std::string &fileName, const std::string &buffer);

#endif

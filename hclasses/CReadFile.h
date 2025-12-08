#ifndef CREADFILE_H
#define CREADFILE_H
#include "CTString.h"
#define CREADFILE_OK 0
#define CREADFILE_FILENOTFOUND 1
#define CREADFILE_FILENOTREAD 2
#define CREADFILE_FILENOTWRITE 3

class CReadFile {
public:
  /**
   * Opens a file into a CT::string
   * @param fileName The file to open
   * @return CT::string containing the file
   * throws exceptions of type int when something goes wrong.
   */
  static CT::string open(const char *fileName);

  static void write(const char *fileName, const char *buffer, size_t length);
};
#endif

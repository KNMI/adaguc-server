#include "CReadFile.h"
#include <sys/stat.h>

CT::string CReadFile::open(const char *fileName) {
  // printf("--> Reading %s\n", fileName);
  FILE *fp = fopen(fileName, "r");
  if (fp == NULL) {
    throw(CREADFILE_FILENOTFOUND);
  }
  fseek(fp, 0L, SEEK_END);
  size_t size = ftell(fp);

  if (size == 0) {
    fclose(fp);
    return "";
  }
  char *data = new char[size + 1];
  fseek(fp, 0L, SEEK_SET);
  size_t result = fread(data, 1, size, fp);
  if (result != size) {
    delete[] data;
    fclose(fp);
    throw CREADFILE_FILENOTREAD;
  }
  data[size] = '\0';
  CT::string dataString;
  dataString.copy(data, size);
  delete[] data;
  fclose(fp);
  return dataString;
}

void CReadFile::write(const char *fileName, const char *buffer, size_t length) {
  if (buffer == NULL) {
    throw CREADFILE_FILENOTWRITE;
  }
  FILE *pFile = fopen(fileName, "wb");
  if (pFile == NULL) {
    throw CREADFILE_FILENOTWRITE;
  }
  size_t bytesWritten = fwrite(buffer, sizeof(char), length, pFile);
  fflush(pFile);
  fclose(pFile);

  if (bytesWritten != length) {
    throw CREADFILE_FILENOTWRITE;
  }
  if (chmod(fileName, 0777) < 0) {
    throw CREADFILE_FILENOTWRITE;
  }
}

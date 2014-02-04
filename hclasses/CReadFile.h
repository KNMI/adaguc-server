#include "CTypes.h"
#define CREADFILE_OK           0
#define CREADFILE_FILENOTFOUND 1
#define CREADFILE_FILENOTREAD  2

class CReadFile{
  private:
  static size_t getFileSize(const char *pszFileName){
    FILE *fp=fopen(pszFileName, "r");
    if (fp == NULL) {
      throw(CREADFILE_FILENOTFOUND);
    }
    fseek( fp, 0L, SEEK_END );
    long endPos = ftell( fp);
    fclose(fp);
    return endPos;
  }

  static int openFileToBuf(const char *pszFileName,char *buf,size_t length){
    FILE *fp=fopen(pszFileName, "r");
    if (fp == NULL) {
      return CREADFILE_FILENOTFOUND;
    }
    size_t result=fread(buf,1,length,fp);
    if(result!=length){
      return CREADFILE_FILENOTREAD;
    }
    fclose(fp);
    return CREADFILE_OK;
  }
  
  public:
    /**
     * Opens a file into a CT::string
     * @param fileName The file to open
     * @return CT::string containing the file
     * throws exceptions of type int when something goes wrong.
     */

    static CT::string open(const char *fileName){
      size_t size = getFileSize(fileName);
      if(size==0)return "";
      char *data = new char[size+1];
      int status = openFileToBuf(fileName,data,size);
      if(status!=0){
        delete[] data;
        throw status;
      }
      data[size]='\0';
      CT::string dataString;
      dataString.copy(data,size);
      delete[] data;
      return dataString;
    }
   
};
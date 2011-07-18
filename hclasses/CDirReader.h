#ifndef CDirReader_H
#define CDirReader_H
#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>
#include "CTypes.h"
#include "CDebugger.h"


class CFileObject{
  public:
    CT::string fullName;
    CT::string baseName;
    int isDir;
};

class CDirReader{
  private: 
    int _ReadDir(const char* directory,const char *ext_filter,int recursive);
    DEF_ERRORFUNCTION();
  public:
    std::vector <CFileObject*> fileList;
    CDirReader();
    ~CDirReader();
    int listDir (const char* directory,const char *ext_filter);
    int listDirRecursive (const char* directory,const char *ext_filter);
    static void makeCleanPath(CT::string *path);
    int testRegEx(const char *string,const char *pattern);
};
#endif





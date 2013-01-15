/* 
 * Copyright (C) 2012, Royal Netherlands Meteorological Institute (KNMI)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any 
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Project    : ADAGUC
 *
 * initial programmer :  M.Plieger
 * initial date       :  20120610
 */

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
#include <sys/stat.h>

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
    static int getFileDate(CT::string *date,const char *file);
    int testRegEx(const char *string,const char *pattern);

    /**
     * Create a public directory writable for everybody 
     * A directory chain can also be given (like mkdir -p)
     * @param dirname 
     */
    static void makePublicDirectory(const char *dirname);
};
#endif





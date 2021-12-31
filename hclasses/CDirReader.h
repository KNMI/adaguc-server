/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#ifndef CDirReader_H
#define CDirReader_H

#include "CTypes.h"
#include "CDebugger.h"
#include <string>
#include <map>
static std::map<std::string, std::string> lookupTableFileModificationDateMap;

class CDirReader {
private:
  int _ReadDir(const char *directory, const char *ext_filter, int recursive);
  // int _listDir (const char* directory,const char *ext_filter);
  int _listDirRecursive(const char *directory, const char *ext_filter);
  CT::string currentDir;
  DEF_ERRORFUNCTION();

public:
  std::vector<std::string> fileList;
  CDirReader();
  ~CDirReader();
  int listDirRecursive(const char *directory, const char *ext_filter);
  static CT::string makeCleanPath(const char *_path);
  static int getFileDate(CT::string *date, const char *file);
  static int testRegEx(const char *string, const char *pattern);

  static CT::string getFileDate(const char *fileName);

  static bool isDir(const char *fileName);

  /**
   * Create a public directory writable for everybody
   * A directory chain can also be given (like mkdir -p)
   * @param dirname
   */
  static void makePublicDirectory(const char *dirname);

  static void compareLists(std::vector<std::string> a, std::vector<std::string> b, void (*handleMissing)(std::string), void (*handleNew)(std::string));

  static void test_compareLists();
  static int test_makeCleanPath();
};

class CCachedDirReader {
private:
  DEF_ERRORFUNCTION();
  static std::map<std::string, CDirReader *> dirReaderMap;

public:
  static void free();
  static CDirReader *getDirReader(const char *directory, const char *ext_filter);
};

#endif

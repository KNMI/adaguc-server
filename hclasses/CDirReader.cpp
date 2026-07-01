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
#include <iostream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <regex.h>
#include <cstddef>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm> /* For std::sort */
#include <regex>
#include "CDirReader.h"
#include "CTString.h"
#include "CDebugger.h"

CDirReader::CDirReader() {}

CDirReader::~CDirReader() { fileList.clear(); }

const std::vector<std::string> CDirReader::listDir(const char *directory, bool recursive, const char *ext_filter, int filesAndOrDirs, bool exceptionOnError) {
  std::vector<std::string> result;
  std::regex self_regex;
  if (ext_filter != nullptr) {
    self_regex = std::regex(ext_filter);
  }

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(directory)) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      std::string fullName = directory;
      fullName += fullName + "/" + ent->d_name;

      // Deal with filesystems that don't provide d_type
      auto d_type = ent->d_type;
      if (d_type == DT_UNKNOWN) {
        struct stat path_stat{};
        int ret = stat(fullName.c_str(), &path_stat);
        if (ret == 0 && S_ISREG(path_stat.st_mode)) {
          d_type = DT_REG;
        }
        if (ret == 0 && S_ISDIR(path_stat.st_mode)) {
          d_type = DT_DIR;
        }
      }

      if (d_type == DT_REG || d_type == DT_DIR) {
        if (ent->d_name[0] != '.') { // Omit files and directories starting with a .
          if (((filesAndOrDirs & CDIRREADER_INCLUDE_FILES) && d_type == DT_REG) || ((filesAndOrDirs & CDIRREADER_INCLUDE_DIRECTORIES) && d_type == DT_DIR)) {
            if (ext_filter == nullptr || std::regex_match(ent->d_name, self_regex)) {
              result.push_back(makeCleanPath(fullName.c_str()).c_str());
            }
          }
          if (recursive && d_type == DT_DIR) {
            auto dirFiles = listDir(fullName.c_str(), recursive, ext_filter, filesAndOrDirs, exceptionOnError);
            // Move elements from dirFiles to result. dirFiles is left in undefined but safe-to-destruct state.
            result.insert(result.end(), std::make_move_iterator(dirFiles.begin()), std::make_move_iterator(dirFiles.end()));
            dirFiles.clear();
          }
        }
      }
    }
    closedir(dir);
  } else {
    if (exceptionOnError) {
      CDBWarning("Unable to open dir [%s]", directory);
      /* could not open directory */
      throw "Error";
    }
  }
  return result;
}

int CDirReader::listDirRecursive(const char *directory, const char *ext_filter) {
  if (fileList.size() != 0) {
    if (currentDir != directory) {
      CDBError("Trying to do listDirRecursive twice with different paths");
      return 1;
    } else {
      /* Already done */
      return 0;
    }
  }
  currentDir = directory;
  return _listDirRecursive(directory, ext_filter);
}

int CDirReader::_listDirRecursive(const char *directory, const char *ext_filter) {
  // CDBDebug("Doing recursive directory scan for [%s]", directory);
  try {
    return _ReadDir(directory, ext_filter, 1);
  } catch (int a) {
    /* Maybe the user provided a file instead of a directory? */
    struct stat fileattr;
    if (stat(directory, &fileattr) == -1) {
      CDBWarning("Unable to stat %s", directory);
      return 1;
    }
    /* Is this a regular file? */
    if (S_ISREG(fileattr.st_mode) == 0) {
      CDBWarning("Not a regular file");
      return 2;
    }
    /* Check filter*/
    if (CT::testRegEx(directory, ext_filter)) {
      fileList.push_back(makeCleanPath(directory).c_str());
    } else {
      CDBWarning("Regexp failed.");
      return 3;
    }
    return 0;
  }
}
int CDirReader::_ReadDir(const char *directory, const char *ext_filter, int recursive) {
  struct dirent *ep;
  auto dp = opendir(directory);
  if (dp == NULL) {
    throw 1;
  }
  ep = readdir(dp);
  while (ep) {
    std::string fullName = std::string(directory) + "/" + ep->d_name;
    bool isdir = isDir(fullName.c_str());
    if (isdir == false) {
      if (CT::testRegEx(ep->d_name, ext_filter)) {
        /* This is a file */
        fileList.push_back(makeCleanPath(fullName));
      }
    } else if (recursive == 1) {
      if (ep->d_name[0] != '.') {
        _ReadDir(fullName.c_str(), ext_filter, recursive);
      }
    }
    ep = readdir(dp);
  }
  (void)closedir(dp);

  return 0;
}

bool CDirReader::isDir(const char *directory) {
  struct stat fileattr;
  if (stat(directory, &fileattr) == -1) {
    return false;
  }
  return S_ISDIR(fileattr.st_mode);
}

bool CDirReader::isFile(const char *filename) {
  struct stat fileattr;
  if (stat(filename, &fileattr) == -1) {
    return false;
  }
  return S_ISREG(fileattr.st_mode);
}

void CDirReader::makePublicDirectory(const char *dirname) {
  if (dirname == NULL) return;
  struct stat stFileInfo;
  int intStat = stat(dirname, &stFileInfo);
  if (intStat != 0) {
    std::string directory = dirname;
    auto directorySplitted = CT::split(directory, "/");
    directory = "";
    for (auto &directoryPart: directorySplitted) {
      directory += "/";
      directory += directoryPart;
      const char *part = directory.c_str();
      int intStat = stat(part, &stFileInfo);
      if (intStat != 0) {
        mode_t permissions = S_IRWXU | S_IRWXG | S_IRWXO;
        mkdir(part, permissions);
        chmod(part, 0777);
      }
    }
  }
}

void CDirReader::compareLists(std::vector<std::string> L1, std::vector<std::string> L2, void (*handleMissing)(std::string), void (*handleNew)(std::string)) {

  std::vector<std::string>::iterator it1 = L1.begin();
  std::vector<std::string>::iterator it2 = L2.begin();

  std::sort(L1.begin(), L1.end());
  std::sort(L2.begin(), L2.end());

  while (it1 != L1.end() && it2 != L2.end()) {
    if (*it1 < *it2) {
      handleMissing(*it1++);
    } else if (*it2 < *it1) {
      handleNew(*it2++);
    } else {
      it1++;
      it2++;
    }
  }

  while (it1 != L1.end()) handleMissing(*it1++);
  while (it2 != L2.end()) handleNew(*it2++);
}

void CDirReader::test_compareLists() {

  std::vector<std::string> oldList;
  oldList.push_back("ABC");
  oldList.push_back("OK");
  oldList.push_back("OK");
  oldList.push_back("DEF");
  oldList.push_back("GHI");
  oldList.push_back("JKL");
  std::vector<std::string> newList;
  newList.push_back("PQR");
  newList.push_back("ABC");
  newList.push_back("DEF");
  newList.push_back("PQR");
  newList.push_back("GHI");
  newList.push_back("JKL");

  class A {
  public:
    static void _handleMissing(std::string a) { CDBDebug("Newlist is missing %s", a.c_str()); }

    static void _handleNew(std::string a) { CDBDebug("Newlist has new %s", a.c_str()); }
  };

  CDirReader::compareLists(oldList, newList, &A::_handleMissing, &A::_handleNew);
}

int CDirReader::test_makeCleanPath() {
  std::string t;
  t = makeCleanPath("data");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("data/");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("/data/");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("/data");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("/data//");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("data/bla");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("/data/bla");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("data/bla/");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("//data//bla////");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("/data//bla////");
  CDBDebug("[%s]", t.c_str());
  ;
  t = makeCleanPath("/data/bla////");
  CDBDebug("[%s]", t.c_str());
  ;

  return 0;
}

std::map<std::string, CDirReader *> CCachedDirReader::dirReaderMap;

CDirReader *CCachedDirReader::getDirReader(const char *directory, const char *ext_filter) {
  std::string key = directory;
  if (ext_filter != NULL) {
    key = key + ext_filter;
  }
  std::map<std::string, CDirReader *>::iterator it;

  it = dirReaderMap.find(key);
  if (it == dirReaderMap.end()) {
    CDirReader *dirReader = new CDirReader();
    dirReaderMap[key] = dirReader;
    return dirReader;
  } else {
    return it->second;
  }
  return 0;
}

void CCachedDirReader::removeFileFromCachedList(std::string fileToRemove) {
  for (auto i: dirReaderMap) {
    CDirReader *dirReader = i.second;
    // Find the file in the vector
    auto itr = std::find(dirReader->fileList.begin(), dirReader->fileList.end(), fileToRemove);
    // If found, remove it from the vector
    if (itr != dirReader->fileList.end()) {
      dirReader->fileList.erase(itr);
    }
  }
}

void CCachedDirReader::free() {
  for (std::map<std::string, CDirReader *>::iterator it = dirReaderMap.begin(); it != dirReaderMap.end(); ++it) {
    delete it->second;
  }
  dirReaderMap.clear();
}

std::string makeCleanPath(const std::string &input) {
  const auto newString = CT::replace(CT::trim(CT::replace(CT::trim(input), "\n", "")), "//", "/");
  if (CT::endsWith(newString, "/")) {
    return CT::substring(newString, 0, newString.length() - 1);
  }
  return newString;
}

std::string getFileDateUnCached(const std::string &file) {
  struct tm *clock;                                /* create a time structure */
  struct stat attrib;                              /* create a file attribute structure */
  if (stat(file.c_str(), &attrib) != 0) return ""; /* get the attributes of afile.txt */
  clock = gmtime(&(attrib.st_mtime));              /* Get the last modified time and put it into the time structure */
  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", clock);
  std::string date = buffer;
  return date;
}

std::string getFileDate(const std::string &fileName) {

  std::map<std::string, std::string>::iterator it = lookupTableFileModificationDateMap.find(fileName);
  if (it != lookupTableFileModificationDateMap.end()) {
    return it->second;
  }
  auto fileDate = getFileDateUnCached(fileName);
  if (fileDate.length() < 10) fileDate = ("1970-01-01T00:00:00Z");

  lookupTableFileModificationDateMap.insert(std::pair<std::string, std::string>(fileName, fileDate.c_str()));
  return fileDate;
}
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
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm> /* For std::sort */
#include <regex>
#include "CDirReader.h"

const char *CDirReader::className = "CDirReader";
CDirReader::CDirReader() {}

CDirReader::~CDirReader() { fileList.clear(); }

std::vector<std::string> CDirReader::listDir(const char *directory, bool recursive, const char *ext_filter, int filesAndOrDirs, bool exceptionOnError) {
  std::vector<std::string> result;
  std::regex self_regex;
  if (ext_filter != nullptr) {
    self_regex = std::regex(ext_filter);
  }

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(directory)) != NULL) {
    errno = 0;
    /* print all the files and directories within directory */
    while ((ent = readdir(dir)) != NULL) {
      CT::string fullName = directory;
      fullName.concat("/");
      fullName.concat(ent->d_name);
      // Deal with filesystems that don't provide d_type
      auto d_type = ent->d_type;
      if (d_type == DT_UNKNOWN) {
        struct stat path_stat {};
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
            // Move elements from dirFiles to result.
            // dirFiles is left in undefined but safe-to-destruct state.
            result.insert(result.end(), std::make_move_iterator(dirFiles.begin()), std::make_move_iterator(dirFiles.end()));
            dirFiles.clear();
          }
        }
      }
    }
    closedir(dir);
    if (errno != 0) {
      if (exceptionOnError) {
        CDBWarning("errno != 0: Unable to readdir [%s]", directory);
        /* could read directory */
        throw "Error: Could not readdir";
      }
    }
  } else {
    if (exceptionOnError) {
      CDBWarning("Unable to open dir [%s]", directory);
      /* could not open directory */
      throw "Error";
    }
  }

  // if (re != nullptr && ext_filter != nullptr) {
  //   regfree(re);
  // }
  return result;
}

int CDirReader::listDirRecursive(const char *directory, const char *ext_filter) {
  if (fileList.size() != 0) {
    if (!currentDir.equals(directory)) {
      CDBError("Trying to do listDirRecursive twice with different paths");
      return 1;
    } else {
      /* Already done */
      // CDBDebug("Using cached results for [%s]", directory);
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
    if (testRegEx(directory, ext_filter) == 1) {
      fileList.push_back(makeCleanPath(directory).c_str());
    } else {
      CDBWarning("Regexp failed.");
      return 3;
    }
    return 0;
  }
}
int CDirReader::_ReadDir(const char *directory, const char *ext_filter, int recursive) {
  DIR *dp;
  struct dirent *ep;
  int filename_len;
  int dir_len = strlen(directory);

  dp = opendir(directory);
  if (dp != NULL) {
    ep = readdir(dp);
    while (ep) {
      filename_len = strlen(ep->d_name);
      CT::string dirpath;
      dirpath.copy(directory, dir_len);
      dirpath.concat("/", 1);
      dirpath.concat(ep->d_name, filename_len);
      bool isdir = isDir(dirpath.c_str());
      if (isdir == false) {
        if (testRegEx(ep->d_name, ext_filter) == 1) {

          /* This is a file */
          CT::string fullName;
          filename_len = strlen(ep->d_name);
          fullName.copy(directory, dir_len);
          fullName.concat("/", 1);
          fullName.concat(ep->d_name, filename_len);
          fullName = makeCleanPath(fullName.c_str());
          fileList.push_back(fullName.c_str());
        }
      } else if (recursive == 1) {
        if (ep->d_name[0] != '.') {
          _ReadDir(dirpath.c_str(), ext_filter, recursive);
        }
      }
      ep = readdir(dp);
    }
    (void)closedir(dp);
  } else {
    /* Could not open the directory */
    throw 1;
  }
  return 0;
}
/* Removes the double //'s from the string and makes sure that the string does not end with a / */
CT::string CDirReader::makeCleanPath(const char *_path) {
  CT::string path;
  if (_path == NULL) return path;
  path = _path;
  if (path.length() == 0) return path;
  path.replaceSelf("\n", "");
  path.trimSelf();
  CT::StackList<CT::string> parts = path.splitToStack("/");

  int startAtIndex = 0;
  if (path.c_str()[0] == '/') {
    /* Check if this should start with a slash or not */
    path.copy("/");
  } else if (path.indexOf("://") != -1) {
    /*Check if this should start with the original prefix"*/
    CT::string leftPart = path.splitToStack("://")[0] + "://";
    path.copy(leftPart);
    startAtIndex = 1;
  } else
    path.copy("");

  std::vector<CT::string> parts2;
  for (size_t j = 0; j < parts.size(); j++) {
    if (parts[j].length() > 0) {
      parts2.push_back(parts[j]);
    }
  }

  for (size_t j = startAtIndex; j < parts2.size(); j++) {
    if (parts2[j].length() > 0) {
      path.concat(&(parts2[j]));
      if (j + 1 < parts2.size()) {
        path.concat("/");
      }
    }
  }
  return path;
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

int CDirReader::testRegEx(const char *string, const char *pattern) {
  if (string == nullptr || pattern == nullptr) return 1;
  int status;
  regex_t re;

  if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
    return (0); /* Report error. */
  }
  status = regexec(&re, string, (size_t)0, NULL, 0);
  regfree(&re);
  if (status != 0) {
    return (0); /* Report error. */
  }

  return (1);
}

int CDirReader::getFileDate(CT::string *date, const char *file) {
  struct tm *clock;                       /* create a time structure */
  struct stat attrib;                     /* create a file attribute structure */
  if (stat(file, &attrib) != 0) return 1; /* get the attributes of afile.txt */
  clock = gmtime(&(attrib.st_mtime));     /* Get the last modified time and put it into the time structure */
  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", clock);
  date->copy(buffer);
  return 0;
}

void CDirReader::makePublicDirectory(const char *dirname) {
  if (dirname == NULL) return;
  struct stat stFileInfo;
  int intStat = stat(dirname, &stFileInfo);
  if (intStat != 0) {
    CT::string directory = dirname;
    CT::string *directorySplitted = directory.splitToArray("/");
    directory = "";
    for (size_t j = 0; j < directorySplitted->count; j++) {
      directory.concat("/");
      directory.concat(directorySplitted[j].c_str());
      const char *part = directory.c_str();
      int intStat = stat(part, &stFileInfo);
      if (intStat != 0) {
        mode_t permissions = S_IRWXU | S_IRWXG | S_IRWXO;
        mkdir(part, permissions);
        chmod(part, 0777);
      }
    }
    delete[] directorySplitted;
  }
}

CT::string CDirReader::getFileDate(const char *fileName) {

  std::map<std::string, std::string>::iterator it = lookupTableFileModificationDateMap.find(fileName);
  if (it != lookupTableFileModificationDateMap.end()) {
    CT::string filemoddate = (*it).second.c_str();
    return filemoddate;
  }
  CT::string fileDate;
  CDirReader::getFileDate(&fileDate, fileName);
  if (fileDate.length() < 10) fileDate.copy("1970-01-01T00:00:00Z");

  lookupTableFileModificationDateMap.insert(std::pair<std::string, std::string>(fileName, fileDate.c_str()));
  return fileDate;
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
  CT::string t;
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
  for (auto i : dirReaderMap) {
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

const char *CCachedDirReader::className = "CCachedDirReader";

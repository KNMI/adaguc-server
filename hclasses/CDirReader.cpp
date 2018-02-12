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
#include <algorithm>    // std::sort

#include "CDirReader.h"





const char *CDirReader::className="CDirReader";
CDirReader::CDirReader(){
}

CDirReader::~CDirReader(){
  for(size_t j=0;j<fileList.size();j++){
    delete fileList[j];
  }
  fileList.clear();
}

int CDirReader::listDirRecursive(const char* directory,const char *ext_filter){
  if (fileList.size() != 0){
    if(!currentDir.equals(directory)) {
      CDBError("Trying to do listDirRecursive twice with different paths");
      return 1;
    } else {
      // Already done
      CDBDebug("Using cached results for [%s]", directory);
      return 0;
    }
  }
  currentDir = directory;
  return _listDirRecursive(directory, ext_filter);
} 

int CDirReader::_listDirRecursive(const char* directory,const char *ext_filter){
  CDBDebug("Doing recursive diretory scan for [%s]", directory);
  try{
    return _ReadDir(directory,ext_filter,1);
  }catch(int a){
    //Maybe the user provided a file instead of a directory?
    struct stat64 fileattr;
    if(stat64(directory,&fileattr)==-1){
      //CDBError("Unable to stat %s",directory);
      return 1;
    }
    //Is this a regular file?
    if(S_ISREG(fileattr.st_mode)==0){
      //CDBError("Not a regular file");
      return 2;
    }
    //Check filter
    if(testRegEx(directory,ext_filter)==1){
      CFileObject * fileObject = new CFileObject();
      fileList.push_back(fileObject);
      fileObject->fullName.copy(directory);
      makeCleanPath(&fileObject->fullName);
      fileObject->baseName.copy(&fileObject->fullName);
    }else{
      //CDBError("Regexp failed.");
      return 3;
    }
    return 0;
  }
}
int CDirReader::_ReadDir(const char* directory,const char *ext_filter,int recursive){ 
  //printf("Entering %s\n",directory);
  DIR *dp;
  DIR *cdp;
  struct dirent *ep;
  //int ext_len=strlen(ext_filter);
  int filename_len;
  int dir_len=strlen(directory);
  
  //char * ext_cmp;
  dp = opendir (directory);
  if (dp != NULL)
  {
    ep = readdir (dp);
    while (ep){
        filename_len=strlen(ep->d_name);
        //int name_offset=filename_len-ext_len;
        //if(name_offset<0)name_offset=0;
        //ext_cmp=&ep->d_name[name_offset];
        if(testRegEx(ep->d_name,ext_filter)==1){
          CFileObject * fileObject = new CFileObject();
          fileList.push_back(fileObject);
          fileObject->fullName.copy(directory,dir_len);
          fileObject->fullName.concat("/",1);
          fileObject->fullName.concat(ep->d_name,filename_len);
          makeCleanPath(&fileObject->fullName);
          fileObject->baseName.copy(ep->d_name,filename_len);
          cdp=opendir(fileObject->fullName.c_str());
          if(cdp)fileObject->isDir=1;else fileObject->isDir=0;
          (void) closedir (cdp);
        }
        if(recursive==1){
          if(ep->d_name[0]!='.')
          {
            CT::string dirpath;
            dirpath.copy(directory,dir_len);
            dirpath.concat("/",1);
            dirpath.concat(ep->d_name,filename_len);
            cdp=opendir(dirpath.c_str());
            if(cdp)_ReadDir(dirpath.c_str(),ext_filter,recursive);
            (void) closedir (cdp);
          }
        }
      ep = readdir (dp);
    }
    (void) closedir (dp);
  }
  else {
    //char szTemp[MAX_STR_LEN+1];
    //snprintf(szTemp,MAX_STR_LEN,"Could not open the directory %s",directory);
    throw 1;
    return 1;
  }
  return 0;
}
int CDirReader::_listDir (const char* directory,const char *ext_filter){
  DIR *dp;
  DIR *cdp;
  struct dirent *ep;
  //int ext_len=strlen(ext_filter);
  int filename_len;
  int dir_len=strlen(directory);
  
  //char * ext_cmp;
  dp = opendir (directory);
  if (dp != NULL)
  {
    ep = readdir (dp);
    while (ep){
        filename_len=strlen(ep->d_name);
        //int name_offset=filename_len-ext_len;
        //if(name_offset<0)name_offset=0;
        //ext_cmp=&ep->d_name[name_offset];
        if(testRegEx(ep->d_name,ext_filter)==1){
          CFileObject * fileObject = new CFileObject();
          fileList.push_back(fileObject);
          filename_len=strlen(ep->d_name);
          fileObject->fullName.copy(directory,dir_len);
          fileObject->fullName.concat("/",1);
          fileObject->fullName.concat(ep->d_name,filename_len);
          makeCleanPath(&fileObject->fullName);
          fileObject->baseName.copy(ep->d_name,filename_len);
          cdp=opendir(fileObject->fullName.c_str());
          if(cdp)fileObject->isDir=1;else fileObject->isDir=0;
          (void) closedir (cdp);
        }
      ep = readdir (dp);
    }
    (void) closedir (dp);
  }
  else {CDBError ("Could not open the directory %s",directory);return 1;}
  return 0;
}

//Removes the double //'s from the string and makes sure that the string does not end with a /
void CDirReader::makeCleanPath(CT::string *path){
  if(path==NULL)return;
  if(path->length()==0)return;
  CT::StackList<CT::string>parts =path->splitToStack("/");
  if(path->c_str()[0]=='/'){
    path->copy("/");
  }else path->copy("");
  for(size_t j=0;j<parts.size();j++){
    if(parts[j].length()>0){
      path->concat(&(parts[j]));
      if(j+1<parts.size()){
        path->concat("/");
      }
    }
  }

}


int CDirReader::testRegEx(const char *string,const char *pattern){
  //printf("Testing '%s' == '%s': ",string,pattern);
  int    status; 
  regex_t    re;


  if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
    //printf("FALSE(1)\n");
    return(0);      /* Report error. */
  }
  status = regexec(&re, string, (size_t) 0, NULL, 0);
  regfree(&re);
  if (status != 0) {
    //printf("FALSE(2)\n");
    return(0);      /* Report error. */
  }
  
  //printf("OK\n");
  return(1);
}


int CDirReader::getFileDate(CT::string *date, const char *file){
  struct tm* clock;       // create a time structure
  struct stat attrib;     // create a file attribute structure
  if(stat(file, &attrib)!=0)return 1;    // get the attributes of afile.txt
  clock = gmtime(&(attrib.st_mtime)); // Get the last modified time and put it into the time structure
  char buffer [80];
  strftime (buffer,80,"%Y-%m-%dT%H:%M:%SZ",clock);
  date->copy(buffer);
  return 0;
}


void CDirReader::makePublicDirectory(const char *dirname){
  if(dirname==NULL)return;
  struct stat stFileInfo;
  int intStat = stat(dirname,&stFileInfo);
  if(intStat != 0){
    CT::string directory = dirname;
    CT::string *directorySplitted = directory.splitToArray("/");
    directory="";
    for(size_t j=0;j<directorySplitted->count;j++){
      directory.concat("/");
      directory.concat(directorySplitted[j].c_str());
      const char *part = directory.c_str();
      int intStat = stat(part,&stFileInfo);
      if(intStat != 0){
        //CDBDebug("making dir %s",part);
        mode_t permissions = S_IRWXU|S_IRWXG|S_IRWXO;
        mkdir (part,permissions);
        chmod(part,0777);
      }
    }
    delete[] directorySplitted;
  }
}

CT::string CDirReader::getFileDate(const char *fileName){
  
  std::map<std::string,std::string>::iterator it=lookupTableFileModificationDateMap.find(fileName);
  if(it!=lookupTableFileModificationDateMap.end()){
    CT::string filemoddate = (*it).second.c_str();
    //CDBDebug("Returning filedate %s from map",filemoddate.c_str());
    return filemoddate;
  }
  CT::string fileDate;
  CDirReader::getFileDate(&fileDate,fileName);
  if(fileDate.length()<10)fileDate.copy("1970-01-01T00:00:00Z");
  
   lookupTableFileModificationDateMap.insert(std::pair<std::string,std::string>(fileName,fileDate.c_str()));
  return fileDate;
}


void CDirReader::compareLists(std::vector <std::string> L1, std::vector <std::string> L2, void (*handleMissing)(std::string), void (*handleNew)(std::string)) {
  
  std::vector<std::string>::iterator it1 = L1.begin();
  std::vector<std::string>::iterator it2 = L2.begin();

  std::sort(L1.begin(), L1.end());
  std::sort(L2.begin(), L2.end());
  
  while( it1 != L1.end() && it2 != L2.end() )
  {
      if( *it1 < *it2 ) {
          handleMissing( *it1++ );
      } else if( *it2 < *it1 ) {
          handleNew( *it2++ );
      } else {
          it1++;
          it2++;
      }
  }

  while( it1 != L1.end() ) handleMissing( *it1++ );
  while( it2 != L2.end() ) handleNew( *it2++ );
}


void CDirReader::test_compareLists() {
  std::vector <std::string> oldList;
  oldList.push_back("ABC");oldList.push_back("OK");oldList.push_back("OK");oldList.push_back("DEF");oldList.push_back("GHI");oldList.push_back("JKL");
  std::vector <std::string> newList;
  newList.push_back("PQR");newList.push_back("ABC");newList.push_back("DEF");newList.push_back("PQR");newList.push_back("GHI");newList.push_back("JKL");

  
  class A{
  public:
      static void _handleMissing(std::string a){
        CDBDebug("Newlist is missing %s", a.c_str());
      }

      static void _handleNew(std::string a){
        CDBDebug("Newlist has new %s", a.c_str());
      }
  };
  
  CDirReader::compareLists(oldList, newList, &A::_handleMissing, &A::_handleNew);
}



// CCachedDirReader *CCachedDirReader::getCachedDirReader(){return &cachedDirReader;};
std::map<std::string,CDirReader*> CCachedDirReader::dirReaderMap;

CDirReader *CCachedDirReader::getDirReader(const char* directory,const char *ext_filter) {
  std::string key = directory;
  if (ext_filter != NULL){
    key = key + ext_filter;
  }
  std::map<std::string,CDirReader*>::iterator it;

  it = dirReaderMap.find(key);
  if (it == dirReaderMap.end()){
    CDirReader *dirReader = new CDirReader();
    dirReaderMap[key] = dirReader;
    return dirReader;
  }else{
    return it->second;
  }
  return 0;
}

void CCachedDirReader::free(){
  CDBDebug("Cleaningup dirReaders");
  for (std::map<std::string,CDirReader*>::iterator it=dirReaderMap.begin(); it!=dirReaderMap.end(); ++it) {
    delete it->second;
  }
  dirReaderMap.clear();
}
  
const char *CCachedDirReader::className="CCachedDirReader";

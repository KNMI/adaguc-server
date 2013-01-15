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

#include "CDirReader.h"



const char *CDirReader::className="CDirReader";
CDirReader::CDirReader(){
}

CDirReader::~CDirReader(){
  for(size_t j=0;j<fileList.size();j++){
    delete fileList[j];
  }
}

int CDirReader::listDirRecursive(const char* directory,const char *ext_filter){
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
int CDirReader::listDir (const char* directory,const char *ext_filter){
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
        CDBDebug("making dir %s",part);
        mode_t permissions = S_IRWXU|S_IRWXG|S_IRWXO;
        mkdir (part,permissions);
        chmod(part,0777);
      }
    }
    delete []directorySplitted;
  }
}
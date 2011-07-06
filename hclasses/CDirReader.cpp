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
  return _ReadDir(directory,ext_filter,1);
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
    char szTemp[MAX_STR_LEN+1];
    snprintf(szTemp,MAX_STR_LEN,"Could not open the directory %s",directory);
    throw szTemp;
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
  CT::stringlist *parts =path->splitN("/");
  if(path->c_str()[0]=='/'){
    path->copy("/");
  }else path->copy("");
  for(size_t j=0;j<parts->size();j++){
    if((*parts)[j]->length()>0){
      path->concat((*parts)[j]);
      if(j+1<parts->size()){
        path->concat("/");
      }
    }
  }
  delete parts;
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

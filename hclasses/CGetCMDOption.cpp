#include "CGetCMDOption.h"
char *CMDOptionHelp=(char*)"";
void printHelp(const char * option){
  printf("There is a problem with flag %s\n",option);
  printf("%s",CMDOptionHelp);
}
void CMDPrintHelp(){
  printf("%s",CMDOptionHelp);
}
void CMDSetHelpText(const char * help){
  CMDOptionHelp=(char*)help;
}

int getCMDOption(const char * pszOption, char *psz_value, const char * psz_default, int argc, const char *argv[]){
  int s=strlen(pszOption);
  for(int j=1;j<argc;j++){
    if(strncmp(pszOption,argv[j],s)==0){
      if(j+1>=argc){
        printHelp(argv[j]);
        return -1;
      }
      if(strstr(argv[j+1],"--")!=NULL){
        printHelp(argv[j]);
        return -1;}
        if(psz_value!=NULL){
          strncpy(psz_value,argv[j+1],MAX_STR_LEN);
          psz_value[MAX_STR_LEN]='\0';
        }
      return 1;
    }
  }
  if(psz_default!=NULL)if(psz_value!=NULL)
      strcpy(psz_value,psz_default);
  return 0;
}

int getCMDOption(const char * pszOption,  CDirReader*dirlist, int argc, const char *argv[]){
  // Returns 1 if a list of blank space separated options is found returns 0 if not.
  int s=strlen(pszOption);
  for(int j=1;j<argc;j++){
    if(strncmp(pszOption,argv[j],s)==0){
      if(j+1>=argc){
        printHelp(argv[j]);
        return -1;
      }
      if(strstr(argv[j+1],"--")!=NULL){
        printHelp(argv[j]);
        return -1;}
        int k=1;
        while(k+j<argc&&strstr(argv[j+k],"--")==NULL){
          char * file=(char*)argv[j+k];
          CFileObject * fileObject = new CFileObject ();
          dirlist->fileList.push_back(fileObject);
          fileObject->fullName.copy(file,strlen(file));
          fileObject->baseName.copy(file,strlen(file));
          fileObject->isDir=0;
          k++;
        }

        return 1;
    }
  }
  return 0;
}

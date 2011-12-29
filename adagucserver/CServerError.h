#ifndef MY_ERROR_H
#define MY_ERROR_H
#include <stdio.h>
#include <CDebugger.h>
#include "CDrawImage.h"
void printerror(const char * text);
void printdebug(const char * text,int prioritylevel);
void seterrormode(int errormode);
void readyerror();
void printerrorImage(void * drawImage);
void resetErrors();
bool errorsOccured();
void setErrorImageSize(int w,int h,int format);
#endif


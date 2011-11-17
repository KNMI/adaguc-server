#include "CCDFNetCDFIO.h"
const char *CDFNetCDFReader::className="NetCDFReader";
const char *CDFNetCDFWriter::className="NetCDFWriter";
const char *CCDFWarper::className="CCDFWarper";

void ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
  
}

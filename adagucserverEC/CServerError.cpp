#include "CServerError.h"
#define ERRORMSGS_SIZE 30000 



static char errormsgs[ERRORMSGS_SIZE];
static int errormsgs_ptr=0;
static int error_raised=0;
static int cerror_mode=0;//0 = text 1 = image 2 = XML
void printerror(const char * text)
{
  error_raised=1;
  if(errormsgs_ptr>ERRORMSGS_SIZE)return;
  size_t messageLength=strlen(text);
  strncpy(errormsgs+errormsgs_ptr,text,ERRORMSGS_SIZE-1-errormsgs_ptr);
  //strncat(errormsgs+errormsgs_ptr,"\n",ERRORMSGS_SIZE-1-errormsgs_ptr-1);
  errormsgs_ptr+=messageLength;
 
}
void seterrormode(int errormode){
  cerror_mode=errormode;
}
void resetErrors(){
  errormsgs[0]='\0';
  error_raised=0;
}

void printerrorImage(void *_drawImage){
  if(error_raised==0)return ;

  CDrawImage *drawImage=(CDrawImage*)_drawImage;
  CT::string errormsg(errormsgs),*messages,*sp,concat;
  messages=errormsg.split("\n");
  int y=0;
  size_t w=70,characters=0;
  for(size_t i=0;i<messages->count;i++){
    sp=messages[i].split(" ");
    concat.copy("");
    for(size_t k=0;k<sp->count;k++){
      if(characters+sp[k].length()<w){
        concat.concat(&sp[k]);
        concat.concat(" ");
        characters=concat.length();
      }
      else
      {
          // drawImage.rectangle(5,y*15+4,499,y*15+20,253,253);
        drawImage->setText(concat.c_str(),concat.length(),12,5+y*15, 240,0);
        y++;
        concat.copy(&sp[k]);
        concat.concat(" ");
        characters=concat.length();
      }
    }
    delete[] sp;
      
        //drawImage.rectangle(5,y*15+4,499,y*15+20,253,253);
    drawImage->setText(concat.c_str(),concat.length(),12,5+y*15, 240,0);
    y++;
  }
  y=(y-1)*15+26;
      //drawImage.rectangle(5,3,499,(y-1)*15+20,252,0);
  drawImage->line(3,3,499,3,254);
  drawImage->line(3,3,5,y,254);
  drawImage->line(3,y,499,y,251);
  drawImage->line(499,3,499,y,251);
  delete [] messages;
}



void readyerror(){


  if(error_raised==0)return ;
  if(errormsgs[0]=='\0')return;

  if(cerror_mode==EXCEPTIONS_PLAINTEXT){//Plain text
//    fprintf(stderr,"%s<br>\n",errormsgs);
    printf("%s%c%c\n","Content-type: text/plain",13,10);  
    fprintf(stdout,"%s",errormsgs);
    return;
  }
  if(cerror_mode==WMS_EXCEPTIONS_XML_1_1_1){//XML exception
    printf("%s%c%c\n","Content-Type:text/xml",13,10);  
    fprintf(stdout,"<?xml version='1.0' encoding=\"ISO-8859-1\" standalone=\"no\" ?>\n");
    fprintf(stdout,"<!DOCTYPE ServiceExceptionReport SYSTEM \"http://schemas.opengis.net/wms/1.1.1/exception_1_1_1.dtd\">\n");
    fprintf(stdout,"<ServiceExceptionReport version=\"1.1.1\">\n");
    fprintf(stdout,"  <ServiceException>\n");
    
    
    CT::string errormsg(errormsgs),*messages;
    messages=errormsg.split("\n");
    fprintf(stdout,"    ");
    for(size_t j=0;j<messages->count;j++){
      fprintf(stdout,"%s",messages[j].c_str());
      if(j+1<messages->count)fprintf(stdout,";\n");
    }
    delete[] messages;
    fprintf(stdout,"\n  </ServiceException>\n");
    fprintf(stdout,"</ServiceExceptionReport>\n");
    return;
  }
  if(cerror_mode==WMS_EXCEPTIONS_IMAGE||cerror_mode==WMS_EXCEPTIONS_BLANKIMAGE){//Image
    printf("%s%c%c\n","Content-Type:image/png",13,10);  
    CDrawImage drawImage;
    drawImage.setBGColor(255,255,255);
    drawImage.createImage(500,500);
    
    drawImage.createGDPalette();
    //palette.createStandard();
//    drawImage.createGDPalette(&palette);
    if(cerror_mode==WMS_EXCEPTIONS_IMAGE){
      printerrorImage(&drawImage);
    }
    drawImage.printImage();
  }
}

void printdebug(const char * text,int prioritylevel)
{
  return;
  //level 1 is just important information
  //level 2 is all
  int maxprioritylevel=2;
  if(maxprioritylevel==1&&prioritylevel==1)printf("Debug: %s\n",text);
  if(maxprioritylevel==2)printf("Debug: %s\n",text);
}

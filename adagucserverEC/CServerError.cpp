/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#include "CServerError.h"
#include "CDrawImage.h"

//#define ERRORMSGS_SIZE 30000 



//static char errormsgs[ERRORMSGS_SIZE];

static std::vector<CT::string> errormsgs;


static int error_raised=0;
static int cerror_mode=0;//0 = text 1 = image 2 = XML
static int errImageWidth=640;
static int errImageHeight=480;
static int errImageFormat=IMAGEFORMAT_IMAGEPNG8;
static ServiceExceptionCode errExceptionCode=OperationNotSupported;
static bool enableTransparency;
void printerror(const char * text){
  error_raised=1;
  CT::string t=text;
  
  //Remove "[E: file,line] spaces"
  if (t.indexOf("[E:")==0) {
    t.substringSelf(t.indexOf("]")+1,-1);
  }
   t.trimSelf();
   t.replaceSelf("\n","");
  if(t.length()>0){
    errormsgs.push_back(t);
  }
}
void seterrormode(int errormode){
  cerror_mode=errormode;
}
void resetErrors(){
  errormsgs.clear();
  error_raised=0;
}

void printerrorImage(void *_drawImage){
  if(error_raised==0)return ;

  CDrawImage *drawImage=(CDrawImage*)_drawImage;
  
  const char *exceptionText = "OGC inimage Exception";
  drawImage->setText(exceptionText,strlen(exceptionText),12,5, 241,0);
  
 
 
  int y=1;
  size_t w=drawImage->Geo->dWidth/6,characters=0;
  for(size_t i=0;i<errormsgs.size()-1;i++){
    CT::string *sp=errormsgs[i].splitToArray(" ");
    CT::string concat ="";
    for(size_t k=0;k<sp->count;k++){
      if(characters+sp[k].length()<w){
        concat.concat(&sp[k]);
        concat.concat(" ");
        characters=concat.length();
      }
      else{
        drawImage->setText(concat.c_str(),concat.length(),12,5+y*15, 240,-1);
        y++;
        concat.copy(&sp[k]);
        concat.concat(" ");
        characters=concat.length();
      }
    }
    delete[] sp;
    drawImage->setText(concat.c_str(),concat.length(),12,5+y*15, 240,-1);
    y++;
  }
  y=(y-1)*15+26;
  
  drawImage->line(3,3,errImageWidth-1,3,254);
  drawImage->line(3,3,3,y,254);
  drawImage->line(3,y,errImageWidth-1,y,251);
  drawImage->line(errImageWidth-1,3,errImageWidth-1,y,251);
  
  
 
}

bool errorsOccured(){
  if(error_raised==0)return false; else return true;
}
void setErrorImageSize(int w,int h,int format,bool _enableTransparency){
  errImageWidth=w;
  errImageHeight=h;
  errImageFormat=format;
  enableTransparency = _enableTransparency;
}
void readyerror(){


  if(error_raised==0)return ;
  if(errormsgs.size()==0)return;

  if(cerror_mode==EXCEPTIONS_PLAINTEXT||cerror_mode==0){//Plain text
    printf("%s%c%c\n","Content-type: text/plain",13,10);  
    for(size_t j=0;j<errormsgs.size();j++){
      fprintf(stdout,"%s\n",errormsgs[j].c_str());
    }
    resetErrors();return;
  }
  if(cerror_mode==WMS_EXCEPTIONS_XML_1_1_1){//XML exception
    printf("%s%c%c\n","Content-Type:text/xml",13,10);  
    fprintf(stdout,"<?xml version='1.0' encoding=\"ISO-8859-1\" standalone=\"no\" ?>\n");
    fprintf(stdout,"<!DOCTYPE ServiceExceptionReport SYSTEM \"http://schemas.opengis.net/wms/1.1.1/exception_1_1_1.dtd\">\n");
    fprintf(stdout,"<ServiceExceptionReport version=\"1.1.1\">\n");
    fprintf(stdout,"  <ServiceException>\n");
    
    
    for(size_t j=0;j<errormsgs.size();j++){
      CT::string msg=errormsgs[j].c_str();
      msg.replaceSelf("<","&lt;");
      msg.replaceSelf(">","&gt;");
      fprintf(stdout,"    %s;\n",msg.c_str());
      //if(j+1<errormsgs.size())fprintf(stdout,";\n");
    }
    fprintf(stdout,"\n  </ServiceException>\n");
    fprintf(stdout,"</ServiceExceptionReport>\n");
    resetErrors();return;
  }
  if(cerror_mode==WMS_EXCEPTIONS_XML_1_3_0){//XML exception
    printf("%s%c%c\n","Content-Type:text/xml",13,10);  
    fprintf(stdout,"<?xml version='1.0' encoding=\"ISO-8859-1\" standalone=\"no\" ?>\n");
    fprintf(stdout,"<ServiceExceptionReport version=\"1.3.0\"  xmlns=\"http://www.opengis.net/ogc\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">\n");
    fprintf(stdout,"  <ServiceException code=\"%s\">\n",getExceptionCodeText(errExceptionCode));
    
    
    for(size_t j=0;j<errormsgs.size();j++){
      CT::string msg=errormsgs[j].c_str();
      msg.replaceSelf("<","&lt;");
      msg.replaceSelf(">","&gt;");
      fprintf(stdout,"    %s;\n",msg.c_str());
      //if(j+1<errormsgs.size())fprintf(stdout,";\n");
    }
    fprintf(stdout,"\n  </ServiceException>\n");
    fprintf(stdout,"</ServiceExceptionReport>\n");
    resetErrors();return;
  }
  if(cerror_mode==WMS_EXCEPTIONS_IMAGE||cerror_mode==WMS_EXCEPTIONS_BLANKIMAGE){//Image
    CDrawImage drawImage;
    if(errImageFormat==IMAGEFORMAT_IMAGEPNG24 || errImageFormat==IMAGEFORMAT_IMAGEPNG32){
      drawImage.setRenderer(CDRAWIMAGERENDERER_CAIRO);
    }
    drawImage.setBGColor(255,255,255);

    drawImage.enableTransparency(enableTransparency);

    drawImage.createImage(errImageWidth,errImageHeight);
    
    drawImage.create685Palette();
    //palette.createStandard();
//    drawImage.createGDPalette(&palette);
    if(cerror_mode==WMS_EXCEPTIONS_IMAGE){
      printerrorImage(&drawImage);
    }
    
    if(errImageFormat==IMAGEFORMAT_IMAGEPNG8){
      printf("%s%c%c\n","Content-Type:image/png",13,10);
      drawImage.printImagePng8(true);
    }else if(errImageFormat==IMAGEFORMAT_IMAGEPNG24){
      drawImage.setRenderer(CDRAWIMAGERENDERER_CAIRO);
      printf("%s%c%c\n","Content-Type:image/png",13,10);
      drawImage.printImagePng24();
    }else if(errImageFormat==IMAGEFORMAT_IMAGEPNG32){
      drawImage.setRenderer(CDRAWIMAGERENDERER_CAIRO);
      printf("%s%c%c\n","Content-Type:image/png",13,10);
      drawImage.printImagePng32();
    }else if(errImageFormat==IMAGEFORMAT_IMAGEGIF){
      printf("%s%c%c\n","Content-Type:image/gif",13,10);
      drawImage.printImageGif();
    }else {
      printf("%s%c%c\n","Content-Type:image/png",13,10);
      drawImage.printImagePng8(true);
    }
     resetErrors();return;
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
const char * getExceptionCodeText(ServiceExceptionCode code){
  switch(code){
    case OperationNotSupported:return "OperationNotSupported";break;
    case InvalidDimensionValue:return "InvalidDimensionValue";break;
  }
  return "OperationNotSupported";
}
void setExceptionType(ServiceExceptionCode code){
  errExceptionCode = code;
}

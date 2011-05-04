#include "CDrawImage.h"

const char *CDrawImage::className="CDrawImage";
const char *CFreeType::className="CFreeType";
float convertValueToClass(float val,float interval){
  float f=int(val/interval);
  if(val<0)f-=1;
  return f*interval;
}

CDrawImage::CDrawImage(){
    dImageCreated=0;
    dPaletteCreated=0;
    _bAntiAliased=false;
    _bEnableTransparency=false;
    _bEnableTrueColor=false;
    dNumImages = 0;
    Geo = new CGeoParams;
    rbuf=NULL;
    wuLine=NULL;
    freeType=NULL;
    RGBAByteBuffer=NULL;
    TTFFontLocation = "/usr/X11R6/lib/X11/fonts/truetype/verdana.ttf";
    TTFFontSize = 9;
}

CDrawImage::~CDrawImage(){
  if(_bAntiAliased==false){
    if(dPaletteCreated==1){
      for(int j=0;j<256;j++)if(_colors[j]!=-1)gdImageColorDeallocate(image,_colors[j]);
    }
  }
  if(dImageCreated==1){
    if(_bAntiAliased==false){
      gdImageDestroy(image);
    };  
  }
  
  delete rbuf;rbuf=NULL;
  delete Geo; Geo=NULL;
  delete wuLine;wuLine=NULL;
  delete freeType;freeType=NULL;
  delete RGBAByteBuffer;RGBAByteBuffer=NULL;
  
}


int CDrawImage::createImage(int _dW,int _dH){
  Geo->dWidth=_dW;
  Geo->dHeight=_dH;
  return createImage(Geo);
}

int CDrawImage::createImage(CGeoParams *_Geo){
#ifdef MEASURETIME
  StopWatch_Stop("start createImage");
#endif  
  
  if(dImageCreated==1){CDBError("createImage: image already created");return 1;}
  //CDBDebug("_bAntiAliased: %d, _bEnableTrueColor:%d",_bAntiAliased,_bEnableTrueColor);
  Geo->copy(_Geo);
  if(_bAntiAliased==true){
    //Always true color
    size_t imageSize=0;
    imageSize=Geo->dWidth * Geo->dHeight * 4;
    RGBAByteBuffer = new unsigned char[imageSize];
    for(size_t j=0;j<imageSize;j=j+1){
      RGBAByteBuffer[j]=0;
    }
    wuLine = new CXiaolinWuLine(Geo->dWidth,Geo->dHeight,RGBAByteBuffer);
    freeType = new CFreeType (Geo->dWidth,Geo->dHeight,RGBAByteBuffer,TTFFontSize,TTFFontLocation);
  }
  if(_bAntiAliased==false){
    if(_bEnableTrueColor==false){
      image = gdImageCreate(Geo->dWidth,Geo->dHeight);
    }else{
      image = gdImageCreateTrueColor(Geo->dWidth,Geo->dHeight);
      gdImageSaveAlpha( image, true );
    }
    fontConfig = (char*)"verdana"; /* fontconfig pattern */
    gdFTUseFontConfig(1);
  }
  dImageCreated=1;
#ifdef MEASURETIME
  StopWatch_Stop("image created.");
#endif  

  return 0;

}



int CDrawImage::printImage(){
  if(dImageCreated==0){CDBError("print: image not created");return 1;}
  
  if(_bAntiAliased==true){
    //writeAGGPng();
    writeRGBAPng(Geo->dWidth,Geo->dHeight,RGBAByteBuffer,stdout,true);
  }
  if(_bAntiAliased==false){
    gdImagePng(image, stdout);
  }
  return 0;
}


void CDrawImage::drawVector(int x,int y,double direction, double strength,int color){
  double wx1,wy1,wx2,wy2,hx1,hy1,hx2,hy2,dx1,dy1;
  dx1=cos(direction)*(strength+3.50);
  dy1=sin(direction)*(strength+3.50);
  wx1=double(x)-dx1;wy1=double(y)-dy1;
  wx2=double(x)+dx1;wy2=double(y)+dy1;
  //hx1=double(x)+cos(direction-0.7f)*(strength/1.8f);hy1=double(y)+sin(direction-0.7f)*(strength/1.8f);
  //hx2=double(x)+cos(direction+0.7f)*(strength/1.8f);hy2=double(y)+sin(direction+0.7f)*(strength/1.8f);
  strength=(-3-strength);
  hx1=double(wx2)+cos(direction-0.7f)*(strength/1.8f);hy1=double(wy2)+sin(direction-0.7f)*(strength/1.8f);
  hx2=double(wx2)+cos(direction+0.7f)*(strength/1.8f);hy2=double(wy2)+sin(direction+0.7f)*(strength/1.8f);
  if(_bAntiAliased==true){
      wuLine->setColor(CDIred[color],CDIgreen[color],CDIblue[color],255);
      wuLine->line(int(wx1),int(wy1),int(wx2),int(wy2));
      wuLine->line(int(wx2),int(wy2),int(hx1),int(hy1));    
      wuLine->line(int(wx2),int(wy2),int(hx2),int(hy2));    
  }else{
    gdImageLine(image, int(wx1),int(wy1),int(wx2),int(wy2),_colors[color]);
    gdImageLine(image, int(wx2),int(wy2),int(hx1),int(hy1),_colors[color]);
    gdImageLine(image, int(wx2),int(wy2),int(hx2),int(hy2),_colors[color]);
  }
}

void CDrawImage::line(int x1,int y1,int x2,int y2,int color){
  if(_bAntiAliased==true){
    if(color>=0&&color<256){
      wuLine->setColor(CDIred[color],CDIgreen[color],CDIblue[color],255);
      //wuLine->setColor(0,255,0,255);
      wuLine->line(x1,y1,x2,y2);
    }
  }else{
    gdImageLine(image, x1,y1,x2,y2,_colors[color]);
  }
}
void CDrawImage::line(int x1,int y1,int x2,int y2,float w,int color){
  if(_bAntiAliased==true){
    if(color>=0&&color<256){
      wuLine->setColor(CDIred[color],CDIgreen[color],CDIblue[color],255);
//      wuLine->setColor(0,0,0,255);
  //    wuLine->setFillColor(255,255,255,255);
      wuLine->line(x1,y1,x2,y2);
    //  wuLine->filledRectangle(0,0,256,256);
//      wuLine->setColor(0,0,0,255);
  //    wuLine->line(0,128,256,130);
    }
  }else{
    gdImageSetThickness(image, int(w)*2);
    gdImageLine(image, x1,y1,x2,y2,_colors[color]);
  }
}


void CDrawImage::setPixelIndexed(int x,int y,int color){
  if(_bAntiAliased==true){
    //if(color>=0&&color<256){
      wuLine-> pixel(x,y,CDIred[color],CDIgreen[color],CDIblue[color]);
    //}
  }else{
    gdImageSetPixel(image, x,y,colors[color]);
  }
}

void CDrawImage::setPixelTrueColor(int x,int y,unsigned int color){
  if(_bAntiAliased==true){
    //if(_bEnableTrueColor){
      wuLine->pixel(x,y,color,color/256,color/(256*256));
    //}
  }else{
    gdImageSetPixel(image, x,y,color);
  }
}

const char* toHex8(char *data,unsigned char hex){
  unsigned char a=hex/16;
  unsigned char b=hex%16;
  data[0]=a<10?a+48:a+55;
  data[1]=b<10?b+48:b+55;
  data[2]='\0';
  return data;
}

void CDrawImage::getHexColorForColorIndex(CT::string *hexValue,int color){
  char data[3];
  hexValue->print("#%s%s%s",toHex8(data, CDIred[color]),toHex8(data, CDIgreen[color]),toHex8(data, CDIblue[color]));
}


map<int,int> myColorMap;
map<int,int>::iterator myColorIter;

void CDrawImage::setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b){
  if(_bAntiAliased==true){
      wuLine->setColor(r,g,b,255);
      wuLine-> pixel(x,y);
  }else{
    if(_bEnableTrueColor){
      gdImageSetPixel(image, x,y,r+g*256+b*65536);
    }else{
      int key = r+g*256+b*65536;
      int color;
      myColorIter=myColorMap.find(key);
      if(myColorIter==myColorMap.end()){
        color = gdImageColorClosest(image,r,g,b);
        myColorMap[key]=color;
      }else{
        color=(*myColorIter).second;
      }
      gdImageSetPixel(image, x,y,color);
      
    }
  }
}

void CDrawImage::setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a){
  if(_bAntiAliased==true){
          wuLine->setColor(r,g,b,a);
      wuLine-> pixel(x,y);
  }else{
    if(_bEnableTrueColor){
      gdImageSetPixel(image, x,y,r+g*256+b*65536);
    }else{
      if(_bEnableTrueColor){
        gdImageSetPixel(image, x,y,r+g*256+b*65536);
      }else{
        int key = r+g*256+b*65536;
        int color;
        myColorIter=myColorMap.find(key);
        if(myColorIter==myColorMap.end()){
          color = gdImageColorClosest(image,r,g,b);
          myColorMap[key]=color;
        }else{
          color=(*myColorIter).second;
        }
        gdImageSetPixel(image, x,y,color);
      }
    }
  }
}
void CDrawImage::setText(const char * text, size_t length,int x,int y,int color,int fontSize){
  if(_bAntiAliased==true){
//    agg::rgba8 colStr(CDIred[color], CDIgreen[color], CDIblue[color],255);
  //  drawFreeTypeText( x, y+11, 0,text,colStr);
    if(color>=0&&color<256){
      freeType->setColor(CDIred[color],CDIgreen[color],CDIblue[color],255);
      freeType->drawFreeTypeText(x,y+10,0,text);
    }
  }else{
    char *pszText=new char[length+1];
    strncpy(pszText,text,length);
    pszText[length]='\0';
    if(fontSize==-1)gdImageString (image, gdFontSmall, x,  y, (unsigned char *)pszText, color);
    if(fontSize==0)gdImageString (image, gdFontMediumBold, x,  y, (unsigned char *)pszText, color);
    if(fontSize==1)gdImageString (image, gdFontLarge, x,  y, (unsigned char *)pszText, color);
    delete[] pszText;
  }
}

void CDrawImage::setTextStroke(const char * text, size_t length,int x,int y, int fgcolor,int bgcolor, int fontSize){
  if(_bAntiAliased==true){
    //Not yet supported...
  }else{
    fgcolor=colors[fgcolor];
    bgcolor=colors[bgcolor];
    char *pszText=new char[length+1];
    strncpy(pszText,text,length);
    pszText[length]='\0';
  
    for(int dy=-1;dy<2;dy=dy+1)
      for(int dx=-1;dx<2;dx=dx+1)
        if(!(dx==0&&dy==0)){
      if(fontSize==-1)gdImageString (image, gdFontSmall, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
          if(fontSize==0)gdImageString (image, gdFontMediumBold, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
          if(fontSize==1)gdImageString (image, gdFontLarge, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
        }
    if(fontSize==-1)gdImageString (image, gdFontSmall, x,  y, (unsigned char *)pszText, fgcolor);
    if(fontSize==0)gdImageString (image, gdFontMediumBold, x,  y, (unsigned char *)pszText, fgcolor);
    if(fontSize==1)gdImageString (image, gdFontLarge, x,  y, (unsigned char *)pszText, fgcolor);
  
    delete[] pszText;
  }
}

void CDrawImage::drawText(int x,int y,float angle,const char *text,unsigned char colorIndex){
  if(_bAntiAliased==true){
    freeType->setColor(CDIred[colorIndex],CDIgreen[colorIndex],CDIblue[colorIndex],255);
    freeType->drawFreeTypeText(x,y,angle,text);
  }else{
    bla(text, strlen(text),angle, x, y, 240,8);
  }
}
void CDrawImage::bla(const char * text, size_t length,double angle,int x,int y,int color,int fontSize){
  char *_text = new char[strlen(text)+1];
  memcpy(_text,text,strlen(text)+1);
  
  //gdImageStringFT(NULL,&brect[0],0,fontConfig,8.0f,angle,0,0,(char*)_text);
  //gdImageFilledRectangle (image,brect[0]+x,brect[1]+y,brect[2]+x,brect[3]+y, _colors[0]);
  int tcolor=-_colors[color];
  if(_bEnableTrueColor)tcolor=-tcolor;
  gdImageStringFT(image, &brect[0], tcolor, fontConfig, 8.0f, angle,  x,  y, (char*)_text);
  delete[] _text;
}

int CDrawImage::create685Palette(){
  int j=0;
  for(int r=0;r<6;r++)
    for(int g=0;g<8;g++)
      for(int b=0;b<5;b++){
        addColor(j++,r*51  ,g*36  ,b *63 );
      }
  
  addColor(240,0  ,0  ,0  );
  addColor(241,32,32  ,32  );
  addColor(242,64 ,64,64  );
  addColor(243,96,96,96  );
  //addColor(244,64  ,64  ,192);
  addColor(244,64  ,64  ,255);
  addColor(245,128,128  ,255);
  addColor(246,64  ,64,192);
  addColor(247,32,32,32);
  addColor(248,0  ,0  ,0  );
  addColor(249,255,255,191);
  addColor(250,191,232,255);
  addColor(251,204,204,204);
  addColor(252,160,160,160);
  addColor(253,192,192,192);
  addColor(254,224,224,224);
  addColor(255,255,255,255);
  
  copyPalette();
  return 0;
}

int CDrawImage::_createStandard(){
  addColor(240,0  ,0  ,0  );
  addColor(241,32,32  ,32  );
  addColor(242,64 ,64,64  );
  addColor(243,96,96,96  );
  //addColor(244,64  ,64  ,192);
  addColor(244,64  ,64  ,255);
  addColor(245,128,128  ,255);
  addColor(246,64  ,64,192);
  addColor(247,32,32,32);
  addColor(248,0  ,0  ,0  );
  addColor(249,255,255,191);
  addColor(250,191,232,255);
  addColor(251,204,204,204);
  addColor(252,160,160,160);
  addColor(253,192,192,192);
  addColor(254,224,224,224);
  addColor(255,255,255,255);
  
  copyPalette();
  return 0;
}
int CDrawImage::createGDPalette(){
  return createGDPalette(NULL);
}
int CDrawImage::createGDPalette(CServerConfig::XMLE_Legend *legend){
  if(dImageCreated==0){CDBError("createGDPalette: image not created");return 1;}
  for(int j=0;j<255;j++){
    CDIred[j]=0;
    CDIgreen[j]=0;
    CDIblue[j]=0;
  }
  if(legend==NULL){
    return _createStandard();
  }
  if(legend->attr.type.equals("colorRange")){
    int controle=0;
    float cx;
    float rc[3];
    for(size_t j=0;j<legend->palette.size()-1&&controle<240;j++){
      if(legend->palette[j]->attr.index>255)legend->palette[j]->attr.index=255;
      if(legend->palette[j]->attr.index<0)  legend->palette[j]->attr.index=0;
      if(legend->palette[j+1]->attr.index>255)legend->palette[j+1]->attr.index=255;
      if(legend->palette[j+1]->attr.index<0)  legend->palette[j+1]->attr.index=0;
      float dif = legend->palette[j+1]->attr.index-legend->palette[j]->attr.index;
      if(dif<0.5f)dif=1;
      rc[0]=float(legend->palette[j+1]->attr.red  -legend->palette[j]->attr.red)/dif;
      rc[1]=float(legend->palette[j+1]->attr.green-legend->palette[j]->attr.green)/dif;
      rc[2]=float(legend->palette[j+1]->attr.blue -legend->palette[j]->attr.blue)/dif;

  
      for(int i=legend->palette[j]->attr.index;i<legend->palette[j+1]->attr.index&&controle<240;i++){
        if(i!=controle){CDBError("Invalid color table");return 1;}
        cx=float(controle-legend->palette[j]->attr.index);
        CDIred[controle]=int(rc[0]*cx)+legend->palette[j]->attr.red;
        CDIgreen[controle]=int(rc[1]*cx)+legend->palette[j]->attr.green;
        CDIblue[controle]=int(rc[2]*cx)+legend->palette[j]->attr.blue;
        if(CDIred[i]==0)CDIred[i]=1;//for transparency
        controle++;
      }
    }
    return _createStandard();
  }
  if(legend->attr.type.equals("interval")){
    for(size_t j=0;j<legend->palette.size();j++){
      for(int i=legend->palette[j]->attr.min;i<=legend->palette[j]->attr.max;i++){
        if(i>=0&&i<240){
          CDIred[i]=legend->palette[j]->attr.red;
          CDIgreen[i]=legend->palette[j]->attr.green;
          CDIblue[i]=legend->palette[j]->attr.blue;
          if(CDIred[i]==0)CDIred[i]=1;//for transparency
        }
      }
    }
    return _createStandard();
  }
  return 1;
}

void CDrawImage::rectangle( int x1, int y1, int x2, int y2,int innercolor,int outercolor){
  if(innercolor>=0&&innercolor<240){
    if(_bAntiAliased==true){
      wuLine->setColor(CDIred[outercolor],CDIgreen[outercolor],CDIblue[outercolor],255);
      wuLine->setFillColor(CDIred[innercolor],CDIgreen[innercolor],CDIblue[innercolor],255);
      wuLine->filledRectangle(x1,y1,x2,y2);
      
      /*float w=1;
      line( x1-1, y1, x2+1, y1,w,outercolor);
      line( x2, y1, x2, y2,w,outercolor);
      line( x2+1, y2, x1-1, y2,w,outercolor);
      line( x1, y2, x1, y1,w,outercolor);
      for(int j=y1+1;j<y2;j++){
        line( x1, j, x2, j,1,innercolor);
      }*/
    }else{
      //Check for transparency
      if(CDIred[innercolor]==0&&CDIgreen[innercolor]==0&&CDIblue[innercolor]==0){
        //In case of transparency, draw a checkerboard
        for(int x=x1;x<x2-3;x=x+6){
          for(int y=y1;y<y2;y=y+3){
            int tx=x+((y%6)/3)*3;
            gdImageFilledRectangle (image,tx,y,tx+2,y+2, 240);
          }
        }
      }else{
        gdImageFilledRectangle (image,x1+1,y1+1,x2-1,y2-1, colors[innercolor]);
      }
      gdImageRectangle (image,x1,y1,x2,y2, colors[outercolor]);
      
    
    }
  }
}

void CDrawImage::rectangle( int x1, int y1, int x2, int y2,int outercolor){
  if(_bAntiAliased==true){
    line( x1, y1, x2, y1,1,outercolor);
    line( x2, y1, x2, y2,1,outercolor);
    line( x2, y2, x1, y2,1,outercolor);
    line( x1, y2, x1, y1,1,outercolor);
  }else{
    gdImageRectangle (image,x1,y1,x2,y2, outercolor);
  }
}

int CDrawImage::addColor(int Color,unsigned char R,unsigned char G,unsigned char B){
  CDIred[Color]=R;
  CDIgreen[Color]=G;
  CDIblue[Color]=B;
  return 0;
}

int CDrawImage::copyPalette(){
  if(_bAntiAliased==false){
    _colors[255] = gdImageColorAllocate(image,BGColorR,BGColorG,BGColorB); 
    for(int j=0;j<255;j++){
      _colors[j] = gdImageColorAllocate(image,CDIred[j],CDIgreen[j],CDIblue[j]); 
    }
  }
  for(int j=0;j<256;j++){
    colors[j]=_colors[j];
    //gdImageSetAntiAliased(image,colors[j])
    if(j<240){
      if(CDIred[j]==0&&CDIgreen[j]==0&&CDIblue[j]==0)colors[j]=_colors[255];
    }
  }
  if(_bAntiAliased==false){
    if(_bEnableTransparency){
      gdImageColorTransparent(image,_colors[255]);
    }
  }
  return 0;
}

int CDrawImage::addImage(int delay){
  //Add the current active image:
  gdImageGifAnimAdd(image, stdout, 0, 0, 0, delay, gdDisposalRestorePrevious, NULL);
  //This image is added, so it can be destroyed.
  for(int j=0;j<256;j++)if(_colors[j]!=-1)gdImageColorDeallocate(image,_colors[j]);
  gdImageDestroy(image); 
  //Make sure a new image is available for drawing
  if(_bEnableTrueColor==false){
    image = gdImageCreate(Geo->dWidth,Geo->dHeight);
  }else{
    image = gdImageCreateTrueColor(Geo->dWidth,Geo->dHeight);
  }
  
  copyPalette();
  return 0;
}

int CDrawImage::beginAnimation(){
  gdImageGifAnimBegin(image, stdout, 1, 0);
  return 0; 
}

int CDrawImage::endAnimation(){
  gdImageGifAnimEnd(stdout);
  return 0;
}

void CDrawImage::enableTransparency(bool enable){
  _bEnableTransparency=enable;
}
void CDrawImage::setBGColor(unsigned char R,unsigned char G,unsigned char B){
  BGColorR=R;
  BGColorG=G;
  BGColorB=B;
}

void CDrawImage::setTrueColor(bool enable){
  _bEnableTrueColor=enable;
}



int CDrawImage::writeRGBAPng(int width,int height,unsigned char *RGBAByteBuffer,FILE *file,bool trueColor){
  
  
#ifdef MEASURETIME
  StopWatch_Stop("start writeRGBAPng.");
#endif  

  png_structp png_ptr;
  png_infop info_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  
  if (!png_ptr){CDBError("png_create_write_struct failed");return 1;}
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_write_struct(&png_ptr, NULL);
    CDBError("png_create_info_struct failed");
    return 1;
  }

  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during init_io");
    return 1;
  }

  png_init_io(png_ptr, file);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during writing header");
    return 1;
  }
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
  PNG_FILTER_TYPE_DEFAULT);*/
  if(trueColor){
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
    PNG_FILTER_TYPE_BASE);*/
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  
  }else{
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_NONE);
  }
  
  
  png_set_filter (png_ptr, 0, PNG_FILTER_NONE);
  png_set_compression_level (png_ptr, -1);
  
  //png_set_invert_alpha(png_ptr);
  png_write_info(png_ptr, info_ptr);
  
  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during writing bytes");
    return 1;
  }
  png_set_packing(png_ptr);
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  

  int i;
  png_bytep row_ptr = 0;
  if(trueColor){
    for (i = 0; i < height; i=i+1){
      row_ptr = RGBAByteBuffer + 4 * i * width;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }else{
    for (i = 0; i < height; i++){
      row_ptr = RGBAByteBuffer + 1 * i * width;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during end of write");
    return 1;
  }

  png_write_end(png_ptr, NULL);
  
#ifdef MEASURETIME
  StopWatch_Stop("end writeRGBAPng.");
#endif  
  return 0;
}

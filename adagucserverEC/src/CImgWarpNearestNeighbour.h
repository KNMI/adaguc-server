#ifndef CImgWarpNearestNeighbour_H
#define CImgWarpNearestNeighbour_H
#include "CImageWarperRenderInterface.h"
#include "float.h"
class CImgWarpNearestNeighbour:public CImageWarperRenderInterface{
  private:
    DEF_ERRORFUNCTION();
    int set(const char *settings){
      return 0;
    }

    class CDrawRawField{
      public:
    //unsigned char pixel;
        float x_div,y_div;
        float sample_sy,sample_sx;
        float line_dx1,line_dy1,line_dx2,line_dy2;
        float rcx_1,rcy_1,rcx_2,rcy_2,rcx_3,rcy_3;
        int x,y;
        int srcpixel_x,srcpixel_y;
        int dstpixel_x,dstpixel_y;
        int k;
        float dfSourceBBOX[4],dfImageBBOX[4];
        float dfNodataValue;
        float legendLowerRange;
        float legendUpperRange;
        bool legendValueRange;
        bool hasNodataValue;
        int dWidth,dHeight;
        float legendLog,legendScale,legendOffset;
        CDataSource * sourceImage;
        CDrawImage *drawImage;
    //size_t prev_imgpointer;
        void init(CDataSource *sourceImage,CDrawImage *drawImage){
          this->sourceImage = sourceImage;
          this->drawImage = drawImage;
          for(int k=0;k<4;k++){
            dfSourceBBOX[k]=sourceImage->dfBBOX[k];
            dfImageBBOX[k]=sourceImage->dfBBOX[k];
          }
          if(sourceImage->dfBBOX[3]<sourceImage->dfBBOX[1]){
            dfSourceBBOX[1]=sourceImage->dfBBOX[3];
            dfSourceBBOX[3]=sourceImage->dfBBOX[1];
          }
          dfNodataValue    = sourceImage->dataObject[0]->dfNodataValue ;
          legendValueRange = sourceImage->legendValueRange;
          legendLowerRange = sourceImage->legendLowerRange;
          legendUpperRange = sourceImage->legendUpperRange;
          hasNodataValue   = sourceImage->dataObject[0]->hasNodataValue;
          dWidth = sourceImage->dWidth;
          dHeight = sourceImage->dHeight;
          legendLog = sourceImage->legendLog;
          legendScale = sourceImage->legendScale;
          legendOffset = sourceImage->legendOffset;
        }
        template <class T>
            int drawRawField(T*data,double *x_corners,double *y_corners,int &dDestX,int &dDestY){
          rcx_1= (x_corners[0] - x_corners[3])/x_div;
          rcy_1= (y_corners[0] - y_corners[3])/x_div;
          rcx_2= (x_corners[1] - x_corners[2])/x_div;
          rcy_2= (y_corners[1] - y_corners[2])/x_div;
          for(k=0;k<4;k++)
            if(fabs(x_corners[k]-x_corners[0])>=fabs(dfSourceBBOX[2]-dfSourceBBOX[0]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(x_corners[k]>dfSourceBBOX[0]&&x_corners[k]<dfSourceBBOX[2])break;
            if(k==4)return __LINE__;
          }
          for(k=0;k<4;k++)
            if(fabs(y_corners[k]-y_corners[0])>=fabs(dfSourceBBOX[3]-dfSourceBBOX[1]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(y_corners[k]>dfSourceBBOX[1]&&y_corners[k]<dfSourceBBOX[3])break;
            if(k==4)return __LINE__;
          }

          line_dx1= x_corners[3];
          line_dx2= x_corners[2];
          line_dy1= y_corners[3];
          line_dy2= y_corners[2];
          bool isNodata=false;
          float val;
          size_t imgpointer;
          for(x=0;x<=x_div;x++){
        /*line_dx1= x_corners[3]+rcx_1*x;
            line_dx2= x_corners[2]+rcx_2*x;
            line_dy1= y_corners[3]+rcy_1*x;
            line_dy2= y_corners[2]+rcy_2*x;
        */
            line_dx1+=rcx_1;line_dx2+=rcx_2;line_dy1+=rcy_1;line_dy2+=rcy_2;
        
            rcx_3= (line_dx2 -line_dx1)/y_div;
            rcy_3= (line_dy2 -line_dy1)/y_div;
            dstpixel_x=int(x)+dDestX;
            for(y=0;y<=y_div;y=y+1){
              dstpixel_y=int(y)+dDestY;
              sample_sx=line_dx1+rcx_3*y;
              if(sample_sx>=dfSourceBBOX[0]&&sample_sx<dfSourceBBOX[2])
              {
                sample_sy=line_dy1+rcy_3*y;
                if(sample_sy>=dfSourceBBOX[1]&&sample_sy<dfSourceBBOX[3])
                {
                  srcpixel_x=int(((sample_sx-dfImageBBOX[0])/(dfImageBBOX[2]-dfImageBBOX[0]))*dWidth);
                  if(srcpixel_x>=0&&srcpixel_x<dWidth){
                    srcpixel_y=int(((sample_sy-dfImageBBOX[1])/(dfImageBBOX[3]-dfImageBBOX[1]))*dHeight);
                    if(srcpixel_y>=0&&srcpixel_y<dHeight)
                    {
                      imgpointer=srcpixel_x+(dHeight-1-srcpixel_y)*dWidth;
                      val=data[imgpointer];
                      isNodata=false;
                      if(hasNodataValue)if(val==dfNodataValue)isNodata=true;
                      if(!isNodata)if(legendValueRange)if(val<legendLowerRange||val>legendUpperRange)isNodata=true;
                      if(!isNodata){
                        if(legendLog!=0)val=log10(val+.000001)/log10(legendLog);
                        val*=legendScale;
                        val+=legendOffset;
                        if(val>=239)val=239;else if(val<1)val=1;
                        drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,drawImage->colors[(unsigned char)val]);
                      }
                    }
                  }
                }
              }
            }
          }
          return 0;
            }
    };
    class CDrawRawFieldByteCache{
      public:
//unsigned char pixel;
        float x_div,y_div;
        float sample_sy,sample_sx;
        float line_dx1,line_dy1,line_dx2,line_dy2;
        float rcx_1,rcy_1,rcx_2,rcy_2,rcx_3,rcy_3;
        int x,y;
        int srcpixel_x,srcpixel_y;
        int dstpixel_x,dstpixel_y;
        int k;
        float dfSourceBBOX[4],dfImageBBOX[4];
        float dfNodataValue;
        float legendLowerRange;
        float legendUpperRange;
        bool legendValueRange;
        bool hasNodataValue;
        int dWidth,dHeight;
        float legendLog,legendScale,legendOffset;
        unsigned char *buf;
        CDataSource * sourceImage;
        CDrawImage *drawImage;
//size_t prev_imgpointer;
        void init(CDataSource *sourceImage,CDrawImage *drawImage){
          this->sourceImage = sourceImage;
          this->drawImage = drawImage;
          for(int k=0;k<4;k++){
            dfSourceBBOX[k]=sourceImage->dfBBOX[k];
            dfImageBBOX[k]=sourceImage->dfBBOX[k];
          }
          if(sourceImage->dfBBOX[3]<sourceImage->dfBBOX[1]){
            dfSourceBBOX[1]=sourceImage->dfBBOX[3];
            dfSourceBBOX[3]=sourceImage->dfBBOX[1];
          }
          dfNodataValue    = sourceImage->dataObject[0]->dfNodataValue ;
          legendValueRange = sourceImage->legendValueRange;
          legendLowerRange = sourceImage->legendLowerRange;
          legendUpperRange = sourceImage->legendUpperRange;
          hasNodataValue   = sourceImage->dataObject[0]->hasNodataValue;
          dWidth = sourceImage->dWidth;
          dHeight = sourceImage->dHeight;
          legendLog = sourceImage->legendLog;
          legendScale = sourceImage->legendScale;
          legendOffset = sourceImage->legendOffset;
// Allocate the Byte Buffer
          size_t imageSize=sourceImage->dWidth*sourceImage->dHeight;
          allocateArray(&buf,imageSize);
          memset ( buf, 255, imageSize);
        }
        CDrawRawFieldByteCache(){
          buf=NULL;
        }
        ~CDrawRawFieldByteCache(){
          deleteArray(&buf);
        }
        template <class T>
            int drawRawField(T*data,double *x_corners,double *y_corners,int &dDestX,int &dDestY){
          rcx_1= (x_corners[0] - x_corners[3])/x_div;
          rcy_1= (y_corners[0] - y_corners[3])/x_div;
          rcx_2= (x_corners[1] - x_corners[2])/x_div;
          rcy_2= (y_corners[1] - y_corners[2])/x_div;
          for(k=0;k<4;k++)
            if(fabs(x_corners[k]-x_corners[0])>=fabs(dfSourceBBOX[2]-dfSourceBBOX[0]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(x_corners[k]>dfSourceBBOX[0]&&x_corners[k]<dfSourceBBOX[2])break;
            if(k==4){
              return __LINE__;
            }
          }
          for(k=0;k<4;k++)
            if(fabs(y_corners[k]-y_corners[0])>=fabs(dfSourceBBOX[3]-dfSourceBBOX[1]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(y_corners[k]>dfSourceBBOX[1]&&y_corners[k]<dfSourceBBOX[3])break;
            if(k==4)return __LINE__;
          }

          line_dx1= x_corners[3];
          line_dx2= x_corners[2];
          line_dy1= y_corners[3];
          line_dy2= y_corners[2];
          bool isNodata=false;
          float val;
          size_t imgpointer;
          for(x=0;x<=x_div;x++){
  /*line_dx1= x_corners[3]+rcx_1*x;
            line_dx2= x_corners[2]+rcx_2*x;
            line_dy1= y_corners[3]+rcy_1*x;
            line_dy2= y_corners[2]+rcy_2*x;
  */
            line_dx1+=rcx_1;line_dx2+=rcx_2;line_dy1+=rcy_1;line_dy2+=rcy_2;
  
            rcx_3= (line_dx2 -line_dx1)/y_div;
            rcy_3= (line_dy2 -line_dy1)/y_div;
            dstpixel_x=int(x)+dDestX;
            for(y=0;y<=y_div;y=y+1){
              dstpixel_y=int(y)+dDestY;
              sample_sx=line_dx1+rcx_3*y;
              if(sample_sx>=dfSourceBBOX[0]&&sample_sx<dfSourceBBOX[2]){
                sample_sy=line_dy1+rcy_3*y;
                if(sample_sy>=dfSourceBBOX[1]&&sample_sy<dfSourceBBOX[3]){
                  srcpixel_x=int(((sample_sx-dfImageBBOX[0])/(dfImageBBOX[2]-dfImageBBOX[0]))*dWidth);
                  if(srcpixel_x>=0&&srcpixel_x<dWidth){
                    srcpixel_y=int(((sample_sy-dfImageBBOX[1])/(dfImageBBOX[3]-dfImageBBOX[1]))*dHeight);
                    if(srcpixel_y>=0&&srcpixel_y<dHeight){
                      imgpointer=srcpixel_x+(dHeight-1-srcpixel_y)*dWidth;
                      if(buf[imgpointer]!=0){
                        if(buf[imgpointer]==255){
                          val=data[imgpointer];
                          isNodata=false;
                          if(hasNodataValue)if(val==dfNodataValue)isNodata=true;
                          if(!isNodata)if(legendValueRange)if(val<legendLowerRange||val>legendUpperRange)isNodata=true;
                          if(!isNodata){
                            if(legendLog!=0)val=log10(val+.000001)/log10(legendLog);
                            val*=legendScale;
                            val+=legendOffset;
                            if(val>=239)val=239;else if(val<1)val=1;
                            buf[imgpointer]=(unsigned char)val;
                            //drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,drawImage->colors[buf[imgpointer]]);
                            drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,buf[imgpointer]);
                          }else buf[imgpointer]=0;
                        }else{
                          //drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,drawImage->colors[buf[imgpointer]]);
                          drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,buf[imgpointer]);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          return 0;
         }
    };


    class CDrawRawFieldConvertToByte{
      public:
        unsigned char pixel;
        float x_div,y_div;
        float sample_sy,sample_sx;
        float line_dx1,line_dy1,line_dx2,line_dy2;
        float rcx_1,rcy_1,rcx_2,rcy_2,rcx_3,rcy_3;
        int x,y;
        int srcpixel_x,srcpixel_y;
        int dstpixel_x,dstpixel_y;
        int k;
        float dfSourceBBOX[4],dfImageBBOX[4];
        float dfNodataValue;
        float legendLowerRange;
        float legendUpperRange;
        bool legendValueRange;
        bool hasNodataValue;
        int dWidth,dHeight;
        float legendLog,legendScale,legendOffset;
        unsigned char *buf;
        CDataSource * sourceImage;
        CDrawImage *drawImage;
//size_t prev_imgpointer;
        
        int to2DByteRaster(CDataSource *sourceImage){
          int status = NC_EBADTYPE;
          size_t ImageSize=sourceImage->dWidth*sourceImage->dHeight;
          double val;
    // Allocate the Byte Buffer
          allocateArray(&buf,ImageSize);
    
        //Datatype char
          if(sourceImage->dataObject[0]->dataType==CDF_CHAR){
            for(size_t j=0;j<ImageSize;j++){
              val=((char*)(sourceImage->dataObject[0]->data))[j];
              if(((val>=sourceImage->legendLowerRange&&val<=sourceImage->legendUpperRange)||sourceImage->legendValueRange==0)&&
                   (val!=dfNodataValue||hasNodataValue==0)){
                if(sourceImage->legendLog!=0){
                  val=log10(val+.000001)/log10(sourceImage->legendLog);
                }
                val*=sourceImage->legendScale;
                val+=sourceImage->legendOffset;
                if(val>=239)val=239;
                if(val<1)val=1;
                buf[j]=(unsigned char)val;
                   }else buf[j]=0;
            }
            status=NC_NOERR;
          }
        //Datatype short
          if(sourceImage->dataObject[0]->dataType==CDF_SHORT){
            for(size_t j=0;j<ImageSize;j++){
              val=((short*)(sourceImage->dataObject[0]->data))[j];
              if(((val>=sourceImage->legendLowerRange&&val<=sourceImage->legendUpperRange)||sourceImage->legendValueRange==0)&&
                   (val!=dfNodataValue||hasNodataValue==0)){
                if(sourceImage->legendLog!=0){
                  val=log10(val)/log10(sourceImage->legendLog);
                }
                val*=sourceImage->legendScale;
                val+=sourceImage->legendOffset;
                if(val>=239)val=239;
                if(val<1)val=1;
                buf[j]=(unsigned char)val;
                   }else buf[j]=0;
            }
            status=NC_NOERR;
          }
        //Datatype int
          if(sourceImage->dataObject[0]->dataType==CDF_INT){
            for(size_t j=0;j<ImageSize;j++){
              val=((int*)(sourceImage->dataObject[0]->data))[j];
              if(((val>=sourceImage->legendLowerRange&&val<=sourceImage->legendUpperRange)||sourceImage->legendValueRange==0)&&
                   (val!=dfNodataValue||hasNodataValue==0)){
                if(sourceImage->legendLog!=0){
                  val=log10(val)/log10(sourceImage->legendLog);
                }
                val*=sourceImage->legendScale;
                val+=sourceImage->legendOffset;
                if(val>=239)val=239;
                if(val<1)val=1;
                buf[j]=(unsigned char)val;
                   }else buf[j]=0;
            }
            status=NC_NOERR;
          }
    
    //Datatype float
          if(sourceImage->dataObject[0]->dataType==CDF_FLOAT){
            for(size_t j=0;j<ImageSize;j++){
              val=((float*)(sourceImage->dataObject[0]->data))[j];
              if(((val>=sourceImage->legendLowerRange&&val<=sourceImage->legendUpperRange)||sourceImage->legendValueRange==0)&&
                   (val!=dfNodataValue||hasNodataValue==0)){
                val+=0.00000000001f;
                if(sourceImage->legendLog!=0){
                  val=log10(val)/log10(sourceImage->legendLog);
                }
                val*=sourceImage->legendScale;
                val+=sourceImage->legendOffset;
        
                if(val>=239)val=239;
                if(val<1)val=1;
                buf[j]=(unsigned char)val;
                   }else buf[j]=0;
            }
            status=NC_NOERR;
          }
    
    //Datatype double
          if(sourceImage->dataObject[0]->dataType==CDF_DOUBLE){
            for(size_t j=0;j<ImageSize;j++){
              val=((double*)(sourceImage->dataObject[0]->data))[j];
              if(((val>=sourceImage->legendLowerRange&&val<=sourceImage->legendUpperRange)||sourceImage->legendValueRange==0)&&
                   (val!=dfNodataValue||hasNodataValue==0)){
                if(sourceImage->legendLog!=0){
                  val=log10(val)/log10(sourceImage->legendLog);
                }
                val*=sourceImage->legendScale;
                val+=sourceImage->legendOffset;
                if(val>=239)val=239;
                if(val<1)val=1;
                buf[j]=(unsigned char)val;
                   }else buf[j]=0;
            }
            status=NC_NOERR;
          }
          if(status != NC_NOERR){
            //CDBError("Invalid data type...");
            return 1;
          }
          return 0;
        }

        void init(CDataSource *sourceImage,CDrawImage *drawImage){
          this->sourceImage = sourceImage;
          this->drawImage = drawImage;
          for(int k=0;k<4;k++){
            dfSourceBBOX[k]=sourceImage->dfBBOX[k];
            dfImageBBOX[k]=sourceImage->dfBBOX[k];
          }
          if(sourceImage->dfBBOX[3]<sourceImage->dfBBOX[1]){
            dfSourceBBOX[1]=sourceImage->dfBBOX[3];
            dfSourceBBOX[3]=sourceImage->dfBBOX[1];
          }
          dfNodataValue    = sourceImage->dataObject[0]->dfNodataValue ;
          legendValueRange = sourceImage->legendValueRange;
          legendLowerRange = sourceImage->legendLowerRange;
          legendUpperRange = sourceImage->legendUpperRange;
          hasNodataValue   = sourceImage->dataObject[0]->hasNodataValue;
          dWidth = sourceImage->dWidth;
          dHeight = sourceImage->dHeight;
          legendLog = sourceImage->legendLog;
          legendScale = sourceImage->legendScale;
          legendOffset = sourceImage->legendOffset;
          
// Allocate the Byte Buffer
          size_t imageSize=sourceImage->dWidth*sourceImage->dHeight;
          allocateArray(&buf,imageSize);
          to2DByteRaster(sourceImage);

//          memset ( buf, 255, imageSize);
        }
        CDrawRawFieldConvertToByte(){
          buf=NULL;
        }
        ~CDrawRawFieldConvertToByte(){
          deleteArray(&buf);
        }
        /*
        int justdraw(CDrawImage *drawImage,CDataSource *sourceImage,double *x_corners,double *y_corners,int dDestX,int dDestY){
          float sample_sy,sample_sx;
          float line_dx1,line_dy1,line_dx2,line_dy2;
          float rcx_1,rcy_1,rcx_2,rcy_2,rcx_3,rcy_3;
          float x,y;      
          int srcpixel_x,srcpixel_y;
          int dstpixel_x,dstpixel_y;

          rcx_1= (x_corners[0] - x_corners[3])/x_div;
          rcy_1= (y_corners[0] - y_corners[3])/x_div;
          rcx_2= (x_corners[1] - x_corners[2])/x_div;
          rcy_2= (y_corners[1] - y_corners[2])/x_div;

          double dfSourceBBOX[4];
          for(int k=0;k<4;k++)
            dfSourceBBOX[k]=sourceImage->dfBBOX[k];
          if(sourceImage->dfBBOX[3]<sourceImage->dfBBOX[1]){
            dfSourceBBOX[1]=sourceImage->dfBBOX[3];
            dfSourceBBOX[3]=sourceImage->dfBBOX[1];
          }
  
          int k; 
          for(k=0;k<4;k++)
            if(fabs(x_corners[k]-x_corners[0])>=fabs(dfSourceBBOX[2]-dfSourceBBOX[0]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(x_corners[k]>dfSourceBBOX[0]&&x_corners[k]<dfSourceBBOX[2])break;
            if(k==4)return 1;
          }
          for(k=0;k<4;k++)
            if(fabs(y_corners[k]-y_corners[0])>=fabs(dfSourceBBOX[3]-dfSourceBBOX[1]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(y_corners[k]>dfSourceBBOX[1]&&y_corners[k]<dfSourceBBOX[3])break;
            if(k==4)return 1;
          }
  
          for(x=0;x<=x_div;x++){
            line_dx1= x_corners[3]+rcx_1*x;
            line_dx2= x_corners[2]+rcx_2*x;
            line_dy1= y_corners[3]+rcy_1*x;
            line_dy2= y_corners[2]+rcy_2*x;
            rcx_3= (line_dx2 -line_dx1)/y_div;
            rcy_3= (line_dy2 -line_dy1)/y_div;
            dstpixel_x=int(x)+dDestX;
            for(y=0;y<=y_div;y=y+1){
              dstpixel_y=int(y)+dDestY;
              sample_sx=line_dx1+rcx_3*y;
              if(sample_sx>=dfSourceBBOX[0]&&sample_sx<dfSourceBBOX[2]){
                sample_sy=line_dy1+rcy_3*y;
                if(sample_sy>=dfSourceBBOX[1]&&sample_sy<dfSourceBBOX[3]){
                  srcpixel_x=int(((sample_sx-sourceImage->dfBBOX[0])/(sourceImage->dfBBOX[2]-sourceImage->dfBBOX[0]))*sourceImage->dWidth);
                  if(srcpixel_x>=0&&srcpixel_x<sourceImage->dWidth){
                    srcpixel_y=int(((sample_sy-sourceImage->dfBBOX[1])/(sourceImage->dfBBOX[3]-sourceImage->dfBBOX[1]))*sourceImage->dHeight);
                    if(srcpixel_y>=0&&srcpixel_y<sourceImage->dHeight){
                      size_t imgpointer=srcpixel_x+(sourceImage->dHeight-1-srcpixel_y)*sourceImage->dWidth;
                      pixel=buf[imgpointer];
                      if(pixel!=0){
                        drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,drawImage->colors[pixel]);
                      }
                    }
                  }
                }
              }
            }
          } 
          return 0;
        }
        */
        int drawRawField(double *x_corners,double *y_corners,int &dDestX,int &dDestY){
          rcx_1= (x_corners[0] - x_corners[3])/x_div;
          rcy_1= (y_corners[0] - y_corners[3])/x_div;
          rcx_2= (x_corners[1] - x_corners[2])/x_div;
          rcy_2= (y_corners[1] - y_corners[2])/x_div;
          for(k=0;k<4;k++)
            if(fabs(x_corners[k]-x_corners[0])>=fabs(dfSourceBBOX[2]-dfSourceBBOX[0]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(x_corners[k]>dfSourceBBOX[0]&&x_corners[k]<dfSourceBBOX[2])break;
            if(k==4)return 1;
          }
          for(k=0;k<4;k++)
            if(fabs(y_corners[k]-y_corners[0])>=fabs(dfSourceBBOX[3]-dfSourceBBOX[1]))break;
          if(k==4){
            for(k=0;k<4;k++)
              if(y_corners[k]>dfSourceBBOX[1]&&y_corners[k]<dfSourceBBOX[3])break;
            if(k==4)return 1;
          }

          line_dx1= x_corners[3];
          line_dx2= x_corners[2];
          line_dy1= y_corners[3];
          line_dy2= y_corners[2];
          //bool isNodata=false;
          //float val;
          //size_t imgpointer;
          for(x=0;x<=x_div;x++){
            line_dx1= x_corners[3]+rcx_1*x;
            line_dx2= x_corners[2]+rcx_2*x;
            line_dy1= y_corners[3]+rcy_1*x;
            line_dy2= y_corners[2]+rcy_2*x;
  
            //line_dx1+=rcx_1;line_dx2+=rcx_2;line_dy1+=rcy_1;line_dy2+=rcy_2;
  
            rcx_3= (line_dx2 -line_dx1)/y_div;
            rcy_3= (line_dy2 -line_dy1)/y_div;
            dstpixel_x=int(x)+dDestX;
            for(y=0;y<=y_div;y=y+1){
              dstpixel_y=int(y)+dDestY;
              sample_sx=line_dx1+rcx_3*y;
              if(sample_sx>=dfSourceBBOX[0]&&sample_sx<dfSourceBBOX[2]){
                sample_sy=line_dy1+rcy_3*y;
                if(sample_sy>=dfSourceBBOX[1]&&sample_sy<dfSourceBBOX[3]){
                  srcpixel_x=int(((sample_sx-dfImageBBOX[0])/(dfImageBBOX[2]-dfImageBBOX[0]))*dWidth);
                  if(srcpixel_x>=0&&srcpixel_x<dWidth){
                    srcpixel_y=int(((sample_sy-dfImageBBOX[1])/(dfImageBBOX[3]-dfImageBBOX[1]))*dHeight);
                    if(srcpixel_y>=0&&srcpixel_y<dHeight){
                      unsigned char pixel = buf[srcpixel_x+(dHeight-1-srcpixel_y)*dWidth];
                      if(pixel!=0){
                        drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,pixel);
                      }
                    }
                  }
                }
              }
            }
          }
          return 0;
        }
    };

  public:
    double y_corners[4],x_corners[4];
    double dfMaskBBOX[4];
    int status;

    int reproj(CImageWarper *warper,CDataSource *sourceImage,CGeoParams *GeoDest,double dfx,double dfy,double x_div,double y_div){
      double psx[4];
      double psy[4];
      double dfTiledBBOX[4];

      double dfTileW=(GeoDest->dfBBOX[2]-GeoDest->dfBBOX[0])/double(x_div);
      double dfTileH=(GeoDest->dfBBOX[3]-GeoDest->dfBBOX[1])/double(y_div);

      dfTiledBBOX[0]=GeoDest->dfBBOX[0]+dfTileW*dfx;
      dfTiledBBOX[1]=GeoDest->dfBBOX[1]+dfTileH*dfy;
      dfTiledBBOX[2]=dfTiledBBOX[0]+(dfTileW);
      dfTiledBBOX[3]=dfTiledBBOX[1]+(dfTileH);
      double dfSourceBBOX[4];
      for(int k=0;k<4;k++)dfSourceBBOX[k]=dfMaskBBOX[k];
      if(dfMaskBBOX[3]<dfMaskBBOX[1]){
        dfSourceBBOX[1]=dfMaskBBOX[3];
        dfSourceBBOX[3]=dfMaskBBOX[1];
      }
      if(
         (dfTiledBBOX[0]>dfSourceBBOX[0]-dfTileW&&dfTiledBBOX[0]<dfSourceBBOX[2]+dfTileW)&&
         (dfTiledBBOX[2]>dfSourceBBOX[0]-dfTileW&&dfTiledBBOX[2]<dfSourceBBOX[2]+dfTileW)&&
         (dfTiledBBOX[1]>dfSourceBBOX[1]-dfTileH&&dfTiledBBOX[1]<dfSourceBBOX[3]+dfTileH)&&
         (dfTiledBBOX[3]>dfSourceBBOX[1]-dfTileH&&dfTiledBBOX[3]<dfSourceBBOX[3]+dfTileH)
        )
        ;else return 1;
      psx[0]=dfTiledBBOX[2];
      psx[1]=dfTiledBBOX[2];
      psx[2]=dfTiledBBOX[0];
      psx[3]=dfTiledBBOX[0];
      psy[0]=dfTiledBBOX[1];
      psy[1]=dfTiledBBOX[3];
      psy[2]=dfTiledBBOX[3];
      psy[3]=dfTiledBBOX[1];
      CT::string destinationCRS;
      warper->decodeCRS(&destinationCRS,&GeoDest->CRS);
      if(destinationCRS.indexOf("longlat")>=0){
        for(int j=0;j<4;j++){
         psx[j]*=DEG_TO_RAD;
          psy[j]*=DEG_TO_RAD;
        }
      }
        pj_transform(warper->destpj,warper->sourcepj, 4,0,psx,psy,NULL);
        if(sourceImage->nativeProj4.indexOf("longlat")>=0)
          for(int j=0;j<4;j++){
          psx[j]/=DEG_TO_RAD;
          psy[j]/=DEG_TO_RAD;
          }
          x_corners[0]=psx[1];
          y_corners[0]=psy[1];

          x_corners[1]=psx[0];
          y_corners[1]=psy[0];

          x_corners[2]=psx[3];
          y_corners[2]=psy[3];

          x_corners[3]=psx[2];
          y_corners[3]=psy[2];
          return 0;
    }

    void render(CImageWarper *warper,CDataSource *sourceImage,CDrawImage *drawImage){
      //CDBDebug("Render");
      warper->findExtent(sourceImage,dfMaskBBOX);
      int x_div=drawImage->Geo->dWidth/16;
      int y_div=drawImage->Geo->dHeight/16;
      if(warper->isProjectionRequired()==false){
        //CDBDebug("No reprojection required");
         x_div=1;
         y_div=1;
      }
      double tile_width=(double(drawImage->Geo->dWidth)/double(x_div));
      double tile_height=(double(drawImage->Geo->dHeight)/double(y_div));
      int tile_offset_x=0;
      int tile_offset_y=0;
    
      if(sourceImage->dataObject[0]->dataType==CDF_CHAR||sourceImage->dataObject[0]->dataType==CDF_BYTE||sourceImage->dataObject[0]->dataType==CDF_UBYTE){
      //Do not cache the calculated results for CDF_CHAR
        CDrawRawField drawFieldClass;
        drawFieldClass.x_div=tile_width;
        drawFieldClass.y_div=tile_height;
        drawFieldClass.init(sourceImage,drawImage);
        for(int x=0;x<x_div;x=x+1)
          for(int y=0;y<y_div;y=y+1){
          status = reproj(warper,sourceImage,drawImage->Geo,x,(y_div-1)-y,x_div,y_div);
          if(status == 0){
            tile_offset_x=int(double(x)*tile_width);
            tile_offset_y=int(double(y)*tile_height);
          // drawByteField(drawImage,sourceImage,x_corners,y_corners,tile_offset_x,tile_offset_y,(int)tile_width,(int)tile_height);
            if(sourceImage->dataObject[0]->dataType==CDF_CHAR||sourceImage->dataObject[0]->dataType==CDF_BYTE)
              drawFieldClass.drawRawField((char*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_UBYTE)
              drawFieldClass.drawRawField((unsigned char*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_SHORT)
              drawFieldClass.drawRawField((short*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_USHORT)
              drawFieldClass.drawRawField((unsigned short*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_INT)
              drawFieldClass.drawRawField((int*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_UINT)
              drawFieldClass.drawRawField((unsigned int*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_FLOAT)
              drawFieldClass.drawRawField((float*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_DOUBLE)
              drawFieldClass.drawRawField((double*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
          }
          }   
      }else{
        /*if(1==1){
          CDrawRawFieldConvertToByte drawFieldClass;
          drawFieldClass.x_div=tile_width;
          drawFieldClass.y_div=tile_height;
          drawFieldClass.init(sourceImage,drawImage);
          for(int x=0;x<x_div;x=x+1){
            for(int y=0;y<y_div;y=y+1){
              status = reproj(warper,sourceImage,drawImage->Geo,x,(y_div-1)-y,x_div,y_div);
              if(status == 0){
              tile_offset_x=int(double(x)*tile_width);
              tile_offset_y=int(double(y)*tile_height);
              drawFieldClass.drawRawField(x_corners,y_corners,tile_offset_x,tile_offset_y);
            }
          }
        }
      }else*/{
      //Cache the calculated results
        CDrawRawFieldByteCache drawFieldClass;
  //      CDrawRawFieldConvertToByte drawFieldClass;
        drawFieldClass.x_div=tile_width;
        drawFieldClass.y_div=tile_height;
        drawFieldClass.init(sourceImage,drawImage);
        //dfMaskBBOX
        //CDBDebug("b %f %f %f %f",sourceImage->dfBBOX[0],sourceImage->dfBBOX[1],sourceImage->dfBBOX[2],sourceImage->dfBBOX[3]);
        //CDBDebug("d %f %f %f %f",drawImage->Geo->dfBBOX[0],drawImage->Geo->dfBBOX[1],drawImage->Geo->dfBBOX[2],drawImage->Geo->dfBBOX[3]);
        int drawStat = -1;
        for(int x=0;x<x_div;x=x+1)
          for(int y=0;y<y_div;y=y+1){
          status = reproj(warper,sourceImage,drawImage->Geo,x,(y_div-1)-y,x_div,y_div);
          /*x_corners[0]=drawImage->Geo->dfBBOX[2];
          x_corners[1]=drawImage->Geo->dfBBOX[2];
          x_corners[2]=drawImage->Geo->dfBBOX[0];
          x_corners[3]=drawImage->Geo->dfBBOX[0];
          y_corners[0]=drawImage->Geo->dfBBOX[3];
          y_corners[1]=drawImage->Geo->dfBBOX[1];
          y_corners[2]=drawImage->Geo->dfBBOX[1];
          y_corners[3]=drawImage->Geo->dfBBOX[3];*/
          if((x_corners[0]>=DBL_MAX||x_corners[0]<=-DBL_MAX)&&x_div==1&&x_div==1){
            x_corners[0]=drawImage->Geo->dfBBOX[2];
            x_corners[1]=drawImage->Geo->dfBBOX[2];
            x_corners[2]=drawImage->Geo->dfBBOX[0];
            x_corners[3]=drawImage->Geo->dfBBOX[0];
            y_corners[0]=drawImage->Geo->dfBBOX[3];
            y_corners[1]=drawImage->Geo->dfBBOX[1];
            y_corners[2]=drawImage->Geo->dfBBOX[1];
            y_corners[3]=drawImage->Geo->dfBBOX[3];
          }

          if(status == 0){
            tile_offset_x=int(double(x)*tile_width);
            tile_offset_y=int(double(y)*tile_height);
            //CDBDebug("tile %d %d ; %f %f",tile_offset_x,tile_offset_y,tile_width,tile_height);
            if(sourceImage->dataObject[0]->dataType==CDF_CHAR||sourceImage->dataObject[0]->dataType==CDF_BYTE)
              drawFieldClass.drawRawField((char*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_UBYTE)
              drawStat=drawFieldClass.drawRawField((unsigned char*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_SHORT)
              drawStat=drawFieldClass.drawRawField((short*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_USHORT)
              drawStat=drawFieldClass.drawRawField((unsigned short*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_INT)
              drawStat=drawFieldClass.drawRawField((int*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_UINT)
              drawStat=drawFieldClass.drawRawField((unsigned int*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_FLOAT)
              drawStat=drawFieldClass.drawRawField((float*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
            else if(sourceImage->dataObject[0]->dataType==CDF_DOUBLE)
              drawStat=drawFieldClass.drawRawField((double*)sourceImage->dataObject[0]->data,x_corners,y_corners,tile_offset_x,tile_offset_y);
          }
          //if(drawStat!=0){
            //CDBDebug("drawStat: %d",drawStat);
          //}
        }
      }
    }
  }
};
#endif

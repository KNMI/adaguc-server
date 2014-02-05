/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Ernst de Vreede vreedede "at" knmi.nl
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

#include "CCairoPlotter.h"
#ifdef ADAGUC_USE_CAIRO

const char *CCairoPlotter::className="CCairoPlotter";

cairo_status_t writerFunc(void *closure, const unsigned char *data, unsigned int length) {
  FILE *fp=(FILE *)closure;
  int nrec=fwrite(data, length, 1, fp);
  if (nrec==1) {
    return CAIRO_STATUS_SUCCESS;
  }

  return CAIRO_STATUS_WRITE_ERROR;
}

//   void CCairoPlotter::pixelBlend(int x, int y,  unsigned char r,unsigned char g,unsigned char b,unsigned char alpha){
// //    fprintf(stderr, "plot([%d,%d], %d,%d,%d,%f)\n", x, y, r, g, b,a);
// //     
//     cairo_surface_flush(surface);
//     //plot the pixel at (x, y) with brightness c (where 0 ≤ c ≤ 1)
//     if(x<0||y<0)return;
//     if(x>=width||y>=height)return;
//     size_t p=x*4+y*stride;
//     float a1=1-(float(a)/255)*alpha;
//     if(a1==0){
//       ARGBByteBuffer[p]=b;
//       ARGBByteBuffer[p+1]=g;
//       ARGBByteBuffer[p+2]=r;
//       ARGBByteBuffer[p+3]=255;
//     }else{
//       // ALpha is increased
//       float sf=ARGBByteBuffer[p+3];
//       float alphaRatio=(alpha*(1-sf/255));
//       float tf=sf+a*alphaRatio;if(tf>255)tf=255;
//       float a2=1-a1;//1-alphaRatio;
//       float sr=ARGBByteBuffer[p+2];sr=sr*a1+r*a2;if(sr>255)sr=255;
//       float sg=ARGBByteBuffer[p+1];sg=sg*a1+g*a2;if(sg>255)sg=255;
//       float sb=ARGBByteBuffer[p];sb=sb*a1+b*a2;if(sb>255)sb=255;
//       ARGBByteBuffer[p]=(unsigned char)sb;
//       ARGBByteBuffer[p+1]=(unsigned char)sg;
//       ARGBByteBuffer[p+2]=(unsigned char)sr;
//       ARGBByteBuffer[p+3]=(unsigned char)tf;
//     }
//     cairo_surface_mark_dirty(surface);
//   }
// void CCairoPlotter::pixelBlend(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a){
//     if(x<0||y<0)return;
//     if(x>=width||y>=height)return;
//     float a1=1-(float(a)/255);//*alpha;
//     if(a==255){
//       
//       size_t p=x*4+y*stride;
//       ARGBByteBuffer[p+2]=r;
//       ARGBByteBuffer[p+1]=g;
//       ARGBByteBuffer[p+0]=b;
//       ARGBByteBuffer[p+3]=255;
//     }else{
//       size_t p=x*4+y*stride;
//       // ALpha is increased
//       float sf=ARGBByteBuffer[p+3];  
//       float alphaRatio=(1-sf/255);
//       float tf=sf+a*alphaRatio;if(tf>255)tf=255;  
//       if(sf==0.0f)a1=0;
//       float a2=1-a1;//1-alphaRatio;
//       //CDBDebug("Ratio: a1=%2.2f a2=%2.2f",a1,sf);
//       
//       float sr=ARGBByteBuffer[p+2];sr=sr*a1+r*a2;if(sr>255)sr=255;
//       float sg=ARGBByteBuffer[p+1];sg=sg*a1+g*a2;if(sg>255)sg=255;
//       float sb=ARGBByteBuffer[p+0];sb=sb*a1+b*a2;if(sb>255)sb=255;
//       ARGBByteBuffer[p+2]=(unsigned char)sr;
//       ARGBByteBuffer[p+1]=(unsigned char)sg;
//       ARGBByteBuffer[p+0]=(unsigned char)sb;
//       
//       ARGBByteBuffer[p+3]=(unsigned char)tf;
//       
//       
//     }
//     
//   }
  
void CCairoPlotter::pixelBlend(int x,int y, unsigned char newR,unsigned char newG,unsigned char newB,unsigned char newA){
  if(x<0||y<0)return;
  if(x>=width||y>=height)return;
  size_t p=x*4+y*stride;
  if(newA!=255){
    float oldB = float(ARGBByteBuffer[p]);
    float oldG = float(ARGBByteBuffer[p+1]);
    float oldR = float(ARGBByteBuffer[p+2]);
    float oldA = float(ARGBByteBuffer[p+3]);
    
    //float alpha = float(newA)/255.;
  
    //float nalpha = 1-alpha;
    
    
    float alphaRatio=(1-oldA/255);
    float tf=oldA+float(newA)*alphaRatio;if(tf>255)tf=255;  
    float a1=1-(float(newA)/255);//*alpha;
    if(oldA==0.0f)a1=0;
    float a2=1-a1;//1-alphaRatio;
    
    ARGBByteBuffer[p]  =float(newB)*a2+oldB*a1;
    ARGBByteBuffer[p+1]=float(newG)*a2+oldG*a1;
    ARGBByteBuffer[p+2]=float(newR)*a2+oldR*a1;
    ARGBByteBuffer[p+3]=tf;//float(newA)*alpha+oldA*nalpha;
    //pixel(x,y,ARGBByteBuffer[p+2], ARGBByteBuffer[p+1], ARGBByteBuffer[p], ARGBByteBuffer[p+3]);
  }else{
  
  
    ARGBByteBuffer[p]=newB;
    ARGBByteBuffer[p+1]=newG;
    ARGBByteBuffer[p+2]=newR;
    ARGBByteBuffer[p+3]=newA;
  }
}
  
    void CCairoPlotter::pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x*4+y*stride;
    if(a!=255){
    ARGBByteBuffer[p]=(unsigned char)((float(b)/256.0)*float(a));
    ARGBByteBuffer[p+1]=(unsigned char)((float(g)/256.0)*float(a));
    ARGBByteBuffer[p+2]=(unsigned char)((float(r)/256.0)*float(a));
    ARGBByteBuffer[p+3]=a;
    }else{
      ARGBByteBuffer[p]=b;
    ARGBByteBuffer[p+1]=g;
    ARGBByteBuffer[p+2]=r;
    ARGBByteBuffer[p+3]=a;
    }
  }
#endif

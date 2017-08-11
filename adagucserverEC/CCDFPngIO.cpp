/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Packages Png into a NetCDF file
 * Author:   Maarten Plieger (KNMI)
 * Date:     2017-08-08
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

#include "CCDFPngIO.h"


const char *CDFPngReader::className="PngReader";

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>


// https://aiddata.rvo.nl/projects

CDFPngReader::CPngRaster * CDFPngReader::read_png_file(const char* file_name, bool pngReadHeaderOnly) {
  //if(pngRaster->data!=NULL)return pngRaster;
  //int x, y;
  
  
  png_byte color_type;
  png_byte bit_depth;
  
  png_structp png_ptr;
  png_infop info_ptr;
//   int number_of_passes;
  png_bytep * row_pointers;
  
  unsigned char header[8];    // 8 is the maximum size that can be checked
  
  /* open file and test for it being a png */
  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    CDBError("[read_png_file] File %s could not be opened for reading", file_name);
    return NULL;
  }
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    CDBError("[read_png_file] File %s is not recognized as a PNG file", file_name);
    return NULL;
  }
  
  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
  if (!png_ptr) {
    CDBError("[read_png_file] png_create_read_struct failed");
    return NULL;
  }
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    CDBError("[read_png_file] png_create_info_struct failed");
    return NULL;
  }
  
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("[read_png_file] Error during init_io");
    return NULL;
  }
  
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  
  png_read_info(png_ptr, info_ptr);
  
  CDFPngReader::CPngRaster *pngRaster = new CPngRaster();
  
  pngRaster->width = png_get_image_width(png_ptr, info_ptr);
  pngRaster->height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  
//  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
  
  if(pngReadHeaderOnly == true){
    return pngRaster;
  }
  
  CDBDebug("read png data");
  
  /* read file */
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("[read_png_file] Error during read_image");
    return NULL;
  }
  
  
  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * pngRaster->height);
  
  for (size_t y=0; y<pngRaster->height; y++)
    row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
  CDBDebug("start png_read_image");
  png_read_image(png_ptr, row_pointers);
  CDBDebug("/done png_read_image");  
  CDBDebug("PNG data: [%d, %d, %d, %d]", pngRaster->width, pngRaster->height, (int)bit_depth, color_type);
  int bpp = 4;
  int pngwidthdivisor = 1;
  if(bit_depth==4){

    pngwidthdivisor=2;
  }
  if(bit_depth==1){
    
    pngwidthdivisor=8;
  }
  if(color_type==2){
     bpp = 3;
  }
  int num_palette;
  png_colorp palette;
  if(color_type==3){
    bpp = 1;
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
  }
  CDBDebug("pngRaster->data already defined, return");
  
  pngRaster->data = new unsigned char[pngRaster->width*pngRaster->height*4];
  for (size_t y=0; y<pngRaster->height; y++){
    unsigned char *line = row_pointers[y];
    for(size_t x=0;x<pngRaster->width/pngwidthdivisor;x+=1){
      
      
      if(bpp==1){
        unsigned char i  = line[x*bpp+0];
        if(bit_depth==1){
          int d=1;
          for(int j=0;j<8;j++){
            pngRaster->data[x*32 + j*4 + 0 +(y*pngRaster->width*4)]= palette[(i&d)/d].red; 
            pngRaster->data[x*32 + j*4 + 1 +(y*pngRaster->width*4)]= palette[(i&d)/d].green; 
            pngRaster->data[x*32 + j*4 + 2 +(y*pngRaster->width*4)]= palette[(i&d)/d].blue; 
            pngRaster->data[x*32 + j*4 + 3 +(y*pngRaster->width*4)]= 255;
            d=d+d;
          }
        }
        if(bit_depth==4){
          pngRaster->data[x*8 + 0 +(y*pngRaster->width*4)]= palette[i/16].red; 
          pngRaster->data[x*8 + 1 +(y*pngRaster->width*4)]= palette[i/16].green; 
          pngRaster->data[x*8 + 2 +(y*pngRaster->width*4)]= palette[i/16].blue; 
          pngRaster->data[x*8 + 3 +(y*pngRaster->width*4)]= 255;
          pngRaster->data[x*8 + 4 +(y*pngRaster->width*4)]= palette[i%16].red; 
          pngRaster->data[x*8 + 5 +(y*pngRaster->width*4)]= palette[i%16].green; 
          pngRaster->data[x*8 + 6 +(y*pngRaster->width*4)]= palette[i%16].blue; 
          pngRaster->data[x*8 + 7 +(y*pngRaster->width*4)]= 255;
        }
        if(bit_depth==8){
          pngRaster->data[x*4 + 0 +(y*pngRaster->width*4)]= palette[i].red; 
          pngRaster->data[x*4 + 1 +(y*pngRaster->width*4)]= palette[i].green; 
          pngRaster->data[x*4 + 2 +(y*pngRaster->width*4)]= palette[i].blue; 
          pngRaster->data[x*4 + 3 +(y*pngRaster->width*4)]= 255;
        }
      }else{
        pngRaster->data[x*4 + 0 +(y*pngRaster->width*4)]= line[x*bpp+0]; 
        if(bpp==3 || bpp ==4){
          pngRaster->data[x*4 + 1 +(y*pngRaster->width*4)]= line[x*bpp+1];
          pngRaster->data[x*4 + 2 +(y*pngRaster->width*4)]= line[x*bpp+2];
        }
        if(bpp==4){
          pngRaster->data[x*4 + 3 +(y*pngRaster->width*4)]= line[x*4+3];
        }else{
          pngRaster->data[x*4 + 3 +(y*pngRaster->width*4)]= 255;
        }
      }

    }
  }
  
  fclose(fp);
  CDBDebug("done");
  return pngRaster;
}


int CDFPngReader::open(const char *fileName){

  if(cdfObject == NULL){
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function should be called by CDFObject open routines.");
    return 1;
  }
  CDBDebug("open [%s]",fileName);
  this->fileName=fileName;
  CDFPngReader::CPngRaster*pngRaster;
  pngRaster= read_png_file(this->fileName.c_str(),true);
  if(pngRaster == NULL){
    CDBError("Unable to open PNG check logs");
    return 1;
  }
  if(cdfCache!=NULL){
    int cacheStatus = cdfCache->open(fileName,cdfObject,false);
    if(cacheStatus == 0) {
      CDBDebug("Succesfully opened from cache for file %s",fileName);
      return 0;
    }
  }
  
  //This is opendap, there the geojson has already been converted to CDM by an IOServiceProvider.
  if(this->fileName.indexOf("http")==0){
    CDBDebug("This is opendap, no conversion needed.");
    
    return 0;
  }
  
  cdfObject->addAttribute(new CDF::Attribute("Conventions","CF-1.6"));
  cdfObject->addAttribute(new CDF::Attribute("history","Metadata adjusted by ADAGUC from PNG to NetCDF-CF"));
  
  CT::string fileBaseName;
  const char *last=rindex(fileName, '/');
  if ((last!=NULL)&&(*last)) {
    fileBaseName.copy(last+1);
  } else {
    fileBaseName.copy(fileName);
  }
  
  if (!((strlen(fileName)>4)||(strcmp("json",fileName+strlen(fileName-4))==0))){
    return 1;
  }
  CT::string jsonData=CReadFile::open(fileName);
  
  CDF::Dimension * xDim = cdfObject->addDimension(new CDF::Dimension("x",pngRaster->width));

  CDF::Variable * xVar = cdfObject->addVariable(new CDF::Variable(xDim->getName().c_str(),CDF_DOUBLE,&xDim,1, true));
  CDF::Dimension * yDim = cdfObject->addDimension(new CDF::Dimension("y",pngRaster->height));
  CDF::Variable * yVar = cdfObject->addVariable(new CDF::Variable(yDim->getName().c_str(),CDF_DOUBLE,&yDim,1, true));

  
  
  CDBDebug("Defining PNG variable");
  CDF::Dimension *varDims[]={yDim, xDim};
  CDF::Variable * PNGData = cdfObject->addVariable(new CDF::Variable("pngdata",CDF_UINT,varDims,2, false));
  
  xVar->setCDFReaderPointer(this);
  xVar->setParentCDFObject(cdfObject);
  
  yVar->setCDFReaderPointer(this);
  yVar->setParentCDFObject(cdfObject);
  
  
  PNGData->setCDFReaderPointer(this);
  PNGData->setParentCDFObject(cdfObject);
  
  
  PNGData->setAttributeText("ADAGUC_BASENAME",fileBaseName.c_str());
  PNGData->setAttributeText("grid_mapping","crs");
  
  CT::string * parts=this->fileName.splitToArray("/");
  
  if(parts->count>3){
    int zoom = parts[parts->count-3].toInt();
    int level = 11-zoom;
    cdfObject->setAttribute("adaguctilelevel",CDF_INT,&level,1);
  }
  delete[] parts;
  
  CDF::Variable * CRS = cdfObject->addVariable(new CDF::Variable("crs",CDF_UINT,NULL,0, false));
  CRS->setAttributeText("proj4","+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs");
  CDBDebug("Done");
  
  
  return 0;
}
#include <math.h>

class BoundingBox {
public:
  double north;
  double south;
  double east;
  double west;   
};
double pi=3.141592654;

static double tile2lon(int x, int z) {
  return double(x) / pow(2.0, z) * 360.0 - 180;
}

static double tile2lat(int y, int z) {
  double n = pi - (2.0 * pi * y) / pow(2.0, z);
  return (180.0 / pi *atan(sinh(n)));
}

BoundingBox tile2boundingBox(int x, int y, int zoom) {
  BoundingBox bb;
  bb.north = tile2lat(y, zoom);
  bb.south = tile2lat(y + 1, zoom);
  bb.west = tile2lon(x, zoom);
  bb.east = tile2lon(x + 1, zoom);
  return bb;
}

/*
 * Mercator transformation
 * accounts for the fact that the earth is not a sphere, but a spheroid
 */
#define D_R (M_PI / 180.0)
#define R_D (180.0 / M_PI)
#define R_MAJOR 6378137.0
#define R_MINOR 6356752.3142
#define RATIO (R_MINOR/R_MAJOR)
#define ECCENT (sqrt(1.0 - (RATIO * RATIO)))
#define COM (0.5 * ECCENT)

static double deg_rad (double ang) {
  return ang * D_R;
}

double merc_x (double lon) {
  return R_MAJOR * deg_rad (lon);
}

double merc_y (double lat) {
  lat = fmin (89.5, fmax (lat, -89.5));
  double phi = deg_rad(lat);
  double sinphi = sin(phi);
  double con = ECCENT * sinphi;
  con = pow((1.0 - con) / (1.0 + con), COM);
  double ts = tan(0.5 * (M_PI * 0.5 - phi)) / con;
  return 0 - R_MAJOR * log(ts);
}

static double rad_deg (double ang) {
  return ang * R_D;
}

double merc_lon (double x) {
  return rad_deg(x) / R_MAJOR;
}

double merc_lat (double y) {
  double ts = exp ( -y / R_MAJOR);
  double phi = M_PI_2 - 2 * atan(ts);
  double dphi = 1.0;
  int i;
  for (i = 0; fabs(dphi) > 0.000000001 && i < 15; i++) {
    double con = ECCENT * sin (phi);
    dphi = M_PI_2 - 2 * atan (ts * pow((1.0 - con) / (1.0 + con), COM)) - phi;
    phi += dphi;
  }
  return rad_deg (phi);
}


class P{
public:
  double x,y;
};
P tileXYZtoMerc(int tile_x, int tile_y, int zoom){
  double tileSize = 256;
  double initialResolution  = 2 * pi * 6378137 / tileSize;
  double originShift = 2 * pi * 6378137 / 2.0;
  double tileRes = initialResolution / pow(2, zoom);
  P p;
  p.x = originShift- tile_x * tileRes;
  p.y = originShift-tile_y * tileRes;
  return p;
}

BoundingBox getBounds(int tile_x, int tile_y, int zoom){
  P p1 = tileXYZtoMerc( (tile_x -1 )* 256, (tile_y )* 256, zoom );
  P p2 = tileXYZtoMerc( (tile_x) * 256, (tile_y +1) * 256, zoom );
  BoundingBox b;
  b.west=p2.x;
  b.east=p1.x;
  b.south=p2.y;
  b.north=p1.y;
  return b;

}
int CDFPngReader::_readVariableData(CDF::Variable *var, CDFType type){
  CDFPngReader::CPngRaster*pngRaster;

  bool isLatlon =false;
  bool isOsm=true;
  double tilex1,tilex2,tiley1,tiley2;
  if(isOsm){
    CT::string * parts=this->fileName.splitToArray("/");
    
    if(parts->count>3){
      int zoom = parts[parts->count-3].toInt();
      int tile_y = parts[parts->count-1].splitToStack(".")[0].toInt();
      int tile_x = parts[parts->count-2].toInt();
      BoundingBox bbox = getBounds(tile_x,tile_y,zoom);
      
      CDBDebug("%d %d %d",tile_x,tile_y,zoom);
      tilex1=(bbox.west);
      tiley1=(bbox.south);
      tilex2=(bbox.east);
      tiley2=(bbox.north);
      CDBDebug("%f %f %f %f",merc_x(bbox.west),merc_y(bbox.south),merc_x(bbox.east),merc_y(bbox.north));
    }
    delete[] parts;
  }
  
  if(var->name.equals("x")){
    pngRaster= read_png_file(this->fileName.c_str(),true);
    if(pngRaster == NULL){
      CDBError("Unable to open PNG check logs");
      return 1;
    }
    CDF::Variable *xVar =var;
    CDF::Dimension *xDim =((CDFObject*)var->getParentCDFObject())->getDimension(var->name.c_str());
    
    xVar->allocateData(xDim->getSize());
    
    if(isLatlon){
      for(size_t j=0;j<xDim->getSize();j++){
        float r=(float(j)/float(xDim->getSize())) * 360.0f;
        ((double*)xVar->data)[j]=(r-180.0f);
      }
    }
    if(isOsm){     
      for(size_t j=0;j<xDim->getSize();j++){
        float r=(float(j)/float(xDim->getSize())) * (tilex2-tilex1);
        ((double*)xVar->data)[j]=(r-tilex1);
      }
    }
  }
  
  if(var->name.equals("y")){
    pngRaster= read_png_file(this->fileName.c_str(),true);
    if(pngRaster == NULL){
      CDBError("Unable to open PNG check logs");
      return 1;
    }
    CDF::Variable *yVar =var;
    CDF::Dimension *yDim =((CDFObject*)var->getParentCDFObject())->getDimension(var->name.c_str());
  
    yVar->allocateData(yDim->getSize());
    if(isLatlon){
      for(size_t j=0;j<yDim->getSize();j++){
        float r=(float(j)/float(yDim->getSize())) * 180.0f;
        ((double*)yVar->data)[j]=(90.0f-r);
      }
    }
    if(isOsm){
      for(size_t j=0;j<yDim->getSize();j++){
        float r=(float(j)/float(yDim->getSize())) * (tiley2-tiley1);
        ((double*)yVar->data)[j]=(tiley2-r);
      }
    }
  }
  if(var->name.equals("pngdata")){
    pngRaster= read_png_file(this->fileName.c_str(),false);  
    var->allocateData(pngRaster->width*pngRaster->height);
    for(size_t y=0;y<pngRaster->height;y++){
      for(size_t x=0;x<pngRaster->width;x++){
        size_t j=x+y*pngRaster->width;
        ((unsigned int*)var->data)[x+y*pngRaster->width]=pngRaster->data[j*4+0]+pngRaster->data[j*4+1]*256+pngRaster->data[j*4+2]*256*256+pngRaster->data[j*4+3]*(256*256*256);
      }
    }
  }
  return 0;
  
}

int CDFPngReader::_readVariableData(CDF::Variable *var,CDFType type,size_t *start,size_t *count,ptrdiff_t  *stride){
  CDBDebug("_readVariableData %s %d",var->name.c_str(),type);
  CDFPngReader::CPngRaster*pngRaster;
  size_t requestedSize = 1;
  
  for(size_t j=0;j<var->dimensionlinks.size();j++){
    requestedSize*=count[j];
    CDBDebug("%d %d %d",j,start[j],count[j]);
  }
  var->allocateData(requestedSize);
  
  if(var->name.equals("pngdata")){
    pngRaster= read_png_file(this->fileName.c_str(),false);
    if(pngRaster == NULL){
      CDBError("Unable to open PNG check logs");
      return 1;
    }
    
    for(size_t y=0;y<count[1];y++){
      for(size_t x=0;x<count[0];x++){
        size_t j=(x+count[0])+(y+count[1])*pngRaster->width;
        ((unsigned int*)var->data)[x+y*count[0]]=pngRaster->data[j*4+0]+pngRaster->data[j*4+1]*256+pngRaster->data[j*4+2]*256*256+pngRaster->data[j*4+3]*(256*256*256);
      }
    }
  }
  return 0;
  
}

int CDFPngReader::close(){
  return 0;
}


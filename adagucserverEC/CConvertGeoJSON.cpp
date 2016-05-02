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
//       * 
      *      http://www.apache.org/licenses/LICENSE-2.0
      * 
      * Unless required by applicable law or agreed to in writing, software
      * distributed under the License is distributed on an "AS IS" BASIS,
      * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
      * See the License for the specific language governing permissions and
      * limitations under the License.
      * 
      ******************************************************************************/

      #include "CConvertGeoJSON.h"
      #include "CConvertUGRIDMesh.h"
//      #include "CFillTriangle.h"
      #include "CImageWarper.h"
      #include <values.h>
      #include <string>
      #include <map>
      #include <cstdlib>
      #include <queue>
      #define CCONVERTGEOJSON_DEBUG
      const char *CConvertGeoJSON::className="CConvertGeoJSON";

      #define CCONVERTUGRIDMESH_NODATA -32000
      #define MEASURETIME 1

      void CConvertGeoJSON::drawpoly2(float *imagedata,int w,int h,int polyCorners,float *polyXY,float value){
        //  public-domain code by Darel Rex Finley, 2007

      int  nodes, nodeX[polyCorners*2+1],  pixelY, i, j, swap ;
      int IMAGE_TOP = 0;
      int IMAGE_BOT = h;
      int IMAGE_LEFT = 0;
      int IMAGE_RIGHT = w;


      //  Loop through the rows of the image.
      for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {


        //  Build a list of nodes.
        nodes=0; j=polyCorners-1;
        for (i=0; i<polyCorners; i++) {
          if ((polyXY[i*2+1]<(double) pixelY && polyXY[j*2+1]>=(double) pixelY)
          || ( polyXY[j*2+1]<(double) pixelY && polyXY[i*2+1]>=(double) pixelY)) {
            nodeX[nodes++]=(int) (polyXY[i*2]+(pixelY-polyXY[i*2+1])/(polyXY[j*2+1]-polyXY[i*2+1])
            *(polyXY[j*2]-polyXY[i*2])); }
          j=i; }

        //  Sort the nodes, via a simple “Bubble” sort.
        i=0;
        while (i<nodes-1) {
          if (nodeX[i]>nodeX[i+1]) {
            swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
          else {
            i++; }}

        //  Fill the pixels between node pairs.
        for (i=0; i<nodes; i+=2) {
          if   (nodeX[i  ]>=IMAGE_RIGHT) break;
          if   (nodeX[i+1]> IMAGE_LEFT ) {
            if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
            if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
            for (j=nodeX[i]; j<nodeX[i+1]; j++){
                imagedata[int(j)+int(pixelY)*w] = value;
              }
            }
          }
        }
      }

    void CConvertGeoJSON::drawpolyWithHoles(float *imagedata,int w,int h,int polyCorners,float *polyXY,float value,int holes, int *holeCorners, float*holeXY[]){
        //  public-domain code by Darel Rex Finley, 2007

      int  nodes, nodeX[polyCorners*2+1],  pixelY, i, j, swap ;
      int IMAGE_TOP = 0;
      int IMAGE_BOT = h;
      int IMAGE_LEFT = 0;
      int IMAGE_RIGHT = w;

      //Allocate  scanline
      float scanline[w];
      //  Loop through the rows of the image.
      for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {

        for (i=0; i<w;i++) scanline[i]=-9999;

        //  Build a list of nodes.
        nodes=0; j=polyCorners-1;
        for (i=0; i<polyCorners; i++) {
          if ((polyXY[i*2+1]<(double) pixelY && polyXY[j*2+1]>=(double) pixelY)
          || ( polyXY[j*2+1]<(double) pixelY && polyXY[i*2+1]>=(double) pixelY)) {
            nodeX[nodes++]=(int) (polyXY[i*2]+(pixelY-polyXY[i*2+1])/(polyXY[j*2+1]-polyXY[i*2+1])
            *(polyXY[j*2]-polyXY[i*2])); }
          j=i; }

        //  Sort the nodes, via a simple “Bubble” sort.
        i=0;
        while (i<nodes-1) {
          if (nodeX[i]>nodeX[i+1]) {
            swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
          else {
            i++; }}

        //  Fill the pixels between node pairs.
        for (i=0; i<nodes; i+=2) {
          if   (nodeX[i  ]>=IMAGE_RIGHT) break;
          if   (nodeX[i+1]> IMAGE_LEFT ) {
            if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
            if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
            for (j=nodeX[i]; j<nodeX[i+1]; j++){
//                imagedata[int(j)+int(pixelY)*w] = value;
                scanline[int(j)] = value;
            }
          }
        }
        
        for (int h=0; h<holes; h++) {
          //  Build a list of hole nodes.
          nodes=0; j=holeCorners[h]-1;
          for (i=0; i<holeCorners[h]; i++) {
            if ((holeXY[h][i*2+1]<(double) pixelY && holeXY[h][j*2+1]>=(double) pixelY)
            || ( holeXY[h][j*2+1]<(double) pixelY && holeXY[h][i*2+1]>=(double) pixelY)) {
              nodeX[nodes++]=(int) (holeXY[h][i*2]+(pixelY-holeXY[h][i*2+1])/(holeXY[h][j*2+1]-holeXY[h][i*2+1])
              *(holeXY[h][j*2]-holeXY[h][i*2])); }
            j=i; }

          //  Sort the nodes, via a simple “Bubble” sort.
          i=0;
          while (i<nodes-1) {
            if (nodeX[i]>nodeX[i+1]) {
              swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
            else {
              i++; }}

          //  Fill the pixels between node pairs.
          for (i=0; i<nodes; i+=2) {
            if   (nodeX[i  ]>=IMAGE_RIGHT) break;
            if   (nodeX[i+1]> IMAGE_LEFT ) {
              if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
              if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
              for (j=nodeX[i]; j<nodeX[i+1]; j++){
  //                imagedata[int(j)+int(pixelY)*w] = value;
                  scanline[int(j)] = -9999;
              }
            }
          }
        }
        int startScanLine=pixelY*w;
        for (i=0; i<w; i++) {
          if (scanline[i]!=-9999) {
            imagedata[i+startScanLine]=scanline[i];
          }
        }
      }  
    }

    void CConvertGeoJSON::drawpolyWithHoles_index(unsigned short int *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short int value,int holes,int *holeCorners, float*holeXY[]){
        //  public-domain code by Darel Rex Finley, 2007

      int  nodes, nodeX[polyCorners*2+1],  pixelY, i, j, swap ;
      int IMAGE_TOP = 0;
      int IMAGE_BOT = h;
      int IMAGE_LEFT = 0;
      int IMAGE_RIGHT = w;

      //Allocate  scanline
      unsigned short scanline[w];
      //  Loop through the rows of the image.
      for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {

        for (i=0; i<w;i++) scanline[i]=65535u;

        //  Build a list of nodes.
        nodes=0; j=polyCorners-1;
        for (i=0; i<polyCorners; i++) {
          if ((polyXY[i*2+1]<(double) pixelY && polyXY[j*2+1]>=(double) pixelY)
          || ( polyXY[j*2+1]<(double) pixelY && polyXY[i*2+1]>=(double) pixelY)) {
            nodeX[nodes++]=(int) (polyXY[i*2]+(pixelY-polyXY[i*2+1])/(polyXY[j*2+1]-polyXY[i*2+1])
            *(polyXY[j*2]-polyXY[i*2])); }
          j=i; }

        //  Sort the nodes, via a simple “Bubble” sort.
        i=0;
        while (i<nodes-1) {
          if (nodeX[i]>nodeX[i+1]) {
            swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
          else {
            i++; }}

        //  Fill the pixels between node pairs.
        for (i=0; i<nodes; i+=2) {
          if   (nodeX[i  ]>=IMAGE_RIGHT) break;
          if   (nodeX[i+1]> IMAGE_LEFT ) {
            if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
            if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
            for (j=nodeX[i]; j<nodeX[i+1]; j++){
//                imagedata[int(j)+int(pixelY)*w] = value;
                scanline[int(j)] = value;
            }
          }
        }
        
        for (int h=0; h<holes; h++) {
          //  Build a list of hole nodes.
          nodes=0; j=holeCorners[h]-1;
          for (i=0; i<holeCorners[h]; i++) {
            if ((holeXY[h][i*2+1]<(double) pixelY && holeXY[h][j*2+1]>=(double) pixelY)
            || ( holeXY[h][j*2+1]<(double) pixelY && holeXY[h][i*2+1]>=(double) pixelY)) {
              nodeX[nodes++]=(int) (holeXY[h][i*2]+(pixelY-holeXY[h][i*2+1])/(holeXY[h][j*2+1]-holeXY[h][i*2+1])
              *(holeXY[h][j*2]-holeXY[h][i*2])); }
            j=i; }

          //  Sort the nodes, via a simple “Bubble” sort.
          i=0;
          while (i<nodes-1) {
            if (nodeX[i]>nodeX[i+1]) {
              swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
            else {
              i++; }}

          //  Fill the pixels between node pairs.
          for (i=0; i<nodes; i+=2) {
            if   (nodeX[i  ]>=IMAGE_RIGHT) break;
            if   (nodeX[i+1]> IMAGE_LEFT ) {
              if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
              if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
              for (j=nodeX[i]; j<nodeX[i+1]; j++){
  //                imagedata[int(j)+int(pixelY)*w] = value;
                  scanline[int(j)] = 65535u;
              }
            }
          }
        }
        unsigned int startScanLineY=pixelY*w;
        for (i=0; i<w; i++) {
          if (scanline[i]!=65535u) {
            imagedata[i+startScanLineY]=scanline[i];
          }
        }
      }  
    }

      void CConvertGeoJSON::drawpoly2_index(unsigned short *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short value){
        //  public-domain code by Darel Rex Finley, 2007

      int  nodes, nodeX[polyCorners*2+1],  pixelY, i, j, swap ;
      int IMAGE_TOP = 0;
      int IMAGE_BOT = h;
      int IMAGE_LEFT = 0;
      int IMAGE_RIGHT = w;

      //  Loop through the rows of the image.
      for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {


        //  Build a list of nodes.
        nodes=0; j=polyCorners-1;
        for (i=0; i<polyCorners; i++) {
          if ((polyXY[i*2+1]<(double) pixelY && polyXY[j*2+1]>=(double) pixelY)
          || ( polyXY[j*2+1]<(double) pixelY && polyXY[i*2+1]>=(double) pixelY)) {
            nodeX[nodes++]=(int) (polyXY[i*2]+(pixelY-polyXY[i*2+1])/(polyXY[j*2+1]-polyXY[i*2+1])
            *(polyXY[j*2]-polyXY[i*2])); }
          j=i; }

        //  Sort the nodes, via a simple “Bubble” sort.
        i=0;
        while (i<nodes-1) {
          if (nodeX[i]>nodeX[i+1]) {
            swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
          else {
            i++; }}

        //  Fill the pixels between node pairs.
        for (i=0; i<nodes; i+=2) {
          if   (nodeX[i  ]>=IMAGE_RIGHT) break;
          if   (nodeX[i+1]> IMAGE_LEFT ) {
            if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
            if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
            for (j=nodeX[i]; j<nodeX[i+1]; j++){
                imagedata[int(j)+int(pixelY)*w] = value;
              }
            }
          }
        }
      }
      
      void CConvertGeoJSON::drawpoly(float *imagedata,int w,int h,int polyCorners,float *polyX,float *polyY,float value){
        //  public-domain code by Darel Rex Finley, 2007

      int  nodes, nodeX[polyCorners*2+1],  pixelY, i, j, swap ;
      int IMAGE_TOP = 0;
      int IMAGE_BOT = h;
      int IMAGE_LEFT = 0;
      int IMAGE_RIGHT = w;


      //  Loop through the rows of the image.
      for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {


        //  Build a list of nodes.
        nodes=0; j=polyCorners-1;
        for (i=0; i<polyCorners; i++) {
          if ((polyY[i]<(double) pixelY && polyY[j]>=(double) pixelY)
          || ( polyY[j]<(double) pixelY && polyY[i]>=(double) pixelY)) {
            nodeX[nodes++]=(int) (polyX[i]+(pixelY-polyY[i])/(polyY[j]-polyY[i])
            *(polyX[j]-polyX[i])); }
          j=i; }

        //  Sort the nodes, via a simple “Bubble” sort.
        i=0;
        while (i<nodes-1) {
          if (nodeX[i]>nodeX[i+1]) {
            swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
          else {
            i++; }}

        //  Fill the pixels between node pairs.
        for (i=0; i<nodes; i+=2) {
          if   (nodeX[i  ]>=IMAGE_RIGHT) break;
          if   (nodeX[i+1]> IMAGE_LEFT ) {
            if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
            if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
            for (j=nodeX[i]; j<nodeX[i+1]; j++){
                imagedata[int(j)+int(pixelY)*w] = value;
              }
            }
          }
        }
      }
      
      std::map<std::string, BBOX> CConvertGeoJSON::BBOXStore;
      std::map<std::string, std::vector<Feature*> > CConvertGeoJSON::featureStore;

      /**
      * This function adjusts the cdfObject by creating virtual 2D variables
      */
      int CConvertGeoJSON::convertGeoJSONHeader( CDFObject *cdfObject ){
        //Check whether this is really an ugrid file
        CDF::Variable * jsonVar;
        try{
          jsonVar=cdfObject->getVariable("jsoncontent");
        }catch(int e){
          return 1;
        }
#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("Using CConvertGeoJSON.cpp");
#endif
        
      //Standard bounding box of adaguc data is worldwide
      //   CDF::Variable *pointLon;
      //   CDF::Variable *pointLat;

        jsonVar->readData(CDF_CHAR);
        CT::string inputjsondata= (char *)jsonVar->data;
 //       CDBDebug("Reading JSON from [%d] %s %d", inputjsondata.length(), inputjsondata.c_str(),inputjsondata.charAt(inputjsondata.length()-1));
        json_value *json= json_parse ((json_char*)inputjsondata.c_str(),inputjsondata.length()); 
#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("JSON result: %x", json);
#endif
        #ifdef MEASURETIME
          StopWatch_Stop("GeoJSON DATA");
        #endif
          
        if (json == 0) {
          CDBDebug("Error parsing jsonfile");
          return 1;
        }

        std::vector<Feature*> features;
        BBOX dfBBOX;
        getBBOX(dfBBOX, *json, features);  

        std::string geojsonkey=jsonVar->getAttributeNE("ADAGUC_BASENAME")->toString().c_str();
        featureStore[geojsonkey]=features;
         
        json_value_free(json);
        #ifdef MEASURETIME
        StopWatch_Stop("DATA READ");
        #endif

        #ifdef MEASURETIME
          StopWatch_Stop("BBOX Calculated");
        #endif
        CDF::Dimension *dimFeatures=new CDF::Dimension();
        dimFeatures->name="features";
        dimFeatures->setSize(features.size());
        cdfObject->addDimension(dimFeatures);         
        
        //Default size of adaguc 2dField is 2x2
        int width=2;
        int height=2;
        
        double cellSizeX=(dfBBOX.urX-dfBBOX.llX)/double(width);
        double cellSizeY=(dfBBOX.urY-dfBBOX.llY)/double(height);
        double offsetX=dfBBOX.llX;
        double offsetY=dfBBOX.llY;
        
        //Add geo variables, only if they are not there already
        CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
        CDF::Dimension *dimY = cdfObject->getDimensionNE("y");
        CDF::Variable *varX = cdfObject->getVariableNE("x");
        CDF::Variable *varY = cdfObject->getVariableNE("y");
        if(dimX==NULL||dimY==NULL||varX==NULL||varY==NULL) {
          //If not available, create new dimensions and variables (X,Y,T)
#ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("CellsizeX: %f", cellSizeX);
#endif
          //For x 
          dimX=new CDF::Dimension();
          dimX->name="x";
          dimX->setSize(width);
          cdfObject->addDimension(dimX);
          varX = new CDF::Variable();
          varX->setType(CDF_DOUBLE);
          varX->name.copy("x");
          varX->isDimension=true;
          varX->dimensionlinks.push_back(dimX);
          cdfObject->addVariable(varX);
          CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);

          //For y 
          dimY=new CDF::Dimension();
          dimY->name="y";
          dimY->setSize(height);
          cdfObject->addDimension(dimY);
          varY = new CDF::Variable();
          varY->setType(CDF_DOUBLE);
          varY->name.copy("y");
          varY->isDimension=true;
          varY->dimensionlinks.push_back(dimY);
          cdfObject->addVariable(varY);
          CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
          
          //Fill in the X and Y dimensions with the array of coordinates
          for(size_t j=0;j<dimX->length;j++){
            double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
            ((double*)varX->data)[j]=x;
          }
          for(size_t j=0;j<dimY->length;j++){
            double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
            ((double*)varY->data)[j]=y;
          }
        }
        CDF::Variable *new2DVar = new CDF::Variable();
        cdfObject->addVariable(new2DVar);

        new2DVar->dimensionlinks.push_back(dimY);
        new2DVar->dimensionlinks.push_back(dimX);
        
        new2DVar->setType(CDF_USHORT);
        new2DVar->name="polygons";
        unsigned short f=65535u;
        new2DVar->setAttribute("_FillValue",CDF_USHORT,&f,1);

#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("CConvertGeoJSON::convertGeoJSONHeader() done");
#endif
        return 0;
      }

      // void CConvertGeoJSON::getPolygons(json_value features) {
      // }

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

      void CConvertGeoJSON::getBBOX(BBOX &bbox, json_value &json, std::vector<Feature*>& featureMap) {
        double minLat=90.,maxLat=-90.,minLon=180,maxLon=-180;
        bool BBOXFound=false;
        if (json.type==json_object){
          CT::string type;
          if (json["type"].type!=json_null) {
#ifdef CCONVERTGEOJSON_DEBUG
            CDBDebug("type found");
#endif
            type=json["type"].u.string.ptr;
#ifdef CCONVERTGEOJSON_DEBUG
            CDBDebug("type: %s\n", type.c_str());
#endif
            if (type.equals("FeatureCollection")) {
              json_value bbox_v=json["bbox"];
              if (bbox_v.type==json_array) {
                bbox.llX=(double)(*bbox_v.u.array.values[0]);
                bbox.llY=(double)(*bbox_v.u.array.values[1]);
                bbox.urX=(double)(*bbox_v.u.array.values[2]);
                bbox.urY=(double)(*bbox_v.u.array.values[3]);
                BBOXFound=true;
              }
            }
            json_value features=json["features"];
            if (features.type==json_array) {
#ifdef CCONVERTGEOJSON_DEBUG
              CDBDebug("features found");
#endif
              for (unsigned int cnt = 0; cnt < features.u.array.length; ++cnt){
                json_value feature = *features.u.array.values[cnt];
                CT::string featureId;
                json_value id = feature["id"];
                if (id.type==json_string) {
//                CDBDebug("id=%s", id.u.string.ptr);
                  featureId=id.u.string.ptr;
                }
                Feature *feat=new Feature();
                
                json_value props = feature["properties"];
//               CDBDebug("props.type=%d", props.type);
                if (props.type==json_object){
                  for (unsigned int propCnt = 0; propCnt < props.u.object.length; ++propCnt){
                    json_object_entry propObject = props.u.object.values[propCnt];
                    CT::string propName(propObject.name);
                    json_value prop = *propObject.value;
                    if (prop.type==json_string) {
//                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
//                      CDBDebug("[%d] prop[%s]S ", cnt, propName.c_str());
                      feat->addProp(propName, prop.u.string.ptr);
                    } else if (prop.type==json_integer) {
//                      CDBDebug("[%d] prop[%s]I =%d", cnt, propName.c_str(), prop.u.integer);
                      feat->addProp(propName, prop.u.integer);
                    } 
                  }
                }
                if (featureId.length()==0) {
                  CT::string id_s=feat->getFp()["id"].toString().c_str();
                  if (!id_s.equals("NONE")) {
                    featureId=id;
                  } else {
                    id_s=feat->getFp()["NUTS_ID"].toString().c_str();
                    if (!id_s.equals("NONE")){
                      featureId=id_s;
                    } else {
                      featureId.print("%04d", cnt);
                    }
                  }
                }
                feat->setId(featureId);
//                CDBDebug("featureId: %s", featureId.c_str());

                json_value geom=feature["geometry"];
                if (geom.type==json_object){
                  json_value geomType=geom["type"];
                  if (geomType.type==json_string){
//                    CDBDebug("geomType: %s", geomType.u.string.ptr);
                    if (strcmp(geomType.u.string.ptr, "Polygon")==0) {
                      json_value coords=geom["coordinates"];
                      if (coords.type==json_array) {
//                        CDBDebug("  array of %d coords",coords.u.array.length);
                        for (unsigned int j=0; j<coords.u.array.length; j++) {
                          if (j==0) {
                            feat->newPolygon();
                          } else {
                            feat->newHole();
                          }
                          json_value polygon=*coords.u.array.values[j];
//                          CDBDebug("polygon: %d", polygon.u.array.length);
                          for (unsigned int i=0; i<polygon.u.array.length;i++) {
                            json_value pt=*polygon.u.array.values[i];
                            json_value lo=*pt.u.array.values[0];
                            json_value la=*pt.u.array.values[1];
                            double lon=(double)lo;
                            double lat=(double)la;
                            if (j==0) {
                              feat->addPoint((float)lon, (float)lat);
                            } else {
                              feat->addHolePoint((float)lon, (float)lat);
                            }
                            if (lat<minLat) minLat=lat;
                            if (lat>maxLat) maxLat=lat;
                            if (lon<minLon) minLon=lon;
                            if (lon>maxLon) maxLon=lon;
//                            CDBDebug("    %f,%f", (double)lo, (double)la);
                          }
                        }
                      } else {
//                        CDBDebug("  coords type of %d", coords.type);
                      }
                    } else if (strcmp(geomType.u.string.ptr, "MultiPolygon")==0) {
                      json_value multicoords=geom["coordinates"];
                      if (multicoords.type==json_array) {
//                        CDBDebug("  array of %d coords",multicoords.u.array.length);
                        for (unsigned int i=0; i<multicoords.u.array.length; i++) {
                          json_value coords=*multicoords.u.array.values[i];
                          if (coords.type==json_array) {
//                            CDBDebug("  array of %d coords",coords.u.array.length);
                            for (unsigned int j=0; j<coords.u.array.length; j++) {
                              if (j==0) {
                                feat->newPolygon();
                              } else {
                                feat->newHole();
                              }
                              json_value polygon=*coords.u.array.values[j];
//                              CDBDebug("polygon: %d", polygon.u.array.length);
                              for (unsigned int i=0; i<polygon.u.array.length;i++) {
                                json_value pt=*polygon.u.array.values[i];
                                json_value lo=*pt.u.array.values[0];
                                json_value la=*pt.u.array.values[1];
                                double lon=(double)lo;
                                double lat=(double)la;
                                if (j==0) {
                                  feat->addPoint((float)lon, (float)lat);
                                }else{
                                  feat->addHolePoint((float)lon, (float)lat);
                                }
                                if (lat<minLat) minLat=lat;
                                if (lat>maxLat) maxLat=lat;
                                if (lon<minLon) minLon=lon;
                                if (lon>maxLon) maxLon=lon;
  //                              CDBDebug("    %f,%f", (double)lo, (double)la);
                              }
                            }
                          } else {
//                            CDBDebug("  coords type of %d", coords.type);
                          }
                        }
                      }
                    }  
                  }
                }
//                CDBDebug("FEAT %s", feat.toString().c_str());
                featureMap.push_back(feat);
              }
            }
          }
        }
        if (!BBOXFound) {
          bbox.llX=minLon;
          bbox.llY=minLat;
          bbox.urX=maxLon;
          bbox.urY=maxLat;
        }
#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("BBOX: %f,%f,%f,%f", bbox.llX, bbox.llY, bbox.urX, bbox.urY);
#endif
        return;
      }

      
      int CConvertGeoJSON::convertGeoJSONData(CDataSource *dataSource,int mode){
        //Get jsondata, parse into a Feature map with id as key
        CDF::Variable * jsonVar;
        CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
        try{
          jsonVar=cdfObject->getVariable("jsoncontent");
        }catch(int e){
          return 1;
        }       

        CDBDebug("convertGEOJSONData");
        int result=0;
        
        //Check whether this is really an geojson file
        try{
          cdfObject->getVariable("polygons");//TODO generate these again
        }catch(int e){
          return 1;
        }
        
        std::string geojsonkey=jsonVar->getAttributeNE("ADAGUC_BASENAME")->toString().c_str();
        std::vector<Feature*>features=featureStore[geojsonkey];
        
        if (features.size()==0) {      
          CDBDebug("Rereading JSON");
          CT::string inputjsondata= (char *)jsonVar->data;
          json_value *json= json_parse ((json_char*)inputjsondata.c_str(),inputjsondata.length()); 
#ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("JSON result: %x", json);
#endif          

          BBOX dfBBOX;
          getBBOX(dfBBOX, *json, features);  
        }
        size_t nrDataObjects = dataSource->getNumDataObjects();
        CDataSource::DataObject *dataObjects[nrDataObjects];
        for(size_t d=0;d<nrDataObjects;d++){
          dataObjects[d] =  dataSource->getDataObject(d);
        }
        CDF::Variable *new2DVar;
        new2DVar = dataObjects[0]->cdfVariable;

        //Make the width and height of the new 2D adaguc field the same as the viewing window
        dataSource->dWidth=dataSource->srvParams->Geo->dWidth;
        dataSource->dHeight=dataSource->srvParams->Geo->dHeight;      

        if(dataSource->dWidth == 1 && dataSource->dHeight == 1){
          dataSource->dfBBOX[0]=dataSource->srvParams->Geo->dfBBOX[0];
          dataSource->dfBBOX[1]=dataSource->srvParams->Geo->dfBBOX[1];
          dataSource->dfBBOX[2]=dataSource->srvParams->Geo->dfBBOX[2];
          dataSource->dfBBOX[3]=dataSource->srvParams->Geo->dfBBOX[3];
        }
        
        if(mode==CNETCDFREADER_MODE_OPEN_ALL)
        {
          #ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("convertGeoJSONData OPEN ALL");
          #endif
                      
          //Width needs to be at least 2 in this case.
          if(dataSource->dWidth == 1)dataSource->dWidth=2;
          if(dataSource->dHeight == 1)dataSource->dHeight=2;
          double cellSizeX=(dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0])/double(dataSource->dWidth);
          double cellSizeY=(dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1])/double(dataSource->dHeight);
          double offsetX=dataSource->srvParams->Geo->dfBBOX[0];
          double offsetY=dataSource->srvParams->Geo->dfBBOX[1];
            
          CDF::Dimension *dimX;
          CDF::Dimension *dimY;
          CDF::Variable *varX ;
          CDF::Variable *varY;

          //Create new dimensions and variables (X,Y,T)
          dimX=cdfObject->getDimension("x");
          dimX->setSize(dataSource->dWidth);
          
          dimY=cdfObject->getDimension("y");
          dimY->setSize(dataSource->dHeight);
          
          varX = cdfObject->getVariable("x");
          varY = cdfObject->getVariable("y");
          
          CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);
          CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
          
          //Fill in the X and Y dimensions with the array of coordinates
          for(size_t j=0;j<dimX->length;j++){
            double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
            ((double*)varX->data)[j]=x;
          }
          for(size_t j=0;j<dimY->length;j++){
            double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
            ((double*)varY->data)[j]=y;
          }
          
          #ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("Drawing %s",new2DVar->name.c_str());
          #endif
          
          size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
          new2DVar->setSize(fieldSize);
          CDF::allocateData(new2DVar->getType(),&(new2DVar->data),fieldSize);
          
          //Draw data!
          CDF::Attribute *fillValue = new2DVar->getAttributeNE("_FillValue");
          if(fillValue!=NULL){
            dataObjects[0]->hasNodataValue=true;
            fillValue->getData(&dataObjects[0]->dfNodataValue,1);
          }
        
          for(size_t j=0;j<fieldSize;j++){
            ((short int*)new2DVar->data)[j]=dataObjects[0]->dfNodataValue;
          }
          unsigned short *sdata = ((unsigned short*)new2DVar->data);

#ifdef MEASURETIME
            StopWatch_Stop("GeoJSON DATA");
          #endif
        
          CImageWarper imageWarper;
          
          #ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("Datasource CRS = %s nativeproj4 = %s",dataSource->nativeEPSG.c_str(),dataSource->nativeProj4.c_str());
          CDBDebug("Datasource bbox:%f %f %f %f",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
          CDBDebug("Datasource width height %d %d",dataSource->dWidth,dataSource->dHeight);
          #endif
          
          int status = imageWarper.initreproj(dataSource,dataSource->srvParams->Geo,&dataSource->srvParams->cfg->Projection);
          if(status !=0 ){
            CDBError("Unable to init projection");
            return 1;
          }
          bool projectionRequired = imageWarper.isProjectionRequired();
          
          #ifdef MEASURETIME
          StopWatch_Stop("Iterating lat/lon data");
          #endif
          CDBDebug("DrawPoly");
          
          int skipped=0;
          double llX=dataSource->srvParams->Geo->dfBBOX[0];
          double llY=dataSource->srvParams->Geo->dfBBOX[1];
          double urX=dataSource->srvParams->Geo->dfBBOX[2];
          double urY=dataSource->srvParams->Geo->dfBBOX[3];
#ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("BBOX:%f,%f,%f,%f", llX, llY, urX, urY);
 #endif
          
#ifdef MEASURETIME
          StopWatch_Stop("Feature drawing starts");
#endif
          int nrFeatures=features.size();
          
          int featureIndex=0;
          typedef std::vector<Feature*>::iterator it_type;
          for(it_type it = features.begin(); it != features.end(); ++it) { //Loop over all features
            CT::string id=(*it)->getId();
//            CDBDebug("feature[%s] of %d", id.c_str(), nrFeatures);
            std::vector<Polygon>polygons=(*it)->getPolygons();
            for(std::vector<Polygon>::iterator itpoly = polygons.begin(); itpoly != polygons.end(); ++itpoly) {
              float *polyX=itpoly->getLons();
              float *polyY=itpoly->getLats();
              int numPoints=itpoly->getSize();
              float projectedXY[numPoints*2];
              float minX=FLT_MAX,minY=FLT_MAX;
              float maxX=-FLT_MAX,maxY=-FLT_MAX;
//              CDBDebug("Plotting a polygon of %d points", numPoints);
              for (int j=0; j<numPoints;j++) {
                double tprojectedX=polyX[j];
                double tprojectedY=polyY[j];
                int status=0;
                if(projectionRequired)status = imageWarper.reprojfromLatLon(tprojectedX,tprojectedY);
                int dlon,dlat;
                if(!status){
                  dlon=int((tprojectedX-offsetX)/cellSizeX);
                  dlat=int((tprojectedY-offsetY)/cellSizeY);
                  minX=MIN(minX, tprojectedX);
                  minY=MIN(minY, tprojectedY);
                  maxX=MAX(maxX, tprojectedX);
                  maxY=MAX(maxY, tprojectedY);
                }else{
                  dlat=CCONVERTUGRIDMESH_NODATA;
                  dlon=CCONVERTUGRIDMESH_NODATA;
                }
                projectedXY[j*2]=dlon;
                projectedXY[j*2+1]=dlat;
              }
              if ((minX>urX)||(maxX<llX)||(maxY<llY)||(minY>urY)){
//                CDBDebug("skip %f,%f,%f,%f for %f,%f,%f,%f", minX,maxX, minY, maxY, llX, urX, urY, llY);
              } else {
                float tolerance=0.001;
                std::deque <float> polyline(projectedXY,projectedXY+numPoints*2);
//                  float *result=new float[polyline.size()];
                
//                  float *last = psimpl::simplify_douglas_peucker<2>(
//                  polyline.begin(), polyline.end(), tolerance, result);

                std::vector<PointArray>holes = itpoly->getHoles();
                int nrHoles=holes.size();
                int holeSize[nrHoles];
                float *holeX[nrHoles];
                float *holeY[nrHoles];
                float *projectedHoleXY[nrHoles];
                int h=0;
                for(std::vector<PointArray>::iterator itholes = holes.begin(); itholes != holes.end(); ++itholes) {
//                   CDBDebug("holes[%d]: %d found in %d", itholes->getSize(), featureIndex);
                  holeX[h]=itholes->getLons();
                  holeY[h]=itholes->getLats();
                  holeSize[h]=itholes->getSize();
                  projectedHoleXY[h]=new float[holeSize[h]*2];
                  for (int j=0; j<holeSize[h];j++) {
//                      CDBDebug("J: %d", j);
                    double tprojectedX=holeX[h][j];
                    double tprojectedY=holeY[h][j];
                    int holeStatus=0;
                    if(projectionRequired)holeStatus = imageWarper.reprojfromLatLon(tprojectedX,tprojectedY);
                    int dlon,dlat;
                    if(!holeStatus){
                      dlon=int((tprojectedX-offsetX)/cellSizeX);
                      dlat=int((tprojectedY-offsetY)/cellSizeY);
                    }else{
                      dlat=CCONVERTUGRIDMESH_NODATA;
                      dlon=CCONVERTUGRIDMESH_NODATA;
                    }
                    projectedHoleXY[h][j*2]=dlon;
                    projectedHoleXY[h][j*2+1]=dlat;
//                      CDBDebug("J: %d", j);
                  } 
                  h++;
                }

//                  CDBDebug("passed %d, %d", featureIndex, nrHoles);
//                  int dpCount=(last-result)/sizeof(float)*2;
                int dpCount=numPoints;
                drawpolyWithHoles_index(sdata,dataSource->dWidth,dataSource->dHeight,dpCount,projectedXY,featureIndex,
                                        nrHoles,holeSize,projectedHoleXY);
//                  delete[]result;

                for (int h=0; h<nrHoles;h++) {
                  delete[]projectedHoleXY[h];
                }
                  
              }
            }
      #ifdef MEASURETIME
        StopWatch_Stop("Feature drawn %d", featureIndex);
      #endif
            featureIndex++;
          }        
        
#ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("/convertGEOJSONData");
#endif
        }
        for (std::map<std::string, std::vector<Feature*> >::iterator itf=featureStore.begin();itf!=featureStore.end();++itf){
          std::string fileName= itf->first.c_str();
          CDBDebug("Deleting features for %s", fileName.c_str());
          for (std::vector<Feature*>::iterator it=featureStore[ fileName].begin();it!=featureStore[fileName].end(); ++it) {
            delete *it;
          }
          
        }
        featureStore.clear();
        return result;
      }
 
      

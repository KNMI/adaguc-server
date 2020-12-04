  /******************************************************************************
  * 
  * Project:  ADAGUC Server
  * Purpose:  Transforms GeoJSON into a Grid.
  * Author:   Ernst de Vreede (KNMI)
  * Date:     2016-08
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

        
  //GeoJSON in non standard geographical projection:
  //http://cariska.mona.uwi.edu/geoserver/wfs?typename=geonode%3ACatchment&outputFormat=json&version=1.0.0&request=GetFeature&service=WFS
  //GeoJSON in latlon geographical projection:
  //http://cariska.mona.uwi.edu/geoserver/wfs?typename=geonode%3ACatchment&outputFormat=json&version=1.0.0&request=GetFeature&service=WFS&srsName=EPSG:4326

        #include "CConvertGeoJSON.h"
        #include "CConvertUGRIDMesh.h"
        #include "CImageWarper.h"
        #include <values.h>
        #include <string>
        #include <map>
        #include <cstdlib>
        #include <queue>
        // #define CCONVERTGEOJSON_DEBUG
        const char *CConvertGeoJSON::className="CConvertGeoJSON";

        #define CCONVERTUGRIDMESH_NODATA -32000
  //      #define MEASURETIME 1

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

      void CConvertGeoJSON::drawpolyWithHoles_indexORG(unsigned short int *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short int value,int holes,int *holeCorners, float*holeXY[]){
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
      int cntCnt=0;
      void buildNodeList(int pixelY, int &nodes, int nodeX[], int polyCorners, float *polyXY) {
          int i,j;
          int i2,j2;
          cntCnt++;
          //  Build a list of nodes.
          nodes=0; j=polyCorners-1;
          for (i=0; i<polyCorners; i++) {
            i2=i*2;
            j2=j*2;
            if ((polyXY[i2+1]<(double) pixelY && polyXY[j2+1]>=(double) pixelY)
            || ( polyXY[j2+1]<(double) pixelY && polyXY[i2+1]>=(double) pixelY)) {
              nodeX[nodes++]=(int) (polyXY[i2]+(pixelY-polyXY[i2+1])/(polyXY[j2+1]-polyXY[i2+1])
              *(polyXY[j2]-polyXY[i2])); }
            j=i; 
          }
        
      }
      
      void bubbleSort(int nodes, int nodeX[]){
          //  Sort the nodes, via a simple “Bubble” sort.
          int i,swap;
          i=0;
          while (i<nodes-1) {
            if (nodeX[i]>nodeX[i+1]) {
              swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; 
          } else {
              i++;
            }
          }
      }      
      
    
      static inline void fillLine(int &nodes, int *nodeX, int &IMAGE_LEFT, int &IMAGE_RIGHT, unsigned short int* scanline, unsigned short value){
          //  Fill the pixels between node pairs.
          for (int i=0; i<nodes; i+=2) {
            if   (nodeX[i  ]>=IMAGE_RIGHT) break;
            if   (nodeX[i+1]> IMAGE_LEFT ) {
              if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
              if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
              for (int j=nodeX[i]; j<nodeX[i+1]; j++){
                  scanline[j] = value;
              }
            }
          }      
      }
      
      
      
      
      void CConvertGeoJSON::drawpolyWithHoles_index(int xMin,int yMin, int xMax, int yMax, unsigned short int *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short int value,int holes,int *holeCorners, float*holeXY[]){
        if(xMax<0||yMax<0||xMin>=w||yMin>=h)return;
        
          //  public-domain code by Darel Rex Finley, 2007

        int  nodes, nodeX[polyCorners*2+1],  pixelY, i ;
      
        
        if(xMin<0)xMin=0;
        if(yMin<5)yMin=0;
        if(xMax>=w)xMax=w;
        if(yMax>=h)yMax=h;
        
        int IMAGE_TOP = yMin;
        int IMAGE_BOT = yMax+1;if(IMAGE_BOT>h)IMAGE_BOT=h;
        int IMAGE_LEFT = xMin;
        int IMAGE_RIGHT = xMax;
        int scanLineWidth = IMAGE_RIGHT-IMAGE_LEFT;

        
        int cntLines=0;
        int cntNodes=0;
        int cntHoles=0;
        int cntHoleLists=0;
        //Allocate  scanline
        unsigned short scanline[scanLineWidth];
        //  Loop through the rows of the image.
        for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {
          
          cntLines++;
          for (i=0; i<scanLineWidth;i++) scanline[i]=65535u;
          buildNodeList(pixelY, nodes, nodeX, polyCorners, polyXY);         
          cntNodes+=nodes;
          bubbleSort(nodes, nodeX);
          //fillLine2(nodes, nodeX, IMAGE_LEFT,IMAGE_RIGHT, scanline, value);
          for (i=0;i<nodes;i+=2){
            int x1=nodeX[i]-IMAGE_LEFT;
            int x2=nodeX[i+1]-IMAGE_LEFT;
            if(x1<0)x1=0;
            if (x2>scanLineWidth)x2=scanLineWidth;
            for(int j=x1;j<x2;j++){
              scanline[j]=value;
            }
          }     
          
          for (int h=0; h<holes; h++) {
            buildNodeList(pixelY, nodes, nodeX, holeCorners[h], holeXY[h]);    
            cntHoleLists++;
            cntHoles+=holes;
            bubbleSort(nodes, nodeX);
            for (i=0;i<nodes;i+=2){
              int x1=nodeX[i]-IMAGE_LEFT;
              int x2=nodeX[i+1]-IMAGE_LEFT;
              if(x1<0)x1=0;
              if (x2>scanLineWidth)x2=scanLineWidth;
              for(int j=x1;j<x2;j++){
                scanline[j]=65535u;
              }
            }     
          }
          unsigned int startScanLineY=pixelY*w;
          for (i=IMAGE_LEFT; i<IMAGE_RIGHT; i++) {
            if (scanline[i-IMAGE_LEFT]!=65535u) {
              imagedata[i+startScanLineY]=scanline[i-IMAGE_LEFT];
            }
          }
        }  
    
  //      CDBDebug("drawPoly %d %d %d %d %d", cntLines, cntNodes, cntHoles, cntHoleLists, cntCnt);
      };
  
      
      
      
      
      
      
      
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
        
        std::map<std::string, std::vector<Feature*> > CConvertGeoJSON::featureStore;

        void CConvertGeoJSON::clearFeatureStore() {
          for (std::map<std::string, std::vector<Feature*> >::iterator itf=featureStore.begin();itf!=featureStore.end();++itf){
            std::string fileName= itf->first.c_str();
  //          CDBDebug("Deleting %d features for %s", featureStore[fileName].size(), fileName.c_str());
            for (std::vector<Feature*>::iterator it=featureStore[ fileName].begin();it!=featureStore[fileName].end(); ++it) {
  //            CDBDebug("deleting %s", (*it)->getId().c_str());
              delete *it;
              *it=NULL;
            }
//            CDBDebug("Deleted %d features for %s", featureStore[fileName].size(), fileName.c_str());
          }
          featureStore.clear(); 
        }

        //Delete one set of features from the featureStore
        void CConvertGeoJSON::clearFeatureStore(CT::string name) {
          for (std::map<std::string, std::vector<Feature*> >::iterator itf=featureStore.begin();itf!=featureStore.end();++itf){
            std::string fileName= itf->first.c_str();
            if (fileName==name.c_str()) {
                      CDBDebug("Deleting %d features ONLY for %s", featureStore[fileName].size(), fileName.c_str());
              for (std::vector<Feature*>::iterator it=featureStore[ fileName].begin();it!=featureStore[fileName].end(); ++it) {
  //                          CDBDebug("deleting %s", (*it)->getId().c_str());
                delete *it;
                *it=NULL;
              }
            }
          }
        }
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
          CDBDebug("convertGeoJSONHeader");
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
          getBBOX(cdfObject, dfBBOX, *json, features);  
          
          getDimensions(cdfObject, *json, false);

          CDBDebug("addCDFInfo");
          addCDFInfo(cdfObject, NULL, dfBBOX, features, false);
          
          std::string geojsonkey=jsonVar->getAttributeNE("ADAGUC_BASENAME")->toString().c_str();
          featureStore[geojsonkey]=features;
          
          json_value_free(json);
          #ifdef MEASURETIME
          StopWatch_Stop("DATA READ");
          #endif

          #ifdef MEASURETIME
            StopWatch_Stop("BBOX Calculated");
          #endif
            

  #ifdef CCONVERTGEOJSON_DEBUG
          CDBDebug("CConvertGeoJSON::convertGeoJSONHeader() done");
  #endif
          return 0;
        }

        
        void CConvertGeoJSON::addCDFInfo(CDFObject *cdfObject, CServerParams *srvParams, BBOX &dfBBOX, std::vector<Feature*>& featureMap, bool openAll) {
          //Create variables for all properties fields
          
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
          
          CDF::Dimension *timeDim = cdfObject->getDimensionNE("time");
          // CDBDebug("timeDim: %d", timeDim);
          CDF::Dimension *elevationDim = cdfObject->getDimensionNE("elevation");
          // CDBDebug("elevationDim: %d", elevationDim);
          
          if(dimX==NULL||dimY==NULL||varX==NULL||varY==NULL) 
          {
            
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
  varX->setType(CDF_DOUBLE);
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
      
          
          CDF::Variable *polygonIndexVar = new CDF::Variable();
          cdfObject->addVariable(polygonIndexVar);

          if (elevationDim!=NULL) {
            polygonIndexVar->dimensionlinks.push_back(elevationDim);
          }
          if (timeDim!=NULL) {
            polygonIndexVar->dimensionlinks.push_back(timeDim);
          }
          polygonIndexVar->dimensionlinks.push_back(dimY);
          polygonIndexVar->dimensionlinks.push_back(dimX);
          polygonIndexVar->setType(CDF_USHORT);
          polygonIndexVar->name="features";
          polygonIndexVar->setAttributeText("long_name","Feature index");
          polygonIndexVar->setAttributeText("adaguc_data_type","CConvertGeoJSON");
          
          unsigned short f=65535u;
          polygonIndexVar->setAttribute("_FillValue",CDF_USHORT,&f,1);

  //        std::vector<Feature*>::iterator sample=featureMap.begin();
  //        Feature *sample=featureMap[0];
          int nrFeatures=featureMap.size();
          CDF::Dimension *dimFeatures;
          bool found=false;
          try {
            dimFeatures=cdfObject->getDimension("features");
            found=true;
          } catch (int e) {}
          if (!found) {
            // CDBDebug("Creating dim %s %d", "features", nrFeatures);
            dimFeatures=new CDF::Dimension();
            dimFeatures->name="features";
            dimFeatures->setSize(nrFeatures);
            cdfObject->addDimension(dimFeatures);            
          } 
          CDF::Variable *featureIdVar = new CDF::Variable();
          
          try {
            cdfObject->getVariable("featureids");
            found=true;
          } catch(int e){}
          if (!found) {
            cdfObject->addVariable(featureIdVar);
            featureIdVar->dimensionlinks.push_back(dimFeatures);
            if (elevationDim!=NULL) {
              featureIdVar->dimensionlinks.push_back(elevationDim);
            }
            if (timeDim!=NULL) {
              featureIdVar->dimensionlinks.push_back(timeDim);
            }
            featureIdVar->setType(CDF_STRING);
            featureIdVar->name="featureids";
            CDF::allocateData(CDF_STRING, &featureIdVar->data, nrFeatures);
          }

          
          int featureCnt=0;
          for (std::vector<Feature*>::iterator sample=featureMap.begin(); sample!=featureMap.end(); ++sample) {
//            CDBDebug("Feature id %s", (*sample)->getId().c_str());
            ((const char**)featureIdVar->data)[featureCnt++]=strdup((*sample)->getId().c_str());
            
//TODO Check if this code is really needed; looks useless
//             for (std::map<std::string, FeatureProperty*>::iterator ftit=(*sample)->getFp().begin(); ftit!=(*sample)->getFp().end(); ++ftit) {
//              CDBDebug("Create var %s %s %d", ftit->first.c_str(), ftit->second->toString().c_str(),ftit->second->getType() );
//               if (ftit->second->getType()!=typeStr) {
//                 bool found=false;
//                 try {
//                   cdfObject->getVariable(ftit->first.c_str());
//                   found=true;
//                 } catch(int e){}
//                 if (!found) {
//                   CDF::Variable *newVar = new CDF::Variable();
//                   cdfObject->addVariable(newVar);
//                   newVar->dimensionlinks.push_back(dimFeatures);
//                   CDF::Variable *polygonIndexVar = new CDF::Variable();
//                   cdfObject->addVariable(polygonIndexVar);
//                   polygonIndexVar->dimensionlinks.push_back(dimY);
//                   polygonIndexVar->dimensionlinks.push_back(dimX);
//                   FeaturePropertyType tp=ftit->second->getType();
//                   if (tp==typeStr) {
//                     newVar->setType(CDF_STRING);
//                     polygonIndexVar->setType(CDF_STRING);
//                   } else if (tp==typeInt) { 
//                     newVar->setType(CDF_USHORT);
//                     polygonIndexVar->setType(CDF_USHORT);
//                     unsigned short f=65535u;
//                     newVar->setAttribute("_FillValue",CDF_USHORT,&f,1);
//                     polygonIndexVar->setAttribute("_FillValue",CDF_USHORT,&f,1);
//                   } else if (tp==typeDouble) {
//                     newVar->setType(CDF_FLOAT);
//                     float f=-99999;
//                     newVar->setAttribute("_FillValue",CDF_FLOAT,&f,1);
//                     polygonIndexVar->setAttribute("_FillValue",CDF_FLOAT,&f,1);
//                   }
//                   newVar->name=(ftit->first+"_backup").c_str();
//                   polygonIndexVar->name=ftit->first.c_str();
//                   polygonIndexVar->setAttributeText("grid_mapping","customgridprojection");
//                   
//                 }
//               }
//             }
          }
        }
    
    
        void CConvertGeoJSON::getDimensions(CDFObject *cdfObject, json_value &json, bool openAll) {
          if (json.type==json_object) {
            CT::string type;
            if (json["type"].type!=json_null) {
              #ifdef CCONVERTGEOJSON_DEBUG
              CDBDebug("type found");
              #endif
              type=json["type"].u.string.ptr;
              #ifdef CCONVERTGEOJSON_DEBUG
              CDBDebug("type: %s\n", type.c_str());
              #endif
              CT::string timeVal;
              double dTimeVal=-9999;
              int iTimeVal=-9999;
              CT::string timeUnits;
              
              if (type.equals("FeatureCollection")) {
                json_value dimensions=json["dimensions"];
                if (dimensions.type==json_object) {
                  for (unsigned int dimCnt = 0; dimCnt < dimensions.u.object.length; dimCnt++){
                    json_object_entry dimObject = dimensions.u.object.values[dimCnt];
                    CT::string dimName(dimObject.name);
                    json_value dim = *dimObject.value;
                    // CDBDebug("[%d] dim[%s] %d %d", dimCnt, dimName.c_str(), dim.type, dim.type==json_string);
                    if (dimName.equals("time")) {
                      // CDBDebug("time found !!!!");
                      
                      if (dim.type==json_object) {
                        for (unsigned int fldCnt = 0; fldCnt < dim.u.object.length; fldCnt++) {
                          json_object_entry fldObject = dim.u.object.values[fldCnt];
                          CT::string fldName(fldObject.name);
                          json_value fldValue = *fldObject.value;
                          if (fldValue.type==json_string) {
                            CT::string value(fldValue.u.string.ptr);
                            // CDBDebug("[ ] dim[%s]: %s=%s", dimName.c_str(), fldName.c_str(), value.c_str());
                            //                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
                            //                      CDBDebug("[%d] prop[%s]S =%s", cnt, propName.c_str(),prop.u.string.ptr);
                            if (fldName.equals("units")) {
                              timeUnits=value.c_str();
                            } else if (fldName.equals("value")) {
                              timeVal=value.c_str();
                            }
                          }
                          if (fldValue.type==json_double) {
                            // CDBDebug("[ ] dim[%s]: dbl", dimName.c_str(), fldName.c_str());
                            if (fldName.equals("value")) {
                              dTimeVal=fldValue.u.dbl;
                            }
                          }
                          if (fldValue.type==json_integer) {
                            if (fldName.equals("value")) {
                              iTimeVal=fldValue.u.integer;
                            }
                          }
                        }
                      }

                      // CDBDebug("time: %s %s %f %d", timeVal.c_str(), timeUnits.c_str(), dTimeVal, iTimeVal);
                      CDF::Variable timeVarHelper;
                      timeVarHelper.setAttributeText("units", "seconds since 1970-1-1");
                      CTime timeHelper;
                      timeHelper.init(&timeVarHelper);
                      
                      double timeOffset;
                      if (timeVal.length()>0) {
                        timeOffset=timeHelper.dateToOffset(timeHelper.freeDateStringToDate(timeVal.c_str()));
                      } else if (dTimeVal>0) {
                        timeOffset=dTimeVal;
                      } else if (iTimeVal>0) {
                        timeOffset=iTimeVal;
                      }
                      // CDBDebug("timeOffset=%f", timeOffset);
                      CDF::Dimension *timeDim=new CDF::Dimension();
                      timeDim->name="time";
                      timeDim->setSize(1);
                      cdfObject->addDimension(timeDim);
                      CDF::Variable *timeVar = new CDF::Variable();
                      timeVar->setType(CDF_DOUBLE);
                      timeVar->name.copy("time");
                      timeVar->isDimension=true;
                      timeVar->setAttributeText("units", "seconds since 1970-1-1");
                      timeVar->setAttributeText("standard_name", "time");
                      timeVar->dimensionlinks.push_back(timeDim);
                      cdfObject->addVariable(timeVar);
                      CDF::allocateData(CDF_DOUBLE,&timeVar->data,timeDim->length);
                      timeVar->setType(CDF_DOUBLE);
                      ((double*)timeVar->data)[0]=timeOffset;
                    } else {
                      // CDBDebug("other dim: %s", dimName.c_str());
                      CT::string dimUnits;
                      CT::string dimVal;
                      double dDimVal;
                      int iDimVal;
                      if (dim.type==json_object) {
                        for (unsigned int fldCnt = 0; fldCnt < dim.u.object.length; fldCnt++) {
                          json_object_entry fldObject = dim.u.object.values[fldCnt];
                          CT::string fldName(fldObject.name);
                          json_value fldValue = *fldObject.value;
                          if (fldValue.type==json_string) {
                            CT::string value(fldValue.u.string.ptr);
                            CDBDebug("[ ] dim[%s]: %s=%s", dimName.c_str(), fldName.c_str(), value.c_str());
                            //                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
                            //                      CDBDebug("[%d] prop[%s]S =%s", cnt, propName.c_str(),prop.u.string.ptr);
                            if (fldName.equals("units")) {
                              dimUnits=value.c_str();
                            } else if (fldName.equals("value")) {
                              dimVal=value.c_str();
                            }
                          }
                          if (fldValue.type==json_double) {
                            // CDBDebug("[ ] dim[%s]: dbl", dimName.c_str(), fldName.c_str());
                            if (fldName.equals("value")) {
                              dDimVal=fldValue.u.dbl;
                            }
                          }
                          if (fldValue.type==json_integer) {
                            if (fldName.equals("value")) {
                              iDimVal=fldValue.u.integer;
                            }
                          }
                        }
                      }
                        
                      // CDBDebug("%s: %s %s %f %d", dimName.c_str(), dimVal.c_str(), dimUnits.c_str(), dDimVal, iDimVal);
                      CDF::Dimension *dim=new CDF::Dimension();
                      dim->name.copy(dimName);
                      dim->setSize(1);
                      cdfObject->addDimension(dim); 
                      CDF::Variable *dimVar = new CDF::Variable();
                      dimVar->setType(CDF_DOUBLE);
                      dimVar->name.copy(dimName);
                      dimVar->isDimension=true;
                      dimVar->setAttributeText("units", dimUnits.c_str());
                      dimVar->setAttributeText("standard_name", dimName.c_str());
                      dimVar->dimensionlinks.push_back(dim);
                      // CDBDebug("Pushed_back %s dim", dim->name.c_str());  
                      cdfObject->addVariable(dimVar);
                      CDF::allocateData(CDF_DOUBLE,&dimVar->data,dim->length);
                      dimVar->setType(CDF_DOUBLE);
                      ((double*)dimVar->data)[0]=dDimVal;
                      
#ifdef USETHIS
                      CDF::Variable timeVarHelper;
                      timeVarHelper.setAttributeText("units", "seconds since 1970-1-1");
                      CTime timeHelper;
                      timeHelper.init(&timeVarHelper);
                      
                      double timeOffset;
                      if (timeVal.length()>0) {
                        timeOffset=timeHelper.dateToOffset(timeHelper.freeDateStringToDate(timeVal.c_str()));
                      } else if (dTimeVal>0) {
                        timeOffset=dTimeVal;
                      } else if (iTimeVal>0) {
                        timeOffset=iTimeVal;
                      }
                      CDBDebug("timeOffset=%f", timeOffset);
                      CDF::Dimension *timeDim=new CDF::Dimension();
                      timeDim->name="time";
                      timeDim->setSize(1);
                      cdfObject->addDimension(timeDim);
                      CDF::Variable *timeVar = new CDF::Variable();
                      timeVar->setType(CDF_DOUBLE);
                      timeVar->name.copy("time");
                      timeVar->isDimension=true;
                      timeVar->setAttributeText("units", "seconds since 1970-1-1");
                      timeVar->setAttributeText("standard_name", "time");
                      timeVar->dimensionlinks.push_back(timeDim);
                      cdfObject->addVariable(timeVar);
                      CDF::allocateData(CDF_DOUBLE,&timeVar->data,timeDim->length);
                      timeVar->setType(CDF_DOUBLE);
                      ((double*)timeVar->data)[0]=timeOffset;
#endif                      
                    }
                  }
                }  
              }
            }
          }
        }
        
  #define MAX(a,b) (((a)>(b))?(a):(b))
  #define MIN(a,b) (((a)<(b))?(a):(b))

        void CConvertGeoJSON::getBBOX(CDFObject *cdfObject, BBOX &bbox, json_value &json, std::vector<Feature*>& featureMap) {
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
                for (unsigned int cnt = 0; cnt < features.u.array.length; cnt++){
                  json_value feature = *features.u.array.values[cnt];
                  CT::string featureId;
                  json_value id = feature["id"];
                  if (id.type==json_string) {
  //                  CDBDebug("Id=%s", id.u.string.ptr);
                    featureId=id.u.string.ptr;
                  } else if (id.type==json_integer) {
                    featureId.print("%1d", id.u.integer);
                  }
  //                CDBDebug("found featureId as attribute %s", featureId.c_str());
                  Feature *feat=new Feature();
                  
                  json_value props = feature["properties"];
  //                CDBDebug("props.type=%d", props.type);
                  if (props.type==json_object){
                    for (unsigned int propCnt = 0; propCnt < props.u.object.length; propCnt++){
                      json_object_entry propObject = props.u.object.values[propCnt];
                      CT::string propName(propObject.name);
                      json_value prop = *propObject.value;
                      if (prop.type==json_string) {
  //                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
  //                      CDBDebug("[%d] prop[%s]S =%s", cnt, propName.c_str(),prop.u.string.ptr);
                        feat->addProp(propName, prop.u.string.ptr);
                      } else if (prop.type==json_integer) {
  //                      CDBDebug("[%d] prop[%s]I =%d", cnt, propName.c_str(), (int)prop.u.integer);
                        feat->addProp(propName, (int)prop.u.integer);
                      } else if (prop.type==json_double) {
  //                      CDBDebug("[%d] prop[%s]D =%f", cnt, propName.c_str(), prop.u.dbl);
                        feat->addProp(propName, prop.u.dbl);
                      }
                    }
                  }
                  if (featureId.length()==0) {
                    std::map<std::string, FeatureProperty*>::iterator it;
                    CT::string id_s;
                    it=feat->getFp().find("id");
                    if (it!=feat->getFp().end()) {
                      id_s=it->second->toString().c_str();
  //                     CDBDebug("Found %s %s", it->first.c_str(), id_s.c_str());
                      if (!id_s.equals("NONE")) {
                        featureId=id_s;
                      }
                    } else {
                      it=feat->getFp().find("NUTS_ID");
                      if (it!=feat->getFp().end()) {
                        id_s=it->second->toString().c_str();
  //                       CDBDebug("Found %s %s", it->first.c_str(), id_s.c_str());
                        if (!id_s.equals("NONE")){
                          featureId=id_s;
                        }
                      } else {
                        it=feat->getFp().find("name");  
                        if (it!=feat->getFp().end()) {
                          id_s=it->second->toString().c_str();
  //                         CDBDebug("Found %s %s", it->first.c_str(), id_s.c_str());
                          if (!id_s.equals("NONE")){
                            featureId=id_s;
                          } 
                        } else {           
  //                        CDBDebug("Fallback to id %d", cnt);
                          featureId.print("%04d", cnt);
                        }
                      }
  //                    CDBDebug("found featureId in properties %s", featureId.c_str());                    
                    }
                  }
                  feat->setId(featureId);

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
                                feat->addPolygonPoint((float)lon, (float)lat);
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
                      } else if (strcmp(geomType.u.string.ptr, "LineString")==0) {
                        json_value coords=geom["coordinates"];
                        if (coords.type==json_array) {
                          feat->newPolyline(); 
                          for (unsigned int i=0; i<coords.u.array.length;i++) {
                            json_value pt=*coords.u.array.values[i];
                            json_value lo=*pt.u.array.values[0];
                            json_value la=*pt.u.array.values[1];
                            double lon=(double)lo;
                            double lat=(double)la;
                            feat->addPolylinePoint((float)lon, (float)lat);
                            if (lat<minLat) minLat=lat;
                            if (lat>maxLat) maxLat=lat;
                            if (lon<minLon) minLon=lon;
                            if (lon>maxLon) maxLon=lon;
//                          CDBDebug("    %f,%f", (double)lo, (double)la);
                          }

                        } else {
                          CDBDebug("  coords type of %d", coords.type);
                        }
                      } else if (strcmp(geomType.u.string.ptr, "MultiLineString")==0) {
                        json_value multicoords=geom["coordinates"];
                        if (multicoords.type==json_array) {
                          //                            CDBDebug("  array of %d coords",coords.u.array.length);
                          for (unsigned int j=0; j<multicoords.u.array.length; j++) {
                            json_value coords=*multicoords.u.array.values[j];
                            if (coords.type==json_array) {
                              feat->newPolyline(); 
                              for (unsigned int i=0; i<coords.u.array.length;i++) {
                                json_value pt=*coords.u.array.values[i];
                                json_value lo=*pt.u.array.values[0];
                                json_value la=*pt.u.array.values[1];
                                double lon=(double)lo;
                                double lat=(double)la;
                                feat->addPolylinePoint((float)lon, (float)lat);
                                if (lat<minLat) minLat=lat;
                                if (lat>maxLat) maxLat=lat;
                                if (lon<minLon) minLon=lon;
                                if (lon>maxLon) maxLon=lon;
                                //                          CDBDebug("    %f,%f", (double)lo, (double)la);
                              }
                            }
                          }
                        } else {
                          CDBDebug("  multicoords type of %d", multicoords.type);
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
                                for (unsigned int k=0; k<polygon.u.array.length;k++) {
                                  json_value pt=*polygon.u.array.values[k];
                                  json_value lo=*pt.u.array.values[0];
                                  json_value la=*pt.u.array.values[1];
                                  double lon=(double)lo;
                                  double lat=(double)la;
                                  if (j==0) {
                                    feat->addPolygonPoint((float)lon, (float)lat);
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

  //         CDBDebug("<><><><><><><>Cleaning up for all properties fields<><><><><><>");
  //         int itctr=0;
  //         for (std::vector<Feature*>::iterator it = featureMap.begin(); it != featureMap.end(); ++it) {
  // //          CDBDebug("FT[%d] has %d items", itctr, (*it)->getFp().size());
  //           for (std::map<std::string, FeatureProperty*>::iterator ftit=(*it)->getFp().begin(); ftit!=(*it)->getFp().end(); ++ftit) {
  // //            CDBDebug("FT: %d %s %s", itctr, ftit->first.c_str(), ftit->second->toString().c_str());
  //             delete ftit->second;
  //           }
  //           (*it)->getFp().clear();
  //           delete *it;
  //           itctr++;
  //         }
  //         featureMap.clear();
          //If no BBOX was found in file, generate it from found geo coordinates
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

          CDBDebug("convertGEOJSONData %s", (mode==CNETCDFREADER_MODE_OPEN_ALL)?"ALL":"NOT ALL");
          int result=0;
          
          //Check whether this is really an geojson file
          try{
            cdfObject->getVariable("features");//TODO generate these again
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
            getBBOX(cdfObject, dfBBOX, *json, features);  
            CDBDebug("addCDFInfo again");
            addCDFInfo(cdfObject, dataSource->srvParams, dfBBOX, features, true);
          }
          //Store featureSet name (geojsonkey) in datasource
          dataSource->featureSet=geojsonkey.c_str();

          //Make the width and height of the new 2D adaguc field the same as the viewing window
          dataSource->dWidth=dataSource->srvParams->Geo->dWidth;
          dataSource->dHeight=dataSource->srvParams->Geo->dHeight;      

          //Set statistics
          if(dataSource->stretchMinMax){
            #ifdef CCONVERTGEOJSON_DEBUG
            CDBDebug("dataSource->stretchMinMax");
            #endif
            if(dataSource->statistics==NULL){
              #ifdef CCONVERTGEOJSON_DEBUG
              CDBDebug("Setting statistics: min/max : %f %f",0,features.size()-1);
              #endif
              dataSource->statistics = new CDataSource::Statistics();
              dataSource->statistics->setMaximum(features.size()-1);
              dataSource->statistics->setMinimum(0);
            }
          }
          
          if (dataSource->srvParams->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
            return 0;
          }
          
          if(dataSource->dWidth == 1 && dataSource->dHeight == 1){
            dataSource->dfBBOX[0]=dataSource->srvParams->Geo->dfBBOX[0];
            dataSource->dfBBOX[1]=dataSource->srvParams->Geo->dfBBOX[1];
            dataSource->dfBBOX[2]=dataSource->srvParams->Geo->dfBBOX[2];
            dataSource->dfBBOX[3]=dataSource->srvParams->Geo->dfBBOX[3];
            return result;//TODO
          }
          
          if(mode==CNETCDFREADER_MODE_OPEN_ALL)
          {
            #ifdef CCONVERTGEOJSON_DEBUG
            CDBDebug("convertGeoJSONData OPEN ALL");
            #endif
            
            size_t nrDataObjects = dataSource->getNumDataObjects();
            CDataSource::DataObject *dataObjects[nrDataObjects];
            for(size_t d=0;d<nrDataObjects;d++){
              dataObjects[d] =  dataSource->getDataObject(d);
            }
            CDF::Variable *polygonIndexVar;
            polygonIndexVar = dataObjects[0]->cdfVariable;                      
            //Width needs to be at least 2 in this case.
            if(dataSource->dWidth == 1)dataSource->dWidth=2;
            if(dataSource->dHeight == 1)dataSource->dHeight=2;
            double cellSizeX=(dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0])/double(dataSource->dWidth);
            double cellSizeY=(dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1])/double(dataSource->dHeight);
            double offsetX=dataSource->srvParams->Geo->dfBBOX[0]+cellSizeX/2;
            double offsetY=dataSource->srvParams->Geo->dfBBOX[1]+cellSizeY/2;
              
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
              double x=offsetX+double(j)*cellSizeX;
              ((double*)varX->data)[j]=x;
            }
            for(size_t j=0;j<dimY->length;j++){
              double y=offsetY+double(j)*cellSizeY;
              ((double*)varY->data)[j]=y;
            }
            bool projectionRequired=false;
            CImageWarper imageWarper;
            if(dataSource->srvParams->Geo->CRS.length()>0){
              projectionRequired=true;
  //            for(size_t d=0;d<nrDataObjects;d++){
                polygonIndexVar->setAttributeText("grid_mapping","customgridprojection");
  //            }
              if(cdfObject->getVariableNE("customgridprojection")==NULL){
                CDF::Variable *projectionVar = new CDF::Variable();
                projectionVar->name.copy("customgridprojection");
                cdfObject->addVariable(projectionVar);
                dataSource->nativeEPSG = dataSource->srvParams->Geo->CRS.c_str();
                imageWarper.decodeCRS(&dataSource->nativeProj4,&dataSource->nativeEPSG,&dataSource->srvParams->cfg->Projection);
                if(dataSource->nativeProj4.length()==0){
                  dataSource->nativeProj4=LATLONPROJECTION;
                  dataSource->nativeEPSG="EPSG:4326";
                  projectionRequired=false;
                }
                projectionVar->setAttributeText("proj4_params",dataSource->nativeProj4.c_str());
              }
            }          
            
            
            #ifdef CCONVERTGEOJSON_DEBUG
            CDBDebug("Drawing %s",polygonIndexVar->name.c_str());
            #endif
            
  //TODO Only draw if datatype is int (or float)        
            size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
            polygonIndexVar->setSize(fieldSize);
            CDF::allocateData(polygonIndexVar->getType(),&(polygonIndexVar->data),fieldSize);
            
            //Draw data!
            dataObjects[0]->dfNodataValue = -1;
            CDF::Attribute *fillValue = polygonIndexVar->getAttributeNE("_FillValue");
            if(fillValue!=NULL){
              dataObjects[0]->hasNodataValue=true;
              fillValue->getData(&dataObjects[0]->dfNodataValue,1);
            }else{
              unsigned short f = dataObjects[0]->dfNodataValue;
              polygonIndexVar->setAttribute("_FillValue",CDF_USHORT,&f,1);
            }
            unsigned short sNodataValue = (unsigned short)dataObjects[0]->dfNodataValue;
            for(size_t j=0;j<fieldSize;j++){
              ((unsigned short*)polygonIndexVar->data)[j]=sNodataValue;
            }
            
            unsigned short *sdata = ((unsigned short*)polygonIndexVar->data);

  #ifdef MEASURETIME
              StopWatch_Stop("GeoJSON DATA");
            #endif
        
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
  //          bool projectionRequired = imageWarper.isProjectionRequired();
            
            #ifdef MEASURETIME
            StopWatch_Stop("Iterating lat/lon data");
            #endif
            CDBDebug("DrawPoly");
            
  //           double llX=dataSource->srvParams->Geo->dfBBOX[0];
  //           double llY=dataSource->srvParams->Geo->dfBBOX[1];
  //           double urX=dataSource->srvParams->Geo->dfBBOX[2];
  //           double urY=dataSource->srvParams->Geo->dfBBOX[3];
  // #ifdef CCONVERTGEOJSON_DEBUG
  //           CDBDebug("BBOX:%f,%f,%f,%f", llX, llY, urX, urY);
  //  #endif
            
  #ifdef MEASURETIME
            StopWatch_Stop("Feature drawing starts");
  #endif
            CDBDebug("nrFeatures: %d", features.size());
            
            int featureIndex=0;
            typedef std::vector<Feature*>::iterator it_type;
            for(it_type feature = features.begin(); feature != features.end(); ++feature) { //Loop over all features
              //if(featureIndex!=0)break;
              std::vector<Polygon>polygons=(*feature)->getPolygons();
  //            CT::string id=(*feature)->getId();
  //            CDBDebug("feature[%s] %d of %d with %d polygons", id.c_str(), featureIndex, features.size(), polygons.size());
              for(std::vector<Polygon>::iterator itpoly = polygons.begin(); itpoly != polygons.end(); ++itpoly) {
                float *polyX=itpoly->getLons();
                float *polyY=itpoly->getLats();
                int numPoints=itpoly->getSize();
                float projectedXY[numPoints*2];
                float minX=FLT_MAX,minY=FLT_MAX;
                float maxX=-FLT_MAX,maxY=-FLT_MAX;
                
                int pxMin,pxMax,pyMin,pyMax,first=0;
                
                //CDBDebug("Plotting a polygon of %d points with %d holes [? of %d]", numPoints, itpoly->getHoles().size(), featureIndex);
                for (int j=0; j<numPoints;j++) {
                  double tprojectedX=polyX[j];
                  double tprojectedY=polyY[j];
                  int status=0;
                  if(projectionRequired)status = imageWarper.reprojfromLatLon(tprojectedX,tprojectedY);
                  int dlon,dlat;
                  if(!status){
                    dlon=int((tprojectedX-offsetX)/cellSizeX)+1;
                    dlat=int((tprojectedY-offsetY)/cellSizeY);
                    
                    if(first == 0){
                      first=1;
                      pxMin=dlon;pxMax=dlon;
                      pyMin=dlat;pyMax=dlat;
                    }else{
                      if(dlon<pxMin)pxMin=dlon;
                      if(dlon>pxMax)pxMax=dlon;
                      if(dlat<pyMin)pyMin=dlat;
                      if(dlat>pyMax)pyMax=dlat;
                    }
                    
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
                
                
                
  /*              if ((minX>urX)||(maxX<llX)||(maxY<llY)||(minY>urY))
                {
  //                CDBDebug("skip %f,%f,%f,%f for %f,%f,%f,%f", minX,maxX, minY, maxY, llX, urX, urY, llY);
                } else 
                */{
              {

      
  //                float tolerance=0.001;
  //                std::deque <float> polyline(projectedXY,projectedXY+numPoints*2);
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
  //                   CDBDebug("holes[%d]: %d found in %d", 0, itholes->getSize(), featureIndex);
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
                        dlon=int((tprojectedX-offsetX)/cellSizeX)+1;
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
                  
                  
                  {

  //                  CDBDebug("passed %d, %d", featureIndex, nrHoles);
  //                  int dpCount=(last-result)/sizeof(float)*2;
                  int dpCount=numPoints;
                  drawpolyWithHoles_index(pxMin,pyMin,pxMax,pyMax,sdata,dataSource->dWidth,dataSource->dHeight,dpCount,projectedXY,featureIndex,nrHoles,holeSize,projectedHoleXY);
  //                  delete[]result;

                  for (int h=0; h<nrHoles;h++) {
                    delete[]projectedHoleXY[h];
                  }
                  }
                }
              }
              }
        #ifdef MEASURETIME
          StopWatch_Stop("Feature drawn %d", featureIndex);
        #endif
              for (std::map<std::string, FeatureProperty*>::iterator ftit=(*feature)->getFp().begin(); ftit!=(*feature)->getFp().end(); ++ftit) {
                if(dataSource->getDataObject(0)->features.count(featureIndex) == 0){
                  dataSource->getDataObject(0)->features[featureIndex]=CFeature(featureIndex);
                }
                dataSource->getDataObject(0)->features[featureIndex].addProperty(ftit->first.c_str(), ftit->second->toString().c_str());
                
              }
              featureIndex++;
            }        

          
  #ifdef CCONVERTGEOJSON_DEBUG
            CDBDebug("/convertGEOJSONData");
  #endif
          }
          return result;
        }
  
        

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

        #include "CImgRenderPolylines.h"
        #include <set>
        #include "CConvertGeoJSON.h"
        #include <values.h>

 //   #define MEASURETIME

        #define MAX(a,b) (((a)>(b))?(a):(b))
        #define MIN(a,b) (((a)<(b))?(a):(b))
        #define CCONVERTUGRIDMESH_NODATA -32000
        
        const char *CImgRenderPolylines::className="CImgRenderPolylines";

        void CImgRenderPolylines::render(CImageWarper*imageWarper, CDataSource*dataSource, CDrawImage*drawImage){
          CColor drawPointTextColor(0,0,0,255);
          CColor drawPointFillColor(0,0,0,128);
          CColor drawPointLineColor(0,0,0,255);
          CColor defaultColor(0,0,0,255);
          
          CStyleConfiguration *styleConfiguration = dataSource->getStyle();
          if(styleConfiguration!=NULL){
            if(styleConfiguration->styleConfig!=NULL){
              CServerConfig::XMLE_Style* s = styleConfiguration->styleConfig;
            }
          }
          
          CT::string name=dataSource->featureSet;
          
          bool projectionRequired=false;
          if(dataSource->srvParams->Geo->CRS.length()>0){
            projectionRequired=true;
          }
          
          int width=drawImage->getWidth();
          int height=drawImage->getHeight();
          
          double cellSizeX=(dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0])/double(dataSource->dWidth);
          double cellSizeY=(dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1])/double(dataSource->dHeight);
          double offsetX=dataSource->srvParams->Geo->dfBBOX[0]+cellSizeX/2;
          double offsetY=dataSource->srvParams->Geo->dfBBOX[1]+cellSizeY/2;
          
          std::map<std::string, std::vector<Feature*> > featureStore=CConvertGeoJSON::featureStore;

          int featureIndex=0;
          for (std::map<std::string, std::vector<Feature*> >::iterator itf=featureStore.begin();itf!=featureStore.end();++itf){
            std::string fileName= itf->first.c_str();
            if (fileName==name.c_str()) {
              CDBDebug("Plotting %d features ONLY for %s", featureStore[fileName].size(), fileName.c_str());
              for (std::vector<Feature*>::iterator feature=featureStore[ fileName].begin();feature!=featureStore[fileName].end(); ++feature) {
                  //if(featureIndex!=0)break;
                  std::vector<Polygon>polygons=(*feature)->getPolygons();
                  //            CT::string id=(*feature)->getId();
                  //            CDBDebug("feature[%s] %d of %d with %d polygons", id.c_str(), featureIndex, features.size(), polygons.size());
                  for(std::vector<Polygon>::iterator itpoly = polygons.begin(); itpoly != polygons.end(); ++itpoly) {
                    float *polyX=itpoly->getLons();
                    float *polyY=itpoly->getLats();
                    int numPoints=itpoly->getSize();
                    float projectedX[numPoints];
                    float projectedY[numPoints];
                    
                    //CDBDebug("Plotting a polygon of %d points with %d holes [? of %d]", numPoints, itpoly->getHoles().size(), featureIndex);
                    int cnt=0;
                    for (int j=0; j<numPoints;j++) {
                      double tprojectedX=polyX[j];
                      double tprojectedY=polyY[j];
                      int status=0;
                      if(projectionRequired)status = imageWarper->reprojfromLatLon(tprojectedX,tprojectedY);
                      int dlon,dlat;
                      if(!status){
                        dlon=int((tprojectedX-offsetX)/cellSizeX)+1;
                        dlat=int((tprojectedY-offsetY)/cellSizeY);
                        projectedX[cnt]=dlon;
                        projectedY[cnt]=height-dlat;
                        cnt++;
                       }else{
                         CDBDebug("status: %d %d [%f,%f]", status, j, tprojectedX, tprojectedY);
//                         dlat=CCONVERTUGRIDMESH_NODATA;
//                         dlon=CCONVERTUGRIDMESH_NODATA;
                      }
//                       projectedX[j]=dlon;
//                       projectedY[j]=height-dlat;
                    }
                    
//                    CDBDebug("Draw polygon: %d points (%d)", cnt, numPoints);
                    drawImage->poly(projectedX, projectedY, cnt, 2, drawPointLineColor, false);
//                    break;
                    if (true) {
                        std::vector<PointArray>holes = itpoly->getHoles();
                        int nrHoles=holes.size();
                        int holeSize[nrHoles];
                        float *holeX[nrHoles];
                        float *holeY[nrHoles];
                        float *projectedHoleX[nrHoles];
                        float *projectedHoleY[nrHoles];
                        int h=0;
                        for(std::vector<PointArray>::iterator itholes = holes.begin(); itholes != holes.end(); ++itholes) {
                          //                   CDBDebug("holes[%d]: %d found in %d", 0, itholes->getSize(), featureIndex);
                          float *holeX=itholes->getLons();
                          float *holeY=itholes->getLats();
                          int holeSize=itholes->getSize();
                          float projectedHoleX[holeSize];
                          float projectedHoleY[holeSize];
                          
                          for (int j=0; j<holeSize;j++) {
                            //                      CDBDebug("J: %d", j);
                            double tprojectedX=holeX[j];
                            double tprojectedY=holeY[j];
                            int holeStatus=0;
                            if(projectionRequired)holeStatus = imageWarper->reprojfromLatLon(tprojectedX,tprojectedY);
                            int dlon,dlat;
                            if(!holeStatus){
                              dlon=int((tprojectedX-offsetX)/cellSizeX)+1;
                              dlat=int((tprojectedY-offsetY)/cellSizeY);
                            }else{
                              dlat=CCONVERTUGRIDMESH_NODATA;
                              dlon=CCONVERTUGRIDMESH_NODATA;
                            }
                            projectedHoleX[j]=dlon;
                            projectedHoleY[j]=height-dlat;
                            //                      CDBDebug("J: %d", j);
                          } 
 //                         CDBDebug("Draw hole[%d]: %d points", h, holeSize);
                          drawImage->poly(projectedHoleX, projectedHoleY, holeSize, 2, drawPointLineColor, false);
                          h++;
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
                
            }
          }
        }

        int CImgRenderPolylines::set(const char*values){

          settings.copy(values);
          return 0;
        }



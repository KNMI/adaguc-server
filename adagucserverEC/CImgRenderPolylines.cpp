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
        #include <string>

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
              CDBDebug("style: %d %d", s->FeatureInterval.size(), styleConfiguration->shadeIntervals->size());
              int numFeatures=s->FeatureInterval.size();
              CT::string attributeValues[numFeatures];
              /* Loop through all configured FeatureInterval elements */
              for(size_t j=0;j<styleConfiguration->featureIntervals->size();j++){
                CServerConfig::XMLE_FeatureInterval *featureInterval=((*styleConfiguration->featureIntervals)[j]);
                if(featureInterval->attr.match.empty()==false&&featureInterval->attr.matchid.empty()==false){
                  /* Get the matchid attribute for the feature */
                  CT::string attributeName = featureInterval->attr.matchid;
                  for(int featureNr = 0;featureNr<numFeatures;featureNr++){
                    attributeValues[featureNr] =  "";
                    std::map<int,CFeature>::iterator feature = dataSource->getDataObject(0)->features.find(featureNr);
                    if(feature!=dataSource->getDataObject(0)->features.end()){
                      std::map<std::string,std::string>::iterator attributeValueItr =  feature->second.paramMap.find(attributeName.c_str());
                      if(attributeValueItr!=feature->second.paramMap.end()){
                        attributeValues[featureNr] = attributeValueItr->second.c_str();
                        // CDBDebug("attributeValues[%d]=%s", featureNr, attributeValues[featureNr].c_str());
                      }
                    }
                  }
                  if(featureInterval->attr.fillcolor.empty()==false){
                    // CDBDebug("feature[%d] %s %s %s", j, featureInterval->attr.fillcolor.c_str(), 
                            //  featureInterval->attr.bgcolor.c_str(), featureInterval->attr.label.c_str());;
//                     std::vector<CImageDataWriter::IndexRange*> ranges=getIndexRangesForRegex(featureInterval->attr.match, attributeValues, numFeatures);
//                     for (size_t i=0; i<ranges.size(); i++) {
//                       CDBDebug("feature[%d] %d-%d %s %s %s",ranges[i]->min,ranges[i]->max, featureInterval->attr.fillcolor, featureInterval->attr.bgcolor?featureInterval->attr.bgcolor:"null", featureInterval->attr.label?featureInterval->attr.label:"null");
//                       CServerConfig::XMLE_ShadeInterval *shadeInterval = new CServerConfig::XMLE_ShadeInterval ();
//                       styleConfiguration->shadeIntervals->push_back(shadeInterval);
//                       shadeInterval->attr.min.print("%d",ranges[i]->min);
//                       shadeInterval->attr.max.print("%d",ranges[i]->max);
//                       shadeInterval->attr.fillcolor=featureInterval->attr.fillcolor;
//                       shadeInterval->attr.bgcolor=featureInterval->attr.bgcolor;
//                       shadeInterval->attr.label=featureInterval->attr.label;
//                       delete ranges[i];
//                     }
                  }
                }
              }
            }
          }
          
          CT::string name=dataSource->featureSet;
          
          bool projectionRequired=false;
          if(dataSource->srvParams->Geo->CRS.length()>0){
            projectionRequired=true;
          }
          
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
              // CDBDebug("Plotting %d features ONLY for %s", featureStore[fileName].size(), fileName.c_str());
              for (std::vector<Feature*>::iterator feature=featureStore[ fileName].begin();feature!=featureStore[fileName].end(); ++feature) {
                  //FindAttributes for this feature
                  BorderStyle borderStyle = getAttributesForFeature(&(dataSource->getDataObject(0)->features[featureIndex]) ,(*feature)->getId(), styleConfiguration);
                  //CDBDebug("bs: %s %s", borderStyle.width.c_str(), borderStyle.color.c_str());
                  CColor drawPointLineColor2(borderStyle.color.c_str());
                  float drawPointLineWidth=borderStyle.width.toFloat();
                  //if(featureIndex!=0)break;
                  std::vector<Polygon>polygons=(*feature)->getPolygons();
                  CT::string id=(*feature)->getId();
//                  CDBDebug("feature[%s] %d of %d with %d polygons", id.c_str(), featureIndex,           featureStore[fileName].size(), polygons.size());
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
                    drawImage->poly(projectedX, projectedY, cnt, drawPointLineWidth, drawPointLineColor2, true, false);
//                    break;
                    if (true) {
                        std::vector<PointArray>holes = itpoly->getHoles();
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
                          drawImage->poly(projectedHoleX, projectedHoleY, holeSize, drawPointLineWidth, drawPointLineColor2, true, false);
                          h++;
                        }
                      }
                  }
                  
                  std::vector<Polyline>polylines=(*feature)->getPolylines();
                              CT::string idl=(*feature)->getId();
                            //  CDBDebug("feature[%s] %d of %d with %d polylines", idl.c_str(), featureIndex, featureStore[fileName].size(), polylines.size());
                  for(std::vector<Polyline>::iterator itpoly = polylines.begin(); itpoly != polylines.end(); ++itpoly) {
                    float *polyX=itpoly->getLons();
                    float *polyY=itpoly->getLats();
                    int numPoints=itpoly->getSize();
                    float projectedX[numPoints];
                    float projectedY[numPoints];
                    
                    //CDBDebug("Plotting a polyline of %d points [? of %d] %f", numPoints, featureIndex);
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
                        // CDBDebug("status: %d %d [%f,%f]", status, j, tprojectedX, tprojectedY);
                        //                         dlat=CCONVERTUGRIDMESH_NODATA;
                        //                         dlon=CCONVERTUGRIDMESH_NODATA;
                      }
                      //                       projectedX[j]=dlon;
                      //                       projectedY[j]=height-dlat;
                    }
                    
//                                        CDBDebug("Draw polygon: %d points (%d)", cnt, numPoints);
                    drawImage->poly(projectedX, projectedY, cnt, drawPointLineWidth, drawPointLineColor2, false, false);
                    //                    break;
 
                  }  
                  #ifdef MEASURETIME
                  StopWatch_Stop("Feature drawn %d", featureIndex);
                  #endif
                  featureIndex++;
                }        
            }
          }
        }

      int CImgRenderPolylines::set(const char*values){

          settings.copy(values);
          return 0;
        }

        BorderStyle CImgRenderPolylines::getAttributesForFeature(CFeature *feature, CT::string id, CStyleConfiguration *styleConfig){
          CT::string borderWidth="3";
          CT::string borderColor="#008000FF";
          for(size_t j=0;j<styleConfig->featureIntervals->size();j++){
            // Draw border if borderWidth>0
            if ((*styleConfig->featureIntervals)[j]->attr.match.empty()==false){
              CT::string match=(*styleConfig->featureIntervals)[j]->attr.match;
              CT::string matchString=id;  
              if ((*styleConfig->featureIntervals)[j]->attr.matchid.empty()==false) {
                //match on matchid
                CT::string matchId;
                matchId = ((*styleConfig->featureIntervals)[j]->attr.matchid);
                std::map<std::string,std::string>::iterator attributeValueItr =  feature->paramMap.find(matchId.c_str());
                if(attributeValueItr!=feature->paramMap.end()){
//                  attributeValues[featureNr] = attributeValueItr->c_str();
//                  CDBDebug("Match on %s", attributeValueItr->second.c_str());
                  matchString=attributeValueItr->second.c_str();
                }
              
              } else {
                // match on id
                matchString=id;
              }
              //CDBDebug("matchString: %s", matchString.c_str());
              regex_t regex;
              int ret=regcomp(&regex, match.c_str(), 0);
              if (!ret){
                if (regexec(&regex, matchString.c_str(), 0, NULL, 0)==0) {
                  //Matched
                  //CDBDebug("Matched %s on %s!!", matchString.c_str(), match.c_str());
                  if(((*styleConfig->featureIntervals)[j]->attr.borderwidth.empty()==false)&&(((*styleConfig->featureIntervals)[j]->attr.borderwidth.toFloat())>0)){
                    borderWidth=(*styleConfig->featureIntervals)[j]->attr.borderwidth;
                    //A border should be drawn
                    if ((*styleConfig->featureIntervals)[j]->attr.bordercolor.empty()==false){
                      borderColor=(*styleConfig->featureIntervals)[j]->attr.bordercolor;
                    } else {
                      //Use default color
                    }
                  } else {
                    //Draw no border
                  }
                }
              }
            }
          }
          //CDBDebug("found: %s %s", borderWidth.c_str(), borderColor.c_str());
          BorderStyle bs={borderWidth, borderColor}; 
          return bs;
        }

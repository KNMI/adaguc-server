#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include "CXMLGen.h"

const char *CXMLGen::className="CXMLGen";
int CXMLGen::WCSDescribeCoverage(CServerParams *srvParam,CT::string *XMLDocument){
  return OGCGetCapabilities(srvParam,XMLDocument);
}

int CXMLGen::getFileNameForLayer(WMSLayer * myWMSLayer){
  /**********************************************/
  /*  Read the file to obtain BBOX parameters   */
  /**********************************************/
  int status;
  if(myWMSLayer->layer->attr.type.equals("database")||
    myWMSLayer->layer->attr.type.equals("styled")){
    //Check if any dimension is given:
    if(myWMSLayer->layer->Dimension.size()==0){
      //If not, just return the filename
      myWMSLayer->fileName.copy(myWMSLayer->layer->FilePath[0]->value.c_str());
      return 0;
    }
    //CDBDebug("Database");  
    CPGSQLDB DB;
    
    status = DB.connect(srvParam->cfg->DataBase[0]->attr.parameters.c_str());if(status!=0)return 1;
    CT::string tableName(myWMSLayer->layer->DataBaseTable[0]->value.c_str());
    CT::string dimName(myWMSLayer->layer->Dimension[0]->attr.name.c_str());
    makeCorrectTableName(&tableName,&dimName);
    CT::string query;
    query.print("select path from %s limit 1",tableName.c_str());
          
    CT::string *values = DB.query_select(query.c_str(),0);
    if(values == NULL){CDBError("Query failed");DB.close();return 1;}
    if(values->count>0){
      myWMSLayer->fileName.copy(&values[0]);
    }
    delete[] values;
    DB.close();
    //CDBDebug("/Database");  
    }
    //If this layer is a file type layer (not a database type layer) TODO type file is deprecated...
    if(myWMSLayer->layer->attr.type.equals("file")){
      CDBWarning("Type 'file' is deprecated");
      CT::string pathFileName("");
      if(myWMSLayer->fileName.c_str()[0]!='/'){
        pathFileName.copy(srvParam->cfg->Path[0]->attr.value.c_str());
        pathFileName.concat("/");
      }
      pathFileName.concat(&myWMSLayer->fileName);
      myWMSLayer->fileName.copy(pathFileName.c_str(),pathFileName.length());
    }
    return 0;
}

int CXMLGen::getDataSourceForLayer(WMSLayer * myWMSLayer){
  CDataReader reader;
  //CDBDebug("Reading %s",myWMSLayer->fileName.c_str());
  myWMSLayer->dataSource = new CDataSource ();
  myWMSLayer->dataSource->addTimeStep(myWMSLayer->fileName.c_str(),"");
  myWMSLayer->dataSource->setCFGLayer(srvParam,srvParam->configObj->Configuration[0],myWMSLayer->layer,myWMSLayer->name.c_str());
  CDBDebug("b");
  int status = reader.open(myWMSLayer->dataSource,CNETCDFREADER_MODE_OPEN_HEADER,NULL);
  CDBDebug("b");
  if(status!=0){
    CDBError("Could not open file: %s",myWMSLayer->dataSource->getFileName());
    return 1;
  }
  reader.close();
  return 0;
}


int CXMLGen::getProjectionInformationForLayer(WMSLayer * myWMSLayer){
  CGeoParams geo;
  CImageWarper warper;
  int status;  
  for(size_t p=0;p< srvParam->cfg->Projection.size();p++){
    geo.CRS.copy(srvParam->cfg->Projection[p]->attr.id.c_str());
    status =  warper.initreproj(myWMSLayer->dataSource,&geo,&srvParam->cfg->Projection);
    if(status!=0)return 1;
    // Find the max extent of the image
    WMSLayer::Projection * myProjection = new WMSLayer::Projection();
    myWMSLayer->projectionList.push_back(myProjection);
    
    //Set the projection string
    myProjection->name.copy(srvParam->cfg->Projection[p]->attr.id.c_str());
    
    //Calculate the extent for this projection
    warper.findExtent(myWMSLayer->dataSource,myProjection->dfBBOX);

    //Calculate the latlonBBOX
    if(srvParam->cfg->Projection[p]->attr.id.equals("EPSG:4326")){
      for(int k=0;k<4;k++)myWMSLayer->dfLatLonBBOX[k]=myProjection->dfBBOX[k];
    }
    warper.closereproj();
  }
  return 0;
}

int CXMLGen::getDimsForLayer(WMSLayer * myWMSLayer){
  char szMaxTime[32];
  char szMinTime[32];
  //char szInterval[32];
  int hastimedomain = 0;

// Dimensions
  if(myWMSLayer->layer->attr.type.equals("database")||
    myWMSLayer->layer->attr.type.equals("styled")){
          //printf("Opening DB\n");
    CPGSQLDB DB;
    int status = DB.connect(srvParam->cfg->DataBase[0]->attr.parameters.c_str());if(status!=0)return 1;
    for(size_t i=0;i<myWMSLayer->dataSource->cfgLayer->Dimension.size();i++){
      //Create a new dim to store in the layer
      WMSLayer::Dim *dim=new WMSLayer::Dim();myWMSLayer->dimList.push_back(dim);
      //Get the tablename
      CT::string tableName(myWMSLayer->layer->DataBaseTable[0]->value.c_str());
      CT::string dimName(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str());
      makeCorrectTableName(&tableName,&dimName);
            
      bool hasMultipleValues=false;
      if(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.c_str()==NULL)hasMultipleValues=true;
      //This is a multival dim.
      if(hasMultipleValues==true){
        //Get all dimension values from the db
        CT::string query;
        query.print("select %s from %s",myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str(),tableName.c_str());
        CT::string *values = DB.query_select(query.c_str(),0);
        if(values == NULL){CDBError("Query failed");DB.close();return 1;}
        if(values->count>0){
          //if(srvParam->requestType==REQUEST_WMS_GETCAPABILITIES)
          {
            if(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.units.c_str()==NULL){
              CDBError("No units specified for dim %s in layer %s",
                      myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str(),
                      myWMSLayer->dataSource->layerName.c_str());
              return 1;
            }
            dim->name.copy(myWMSLayer->dataSource->cfgLayer->Dimension[i]->value.c_str());
            dim->units.copy(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.units.c_str());
            dim->hasMultipleValues=1;
            dim->defaultValue.copy(values[0].c_str());
            dim->values.copy(&values[0]);
            for(size_t j=1;j<size_t(values->count);j++){
              dim->values.printconcat(",%s",values[j].c_str());
            }
            
          }
        }
        delete[] values;
      }

      //This is an interval
      if(hasMultipleValues==false){
        // Retrieve the max dimension value
        CT::string query;
        query.print("select max(%s) from %s",myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str(),tableName.c_str());
        CT::string *values = DB.query_select(query.c_str(),0);
        if(values == NULL){CDBError("Query failed");DB.close();return 1;}
        if(values->count>0){snprintf(szMaxTime,31,"%s",values[0].c_str());szMaxTime[10]='T';}
        delete[] values;
              // Retrieve the minimum dimension value
        query.print("select min(%s) from %s",myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str(),tableName.c_str());
        values = DB.query_select(query.c_str(),0);
        if(values == NULL){CDBError("Query failed");DB.close();return 1;}
        if(values->count>0){snprintf(szMinTime,31,"%s",values[0].c_str());szMinTime[10]='T';}
        delete[] values;
              // Retrieve all values for time position
        //    if(srvParam->serviceType==SERVICE_WCS){
        query.print("select %s from %s",myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str(),tableName.c_str());
        values = DB.query_select(query.c_str(),0);
        if(values == NULL){CDBError("Query failed");DB.close();return 1;}
        if(values->count>0){
          if(TimePositions!=NULL){
            delete[] TimePositions;
            TimePositions=NULL;
          }
          TimePositions=new CT::string[values->count];
          char szTemp[32];
          for(size_t l=0;l<values->count;l++){
            snprintf(szTemp,31,"%s",values[l].c_str());szTemp[10]='T';
            TimePositions[l].copy(szTemp);
            TimePositions[l].count=values[l].count;
          }
        }
        delete[] values;
        if(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.c_str()==NULL){
          //TODO
          CDBError("Dimension interval '%d' not defined",i);return 1;
        }
        //strncpy(szInterval,myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.c_str(),32);szInterval[31]='\0';
        const char *pszInterval=myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.c_str();
            hastimedomain = 1;
        //if(srvParam->requestType==REQUEST_WMS_GETCAPABILITIES)
        {
          CT::string dimUnits("ISO8601");
          if(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.units.c_str()!=NULL){
            dimUnits.copy(myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.units.c_str());
          }
          dim->name.copy(myWMSLayer->dataSource->cfgLayer->Dimension[i]->value.c_str());
          dim->units.copy(dimUnits.c_str());
          dim->hasMultipleValues=0;
          //myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.defaultV.c_str()
          const char *pszDefaultV=myWMSLayer->dataSource->cfgLayer->Dimension[i]->attr.defaultV.c_str();
          CT::string defaultV;if(pszDefaultV!=NULL)defaultV=pszDefaultV;
          if(defaultV.length()==0||defaultV.equals("max",3)){
            dim->defaultValue.copy(szMaxTime);dim->defaultValue.concat("Z");
          }else if(defaultV.equals("min",3)){
            dim->defaultValue.copy(szMinTime);dim->defaultValue.concat("Z");
          }else {
            dim->defaultValue.copy(&defaultV);
          }
          //dim->defaultValue.copy(szMinTime);dim->defaultValue.concat("Z");
          dim->values.print("%sZ/%sZ/%s",szMinTime,szMaxTime,pszInterval);
        }
      }
    }
    DB.close();
    }
    return 0;
}

int CXMLGen::getStylesForLayer(WMSLayer * myWMSLayer){
  CT::string styleName("default");
  std::vector <std::string> styleNames;
  if(myWMSLayer->dataSource->cfgLayer->Styles.size()==1){
    CT::string *layerStyleNames=NULL;
    CT::string styles(myWMSLayer->dataSource->cfgLayer->Styles[0]->value.c_str());
    layerStyleNames = styles.split(",");
              
    CT::string name;
    if(layerStyleNames->count>0){
                //styleNames.push_back(layerStyleNames[0].c_str());
                
      for(size_t s=0;s<layerStyleNames->count;s++){
        if(layerStyleNames[s].length()>0){
                    //Check wheter we should at this style or not...
          int dConfigStyleIndex=-1;
          if(srvParam->cfg->Style.size()>0){
            for(size_t j=0;j<srvParam->cfg->Style.size()&&dConfigStyleIndex==-1;j++){
              if(layerStyleNames[s].equals(srvParam->cfg->Style[j]->attr.name.c_str())){
                dConfigStyleIndex=j;
                break;
              }
            }
          }
          if(dConfigStyleIndex==-1){
            CDBError("Style %s is configured in the Layer, but this style is not configured in the servers configuration file",layerStyleNames[s].c_str());
            delete[] layerStyleNames;
            return 1;
          }
          if(dConfigStyleIndex>=int(srvParam->cfg->Style.size())){
            CDBError("dConfigStyleIndex>=srvParam->cfg->Style.size()");
            delete[] layerStyleNames;
            return 1;
          }
                    //Now list all the desired rendermethods
          CT::string renderMethodList;
          if(srvParam->cfg->Style[dConfigStyleIndex]->RenderMethod.size()==1){
            renderMethodList.copy(srvParam->cfg->Style[dConfigStyleIndex]->RenderMethod[0]->value.c_str());
          }
                    //rendermethods defined in the layers override rendermethods defined in the style
          if(myWMSLayer->dataSource->cfgLayer->RenderMethod.size()==1){
            renderMethodList.copy(myWMSLayer->dataSource->cfgLayer->RenderMethod[0]->value.c_str());
          }
                    //If still no list of rendermethods is found, use the default list
          if(renderMethodList.length()==0){
            renderMethodList.copy(CImageDataWriter::RenderMethodStringList);
          }
          CT::string *renderMethods = renderMethodList.split(",");
          for(size_t r=0;r<renderMethods->count;r++){
            if(renderMethods[r].length()>0){
                        //Check wether we should support this rendermethod or not:
              bool skipRenderMethod=false;
              if(srvParam->cfg->Style[dConfigStyleIndex]->ContourIntervalL.size()<=0||
                srvParam->cfg->Style[dConfigStyleIndex]->ContourIntervalH.size()<=0){
                if(renderMethods[r].equals("contour")||
                  renderMethods[r].equals("bilinearcontour")||
                  renderMethods[r].equals("nearestcontour")){
                  skipRenderMethod=true;
                  }
                }
                if(skipRenderMethod==false){
                  if(renderMethods[r].equals("nearest")){
                    //Default is always nearest, so we do not need to add it with style/nearest
                    name.print("%s",layerStyleNames[s].c_str());
                  }else{
                    //Otherwise we add the rendermethod by /bilinear etc...
                    name.print("%s/%s",layerStyleNames[s].c_str(),renderMethods[r].c_str());
                  }
                  styleNames.push_back(name.c_str());
                }
            }
          }
          delete[] renderMethods;
        }
      }
    }
    delete[] layerStyleNames;
  }
  for(int s=-1;s<(int)styleNames.size();s++){
    if(s>=0){
      styleName.copy(styleNames[s].c_str());
    }
    if(styleName.length()>0){
      WMSLayer::Style *style = new WMSLayer::Style();
      style->name.copy(&styleName);
      myWMSLayer->styleList.push_back(style);
    }
  }
  return 0;
}


int CXMLGen::getWMS_1_0_0_Capabilities(CT::string *XMLDoc,std::vector<WMSLayer*> *myWMSLayerList){
  CFile header;
  CT::string OnlineResource(srvParam->cfg->OnlineResource[0]->attr.value.c_str());
  OnlineResource.concat("SERVICE=WMS&amp;");
  int status=header.     open(srvParam->cfg->Path[0]->attr.value.c_str(),WMS_1_0_0_HEADERFILE);if(status!=0)return 1;
  
  XMLDoc->copy(header.data);
  XMLDoc->replace("[SERVICETITLE]",srvParam->cfg->WMS[0]->Title[0]->value.c_str());
  XMLDoc->replace("[SERVICEABSTRACT]",srvParam->cfg->WMS[0]->Abstract[0]->value.c_str());
  XMLDoc->replace("[GLOBALLAYERTITLE]",srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.c_str());
  XMLDoc->replace("[SERVICEONLINERESOURCE]",OnlineResource.c_str());
  if(myWMSLayerList->size()>0){
    for(size_t p=0;p<(*myWMSLayerList)[0]->projectionList.size();p++){
      WMSLayer::Projection *proj = (*myWMSLayerList)[0]->projectionList[p];
      XMLDoc->concat("<SRS>"); 
      XMLDoc->concat(&proj->name);
      XMLDoc->concat("</SRS>\n");
    }
    
    for(size_t lnr=0;lnr<myWMSLayerList->size();lnr++){
      WMSLayer *layer = (*myWMSLayerList)[lnr];
      if(layer->hasError==0){
        XMLDoc->printconcat("<Layer queryable=\"%d\">\n",layer->isQuerable);
        XMLDoc->concat("<Name>"); XMLDoc->concat(&layer->name);XMLDoc->concat("</Name>\n");
        XMLDoc->concat("<Title>"); XMLDoc->concat(&layer->title);XMLDoc->concat("</Title>\n");
        
        XMLDoc->concat("<SRS>"); 
        for(size_t p=0;p<layer->projectionList.size();p++){
          WMSLayer::Projection *proj = layer->projectionList[p];
          XMLDoc->concat(&proj->name);
          if(p+1<layer->projectionList.size())XMLDoc->concat(" ");
        }
        XMLDoc->concat("</SRS>\n");
        XMLDoc->printconcat("<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n",
                            layer->dfLatLonBBOX[0],layer->dfLatLonBBOX[1],layer->dfLatLonBBOX[2],layer->dfLatLonBBOX[3]);
        //Dims
        for(size_t d=0;d<layer->dimList.size();d++){
          WMSLayer::Dim * dim = layer->dimList[d];
          XMLDoc->printconcat("<Dimension name=\"%s\" units=\"%s\"/>\n",dim->name.c_str(),dim->units.c_str());
          XMLDoc->printconcat("<Extent name=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\">\n",
                              dim->name.c_str(),dim->defaultValue.c_str(),1);
          XMLDoc->concat(dim->values.c_str());
          XMLDoc->concat("</Extent>\n");
        }
        XMLDoc->concat("</Layer>\n");
      }else{
        CDBError("Skipping layer %s",layer->name.c_str());
      }
    }
  }
  XMLDoc->concat("    </Layer>\n  </Capability>\n</WMT_MS_Capabilities>\n");
  return 0;
}


int CXMLGen::getWMS_1_1_1_Capabilities(CT::string *XMLDoc,std::vector<WMSLayer*> *myWMSLayerList){
  CFile header;
  CT::string OnlineResource(srvParam->cfg->OnlineResource[0]->attr.value.c_str());
  OnlineResource.concat("SERVICE=WMS&amp;");
  int status=header.     open(srvParam->cfg->Path[0]->attr.value.c_str(),WMS_1_1_1_HEADERFILE);if(status!=0)return 1;
  
  XMLDoc->copy(header.data);
  XMLDoc->replace("[SERVICETITLE]",srvParam->cfg->WMS[0]->Title[0]->value.c_str());
  XMLDoc->replace("[SERVICEABSTRACT]",srvParam->cfg->WMS[0]->Abstract[0]->value.c_str());
  XMLDoc->replace("[GLOBALLAYERTITLE]",srvParam->cfg->WMS[0]->RootLayer[0]->Title[0]->value.c_str());
  XMLDoc->replace("[SERVICEONLINERESOURCE]",OnlineResource.c_str());
  
  if(myWMSLayerList->size()>0){
    for(size_t p=0;p<(*myWMSLayerList)[0]->projectionList.size();p++){
      WMSLayer::Projection *proj = (*myWMSLayerList)[0]->projectionList[p];
      XMLDoc->concat("<SRS>"); 
      XMLDoc->concat(&proj->name);
      XMLDoc->concat("</SRS>\n");
    }
    
    //Make a unique list of all groups
    std::vector <std::string>  groupKeys;
    for(size_t lnr=0;lnr<myWMSLayerList->size();lnr++){
      WMSLayer *layer = (*myWMSLayerList)[lnr];
      std::string key="";
      if(layer->group.length()>0)key=layer->group.c_str();
      size_t j=0;
      for(j=0;j<groupKeys.size();j++){if(groupKeys[j]==key)break;}
      if(j>=groupKeys.size())groupKeys.push_back(key);
    }
    //Sort the groups alphabetically
    std::sort(groupKeys.begin(), groupKeys.end());
    
    //Loop through the groups
    int currentGroupDepth=0;
    for(size_t groupIndex=0;groupIndex<groupKeys.size();groupIndex++){
      //CDBError("group %s",groupKeys[groupIndex].c_str());
      int groupDepth=0;
      
      //if(groupKeys[groupIndex].size()>0)
      {
        CT::string key=groupKeys[groupIndex].c_str();
        CT::string *subGroups=key.split("/");
        groupDepth=subGroups->count;
        
        if(groupIndex>0){
          CT::string prevKey=groupKeys[groupIndex-1].c_str();
          CT::string *prevSubGroups=prevKey.split("/");
          

          for(size_t j=subGroups->count;j<prevSubGroups->count;j++){
            CDBError("<");
            currentGroupDepth--;
            XMLDoc->concat("</Layer>\n");
          }
            
          //CDBError("subGroups->count %d",subGroups->count);
          //CDBError("prevSubGroups->count %d",prevSubGroups->count);
          int removeGroups=0;
          for(size_t j=0;j<subGroups->count&&j<prevSubGroups->count;j++){
            //CDBError("CC %d",j);
            if(subGroups[j].equals(&prevSubGroups[j])==false||removeGroups==1){
              removeGroups=1;
              //CDBError("!=%d %s!=%s",j,subGroups[j].c_str(),prevSubGroups[j].c_str());
              //CDBError("<");
              XMLDoc->concat("</Layer>\n");
              currentGroupDepth--;
              //break;
            }
          }
          //CDBDebug("!!! %d",currentGroupDepth);
          for(size_t j=currentGroupDepth;j<subGroups->count;j++){
            XMLDoc->concat("<Layer>\n");
            XMLDoc->concat("<Title>");
            //CDBError("> %s",subGroups[j].c_str());
            XMLDoc->concat(subGroups[j].c_str());
            XMLDoc->concat("</Title>\n");
          }
            
          delete[] prevSubGroups;
        }
        else{
          for(size_t j=0;j<subGroups->count;j++){
            XMLDoc->concat("<Layer>\n");
            XMLDoc->concat("<Title>");
            //CDBError("> %s grpupindex %d",subGroups[j].c_str(),groupIndex);
            XMLDoc->concat(subGroups[j].c_str());
            XMLDoc->concat("</Title>\n");
          }
        }
        delete[] subGroups;
        currentGroupDepth=groupDepth;
        //CDBDebug("currentGroupDepth = %d",currentGroupDepth);
      }
      
      for(size_t lnr=0;lnr<myWMSLayerList->size();lnr++){
        WMSLayer *layer = (*myWMSLayerList)[lnr];
        if(layer->group.equals(groupKeys[groupIndex].c_str())){
          //CDBError("layer %d %s",groupDepth,layer->name.c_str());
          if(layer->hasError==0){
            XMLDoc->printconcat("<Layer queryable=\"%d\" opaque=\"1\" cascaded=\"0\">\n",layer->isQuerable);
            XMLDoc->concat("<Name>"); XMLDoc->concat(&layer->name);XMLDoc->concat("</Name>\n");
            XMLDoc->concat("<Title>"); XMLDoc->concat(&layer->title);XMLDoc->concat("</Title>\n");
            
            
            for(size_t p=0;p<layer->projectionList.size();p++){
              WMSLayer::Projection *proj = layer->projectionList[p];
              XMLDoc->concat("<SRS>"); 
              XMLDoc->concat(&proj->name);
              XMLDoc->concat("</SRS>\n");
              XMLDoc->printconcat("<BoundingBox SRS=\"%s\" minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n",
                                  proj->name.c_str(),proj->dfBBOX[0],proj->dfBBOX[1],proj->dfBBOX[2],proj->dfBBOX[3]);
            }
            
            XMLDoc->printconcat("<LatLonBoundingBox minx=\"%f\" miny=\"%f\" maxx=\"%f\" maxy=\"%f\" />\n",
                                layer->dfLatLonBBOX[0],layer->dfLatLonBBOX[1],layer->dfLatLonBBOX[2],layer->dfLatLonBBOX[3]);
            //Dims
            for(size_t d=0;d<layer->dimList.size();d++){
              WMSLayer::Dim * dim = layer->dimList[d];
              XMLDoc->printconcat("<Dimension name=\"%s\" units=\"%s\"/>\n",dim->name.c_str(),dim->units.c_str());
              XMLDoc->printconcat("<Extent name=\"%s\" default=\"%s\" multipleValues=\"%d\" nearestValue=\"0\">\n",
                                  dim->name.c_str(),dim->defaultValue.c_str(),1);
              XMLDoc->concat(dim->values.c_str());
              XMLDoc->concat("</Extent>\n");
            }
            
            //Styles
            for(size_t s=0;s<layer->styleList.size();s++){
              WMSLayer::Style * style = layer->styleList[s];
              
              XMLDoc->concat("   <Style>");
              XMLDoc->printconcat("    <Name>%s</Name>",style->name.c_str());
              XMLDoc->printconcat("    <Title>%s</Title>",style->name.c_str());
              XMLDoc->printconcat("    <LegendURL width=\"%d\" height=\"%d\">",LEGEND_WIDTH,LEGEND_HEIGHT);
              XMLDoc->concat("       <Format>image/png</Format>");
              XMLDoc->printconcat("       <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s&amp;version=1.1.1&amp;service=WMS&amp;request=GetLegendGraphic&amp;layer=%s&amp;format=image/png&amp;STYLE=%s\"/>"
                  ,OnlineResource.c_str(),layer->name.c_str(),style->name.c_str());
              XMLDoc->concat("    </LegendURL>");
              XMLDoc->concat("  </Style>");
            
              if(layer->dataSource->cfgLayer->MetadataURL.size()>0){
                XMLDoc->concat("   <MetadataURL type=\"TC211\">\n");
                XMLDoc->concat("     <Format>text/xml</Format>\n");
                XMLDoc->printconcat("     <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\"/>",layer->dataSource->cfgLayer->MetadataURL[0]->value.c_str());
                XMLDoc->concat("   </MetadataURL>\n");
              } 
            }
            XMLDoc->concat("        <ScaleHint min=\"0\" max=\"10000\" />\n");
            XMLDoc->concat("</Layer>\n");
          }else{
            CDBError("Skipping layer %s",layer->name.c_str());
          }
        }
      }
      
      
      
    }
    
    //CDBDebug("** %d",currentGroupDepth);
    for(int j=0;j<currentGroupDepth;j++){
      XMLDoc->concat("</Layer>\n");
    }
  }
  XMLDoc->concat("    </Layer>\n  </Capability>\n</WMT_MS_Capabilities>\n");
  return 0;
}

int CXMLGen::getWCS_1_0_0_Capabilities(CT::string *XMLDoc,std::vector<WMSLayer*> *myWMSLayerList){
  CFile header;
  CT::string OnlineResource(srvParam->cfg->OnlineResource[0]->attr.value.c_str());
  OnlineResource.concat("SERVICE=WCS&amp;");
  int status=header.     open(srvParam->cfg->Path[0]->attr.value.c_str(),WCS_1_0_HEADERFILE);if(status!=0)return 1;
  
  XMLDoc->copy(header.data);
  if(srvParam->cfg->WCS[0]->Title.size()==0){
    CDBError("No title defined for WCS");
    return 1;
  }
  if(srvParam->cfg->WCS[0]->Name.size()==0){
    CDBError("No Name defined for WCS");
    return 1;
  }
  if(srvParam->cfg->WCS[0]->Abstract.size()==0)
  {
    srvParam->cfg->WCS[0]->Abstract.push_back(new CServerConfig::XMLE_Abstract());
    srvParam->cfg->WCS[0]->Abstract[0]->value.copy(srvParam->cfg->WCS[0]->Title[0]->value.c_str());
  }
  XMLDoc->replace("[SERVICENAME]",srvParam->cfg->WCS[0]->Title[0]->value.c_str());
  XMLDoc->replace("[SERVICETITLE]",srvParam->cfg->WCS[0]->Name[0]->value.c_str());
  XMLDoc->replace("[SERVICEABSTRACT]",srvParam->cfg->WCS[0]->Abstract[0]->value.c_str());
  XMLDoc->replace("[SERVICEONLINERESOURCE]",OnlineResource.c_str());
  
  if(myWMSLayerList->size()>0){
    
    for(size_t lnr=0;lnr<myWMSLayerList->size();lnr++){
      WMSLayer *layer = (*myWMSLayerList)[lnr];
      if(layer->hasError==0){
        XMLDoc->printconcat("<CoverageOfferingBrief>\n");
        XMLDoc->concat("<description>"); XMLDoc->concat(&layer->name);XMLDoc->concat("</description>\n");
        XMLDoc->concat("<name>"); XMLDoc->concat(&layer->name);XMLDoc->concat("</name>\n");
        XMLDoc->concat("<label>"); XMLDoc->concat(&layer->title);XMLDoc->concat("</label>\n");
        XMLDoc->printconcat("  <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n"
            "    <gml:pos>%f %f</gml:pos>\n"
            "    <gml:pos>%f %f</gml:pos>\n",
        layer->dfLatLonBBOX[0],
        layer->dfLatLonBBOX[1],
        layer->dfLatLonBBOX[2],
        layer->dfLatLonBBOX[3]);
        
        XMLDoc->printconcat("</lonLatEnvelope>\n");
        XMLDoc->printconcat("</CoverageOfferingBrief>\n");
        
      }else{
        CDBError("Skipping layer %s",layer->name.c_str());
      }
    }
  }
  XMLDoc->concat("</ContentMetadata>\n</WCS_Capabilities>\n");
  return 0;
}

int CXMLGen::getWCS_1_0_0_DescribeCoverage(CT::string *XMLDoc,std::vector<WMSLayer*> *myWMSLayerList){
  
  XMLDoc->copy("<?xml version='1.0' encoding=\"ISO-8859-1\" ?>\n"
      "<CoverageDescription\n"
      "   version=\"1.0.0\" \n"
      "   updateSequence=\"0\" \n"
      "   xmlns=\"http://www.opengis.net/wcs\" \n"
      "   xmlns:xlink=\"http://www.w3.org/1999/xlink\" \n"
      "   xmlns:gml=\"http://www.opengis.net/gml\" \n"
      "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
      "   xsi:schemaLocation=\"http://www.opengis.net/wcs http://schemas.opengis.net/wcs/1.0.0/describeCoverage.xsd\">\n");
  if(myWMSLayerList->size()>0){
    for(size_t layerIndex=0;layerIndex<(unsigned)srvParam->WMSLayers->count;layerIndex++){  
      for(size_t lnr=0;lnr<myWMSLayerList->size();lnr++){
        WMSLayer *layer = (*myWMSLayerList)[lnr];
        if(layer->name.equals(srvParam->WMSLayers[layerIndex].c_str())){
          if(layer->hasError==0){
          
          //Look wether and which dimension is a time dimension
            int timeDimIndex=-1;
            for(size_t d=0;d<layer->dimList.size();d++){
              WMSLayer::Dim * dim = layer->dimList[d];
              if(dim->hasMultipleValues==0){
                if(dim->units.equals("ISO8601")){
                  timeDimIndex=d;
                }
              }
            }
  
            if(srvParam->requestType==REQUEST_WCS_DESCRIBECOVERAGE){
            //XMLDoc->print("<?xml version='1.0' encoding=\"ISO-8859-1\" ?>\n");
            
              XMLDoc->printconcat("  <CoverageOffering>\n"
                  "  <description>%s</description>\n"
                  "  <name>%s</name>\n"
                  "  <label>%s</label>\n"
                  "  <lonLatEnvelope srsName=\"urn:ogc:def:crs:OGC:1.3:CRS84\">\n"
                  "    <gml:pos>%f %f</gml:pos>\n"
                  "    <gml:pos>%f %f</gml:pos>\n",
              layer->name.c_str(),layer->name.c_str(),layer->title.c_str(),
              layer->dfLatLonBBOX[0],layer->dfLatLonBBOX[1],layer->dfLatLonBBOX[2],layer->dfLatLonBBOX[3]
                                );
  
              if(timeDimIndex>=0){
              //For information about this, visit http://www.galdosinc.com/archives/151
                CT::string * timeDimSplit = layer->dimList[timeDimIndex]->values.split("/");
                if(timeDimSplit->count==3){
                  XMLDoc->concat("        <gml:TimePeriod>\n");
                  XMLDoc->printconcat("          <gml:begin>%s</gml:begin>\n",timeDimSplit[0].c_str());
                  XMLDoc->printconcat("          <gml:end>%s</gml:end>\n",timeDimSplit[1].c_str());
                  XMLDoc->printconcat("          <gml:duration>%s</gml:duration>\n",timeDimSplit[2].c_str());
                  XMLDoc->concat("        </gml:TimePeriod>\n");
                }
                delete[] timeDimSplit;
              }
              XMLDoc->concat("  </lonLatEnvelope>\n"
                  "  <domainSet>\n"
                  "    <spatialDomain>\n");
              for(size_t p=0;p< layer->projectionList.size();p++){
                WMSLayer::Projection *proj = (*myWMSLayerList)[0]->projectionList[p];
                XMLDoc->printconcat("        <gml:Envelope srsName=\"%s\">\n"
                    "          <gml:pos>%f %f</gml:pos>\n"
                    "          <gml:pos>%f %f</gml:pos>\n"
                    "        </gml:Envelope>\n",
                proj->name.c_str(),
                proj->dfBBOX[0],proj->dfBBOX[1],proj->dfBBOX[2],proj->dfBBOX[3]
                                  );
              }
              XMLDoc->printconcat(
                  "        <gml:RectifiedGrid dimension=\"2\">\n"
                  "          <gml:limits>\n"
                  "            <gml:GridEnvelope>\n"
                  "              <gml:low>0 0</gml:low>\n"
                  "              <gml:high>%d %d</gml:high>\n"
                  "            </gml:GridEnvelope>\n"
                  "          </gml:limits>\n"
                  "          <gml:axisName>x</gml:axisName>\n"
                  "          <gml:axisName>y</gml:axisName>\n"
                  "          <gml:origin>\n"
                  "            <gml:pos>%f %f</gml:pos>\n"
                  "          </gml:origin>\n"
                  "          <gml:offsetVector>%f 0</gml:offsetVector>\n"
                  "          <gml:offsetVector>0 %f</gml:offsetVector>\n"
                  "        </gml:RectifiedGrid>\n"
                  "      </spatialDomain>\n",
              layer->dataSource->dWidth-1,
              layer->dataSource->dHeight-1,
              layer->dataSource->dfBBOX[0],
              layer->dataSource->dfBBOX[1],
              layer->dataSource->dfCellSizeX,
              layer->dataSource->dfCellSizeY
                                );
          
              if(timeDimIndex>=0){
                XMLDoc->concat("      <temporalDomain>\n");
                {
              //For information about this, visit http://www.galdosinc.com/archives/151
                  CT::string * timeDimSplit = layer->dimList[timeDimIndex]->values.split("/");
                  if(timeDimSplit->count==3){
                    XMLDoc->concat("        <gml:TimePeriod>\n");
                    XMLDoc->printconcat("          <gml:begin>%s</gml:begin>\n",timeDimSplit[0].c_str());
                    XMLDoc->printconcat("          <gml:end>%s</gml:end>\n",timeDimSplit[1].c_str());
                    XMLDoc->printconcat("          <gml:duration>%s</gml:duration>\n",timeDimSplit[2].c_str());
                    XMLDoc->concat("        </gml:TimePeriod>\n");
                  }
                  delete[] timeDimSplit;
                }
                XMLDoc->concat("      </temporalDomain>\n");
              }
              XMLDoc->concat("    </domainSet>\n"
                  "    <rangeSet>\n"
                  "      <RangeSet>\n"
                  "        <name>bands</name>\n"
                  "        <label>bands</label>\n"
                  "        <axisDescription>\n"
                  "          <AxisDescription>\n"
                  "            <name>bands</name>\n"
                  "            <label>Bands/Channels/Samples</label>\n"
                  "            <values>\n"
                  "              <singleValue>1</singleValue>\n"
                  "            </values>\n"
                  "          </AxisDescription>\n"
                  "        </axisDescription>\n"
                  "      </RangeSet>\n"
                  "    </rangeSet>\n");
                // Supported CRSs
              XMLDoc->concat("    <supportedCRSs>\n");
          
          
              for(size_t p=0;p< layer->projectionList.size();p++){
                WMSLayer::Projection *proj = (*myWMSLayerList)[0]->projectionList[p];
                CT::string encodedProjString(proj->name.c_str());
                encodedProjString.encodeURL();
                XMLDoc->printconcat("      <requestResponseCRSs>%s</requestResponseCRSs>\n",
                                    encodedProjString.c_str());
              }
          
          
              XMLDoc->printconcat("      <nativeCRSs>%s</nativeCRSs>\n    </supportedCRSs>\n",
                                  layer->dataSource->nativeEPSG.c_str());
  
              XMLDoc->concat(
                  "    <supportedFormats nativeFormat=\"NetCDF4\">\n"
                  "      <formats>GeoTIFF</formats>\n"
                  "      <formats>AAIGRID</formats>\n"
                            );
          
              for(size_t p=0;p<srvParam->cfg->WCS[0]->WCSFormat.size();p++){
                XMLDoc->printconcat("      <formats>%s</formats>\n",srvParam->cfg->WCS[0]->WCSFormat[p]->attr.name.c_str());
              }
              XMLDoc->concat("    </supportedFormats>\n");
              XMLDoc->printconcat("    <supportedInterpolations default=\"nearest neighbor\">\n"
                  "      <interpolationMethod>nearest neighbor</interpolationMethod>\n"
              //     "      <interpolationMethod>bilinear</interpolationMethod>\n"
                  "    </supportedInterpolations>\n");
              XMLDoc->printconcat("</CoverageOffering>\n");
            }
          }
        }
      }
    }
  }
  XMLDoc->concat("</CoverageDescription>\n");
    
  return 0;
}

int CXMLGen::OGCGetCapabilities(CServerParams *_srvParam,CT::string *XMLDocument){
  this->srvParam=_srvParam;
  
  
  int status=0;
  std::vector<WMSLayer*> myWMSLayerList;
  
  for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
    CT::string layerUniqueName;
    srvParam->makeUniqueLayerName(&layerUniqueName,srvParam->cfg->Layer[j]);
    CT::string layerGroup;
    if(srvParam->cfg->Layer[j]->Group.size()>0){
      if(srvParam->cfg->Layer[j]->Group[0]->attr.value.c_str()!=NULL){
        layerGroup.copy(srvParam->cfg->Layer[j]->Group[0]->attr.value.c_str());
      }
    }
    //Create a new layer and push it in the list
    WMSLayer *myWMSLayer = new WMSLayer();myWMSLayerList.push_back(myWMSLayer);
    
    myWMSLayer->name.copy(&layerUniqueName);
    myWMSLayer->title.copy(srvParam->cfg->Layer[j]->Title[0]->value.c_str());
    myWMSLayer->group.copy(&layerGroup);
    //Set the configuration layer for this layer, as easy reference
    myWMSLayer->layer=srvParam->cfg->Layer[j];
    
    //Check if this layer is querable
    int datasetRestriction = checkDataRestriction();
    if((datasetRestriction&ALLOW_GFI)){
      myWMSLayer->isQuerable=1;
    }

    //Get a default file name for this layer to obtain some information
    status = getFileNameForLayer(myWMSLayer);if(status != 0)myWMSLayer->hasError=1;
    CDBDebug("b");
    //Try to open the file, and make a datasource for the layer
    status = getDataSourceForLayer(myWMSLayer);if(status != 0)myWMSLayer->hasError=1;
    CDBDebug("b");
    //Generate a common projection list information
    status = getProjectionInformationForLayer(myWMSLayer);if(status != 0)myWMSLayer->hasError=1;
    CDBDebug("b");
    //Get the dimensions and its extents for this layer
    status = getDimsForLayer(myWMSLayer);if(status != 0)myWMSLayer->hasError=1;
    
    //Get the defined styles for this layer
    status = getStylesForLayer(myWMSLayer);if(status != 0)myWMSLayer->hasError=1;

  }
  
  //Generate an XML document on basis of the information gathered above.
  CT::string XMLDoc;
  status = 0;
  if(srvParam->requestType==REQUEST_WMS_GETCAPABILITIES){
    if(srvParam->OGCVersion==WMS_VERSION_1_0_0){
      status = getWMS_1_0_0_Capabilities(&XMLDoc,&myWMSLayerList);
    }
    if(srvParam->OGCVersion==WMS_VERSION_1_1_1){
      status = getWMS_1_1_1_Capabilities(&XMLDoc,&myWMSLayerList);
    }
  }

  if(srvParam->requestType==REQUEST_WCS_GETCAPABILITIES){
    status = getWCS_1_0_0_Capabilities(&XMLDoc,&myWMSLayerList);
  }
  
  if(srvParam->requestType==REQUEST_WCS_DESCRIBECOVERAGE){
    status = getWCS_1_0_0_DescribeCoverage(&XMLDoc,&myWMSLayerList);
  }
  
  if(status != 0){
    CDBError("XML gen failed!");
    return 1;
  }
  XMLDocument->concat(&XMLDoc);
  
  for(size_t j=0;j<myWMSLayerList.size();j++){delete myWMSLayerList[j];myWMSLayerList[j]=NULL;}
  
  resetErrors();
  return 0;
}

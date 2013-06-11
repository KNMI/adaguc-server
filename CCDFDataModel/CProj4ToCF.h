/******************************************************************************
 * 
 * Project:  Proj4ToCF
 * Purpose:  Functions to convert proj4 strings to CF projection descriptions and vice versa
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


#ifndef CPROJ4TOCF_H
#define CPROJ4TOCF_H


#include <stdio.h>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <CTypes.h>
#include "CDebugger.h" 
#include "CCDFDataModel.h"

/*******************************/
/*  CF functions               */
/*******************************/

#define CPROJ4TOCF_UNSUPPORTED_PROJECTION 1 /*Projection is not supported*/

class CProj4ToCF{
  private:
    DEF_ERRORFUNCTION();
    class KVP{
      public:
        CT::string name;
        CT::string value; 
    };
    
    static float convertToM(float fValue){
      if(fValue<50000)fValue*=1000;
      return fValue;
    }
    /*static void convertToCenterOfEarth(CDF::Attribute *option){
    * if(option->type!=CDF_FLOAT)return;
    * float fValue;
    * option->getData(&fValue,1);
    * if(fValue<50000)fValue*=1000;
    * //fValue+=(6371229.0f);
    * //fValue=42000*1000.0f;
    * fValue/=2000;
    * option->setData(CDF_FLOAT,&fValue,1);
}*/
    
    
    /*void initVerticalPerspective(CDF::Variable *projectionVariable, std::vector <KVP*>){
      add("proj","grid_mapping_name",CDF_CHAR,"vertical_perspective");
      add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"90");
      add("lon_0","longitude_of_projection_origin",CDF_FLOAT,"0");
      add("h","perspective_point_height",CDF_FLOAT,"0",convertToM);
      add("h","height_above_earth",CDF_FLOAT,"0");
      
      add("x","false_easting",CDF_FLOAT,"0");
      add("y","false_northing",CDF_FLOAT,"0");
    }*/
    
    CT::string *getProj4Value(const char *proj4Key,std::vector <KVP*> projKVPList){
      for(size_t j=0;j<projKVPList.size();j++){
        if(projKVPList[j]->name.equals(proj4Key)==true){
          return &projKVPList[j]->value;
        }
      }
      throw(0);
    }
    
    float getProj4ValueF(const char *proj4Key,std::vector <KVP*> projKVPList,float defaultValue,float ((*conversionfunction)(float))){
      float value = defaultValue;
      try{value = getProj4Value(proj4Key,projKVPList)->toFloat();}catch(int e){value = defaultValue;}
      if(conversionfunction!=NULL){
        value=conversionfunction(value);
      }
      return value;
    }
    
    float getProj4ValueF(const char *proj4Key,std::vector <KVP*> projKVPList,float defaultValue){
      return getProj4ValueF(proj4Key,projKVPList,defaultValue,NULL);
    }
    

    
    void initMSGPerspective(CDF::Variable *projectionVariable, std::vector <KVP*> projKVPList){
      //+proj=geos +lon_0=0.000000 +lat_0=0 +h=35807.414063 +a=6378.169 +b=6356.5838
      projectionVariable->removeAttributes();
      float v = 0;
      projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","MSGnavigation"));
      v=getProj4ValueF("h1"     ,projKVPList,4.2163970098E7,convertToM);projectionVariable->addAttribute(new CDF::Attribute("height_from_earth_center"      ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("a"      ,projKVPList,6378140.0,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"               ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("b"      ,projKVPList,6356755.5,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"               ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("lat_0"  ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("lon_0"  ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("longitude_of_projection_origin",CDF_FLOAT,&v,1));
      v=getProj4ValueF("scale_x",projKVPList,35785.830098);             projectionVariable->addAttribute(new CDF::Attribute("scale_x"                       ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("scale_y",projKVPList,-35785.830098);            projectionVariable->addAttribute(new CDF::Attribute("scale_y"                       ,CDF_FLOAT,&v,1));
      
      
      
    /* add("proj","grid_mapping_name",CDF_CHAR,"MSGnavigation");
      add("lon_0","longitude_of_projection_origin",CDF_FLOAT,"0");
      add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"0");
      
      add("b","semi_major_axis",CDF_FLOAT,"6356755.5",convertToM);
      add("a","semi_minor_axis",CDF_FLOAT,"6378140.0",convertToM);
      
      add("h1","height_from_earth_center",CDF_FLOAT,"4.2163970098E7",convertToM);
      
      add("x","scale_x",CDF_FLOAT,"35785.830098");
      add("y","scale_y",CDF_FLOAT,"-35785.830098");*/
      /*add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",convertToM);
      * add("b","semi_major_axis",CDF_FLOAT,"6356.7523",convertToM);*/
      
      
      //add("bestaatniet","earth_radius",CDF_FLOAT,"6371229");
      /*add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",convertToM);
      * add("b","semi_major_axis",CDF_FLOAT,"6356.7523",convertToM);*/
      
      //Uit de HRIT files zijn de volgende getallen gehaald:
      /*
      * In de HRIT-files vind ik het volgende:
      *        
      * - linedirgridstep = 3.0004032
      * - columndirgridstep = 3.0004032
      * 
      * Verder zijn er ook de volgende metadata:
      * - typeofearthmodel = 1
      * - equatorialradius = 6378.169
      * - northpolarradius = 6356.5838
      * - southpolarradius = 6356.5838
      */
    }
    void initStereoGraphic(CDF::Variable *projectionVariable, std::vector <KVP*> projKVPList){
      projectionVariable->removeAttributes();
      float v = 0;
      projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","polar_stereographic"));
      v=getProj4ValueF("lat_0"  ,projKVPList,90);            projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin"         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("lon_0"  ,projKVPList,0);             projectionVariable->addAttribute(new CDF::Attribute("straight_vertical_longitude_from_pole" ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("lat_ts" ,projKVPList,0);             projectionVariable->addAttribute(new CDF::Attribute("standard_parallel"                     ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("x"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("y"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("a"      ,projKVPList,6378140.0,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("b"      ,projKVPList,6356755.5,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
      
      /*add("proj","grid_mapping_name",CDF_CHAR,"polar_stereographic");
      add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"90");
      add("lon_0","straight_vertical_longitude_from_pole",CDF_FLOAT,"0");
      add("lat_ts","standard_parallel",CDF_FLOAT,"0");
      add("x","false_easting",CDF_FLOAT,"0");
      add("y","false_northing",CDF_FLOAT,"0");
      add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",convertToM);
      add("b","semi_major_axis",CDF_FLOAT,"6356.7523",convertToM);*/
    }
    
    void initLCCPerspective(CDF::Variable *projectionVariable, std::vector <KVP*> projKVPList){
      //+proj=lcc +lat_0=46.8 +lat_1=45.89892 +lat_2=47.69601 +lon_0=2.337229 +k_0=1.00 +x_0=600000 +y_0=2200000");
      /*add("proj","grid_mapping_name",CDF_CHAR,"lambert_conformal_conic");
      add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"46.8");
      add("lon_0","longitude_of_central_meridian",CDF_FLOAT,"2.337229");
      add("lat_ts","standard_parallel",CDF_FLOAT,"45.89892f, 47.69601f");
      add("x","false_easting",CDF_FLOAT,"600000");
      add("y","false_northing",CDF_FLOAT,"2200000");
      add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",convertToM);
      add("b","semi_major_axis",CDF_FLOAT,"6356.7523",convertToM);*/
      projectionVariable->removeAttributes();
      float v = 0;
      projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","lambert_conformal_conic"));
      v=getProj4ValueF("lat_0"  ,projKVPList,46.8);            projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin"         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("lon_0"  ,projKVPList,2.337229);        projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian" ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("x_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("y_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("a"      ,projKVPList,6378140.0,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("b"      ,projKVPList,6356755.5,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
      
      
      //Standard parallels can have one or two values.
      int numStandardParallels = 0;
      float standard_parallels[2];
      try{
        CT::string *lat_1 = getProj4Value("lat_1", projKVPList);
        standard_parallels[0]=lat_1->toFloat();
        numStandardParallels++;
      }catch(int e){}
      
      try{
        CT::string *lat_2 = getProj4Value("lat_2", projKVPList);
        standard_parallels[1]=lat_2->toFloat();
        numStandardParallels++;
      }catch(int e){}
      if(numStandardParallels>0&&numStandardParallels<3){
        projectionVariable->addAttribute(new CDF::Attribute("standard_parallel" ,CDF_FLOAT,standard_parallels,numStandardParallels));
      }
    }
    
    void initRPPerspective(CDF::Variable *projectionVariable, std::vector <KVP*> projKVPList){
      //+proj=ob_tran +o_proj=longlat +lon_0=15 +o_lat_p=47 +o_lon_p=0 +a=6378.140 +b=6356.750 +x_0=0 +y_0=0 +no_defs
      //try{projectionVariable->getAttribute("grid_north_pole_latitude")->getDataAsString(&grid_north_pole_latitude);}catch(int e){};
      //try{projectionVariable->getAttribute("grid_north_pole_longitude")->getDataAsString(&grid_north_pole_longitude);}catch(int e){};
      //try{projectionVariable->getAttribute("north_pole_grid_longitude")->getDataAsString(&north_pole_grid_longitude);}catch(int e){};
      
      projectionVariable->removeAttributes();
      float v = 0;
      projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","rotated_latitude_longitude"));
      
      v=getProj4ValueF("lon_0"  ,projKVPList,0)-180.0f;        projectionVariable->addAttribute(new CDF::Attribute("grid_north_pole_longitude" ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("o_lat_p"  ,projKVPList,47);            projectionVariable->addAttribute(new CDF::Attribute("grid_north_pole_latitude"         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("o_lon_p"  ,projKVPList,0);            projectionVariable->addAttribute(new CDF::Attribute("north_pole_grid_longitude"         ,CDF_FLOAT,&v,1));
      
      v=getProj4ValueF("x_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("y_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("a"      ,projKVPList,6378140.0,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("b"      ,projKVPList,6356755.5,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
      
      
      //Standard parallels can have one or two values.
      int numStandardParallels = 0;
      float standard_parallels[2];
      try{
        CT::string *lat_1 = getProj4Value("lat_1", projKVPList);
        standard_parallels[0]=lat_1->toFloat();
        numStandardParallels++;
      }catch(int e){}
      
      try{
        CT::string *lat_2 = getProj4Value("lat_2", projKVPList);
        standard_parallels[1]=lat_2->toFloat();
        numStandardParallels++;
      }catch(int e){}
      if(numStandardParallels>0&&numStandardParallels<3){
        projectionVariable->addAttribute(new CDF::Attribute("standard_parallel" ,CDF_FLOAT,standard_parallels,numStandardParallels));
      }
    }
    
    void initObliqueStereographicPerspective(CDF::Variable *projectionVariable, std::vector <KVP*> projKVPList){
      //+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs
      //TODO ellps=bessel is not supported!
      projectionVariable->removeAttributes();
      float v = 0;
      projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","oblique_stereographic"));
      v=getProj4ValueF("lat_0"  ,projKVPList,0);        projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("lon_0"  ,projKVPList,0);            projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian"         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("k"  ,projKVPList,1);            projectionVariable->addAttribute(new CDF::Attribute("scale_factor_at_projection_origin"         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("x_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("y_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("a"      ,projKVPList,6377397.155,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
      v=getProj4ValueF("b"      ,projKVPList,6356079,convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
    }

    int convertBackAndFort(const char *projString,CDF::Variable *projectionVariable){
      CProj4ToCF proj4ToCF;
      proj4ToCF.debug=true;
      int status = proj4ToCF.convertProjToCF(projectionVariable,projString);
      if(status!=0){
        CDBError("Could not convert %s\n",projString);return 1;
      }

      CT::string dumpString = "";
      CDF::dump(projectionVariable,&dumpString);
      CDBDebug("\n%s",dumpString.c_str());
      
      CT::string projCTString;
      status = proj4ToCF.convertCFToProj(projectionVariable,&projCTString);
      if(status!=0){
        CDBError("Could not convert\n");return 1;
      }
      return 0;
    }
    
  public:
    
  
    CProj4ToCF(){
    }
    ~CProj4ToCF(){
    }
    
    /**
     * Set to true and additonal debug attributes will be written to the CDF Variable.
     */
    bool debug;
    
    
    /**
    * Converts a proj4 string to CF mappings
    * @param projectionVariable The variable which will contain attributes with projection information after the conversion has finished
    * @param proj4String The proj4 string which holds the string to convert to CF mapping
    * @return Zero on succes and nonzero on failure
    */
    int convertProjToCF( CDF::Variable *projectionVariable, const char *proj4String){
      //Create a list with key value pairs of projection options
    
      std::vector <KVP*> projKVPList;
      CT::string proj4CTString;
      proj4CTString.copy(proj4String);
      CT::string *projElements=proj4CTString.splitToArray(" +");
      
      if(projElements->count<2){delete[] projElements;return 1;}
      for(size_t j=0;j<projElements->count;j++){
        KVP *option = new KVP();
        CT::string *element=projElements[j].splitToArray("=");
        option->name.copy(&element[0]);
        option->value.copy(&element[1]);
        projKVPList.push_back(option);
        delete[] element;
      }
      delete[] projElements;
        
      CT::string cmpStr;
      int foundProj=0;
      for(size_t j=0;j<projKVPList.size();j++){
        if(projKVPList[j]->name.equals("proj")){
          if(projKVPList[j]->value.equals("stere")){initStereoGraphic(projectionVariable,projKVPList); foundProj=1;}
          if(projKVPList[j]->value.equals("geos")){ initMSGPerspective(projectionVariable,projKVPList);foundProj=1;}
          if(projKVPList[j]->value.equals("lcc")){  initLCCPerspective(projectionVariable,projKVPList);foundProj=1;}
          if(projKVPList[j]->value.equals("ob_tran")){  initRPPerspective(projectionVariable,projKVPList);foundProj=1;}
          if(projKVPList[j]->value.equals("sterea")){  initObliqueStereographicPerspective(projectionVariable,projKVPList);foundProj=1;}
        }
      }
      if(projectionVariable->name.c_str()==NULL)projectionVariable->name="projection";
  
      projectionVariable->setAttributeText("proj4_params",proj4String);
      CT::string kvpProjString = "";
      for(size_t j=0;j<projKVPList.size();j++){
      
        if(projKVPList[j]->name.equals("proj")==false){
          if(projKVPList[j]->value.c_str()!=NULL){
            kvpProjString.printconcat(" +%s=%f",projKVPList[j]->name.c_str(),projKVPList[j]->value.toDouble());
          }          
        }else{
          kvpProjString.printconcat("+%s=%s",projKVPList[j]->name.c_str(),projKVPList[j]->value.c_str());
        }
      }
   
      projectionVariable->setAttributeText("proj4_origin",kvpProjString.c_str());
    
      //CT::string reconstrProjString = "";
      //convertCFToProj(projectionVariable,&reconstrProjString);
      //projectionVariable->setAttributeText("proj4_recons",reconstrProjString.c_str());
      
      for(size_t j=0;j<projKVPList.size();j++){delete projKVPList[j];};projKVPList.clear();
      if(foundProj==0)return 1;
      
      return 0;
    }
    
    /**
    * Converts a CF projection variable to a proj4 stringtring to CF mappings
    * @param projectionVariable The variable with CF projection attributes to convert to the proj4 string
    * @param proj4String The proj4 string which will contain the new proj string after conversion is finished
    * @return Zero on succes and nonzero on failure
    */
    int convertCFToProj( CDF::Variable *projectionVariable,CT::string *proj4String){
      proj4String->copy("Unsupported projection");
      try{
        CT::string grid_mapping_name;
        projectionVariable->getAttribute("grid_mapping_name")->getDataAsString(&grid_mapping_name);
        grid_mapping_name.toLowerCaseSelf();

        if(grid_mapping_name.equals("msgnavigation")){
          // Meteosat Second Generation projection
          CT::string longitude_of_projection_origin = "0.000000f" ;
          CT::string latitude_of_projection_origin = "0.000000f" ;
          CT::string height_from_earth_center = "42163972.000000f" ;
          CT::string semi_major_axis = "6378140.000000f" ;
          CT::string semi_minor_axis = "6356750.000000f" ;

          try{projectionVariable->getAttribute("longitude_of_projection_origin")->getDataAsString(&longitude_of_projection_origin);}catch(int e){};
          try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
          try{projectionVariable->getAttribute("height_from_earth_center")->getDataAsString(&height_from_earth_center);}catch(int e){};
          try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
          try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
          
          proj4String->print("+proj=geos +lon_0=%f +lat_0=%f +h=%f +a=%f +b=%f",
                            longitude_of_projection_origin.toDouble(),
                            latitude_of_projection_origin.toDouble(),
                            35807.414063,
                            semi_major_axis.toDouble()/1000,
                            semi_minor_axis.toDouble()/1000
                            );
        }else if(grid_mapping_name.equals("polar_stereographic")){
          // Polar stereographic projection
          CT::string straight_vertical_longitude_from_pole = "0.000000f" ;
          CT::string latitude_of_projection_origin = "90.000000f" ;
          CT::string standard_parallel = "90.000000f" ;
          CT::string semi_major_axis = "6378140.000000f" ;
          CT::string semi_minor_axis = "6356750.000000f" ;
          CT::string false_easting = "0.000000f" ;
          CT::string false_northing = "0.000000f" ;
          try{projectionVariable->getAttribute("straight_vertical_longitude_from_pole")->getDataAsString(&straight_vertical_longitude_from_pole);}catch(int e){};
          try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
          try{projectionVariable->getAttribute("standard_parallel")->getDataAsString(&standard_parallel);}catch(int e){};
          try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
          try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
          try{projectionVariable->getAttribute("false_easting")->getDataAsString(&false_easting);}catch(int e){};
          try{projectionVariable->getAttribute("false_northing")->getDataAsString(&false_northing);}catch(int e){};
          
          proj4String->print("+proj=stere +lat_0=%f +lon_0=%f +lat_ts=%f +a=%f +b=%f +x_0=%f +y_0=%f",
                            latitude_of_projection_origin.toDouble(),
                            straight_vertical_longitude_from_pole.toDouble(),
                            standard_parallel.toDouble(),
                            semi_major_axis.toDouble()/1000.0f,
                            semi_minor_axis.toDouble()/1000.0f,
                            false_easting.toDouble(),
                            false_northing.toDouble());

        }else if(grid_mapping_name.equals("lambert_conformal_conic")){
          // Lambert conformal conic projection
          //+proj=lcc +lat_0=46.8 +lat_1=45.89892 +lat_2=47.69601 +lon_0=2.337229 +k_0=1.00 +x_0=600000 +y_0=2200000
          CT::string longitude_of_central_meridian = "0" ;
          CT::string latitude_of_projection_origin = "90" ;
          CT::string standard_parallel = "" ;
          CT::string semi_major_axis = "6378140.000000f" ;
          CT::string semi_minor_axis = "6356750.000000f" ;
          CT::string false_easting = "0" ;
          CT::string false_northing = "0";
          
          try{projectionVariable->getAttribute("longitude_of_central_meridian")->getDataAsString(&longitude_of_central_meridian);}catch(int e){};
          try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
          try{projectionVariable->getAttribute("standard_parallel")->getDataAsString(&standard_parallel);}catch(int e){};
          try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
          try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
          try{projectionVariable->getAttribute("false_easting")->getDataAsString(&false_easting);}catch(int e){};
          try{projectionVariable->getAttribute("false_northing")->getDataAsString(&false_northing);}catch(int e){};
          
          proj4String->print("+proj=lcc +lat_0=%f",
                            latitude_of_projection_origin.toDouble());
        
          
          CT::string *stpList=standard_parallel.splitToArray(" ");
          if(stpList->count==1){
            proj4String->printconcat(" +lat_1=%f",stpList[0].toDouble());
          }
          if(stpList->count==2){
            proj4String->printconcat(" +lat_1=%f +lat_2=%f",stpList[0].toDouble(),stpList[1].toDouble());
          }
          delete[] stpList;
          
          
          proj4String->printconcat(" +lon_0=%f +k_0=1.0 +x_0=%f +y_0=%f",
                            longitude_of_central_meridian.toDouble(), 
                            false_easting.toDouble(),
                            false_northing.toDouble());
          
        }else if(grid_mapping_name.equals("rotated_latitude_longitude")){
          //"+proj=ob_tran +o_proj=longlat +lon_0=15 +o_lat_p=47 +o_lon_p=0 +ellps=WGS84 +towgs84=0,0,0 +no_defs"
          CT::string grid_north_pole_longitude = "-165"; //--> should become 15 in proj4 (-165+180)
          CT::string grid_north_pole_latitude = "47";
          CT::string north_pole_grid_longitude = "0";
          CT::string semi_major_axis = "6378140.000000f" ;
          CT::string semi_minor_axis = "6356750.000000f" ;
          CT::string false_easting = "0" ;
          CT::string false_northing = "0";
          try{projectionVariable->getAttribute("grid_north_pole_latitude")->getDataAsString(&grid_north_pole_latitude);}catch(int e){};
          try{projectionVariable->getAttribute("grid_north_pole_longitude")->getDataAsString(&grid_north_pole_longitude);}catch(int e){};
          try{projectionVariable->getAttribute("north_pole_grid_longitude")->getDataAsString(&north_pole_grid_longitude);}catch(int e){};
          try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
          try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
          try{projectionVariable->getAttribute("false_easting")->getDataAsString(&false_easting);}catch(int e){};
          try{projectionVariable->getAttribute("false_northing")->getDataAsString(&false_northing);}catch(int e){};
          proj4String->print("+proj=ob_tran +o_proj=longlat +lon_0=%f +o_lat_p=%f +o_lon_p=%f +a=%f +b=%f +x_0=%f +y_0=%f +no_defs",
                             grid_north_pole_longitude.toDouble()+180.0f,
                             grid_north_pole_latitude.toDouble(),
                             north_pole_grid_longitude.toDouble(),
                             semi_major_axis.toDouble()/1000.0f,
                             semi_minor_axis.toDouble()/1000.0f,
                             false_easting.toDouble(),
                             false_northing.toDouble());
        }else if(grid_mapping_name.equals("oblique_stereographic")){
          
          //+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs
          //TODO atm we cannot detect +ellps=bessel with cf conventions
          CT::string latitude_of_projection_origin = "0";
          CT::string longitude_of_central_meridian = "0";
          CT::string scale_factor_at_projection_origin = "1";
          
          //Get latitude_of_projection_origin
          try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
          //Get central_meridian
          try{projectionVariable->getAttribute("longitude_of_central_meridian")->getDataAsString(&longitude_of_central_meridian);}catch(int e){};
          try{projectionVariable->getAttribute("central_meridian")->getDataAsString(&longitude_of_central_meridian);}catch(int e){};
          //Get scale_factor
          try{projectionVariable->getAttribute("scale_factor_at_projection_origin")->getDataAsString(&scale_factor_at_projection_origin);}catch(int e){};
          try{projectionVariable->getAttribute("scale_factor")->getDataAsString(&scale_factor_at_projection_origin);}catch(int e){};

          //Get common stuffs
          CT::string semi_major_axis = "6377397.155";//299.1528128,f=a/(a-b) b=(-(a/f))+a   (-(6377397.155/299.1528128))+6377397.155 = 6356079
          CT::string semi_minor_axis = "6356079";
          CT::string false_easting = "0" ;
          CT::string false_northing = "0";
          try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
          try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
          try{projectionVariable->getAttribute("false_easting")->getDataAsString(&false_easting);}catch(int e){};
          try{projectionVariable->getAttribute("false_northing")->getDataAsString(&false_northing);}catch(int e){};
          
          proj4String->print("+proj=sterea +lat_0=%f +lon_0=%f +k=%f +a=%f +b=%f +x_0=%f +y_0=%f +units=m +no_defs",
                             latitude_of_projection_origin.toDouble(),
                             longitude_of_central_meridian.toDouble(),
                             scale_factor_at_projection_origin.toDouble(),
                             semi_major_axis.toDouble(),
                             semi_minor_axis.toDouble(),
                             false_easting.toDouble(),
                             false_northing.toDouble());
        }
        else{
          CDBError("Projection '%s' not supported",grid_mapping_name.c_str());
          return CPROJ4TOCF_UNSUPPORTED_PROJECTION;
        }
      }catch(int e){
        //CDBError("%s\n",CDF::lastErrorMessage.c_str());
        return CPROJ4TOCF_UNSUPPORTED_PROJECTION;
      }
      
      
      return 0;
    }
    
    
  
    /**
    * Tests CProj4ToCF with several projections
    * @return Zero on success, nonzero on failure
    */
    int unitTest(){
      int status = 0;
      CT::string proj4_origin;
      CT::string proj4_recons;
      
      
      
      CDF::Variable *projectionVariable = new CDF::Variable ();;
      try{
        //MSG navigation
        if(convertBackAndFort("+proj=geos +lon_0=0.000000 +lat_0=0 +h=35807.414063 +a=6378.169 +b=6356.5838",projectionVariable)!=0){throw(__LINE__);}
        projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
        projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
        if(!proj4_origin.equals("+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=35807.414063 +a=6378.169000 +b=6356.583800")){throw(__LINE__);}
        if(!proj4_recons.equals("+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=35807.414063 +a=6378.169000 +b=6356.584000")){throw(__LINE__);}
        
        //Polar stereographic
        if(convertBackAndFort("+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0",projectionVariable)!=0){throw(__LINE__);}
        projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
        projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
        if(!proj4_origin.equals("+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000")){throw(__LINE__);}
        if(!proj4_recons.equals("+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000")){throw(__LINE__);}
        
        //Lambert conformal conic
        if(convertBackAndFort("+proj=lcc +lat_0=46.8 +lat_1=45.89892 +lat_2=47.69601 +lon_0=2.337229 +k_0=1.00 +x_0=600000 +y_0=2200000",projectionVariable)!=0){throw(__LINE__);}
        projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
        projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
        if(!proj4_origin.equals("+proj=lcc +lat_0=46.800000 +lat_1=45.898920 +lat_2=47.696010 +lon_0=2.337229 +k_0=1.000000 +x_0=600000.000000 +y_0=2200000.000000")){throw(__LINE__);}
        if(!proj4_recons.equals("+proj=lcc +lat_0=46.799999 +lat_1=45.898918 +lat_2=47.696011 +lon_0=2.337229 +k_0=1.0 +x_0=600000.000000 +y_0=2200000.000000")){throw(__LINE__);}
        
        //Rotated pole
        if(convertBackAndFort("+proj=ob_tran +o_proj=longlat +lon_0=15 +o_lat_p=47 +o_lon_p=0 +a=6378.140 +b=6356.750 +x_0=0 +y_0=0 +no_defs",projectionVariable)!=0){throw(__LINE__);}
        projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
        projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
        if(!proj4_origin.equals("+proj=ob_tran +o_proj=0.000000 +lon_0=15.000000 +o_lat_p=47.000000 +o_lon_p=0.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000")){throw(__LINE__);}
        if(!proj4_recons.equals("+proj=ob_tran +o_proj=longlat +lon_0=15.000000 +o_lat_p=47.000000 +o_lon_p=0.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000 +no_defs")){throw(__LINE__);}
        
        //oblique_stereographic
        if(convertBackAndFort("+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs",projectionVariable)!=0){throw(__LINE__);}
        projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
        projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
        if(!proj4_origin.equals("+proj=sterea +lat_0=52.156161 +lon_0=5.387639 +k=0.999908 +x_0=155000.000000 +y_0=463000.000000 +ellps=0.000000 +units=0.000000")){throw(__LINE__);}
        if(!proj4_recons.equals("+proj=sterea +lat_0=52.156162 +lon_0=5.387639 +k=0.999908 +a=6378140.000000 +b=6356755.500000 +x_0=155000.000000 +y_0=463000.000000 +units=m +no_defs" )){throw(__LINE__);}
        
        
      }catch(int e){
        CDBError("*** Unit test failed at line %d ***",e);
        status = 1;
      }
      delete projectionVariable;
      if(status == 0){
        printf("[OK] CProj4ToCF test succeeded!\n");
      }
      return status;
    }
    

};


#endif

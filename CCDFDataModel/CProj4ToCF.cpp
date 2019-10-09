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

#include "CProj4ToCF.h" 

#include "CReporter.h"

const char *CProj4ToCF::className="CProj4ToCF";

float CProj4ToCF::CProj4ToCF::convertToM(float fValue){
  if(fValue<50000)fValue*=1000;
  return fValue;
}

int CProj4ToCF::getProjectionUnits(const CDF::Variable *const projectionVariable) const {
  try{
    CDFObject *cdfObject = (CDFObject*)projectionVariable->getParentCDFObject();
    if(cdfObject != NULL){
      for(size_t j=0;j<cdfObject->variables.size();j++){
        if(cdfObject->variables[j]->isDimension){
          if(cdfObject->variables[j]->dimensionlinks.size() == 1){
            try{
              if(cdfObject->variables[j]->getAttribute("standard_name")->getDataAsString().equals("projection_x_coordinate")){
                CT::string units = cdfObject->variables[j]->getAttribute("units")->getDataAsString();
                units.toLowerCaseSelf();
                if(units.equals("km")){
                  return CPROJ4TOCF_UNITS_KILOMETER;
                }else if(units.equals("m")){
                  return CPROJ4TOCF_UNITS_METER;
                }else if(units.equals("rad")){
                  return CPROJ4TOCF_UNITS_RADIANS;
                }
              }
            }catch(int e){
            }
          }
        }
      }
    }
  }catch(int e){
  }
  return  CPROJ4TOCF_UNITS_METER;
}

CT::string CProj4ToCF::setProjectionUnits(const CDF::Variable *const projectionVariable) const {
    CT::string units = "m";
    int projectionUnits = getProjectionUnits(projectionVariable);
    if(projectionUnits == CPROJ4TOCF_UNITS_KILOMETER){
        units="km";
    } else {
        CREPORT_INFO_NODOC(CT::string("Projection unit is not km. Assuming 'm' as projection unit."), CReportMessage::Categories::GENERAL);
    }
    return units;
}

CT::string *CProj4ToCF::getProj4Value(const char *proj4Key,std::vector <CProj4ToCF::KVP*> projKVPList){
  for(size_t j=0;j<projKVPList.size();j++){
    if(projKVPList[j]->name.equals(proj4Key)==true){
      return &projKVPList[j]->value;
    }
  }
  throw(0);
}

float CProj4ToCF::getProj4ValueF(const char *proj4Key,std::vector <CProj4ToCF::KVP*> projKVPList,float defaultValue,float ((*conversionfunction)(float))){
  float value = defaultValue;
  try{value = getProj4Value(proj4Key,projKVPList)->toFloat();}catch(int e){value = defaultValue;}
  if(conversionfunction!=NULL){
    value=conversionfunction(value);
  }
  return value;
}

float CProj4ToCF::getProj4ValueF(const char *proj4Key,std::vector <CProj4ToCF::KVP*> projKVPList,float defaultValue){
  return getProj4ValueF(proj4Key,projKVPList,defaultValue,NULL);
}



void CProj4ToCF::initMSGPerspective(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  //+proj=geos +lon_0=0.000000 +lat_0=0 +h=35807.414063 +a=6378.169 +b=6356.5838
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","MSGnavigation"));
  v=getProj4ValueF("h1"     ,projKVPList,4.2163970098E7,CProj4ToCF::convertToM);projectionVariable->addAttribute(new CDF::Attribute("height_from_earth_center"      ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6378140.0,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"               ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356755.5,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"               ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lat_0"  ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("longitude_of_projection_origin",CDF_FLOAT,&v,1));
  v=getProj4ValueF("scale_x",projKVPList,35785.830098);             projectionVariable->addAttribute(new CDF::Attribute("scale_x"                       ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("scale_y",projKVPList,-35785.830098);            projectionVariable->addAttribute(new CDF::Attribute("scale_y"                       ,CDF_FLOAT,&v,1));
  
  
  
/* add("proj","grid_mapping_name",CDF_CHAR,"MSGnavigation");
  add("lon_0","longitude_of_projection_origin",CDF_FLOAT,"0");
  add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"0");
  
  add("b","semi_major_axis",CDF_FLOAT,"6356755.5",CProj4ToCF::convertToM);
  add("a","semi_minor_axis",CDF_FLOAT,"6378140.0",CProj4ToCF::convertToM);
  
  add("h1","height_from_earth_center",CDF_FLOAT,"4.2163970098E7",CProj4ToCF::convertToM);
  
  add("x","scale_x",CDF_FLOAT,"35785.830098");
  add("y","scale_y",CDF_FLOAT,"-35785.830098");*/
  /*add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",CProj4ToCF::convertToM);
  * add("b","semi_major_axis",CDF_FLOAT,"6356.7523",CProj4ToCF::convertToM);*/
  
  
  //add("bestaatniet","earth_radius",CDF_FLOAT,"6371229");
  /*add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",CProj4ToCF::convertToM);
  * add("b","semi_major_axis",CDF_FLOAT,"6356.7523",CProj4ToCF::convertToM);*/
  
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

void CProj4ToCF::initStereoGraphic(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","polar_stereographic"));
  v=getProj4ValueF("lat_0"  ,projKVPList,90);            projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin"         ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,0);             projectionVariable->addAttribute(new CDF::Attribute("straight_vertical_longitude_from_pole" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lat_ts" ,projKVPList,0);             projectionVariable->addAttribute(new CDF::Attribute("standard_parallel"                     ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("x"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("y"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
    
  int projectionUnits = getProjectionUnits(projectionVariable);
  double dfsemi_major_axis = 6378140.0;
  double dfsemi_minor_axis = 6356755.0;
  if(projectionUnits == CPROJ4TOCF_UNITS_KILOMETER){
    dfsemi_major_axis = dfsemi_major_axis/1000;
    dfsemi_minor_axis = dfsemi_minor_axis/1000;
  }
  v=getProj4ValueF("a"      ,projKVPList,dfsemi_major_axis);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,dfsemi_minor_axis);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
  
  

  /*add("proj","grid_mapping_name",CDF_CHAR,"polar_stereographic");
  add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"90");
  add("lon_0","straight_vertical_longitude_from_pole",CDF_FLOAT,"0");
  add("lat_ts","standard_parallel",CDF_FLOAT,"0");
  add("x","false_easting",CDF_FLOAT,"0");
  add("y","false_northing",CDF_FLOAT,"0");
  add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",CProj4ToCF::convertToM);
  add("b","semi_major_axis",CDF_FLOAT,"6356.7523",CProj4ToCF::convertToM);*/
}

void CProj4ToCF::initLCCPerspective(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  //+proj=lcc +lat_0=46.8 +lat_1=45.89892 +lat_2=47.69601 +lon_0=2.337229 +k_0=1.00 +x_0=600000 +y_0=2200000");
  /*add("proj","grid_mapping_name",CDF_CHAR,"lambert_conformal_conic");
  add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"46.8");
  add("lon_0","longitude_of_central_meridian",CDF_FLOAT,"2.337229");
  add("lat_ts","standard_parallel",CDF_FLOAT,"45.89892f, 47.69601f");
  add("x","false_easting",CDF_FLOAT,"600000");
  add("y","false_northing",CDF_FLOAT,"2200000");
  add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",CProj4ToCF::convertToM);
  add("b","semi_major_axis",CDF_FLOAT,"6356.7523",CProj4ToCF::convertToM);*/
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","lambert_conformal_conic"));
  v=getProj4ValueF("lat_0"  ,projKVPList,46.8);            projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin"         ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,2.337229);        projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("x_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("y_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6378140.0,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356755.5,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
  
  
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

void CProj4ToCF::initLAEAPerspective(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  //+proj=laea +lat_0=90 +lon_0=0 +x_0=0 +y_0=0");
  /*add("proj","grid_mapping_name",CDF_CHAR,"lambert_azimuthal_equal_area");
  add("lat_0","latitude_of_projection_origin",CDF_FLOAT,"90.0");
  add("lon_0","longitude_of_central_meridian",CDF_FLOAT,"0.0");
  add("x","false_easting",CDF_FLOAT,"0");
  add("y","false_northing",CDF_FLOAT,"0");
  add("a","semi_minor_axis",CDF_FLOAT,"6378.1370",CProj4ToCF::convertToM);
  add("b","semi_major_axis",CDF_FLOAT,"6356.7523",CProj4ToCF::convertToM);*/
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","lambert_azimuthal_equal_area"));
  v=getProj4ValueF("lat_0"  ,projKVPList,46.8);            projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin"         ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,2.337229);        projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("x_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_easting"                         ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("y_0"      ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("false_northing"                        ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6378140.0,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356755.5,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("pm"     ,projKVPList,0);               projectionVariable->addAttribute(new CDF::Attribute("longitude_of_prime_meridian", CDF_FLOAT, &v, 1));
}

void CProj4ToCF::initRPPerspective(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
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
  v=getProj4ValueF("a"      ,projKVPList,6378140.0,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356755.5,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
  
  
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

void CProj4ToCF::initObliqueStereographicPerspective(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
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
  v=getProj4ValueF("a"      ,projKVPList,6377397.155,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"                       ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356079,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"                        ,CDF_FLOAT,&v,1));
}

void CProj4ToCF::initLatitudeLongitude(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  //+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs
  //TODO ellps=bessel is not supported!
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","latitude_longitude"));
  v=getProj4ValueF("lat_0"  ,projKVPList,0);                     projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,0);                     projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6377397.155,CProj4ToCF::convertToM);projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356079,CProj4ToCF::convertToM);    projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"     ,CDF_FLOAT,&v,1));
}

void CProj4ToCF::initMercator(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  CDBDebug("initMercator ");
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","mercator"));
  v=getProj4ValueF("lat_0"  ,projKVPList,0);                     projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,0);                     projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6377397.155,CProj4ToCF::convertToM);projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356079,CProj4ToCF::convertToM);    projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"     ,CDF_FLOAT,&v,1));
}

void CProj4ToCF::initTransverseMercator(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  CDBDebug("initTransverseMercator()");
  projectionVariable->removeAttributes();
  float v = 0;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","transverse_mercator"));
  v=getProj4ValueF("lat_0"  ,projKVPList,0);
  projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,0);
  projectionVariable->addAttribute(new CDF::Attribute("longitude_of_central_meridian" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6377397.155,CProj4ToCF::convertToM);
  projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356079,CProj4ToCF::convertToM);
  projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"     ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("x_0"      ,projKVPList,0,CProj4ToCF::convertToM);
  projectionVariable->addAttribute(new CDF::Attribute("false_easting"     ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("y_0"      ,projKVPList,0,CProj4ToCF::convertToM);
  projectionVariable->addAttribute(new CDF::Attribute("false_northing"     ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("k_0"      ,projKVPList,1.0,CProj4ToCF::convertToM);
  projectionVariable->addAttribute(new CDF::Attribute("scale_factor_at_projection_origin"     ,CDF_FLOAT,&v,1));
}


void CProj4ToCF::initGeosPerspective(CDF::Variable *projectionVariable, std::vector <CProj4ToCF::KVP*> projKVPList){
  //+proj=geos +lon_0=0.000000 +lat_0=0 +h=35807.414063 +a=6378.169 +b=6356.5838
  projectionVariable->removeAttributes();
  float v = 0;
  CT::string s;
  projectionVariable->addAttribute(new CDF::Attribute("grid_mapping_name","geostationary"));
  v=getProj4ValueF("h"     ,projKVPList,4.2163970098E7,CProj4ToCF::convertToM);projectionVariable->addAttribute(new CDF::Attribute("perspective_point_height"      ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("a"      ,projKVPList,6378137.0,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_major_axis"               ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("b"      ,projKVPList,6356752.31414,CProj4ToCF::convertToM);     projectionVariable->addAttribute(new CDF::Attribute("semi_minor_axis"               ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lat_0"  ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("latitude_of_projection_origin" ,CDF_FLOAT,&v,1));
  v=getProj4ValueF("lon_0"  ,projKVPList,0);                        projectionVariable->addAttribute(new CDF::Attribute("longitude_of_projection_origin",CDF_FLOAT,&v,1));
  s=getProj4Value("sweep",projKVPList);             projectionVariable->addAttribute(new CDF::Attribute("sweep_angle_axis", s.c_str()));
}
  
  
  
int CProj4ToCF::convertBackAndFort(const char *projString,CDF::Variable *projectionVariable){
  CProj4ToCF proj4ToCF;
  proj4ToCF.debug=true;
  int status = proj4ToCF.convertProjToCF(projectionVariable,projString);
  if(status!=0){
    CDBError("Could not convert %s\n",projString);return 1;
  }

  CT::string dumpString = "";
  CDF::_dump(projectionVariable, &dumpString, CCDFDATAMODEL_DUMP_STANDARD);
  CDBDebug("\n%s",dumpString.c_str());
  
  CT::string projCTString;
  status = proj4ToCF.convertCFToProj(projectionVariable,&projCTString);
  if(status!=0){
    CDBError("Could not convert\n");return 1;
  }
  return 0;
}




CProj4ToCF::CProj4ToCF(){
}
CProj4ToCF::~CProj4ToCF(){
}

int CProj4ToCF::convertProjToCF( CDF::Variable *projectionVariable, const char *proj4String){
  //Create a list with key value pairs of projection options

  std::vector <CProj4ToCF::KVP*> projKVPList;
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
  try{

    for(size_t j=0;j<projKVPList.size();j++){
      if(projKVPList[j]->name.equals("proj")){
        if(projKVPList[j]->value.equals("stere")){initStereoGraphic(projectionVariable,projKVPList); foundProj=1;}
        if(projKVPList[j]->value.equals("geos")){ 
           for (size_t j2=0; j2<projKVPList.size();j2++){
             if(projKVPList[j2]->name.equals("height_from_earth_center")){
               initMSGPerspective(projectionVariable,projKVPList);
               foundProj=1;
             }
           }
           if (foundProj==0) {
               initGeosPerspective(projectionVariable,projKVPList);
               foundProj=1;
           }
        }
        if(projKVPList[j]->value.equals("lcc")){  initLCCPerspective(projectionVariable,projKVPList);foundProj=1;}
        if(projKVPList[j]->value.equals("ob_tran")){  initRPPerspective(projectionVariable,projKVPList);foundProj=1;}
        if(projKVPList[j]->value.equals("sterea")){  initObliqueStereographicPerspective(projectionVariable,projKVPList);foundProj=1;}
        if(projKVPList[j]->value.equals("latitude_longitude")){  initLatitudeLongitude(projectionVariable,projKVPList);foundProj=1;}
        if(projKVPList[j]->value.equals("merc")){  initMercator(projectionVariable,projKVPList);foundProj=1;}
        if(projKVPList[j]->value.equals("tmerc")){  initTransverseMercator(projectionVariable,projKVPList);foundProj=1;}
        if(projKVPList[j]->value.equals("laea")){  initLAEAPerspective(projectionVariable,projKVPList);foundProj=1;}
      }
    }
    if(projectionVariable->name.empty())projectionVariable->name="projection";

    projectionVariable->setAttributeText("proj4_params",proj4String);
    CT::string kvpProjString = "";
    for(size_t j=0;j<projKVPList.size();j++){
    
      if(projKVPList[j]->name.equals("proj")==false&&projKVPList[j]->name.equals("units")==false){
        if(projKVPList[j]->value.empty()==false){
          kvpProjString.printconcat(" +%s=%f",projKVPList[j]->name.c_str(),projKVPList[j]->value.toDouble());
        }          
      }else{
        kvpProjString.printconcat(" +%s=%s",projKVPList[j]->name.c_str(),projKVPList[j]->value.c_str());
        kvpProjString.trimSelf();
      }
    }

    projectionVariable->setAttributeText("proj4_origin",kvpProjString.c_str());
  }catch(int e){
    CDBError("Exception %d occured",e);
  }
  //CT::string reconstrProjString = "";
  //convertCFToProj(projectionVariable,&reconstrProjString);
  //projectionVariable->setAttributeText("proj4_recons",reconstrProjString.c_str());
  
  for(size_t j=0;j<projKVPList.size();j++){delete projKVPList[j];};projKVPList.clear();
  if(foundProj==0)return 1;
  
  return 0;
}

int CProj4ToCF::convertCFToProj( CDF::Variable *projectionVariable,CT::string *proj4String){
    
  proj4String->copy("Unsupported projection");
  try{
    CT::string grid_mapping_name;
    projectionVariable->getAttribute("grid_mapping_name")->getDataAsString(&grid_mapping_name);
    CREPORT_INFO_NODOC(CT::string("Unable to obtain projection from proj4 string variable. Trying to use CF conventions to determine projection. grid_mapping_name variable: ")+grid_mapping_name,
                       CReportMessage::Categories::GENERAL);
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
    }else if(grid_mapping_name.equals("geostationary")){
      // Meteosat Second Generation projection
      CT::string longitude_of_projection_origin = "0.000000f" ;
      CT::string latitude_of_projection_origin = "0.000000f" ;
      CT::string perspective_point_height = "35807414.063f" ; //"35807.414063f" ;
      CT::string semi_major_axis = "6378140.000000f" ;
      CT::string semi_minor_axis = "6356750.000000f" ;
      CT::string sweep_angle_axis = "x";

      try{projectionVariable->getAttribute("longitude_of_projection_origin")->getDataAsString(&longitude_of_projection_origin);}catch(int e){};
      try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
      try{projectionVariable->getAttribute("perspective_point_height")->getDataAsString(&perspective_point_height);}catch(int e){};
      try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
      try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
      try{projectionVariable->getAttribute("sweep_angle_axis")->getDataAsString(&sweep_angle_axis);}catch(int e){};
      
      proj4String->print("+proj=geos +lon_0=%f +lat_0=%f +h=%f +a=%f +b=%f +sweep=%s",
                        longitude_of_projection_origin.toDouble(),
                        latitude_of_projection_origin.toDouble(),
                        perspective_point_height.toDouble(),
                        semi_major_axis.toDouble(),
                        semi_minor_axis.toDouble(),
                        sweep_angle_axis.c_str()
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
      
      double  dfsemi_major_axis = semi_major_axis.toDouble();
      double  dfsemi_minor_axis = semi_minor_axis.toDouble();
      CT::string units = setProjectionUnits(projectionVariable);
      proj4String->print("+proj=stere +lat_0=%f +lon_0=%f +lat_ts=%f +a=%f +b=%f +x_0=%f +y_0=%f +units=%s +ellps=WGS84 +datum=WGS84",
                        latitude_of_projection_origin.toDouble(),
                        straight_vertical_longitude_from_pole.toDouble(),
                        standard_parallel.toDouble(),
                        dfsemi_major_axis,
                        dfsemi_minor_axis,
                        false_easting.toDouble(),
                        false_northing.toDouble(),
                        units.c_str()
                        );
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
      
      double  dfsemi_major_axis = semi_major_axis.toDouble();
      double  dfsemi_minor_axis = semi_minor_axis.toDouble();
      
      CT::string units = setProjectionUnits(projectionVariable);      
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
      
      proj4String->printconcat(" +lon_0=%f +k_0=1.0 +x_0=%f +y_0=%f +a=%f +b=%f +units=%s",
                        longitude_of_central_meridian.toDouble(), 
                        false_easting.toDouble(),
                        false_northing.toDouble(),
                        dfsemi_major_axis,
                        dfsemi_minor_axis,
                        units.c_str()
                              );
    }else if(grid_mapping_name.equals("lambert_azimuthal_equal_area")){
      // Lambert conformal conic projection
      //+proj=lcc +lat_0=0 +lon_0=0 +x_0=0 +y_0=0
      CT::string longitude_of_projection_origin = "0" ;
      CT::string latitude_of_projection_origin = "90" ;
      CT::string semi_major_axis = "6378140.000000f" ;
      CT::string semi_minor_axis = "-9999" ;
      CT::string inverse_flattening = "-9999" ;
      CT::string longitude_of_prime_meridian = "0";
      CT::string false_easting = "0" ;
      CT::string false_northing = "0";
      
      try{projectionVariable->getAttribute("longitude_of_projection_origin")->getDataAsString(&longitude_of_projection_origin);}catch(int e){};
      try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
      try{projectionVariable->getAttribute("longitude_of_prime_meridian")->getDataAsString(&longitude_of_prime_meridian);}catch(int e){};      
      try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
      try{
        projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);
      }catch(int e){
         try{
           projectionVariable->getAttribute("inverse_flattening")->getDataAsString(&inverse_flattening);
         } catch (int e) {}
      };
      try{projectionVariable->getAttribute("false_easting")->getDataAsString(&false_easting);}catch(int e){};
      try{projectionVariable->getAttribute("false_northing")->getDataAsString(&false_northing);}catch(int e){};
      
      double  dfsemi_major_axis = semi_major_axis.toDouble();
      double inv_flat = inverse_flattening.toDouble();
      double  dfsemi_minor_axis;
      if (inv_flat>0) {
        dfsemi_minor_axis=dfsemi_major_axis*(1-1/inv_flat);
      } else {
        dfsemi_minor_axis = semi_minor_axis.toDouble();
        if (dfsemi_minor_axis<=0) {
          dfsemi_minor_axis=dfsemi_major_axis;
        }
      }
      CT::string units = setProjectionUnits(projectionVariable);
      proj4String->print("+proj=laea +lat_0=%f +lon_0=%f",
                        latitude_of_projection_origin.toDouble(), longitude_of_projection_origin.toDouble());
      
      proj4String->printconcat(" +k_0=1.0 +x_0=%f +y_0=%f +a=%f +b=%f +pm=%f +units=%s",
                        false_easting.toDouble(),
                        false_northing.toDouble(),
                        dfsemi_major_axis,
                        dfsemi_minor_axis,
                        longitude_of_prime_meridian.toDouble(),
                        units.c_str()
                              );
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
    }else if(grid_mapping_name.equals("latitude_longitude")){
      CT::string latitude_of_projection_origin = "0";
      CT::string longitude_of_central_meridian = "0";
      
      //Get latitude_of_projection_origin
      try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
      //Get central_meridian
      try{projectionVariable->getAttribute("longitude_of_central_meridian")->getDataAsString(&longitude_of_central_meridian);}catch(int e){};
      try{projectionVariable->getAttribute("central_meridian")->getDataAsString(&longitude_of_central_meridian);}catch(int e){};

      //Get common stuffs
      CT::string semi_major_axis = "6377397.155";//299.1528128,f=a/(a-b) b=(-(a/f))+a   (-(6377397.155/299.1528128))+6377397.155 = 6356079
      CT::string semi_minor_axis = "6356079";
      try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
      try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);}catch(int e){};
      
      proj4String->print("+proj=longlat +ellps=WGS84 +datum=WGS84 +lat_0=%f +lon_0=%f +a=%f +b=%f +no_defs",
                          latitude_of_projection_origin.toDouble(),
                          longitude_of_central_meridian.toDouble(),
                          semi_major_axis.toDouble(),
                          semi_minor_axis.toDouble());
    }else if(grid_mapping_name.equals("mercator")){
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
      
      CT::string units = setProjectionUnits(projectionVariable);
      
      proj4String->print("+proj=merc +lat_0=%f",
                        latitude_of_projection_origin.toDouble());

      CT::string *stpList=standard_parallel.splitToArray(" ");
      if(stpList->count==1){
        proj4String->printconcat(" +lat_1=%f",stpList[0].toDouble());
      }
      if(stpList->count==2){
        proj4String->printconcat(" +lat_1=%f +lat_2=%f",stpList[0].toDouble(),stpList[1].toDouble());
      }
      delete[] stpList;
      
      proj4String->printconcat(" +lon_0=%f +k_0=1.0 +x_0=%f +y_0=%f +a=%f +units=%s",
                        longitude_of_central_meridian.toDouble(), 
                        false_easting.toDouble(),
                        false_northing.toDouble(),
                        semi_major_axis.toDouble(),
                        units.c_str()
                              );
    }else if(grid_mapping_name.equals("transverse_mercator")){
      // Transverse mercator projection
      //+proj=tmerc +lat_0=46.8 +lon_0=2.337229 +k_0=1.00 +x_0=600000 +y_0=2200000
      CT::string longitude_of_central_meridian = "0" ;
      CT::string latitude_of_projection_origin = "90" ;
      CT::string standard_parallel = "" ;
      CT::string semi_major_axis = "6378140.000000f" ;
      CT::string semi_minor_axis = "6356750.000000f" ;
      CT::string inverse_flattening = "297.183263207";
      CT::string false_easting = "0" ;
      CT::string false_northing = "0";
      CT::string scale_factor_at_projection_origin = "1.0";
      
      bool found;
      
      found=false;
      try{projectionVariable->getAttribute("longitude_of_central_meridian")->getDataAsString(&longitude_of_central_meridian);found=true;}catch(int e){};
      if (!found) {
        try{projectionVariable->getAttribute("longitude_of_projection_origin")->getDataAsString(&longitude_of_central_meridian);found=true;}catch(int e){};
      }
      try{projectionVariable->getAttribute("latitude_of_projection_origin")->getDataAsString(&latitude_of_projection_origin);}catch(int e){};
      
      found=false;
      try{projectionVariable->getAttribute("scale_factor_at_projection_origin")->getDataAsString(&scale_factor_at_projection_origin);found=true;}catch(int e){};
      if (!found) {
        try{projectionVariable->getAttribute("scale_factor_at_central_meridian")->getDataAsString(&scale_factor_at_projection_origin);found=true;}catch(int e){};
        if (!found) {
          CREPORT_ERROR_NODOC(CT::string("Projection: ")+grid_mapping_name+CT::string(" needs scale_factor_at_projection_origin or scale_factor_at_central_meridian"),
                              CReportMessage::Categories::GENERAL);
          return CPROJ4TOCF_UNSUPPORTED_PROJECTION;
        }
      }
      
      try{projectionVariable->getAttribute("semi_major_axis")->getDataAsString(&semi_major_axis);}catch(int e){};
      
      found=false;
      try{projectionVariable->getAttribute("semi_minor_axis")->getDataAsString(&semi_minor_axis);found=true;}catch(int e){};
      if (!found) {
        try{projectionVariable->getAttribute("inverse_flattening")->getDataAsString(&inverse_flattening);found=true;}catch(int e){};
        if (found) {
          float semi_minor_axis_value=semi_major_axis.toDouble()*(1-1/inverse_flattening.toDouble());
          semi_minor_axis.print("%f", semi_minor_axis_value);
        } else {
          CREPORT_ERROR_NODOC(CT::string("Projection: ")+grid_mapping_name+CT::string(" needs semi_minor_axis or inverse_flattening"),
                              CReportMessage::Categories::GENERAL);
          return CPROJ4TOCF_UNSUPPORTED_PROJECTION;
        }  
      }
      try{projectionVariable->getAttribute("false_easting")->getDataAsString(&false_easting);}catch(int e){};
      try{projectionVariable->getAttribute("false_northing")->getDataAsString(&false_northing);}catch(int e){};

      CT::string units = setProjectionUnits(projectionVariable);
      
      proj4String->print("+proj=tmerc +lat_0=%f", latitude_of_projection_origin.toDouble());
      proj4String->printconcat(" +lon_0=%f +k_0=%f +x_0=%f +y_0=%f +a=%f +b=%f +units=%s",
                               longitude_of_central_meridian.toDouble(), 
                               scale_factor_at_projection_origin.toDouble(),
                               false_easting.toDouble(),
                               false_northing.toDouble(),
                               semi_major_axis.toDouble(),
                               semi_minor_axis.toDouble(),
                               units.c_str()
                              );
     }else{
      CREPORT_INFO_NODOC(CT::string("Unsupported projection: ")+grid_mapping_name, CReportMessage::Categories::GENERAL);
      return CPROJ4TOCF_UNSUPPORTED_PROJECTION;
    }
    CREPORT_INFO_NODOC(CT::string("Determined the projection string using the CF conventions: ")+proj4String, CReportMessage::Categories::GENERAL);
  }catch(int e){
    //CDBError("%s\n",CDF::lastErrorMessage.c_str());
    try{
      CREPORT_INFO_NODOC(CT::string("Unsupported projection: ")+projectionVariable->getAttribute("grid_mapping_name")->toString(),
                       CReportMessage::Categories::GENERAL);
    } catch(int e) {
      CREPORT_INFO_NODOC(CT::string("Unsupported projection: ")+projectionVariable->name,
                       CReportMessage::Categories::GENERAL);
    }
    return CPROJ4TOCF_UNSUPPORTED_PROJECTION;
  }
  return 0;
}


int CProj4ToCF::__checkProjString(const char * name,const char *string){
  CDBDebug("Checking %s",name);
  CT::string proj4_origin;
  CT::string proj4_recons;
  CDF::Variable *projectionVariable = new CDF::Variable ();;
  try{
    if(convertBackAndFort(string,projectionVariable)!=0){
      CDBError("FAILED: %s: Proj string conversion for %s / [%s]",name,name,string);
      delete projectionVariable;
      return 1;
    }
    projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
    projectionVariable->getAttribute("proj4_params")->getDataAsString(&proj4_recons);
    if(!proj4_origin.equals(string)){
      CDBError("FAILED: %s Proj string parsing failed: \nIN:  [%s]\nOUT: [%s]",name,string,proj4_origin.c_str());
      delete projectionVariable;
      return 2;
    }
    if(!proj4_recons.equals(proj4_origin.c_str())){
      CDBError("FAILED: %s Proj string reconstruction failed: \nIN:  [%s]\nPRS: [%s]\nREC: [%s]",name,string,proj4_origin.c_str(),proj4_recons.c_str());
      delete projectionVariable;
      return 3;
    }
  }catch(int e){
    CDBError("FAILED: %s Exception %d",name,e);
    delete projectionVariable;
    return -1;
  }
  CDBDebug("[GOOD] %s is OK",name);
  delete projectionVariable;
  return 0;
}

/**
* Tests CProj4ToCF with several projections
* @return Zero on success, nonzero on failure
*/
int CProj4ToCF::unitTest(){
  int status = 0;

  
  
  
  
    bool error = false;
    if(__checkProjString("MSG Navigation","+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=35807.414063 +a=6378.169000 +b=6356.583800") != 0)error=true;
    if(__checkProjString("Polar stereographic in KM","+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000") != 0)error=true;
    if(__checkProjString("Polar stereographic in M","+proj=stere +lat_0=90.000000 +lon_0=263.000000 +lat_ts=60.000000 +a=6378140.000000 +b=6356750.000000 +x_0=3475000.000000 +y_0=7475000.000000") != 0)error=true;
    if(__checkProjString("Lambert conformal conic","+proj=lcc +lat_0=46.799999 +lat_1=45.898918 +lat_2=47.696011 +lon_0=2.337229 +k_0=1.000000 +x_0=600000.000000 +y_0=2200000.000000") != 0)error=true;
    if(__checkProjString("Rotated pole","+proj=ob_tran +o_proj=0.000000 +lon_0=15.000000 +o_lat_p=47.000000 +o_lon_p=0.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000") != 0)error=true;
    if(__checkProjString("Oblique stereographic","+proj=sterea +lat_0=52.156162 +lon_0=5.387639 +k=0.999908 +a=6378140.000000 +b=6356755.500000 +x_0=155000.000000 +y_0=463000.000000 +units=m") != 0)error=true;
    if(__checkProjString("Mercator","+proj=tmerc +lat_0=16.930000 +lon_0=67.180000 +units=km") != 0)error=true;
    
    


//         //MSG navigation
//         if(convertBackAndFort("+proj=geos +lon_0=0.000000 +lat_0=0 +h=35807.414063 +a=6378.169 +b=6356.5838",projectionVariable)!=0){throw(__LINE__);}
//         projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
//         projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
//         if(!proj4_origin.equals("+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=35807.414063 +a=6378.169000 +b=6356.583800")){throw(__LINE__);}
//         if(!proj4_recons.equals("+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=35807.414063 +a=6378.169000 +b=6356.584000")){throw(__LINE__);}
//         
//         //Polar stereographic in KM
//         if(convertBackAndFort("+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0",projectionVariable)!=0){throw(__LINE__);}
//         projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
//         projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
//         if(!proj4_origin.equals("+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000")){throw(__LINE__);}
//         if(!proj4_recons.equals("+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000")){throw(__LINE__);}
    
//         //Polar stereographic in M
//         if(convertBackAndFort("+proj=stere +lat_0=90.000000 +lon_0=263.000000 +lat_ts=60.000000 +a=6378140.000000 +b=6356750.000000 +x_0=3475000.000000 +y_0=7475000.000000",projectionVariable)!=0){throw(__LINE__);}
//         projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
//         projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
//         if(!proj4_origin.equals("+proj=stere +lat_0=90.000000 +lon_0=263.000000 +lat_ts=60.000000 +a=6378140.000000 +b=6356750.000000 +x_0=3475000.000000 +y_0=7475000.000000")){throw(__LINE__);}
//         if(!proj4_recons.equals("+proj=stere +lat_0=90.000000 +lon_0=263.000000 +lat_ts=60.000000 +a=6378140.000000 +b=6356750.000000 +x_0=3475000.000000 +y_0=7475000.000000")){throw(__LINE__);}
//         
//         //Lambert conformal conic
//         if(convertBackAndFort("+proj=lcc +lat_0=46.8 +lat_1=45.89892 +lat_2=47.69601 +lon_0=2.337229 +k_0=1.00 +x_0=600000 +y_0=2200000",projectionVariable)!=0){throw(__LINE__);}
//         projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
//         projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
//         if(!proj4_origin.equals("+proj=lcc +lat_0=46.800000 +lat_1=45.898920 +lat_2=47.696010 +lon_0=2.337229 +k_0=1.000000 +x_0=600000.000000 +y_0=2200000.000000")){throw(__LINE__);}
//         if(!proj4_recons.equals("+proj=lcc +lat_0=46.799999 +lat_1=45.898918 +lat_2=47.696011 +lon_0=2.337229 +k_0=1.0 +x_0=600000.000000 +y_0=2200000.000000")){throw(__LINE__);}
//         
//         //Rotated pole
//         if(convertBackAndFort("+proj=ob_tran +o_proj=longlat +lon_0=15 +o_lat_p=47 +o_lon_p=0 +a=6378.140 +b=6356.750 +x_0=0 +y_0=0 +no_defs",projectionVariable)!=0){throw(__LINE__);}
//         projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
//         projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
//         if(!proj4_origin.equals("+proj=ob_tran +o_proj=0.000000 +lon_0=15.000000 +o_lat_p=47.000000 +o_lon_p=0.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000")){throw(__LINE__);}
//         if(!proj4_recons.equals("+proj=ob_tran +o_proj=longlat +lon_0=15.000000 +o_lat_p=47.000000 +o_lon_p=0.000000 +a=6378.140000 +b=6356.750000 +x_0=0.000000 +y_0=0.000000 +no_defs")){throw(__LINE__);}
//         
//         //oblique_stereographic
//         if(convertBackAndFort("+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs",projectionVariable)!=0){throw(__LINE__);}
//         projectionVariable->getAttribute("proj4_origin")->getDataAsString(&proj4_origin);
//         projectionVariable->getAttribute("proj4_recons")->getDataAsString(&proj4_recons);
//         if(!proj4_origin.equals("+proj=sterea +lat_0=52.156161 +lon_0=5.387639 +k=0.999908 +x_0=155000.000000 +y_0=463000.000000 +ellps=0.000000 +units=0.000000")){throw(__LINE__);}
//         if(!proj4_recons.equals("+proj=sterea +lat_0=52.156162 +lon_0=5.387639 +k=0.999908 +a=6378140.000000 +b=6356755.500000 +x_0=155000.000000 +y_0=463000.000000 +units=m +no_defs" )){throw(__LINE__);}
//         
    
  if(error){
    CDBError("*** Unit test failed ***");
    status = 1;
  }

  if(status == 0){
    printf("[OK] CProj4ToCF test succeeded!\n");
  }
  return status;
}
  

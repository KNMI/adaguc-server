/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-05-06
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

#include "CDBFactory.h"
#include "CDBAdapter.h"

CDBAdapter::~CDBAdapter(){
}

const char *CDBFactory::className="CDBFactory";

CDBAdapter *CDBFactory::staticCDBAdapter = NULL;

CDBAdapter *CDBFactory::getDBAdapter(CServerConfig::XMLE_Configuration *cfg){
  if(staticCDBAdapter == NULL){
    //CDBDebug("CREATE");
    if(cfg->DataBase.size()!=1){
      CDBError("DataBase not properly configured");
      return NULL;
    }
    if(cfg->DataBase[0]->attr.dbtype.equals("sqlite")){
      CDBDebug("Using sqlite");
      #ifdef ADAGUC_USE_SQLITE
      staticCDBAdapter = new CDBAdapterSQLLite();
      #elseif
      CDBError("SQLITE is not compiled for ADAGUC, not available!");
      #endif
    }else{
      CDBDebug("Using postgresql");
      staticCDBAdapter = new CDBAdapterPostgreSQL();
    }
    
    staticCDBAdapter->setConfig(cfg);
  }
  return staticCDBAdapter;
}

void CDBFactory::clear(){
  //CDBDebug("CLEAR");
  delete staticCDBAdapter;
  staticCDBAdapter = NULL;
}
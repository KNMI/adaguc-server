/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  MongoDB driver ADAGUC Server
 * Author:   Rob Tjalma, tjalma "at" knmi.nl
 * Date:     2015-09-18
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
#ifdef ADAGUC_USE_KDCMONGODB
#include "CDBAdapter.h"
#include "CDebugger.h"
#include "CPGSQLDB.h"

/* 
 * The parameter names are exactly the same as for PostgreSQL and SQLite.
 * This must remain, in order to make it work.
 */
class CDBAdapterMongoDB:public CDBAdapter{
  private:
    DEF_ERRORFUNCTION();
    std::map <std::string ,std::string> lookupTableNameCacheMap;
    std::map <std::string ,std::vector<std::string> > fileListPerTable;
    int createDimTableOfType(const char *dimname,const char *tablename,int type);

  public:
    /* Constructor. */
    CDBAdapterMongoDB();
    /* Destructor. */
    ~CDBAdapterMongoDB();
    /* Setting the config file. */
    int setConfig(CServerConfig::XMLE_Configuration *cfg);
    
    /*
     * Because mongoDB is completely different then RDBMS, we need to
     * correct the layername. So 'time' becomes 'adaguc.dimension.time.time' for example. 
     * @param 	const char*		the column name that must be returned.
     * 
     * @return	const char*		the corrected column name. 
     */
    const char* 	 getCorrectedColumnName(const char* column_name);
    
    /*
     * No desciption yet!
     */
    CDBStore::Store *getReferenceTime(const char *netcdfDimName,const char *netcdfTimeDimName,const char *timeValue,const char *timeTableName,const char *tableName);
    
    /*
     * No desciption yet!
     */
    CDBStore::Store *getClosestDataTimeToSystemTime(const char *netcdfDimName,const char *tableName);

    /*
     *  Getting the granule name so ADAGUC knows where the information is stored. 
     *  In Postgres it was a tablename like t201509...., in MongoDB the filename is fine!
     *  @param		const char*		The full path of the dataset.
     *  @param		const char*		Not used parameter ( only applicable for SQL databases.
     *  @param 		const char* 	Not used parameter ( only applicable for SQL databases.
     *  @param		CDataSource		Not used parameter ( only applicable for SQL databases.
     * 
     *  @return 	CT::string 		The filename of the granule.
     */
    CT::string       getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource);
    
    /*
     * No desciption yet!
     */
    int              autoUpdateAndScanDimensionTables(CDataSource *dataSource);
    
    /*
     *  Getting the minimum value of the selected name.
     *  @param		const char*		The full path of the dataset.
     *  @param		const char*		The filename of the granule.
     * 
     *  @return 	CDBStore::Store A store containing the minimum value.
     */
    CDBStore::Store *getMin(const char *name,const char *table);
    
    /*
     *  Getting the maximum value of the selected name.
     *  @param		const char*		The full path of the dataset.
     *  @param		const char*		The filename of the granule.
     * 
     *  @return 	CDBStore::Store A store containing the maximum value.
     */
    CDBStore::Store *getMax(const char *name,const char *table);
    
    /*  
     * Return specific values ordered by the same value.
     * @param 		const char* 	The column that must be returned.
     * @param 		int				If applicable, limit the return size.
     * @param		bool			Return the values in ascending or descending order.
     * @param		const char*		The filename of the granule.
     * 
     * @return		CDBStore::Store	A store containing the values.
     */
    CDBStore::Store *getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table);
    
    /*  
     * Return specific values ordered by two columns.
     * Most of the time, the tim and dimtime field are being used.
     * @param 		const char* 	The column that must be returned.
     * @param 		int				If applicable, limit the return number.
     * @param		bool			Return the values in ascending or descending order.
     * @param		const char*		The filename of the granule.
     * 
     * @return		CDBStore::Store	A store containing the values.
     */
    CDBStore::Store *getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table);
    
    /*
     * Get the correct data, corresponding with the correct dimensions.
     * @param 		CDataSource		Data containing column names.
     * @param		int				If applicable, limit the return number.
     * 
     * @return		CDBStore::Store	A store containing the values.
     */
    CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource,int limit);
    
    /*
     * No desciption yet!
     */
    CDBStore::Store *getFilesForIndices(CDataSource *dataSource,size_t *start,size_t *count,ptrdiff_t *stride,int limit);
    
    /*
     *  Getting the dimension info about the selected file. 
     * 
     *  @param 		const char* 	Misleading param, but it is the fileName of the granule.
     *  @param 		const char* 	The layer name used in ADAGUC.
     * 
     *  @return 	CDBStore::Store All records specific for some dimension.
     */
    CDBStore::Store *getDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername);
    
    /*
     *  Setting the dimension info about the selected file. 
     * 
     *  @param 		const char* 	Misleading param, but it is the fileName of the granule.
     *  @param 		const char* 	The layer name used in ADAGUC.
     *  @param		const char*		Column name.
     *  @param		const char*		Column name.
     *  @param		const char*		Column name.
     * 
     *  @return 	int 			Status for succeeded or not.
     */
    int              storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units);
    
    int              dropTable(const char *tablename);
    int              createDimTableInt(const char *dimname,const char *tablename);
    int              createDimTableReal(const char *dimname,const char *tablename);
    int              createDimTableString(const char *dimname,const char *tablename);
    int              createDimTableTimeStamp(const char *dimname,const char *tablename);
    
    /*
     *  Checking if the correct data is available in the database.datagranules database. 
     * 
     *  @param 		const char* 	Filename of the granule.
     *  @param 		const char* 	Full path of the datagranule.
     * 
     *  @return 	int 			0 for available in the table, 1 for not available.
     */
    int              checkIfFileIsInTable(const char *tablename,const char *filename);
    
    int              removeFile(const char *tablename,const char *file);
    int              removeFilesWithChangedCreationDate(const char *tablename,const char *file,const char *creationDate);
    int              setFileInt(const char *tablename,const char *file,int dimvalue,int dimindex,const char*filedate);
    int              setFileReal(const char *tablename,const char *file,double dimvalue,int dimindex,const char*filedate);
    int              setFileString(const char *tablename,const char *file,const char * dimvalue,int dimindex,const char*filedate);
    int              setFileTimeStamp(const char *tablename,const char *file,const char *dimvalue,int dimindex,const char*filedate);
    int              addFilesToDataBase();
};
#endif

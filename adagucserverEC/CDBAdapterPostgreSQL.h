#include "CDBAdapter.h"
#include "CDebugger.h"
#include "CPGSQLDB.h"
class CDBAdapterPostgreSQL:public CDBAdapter{
  private:
    DEF_ERRORFUNCTION();
     CPGSQLDB *dataBaseConnection;
     CPGSQLDB *getDataBaseConnection(){
      if(dataBaseConnection == NULL){
        dataBaseConnection = new CPGSQLDB();
        int status = dataBaseConnection->connect(configurationObject->DataBase[0]->attr.parameters.c_str());if(status!=0){CDBError("Unable to connect to DB");return NULL;}
      }
      return dataBaseConnection;
    }
    CServerConfig::XMLE_Configuration *configurationObject;
    std::map <std::string ,std::string> lookupTableNameCacheMap;
    std::map <std::string ,std::vector<std::string> > fileListPerTable;
    int createDimTableOfType(const char *dimname,const char *tablename,int type);
  public:
    CDBAdapterPostgreSQL();
    ~CDBAdapterPostgreSQL();
    int setConfig(CServerConfig::XMLE_Configuration *cfg);
    
    int              generateGetReferenceTimesDoc(CT::string *result,CDataSource *dataSource);
    CDBStore::Store *getReferenceTime(const char *netcdfDimName,const char *netcdfTimeDimName,const char *timeValue,const char *timeTableName,const char *tableName);
    CDBStore::Store *getClosestReferenceTimeToSystemTime(const char *netcdfDimName,const char *tableName);

    CT::string       getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource);
    int              makeDimensionTables(CDataSource *dataSource);
    CDBStore::Store *getMin(const char *name,const char *table);
    CDBStore::Store *getMax(const char *name,const char *table);
    CDBStore::Store *getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table);
    CDBStore::Store *getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table);
    CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource);
    
    CDBStore::Store *getDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername);
    int              storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units);
    
    int              dropTable(const char *tablename);
    int              createDimTableInt(const char *dimname,const char *tablename);
    int              createDimTableReal(const char *dimname,const char *tablename);
    int              createDimTableString(const char *dimname,const char *tablename);
    int              createDimTableTimeStamp(const char *dimname,const char *tablename);
    int              checkIfFileIsInTable(const char *tablename,const char *filename);
    
    
    int              dropFilesWithDifferentCreationDate(const char *tablename,const char *file,const char *creationDate);
    int              setFileInt(const char *tablename,const char *file,int dimvalue,int dimindex,const char*filedate);
    int              setFileReal(const char *tablename,const char *file,double dimvalue,int dimindex,const char*filedate);
    int              setFileString(const char *tablename,const char *file,const char * dimvalue,int dimindex,const char*filedate);
    int              setFileTimeStamp(const char *tablename,const char *file,const char *dimvalue,int dimindex,const char*filedate);
    int              addFilesToDataBase();
      
};
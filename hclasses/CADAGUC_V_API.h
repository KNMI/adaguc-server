#ifndef CADAGUC_V_API_H
#define CADAGUC_V_API_H

#include <netcdf.h>
#include "CDefinitions.h"
#include "CADAGUCNCMLWriter.h"
#include "CADAGUC_time.h"
#define MAX_VARS 512
#define SOFTWARE_VERSION "ADAGUC NetCDF4 vector API V0.2"
#include "CTypes.h"
#include "CDebugger.h"

void ncerror(const char * msg,int e) ;
struct node_struct{
  int index;
  double val;
  node_struct *next; 
  node_struct *prev; 
};
class Variable_class{
  public:
    DEF_ERRORFUNCTION();
    Variable_class(){
      name=new char[NC_MAX_NAME+1];data=NULL;
    }
    ~Variable_class(){
      delete[] name;if(data!=NULL)free(data);
    }
    char *name; 
    nc_type type;
    size_t typeSize;
    int width;
    void *data;
};


class CADAGUC_V_API{
  private:
    DEF_ERRORFUNCTION();
    char szDebug[MAX_STR_LEN+1];
    int status;
    Variable_class * Vars;
    int Allocated;
    char time_unit[NC_MAX_NAME];
    int destncid;
    int shuffle,deflate,deflate_level,chunking_level[NC_MAX_DIMS];
    size_t nrOfValuesOut;
    int NumVars;
    size_t nRecords;
    char szFileName[MAX_STR_LEN+1];
  public:
    CADAGUC_V_API();
    ~CADAGUC_V_API();

    struct lon_bnds_struct{
      float lon[4];
    };

    struct lat_bnds_struct{
      float lat[4];
    };
    lon_bnds_struct *lon_bnds;
    lat_bnds_struct *lat_bnds;
    float  *lon;
    float  *lat;
    double *time;

    CADAGUCNCMLWriter *NCMLWriter;

    int   Initialize(const char *_pszFileName, size_t _nMaxRecords);
    void  SetTimeUnit(const char *_unit);
    void *AddVariable(const char *name,nc_type type);
    void *AddFieldVariable(const char *name,nc_type type,int width);
    void *GetVarPtr(int id);
    int   GetVarName(const int id, char *pszName);
    int   GetVarWidth(const int id);
    int   GetVarType(const int id, nc_type &type);
    int   GetVarTypeSize(const int id);
    int   GetVarClass(const int id, Variable_class **Variable);
    int   GetVarID(const char *name);
    int   GetNumVars(){return NumVars;}
    void  SetNrOfRecords(size_t _nr);
    size_t GetNumRecords(){return nrOfValuesOut;}
    int   Close();
};
#endif


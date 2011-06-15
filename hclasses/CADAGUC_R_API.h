#ifndef CADAGUC_R_API_H
#define CADAGUC_R_API_H

#include <netcdf.h>
#include <proj_api.h>
#include "CDefinitions.h"
#include "CADAGUCNCMLWriter.h"
#include "CADAGUC_time.h"

#define MAX_VARS 512
#define MAX_DIMS NC_MAX_DIMS
#define RAPI_SOFTWARE_VERSION "ADAGUC NetCDF4 raster API V0.1"
#include "CDebugger.h"

class CNC4DataWriter{
  private:
    DEF_ERRORFUNCTION();
    int  status;
    char szDebug[MAX_STR_LEN+1];
  public:
    void ncerror(const char * msg,int e);
    int  WriteNC4Data(int destncid, int var_id,const char * name, void *data,nc_type type);
    int  AllocateData(void **data,size_t len,nc_type type);
};

class Dimension_class {
  private:
    DEF_ERRORFUNCTION();
    CNC4DataWriter * NC4Writer;
    int      Written, status, destncid;
    char    *name; 
    nc_type  type;

  public:
    Dimension_class();
    ~Dimension_class();
    void  *data;
    size_t offset;
    int    Write(int destncid);
    int    nc_var_id, nc_dim_id, dLevels;
    int    Init(int _destncid,const char *_name, int _dLevels, nc_type _type);
    void   SetName(const char *_name);
    char  *GetName();
};

class RasterVar_class {
  private:
    DEF_ERRORFUNCTION();
    int Written;
    CNC4DataWriter * NC4Writer;
    char *name; 
  public:
    RasterVar_class();
    ~RasterVar_class();
    Dimension_class *dimclass_ids[MAX_DIMS]; 
    nc_type type;
    int   ndims,nc_var_id,status;
    int   Init(int destncid,const char *_name, Dimension_class** _dim_ids,int _ndims, nc_type _type);
    void  SetName(const char *_name);
    char *GetName();
};

class CADAGUC_R_API{
  private:
    DEF_ERRORFUNCTION();
    Dimension_class   *primedim_x;
    Dimension_class   *primedim_y;
    //Dimension_class   *timedim;
    Dimension_class   *Dims;
    RasterVar_class   *Vars;
    char  szFileName  [MAX_STR_LEN+1];
    char  time_unit   [NC_MAX_NAME+1];
    char  szProjection[NC_MAX_NAME+1];
    char  szDebug     [MAX_STR_LEN+1];
    int   NumVars, NumDims, dColumns, dRows;//, dNTimes;
    int   isProjected, DimsWritten, Allocated, status, destncid;
    double dfBBOX[4];
    size_t start[MAX_DIMS], count[MAX_DIMS];
    int   _WriteDIMS();
    void  ncerror(const char * msg,int e);
  public:
    CADAGUC_R_API();
    ~CADAGUC_R_API();
    CADAGUCNCMLWriter *NCMLWriter;
    CADAGUC_time      *ADAGUC_time;
    //double *time;
    int dLatLonFields;
    int   Initialize(const char * _pszFileName,int _dColumns,int _dRows,double *_dfBBOX);
    int   SetTimeUnit(const char *_unit);
    int   SetTime(int dLevel,int year,int month,int day,int hour,int minute, float second);
    int   SetTime(int dLevel,const char *pszISOTime);
    int   SetNetCDF3Mode();
    int   SetDeflate(int _shuffle,int _deflate,int _deflate_level);
    int   SetProjection(const char * _pszProjection);
    int   AddVariable(const char *name,int *pdDimIDs,int nDims, nc_type type);
    int   GetVarID(const char *name);
    int   AddDimension(const char *name,int dNLevels, nc_type type);
    int   GetDimID(const char *name);
    int   SetDim(int dDim_ID,int dLevel);  
    void *GetDimPtr(int dDim_ID);
    int   WriteLine(int dVar_ID,int dY,void *Data);
    int   Close();
};

#endif

#include "CADAGUC_R_API.h" 
static int   shuffle=0,deflate=0,deflate_level=0,isNetCDF3=0,enableChunkDeflate=0;
/*************************************************/
/*  CADAGUC_R_API class                          */
/*************************************************/
const char *CADAGUC_R_API::className="CADAGUC_R_API";
const char *Dimension_class::className="Dimension_class";
const char *CNC4DataWriter::className="CNC4DataWriter";

CADAGUC_R_API::CADAGUC_R_API(){
  primedim_x    = NULL;
  primedim_y    = NULL;
//  time          = NULL;
  Vars          = NULL;
  ADAGUC_time   = NULL;
  NumVars       = 0;
  NumDims       = 0;
  Allocated     = 0;
  time_unit[0]  = '\0';
  shuffle       = 1;
  deflate       = 1;
  deflate_level = 2;
  DimsWritten   = 0;
  isProjected   = 0;
  isNetCDF3     = 0;
  enableChunkDeflate = 1;
  dLatLonFields = 1;
  
}

int   CADAGUC_R_API::Initialize(const char * _pszFileName,int _dColumns,int _dRows,double *_dfBBOX){
  if(Allocated==1){
    CDBError("CADAGUC_R_API already Initialized");
    return 1;
  }
  CDBDebug("[Initialize]",1);
  strncpy(szFileName,_pszFileName,MAX_STR_LEN);szFileName[MAX_STR_LEN]='\0';
  dColumns= _dColumns;
  dRows   = _dRows;
//  dNTimes = _dNTimes;
  for(int j=0;j<4;j++)dfBBOX[j]=_dfBBOX[j];

  // Check type sizes 
  if(sizeof(char)  !=1){CDBError("The size of char is unequeal to 8 Bits");return 1;}
  if(sizeof(short) !=2){CDBError("The size of short is unequeal to 16 Bits");return 1;}
  if(sizeof(int)   !=4){CDBError("The size of int is unequeal to 32 Bits");return 1;}
  if(sizeof(float) !=4){CDBError("The size of float is unequeal to 32 Bits");return 1;}
  if(sizeof(double)!=8){CDBError("The size of double is unequeal to 64 Bits");return 1;}

  Allocated   = 1;
  NumVars     = 0; 
  NumDims     = 0;
  NCMLWriter  = new CADAGUCNCMLWriter;
  primedim_x  = new Dimension_class();
  primedim_y  = new Dimension_class();
//  timedim     = new Dimension_class();
  Dims        = new Dimension_class[MAX_DIMS];
  Vars        = new RasterVar_class[MAX_VARS];
  if(isNetCDF3 == 0)
    status = nc_create(szFileName , NC_NETCDF4|NC_CLOBBER, &destncid);
  if(isNetCDF3 == 1)
    status = nc_create(szFileName , NC_CLOBBER, &destncid);
  ncerror("nc_create",status);if(status != NC_NOERR)return 1;
  NCMLWriter->SetNetCDFId(destncid);
  return 0;
}

CADAGUC_R_API::~CADAGUC_R_API(){
  if(Allocated==0)return;
  delete NCMLWriter;
  delete primedim_x;
  delete primedim_y;
//  delete timedim     ;
  delete[] Dims     ;
  delete[] Vars     ;
  if(ADAGUC_time!=NULL)delete ADAGUC_time;
}

int CADAGUC_R_API::SetTimeUnit(const char *_unit){
  if(ADAGUC_time!=NULL){
    CDBError("SetTimeUnit: Time unit already defined");
    return 1;
  }
  strncpy(time_unit,_unit,NC_MAX_NAME);
  time_unit[NC_MAX_NAME]='\0';
  ADAGUC_time=new CADAGUC_time(time_unit);
  return 0;
}

int CADAGUC_R_API::SetNetCDF3Mode(){
  if(Allocated == 1){
    CDBError("CADAGUC_R_API already Initialized");
    return 1;
  }
  isNetCDF3 = 1;
  return 0;
}

int CADAGUC_R_API::SetDeflate(int _shuffle,int _deflate,int _deflate_level){
  if(Allocated==1){
    CDBError("CADAGUC_R_API already Initialized");
    return 1;
  }
  if(isNetCDF3==1){
    CDBError("SetDeflate not possible in NetCDF3 mode");
    return 1;
  }
  shuffle=_shuffle;
  deflate=_deflate;
  deflate_level=_deflate_level;
  enableChunkDeflate=1;
  return 0;
}

int CADAGUC_R_API::AddVariable(const char *name,int *pdDimIDs,int nDims, nc_type type){
  
  if(DimsWritten==0){
    _WriteDIMS();
  }
  int   id;

  Dimension_class *dim_ids[MAX_DIMS]; 
  id = NumVars;

  dim_ids[0]=primedim_x;
  dim_ids[1]=primedim_y;
//  dim_ids[2]=timedim;
  
  for(int j=0;j<nDims;j++){
    int id=pdDimIDs[j];
    if(id<0||id>NumDims){
      CDBError("AddVariable: invalid id");
      throw(1);
    }
    dim_ids[j+2]=&Dims[id];
  }

  if(Vars[id].Init(destncid,name, dim_ids, nDims+2, type)!=0)throw(1);
  NumVars++; 
  NCMLWriter->SetAttributeDirectly(name, "grid_mapping" , "projection", "String");
  NCMLWriter->SetAttributeDirectly(name, "units"        , "units"     , "String");
  NCMLWriter->SetAttributeDirectly(name, "long_name"    , name        , "String");
  /*char *pszFillValueType=(char*)"";
  if(type == NC_CHAR||type == NC_UBYTE||type == NC_BYTE)pszFillValueType="char";
  if(type == NC_SHORT||type == NC_USHORT) pszFillValueType=(char*)"short";
  if(type == NC_INT||type == NC_UINT)     pszFillValueType=(char*)"int";
  if(type == NC_FLOAT)                    pszFillValueType=(char*)"float";
  if(type == NC_DOUBLE)                   pszFillValueType=(char*)"double";
  if(pszFillValueType!=NULL)
  NCMLWriter->SetAttributeDirectly(name,"_FillValue" ,"1" ,pszFillValueType);*/
  if(isProjected==1&&dLatLonFields==1)
    NCMLWriter->SetMetaDataAttribute(name,"coordinates" ,"lon lat","String"); 
  return 0;
}

int CADAGUC_R_API::GetVarID(const char *name){
  int j,s=strlen(name);
  for(j=0;j<NumVars;j++){
    int v_sl=strlen(Vars[j].GetName());
    if(v_sl==s)if(strncmp(Vars[j].GetName(),name,s)==0)return j;
  }
  CDBError("GetVarID: invalid id");
  return -1;
}

int CADAGUC_R_API::AddDimension(const char *name,int dNLevels, nc_type type){
  return Dims[NumDims++].Init(destncid,name, dNLevels, type);
}

int CADAGUC_R_API::GetDimID(const char *name){
  int j,s=strlen(name);
  for(j=0;j<NumDims;j++){
    int v_sl=strlen(Dims[j].GetName());
    if(v_sl==s)if(strncmp(Dims[j].GetName(),name,s)==0)return j;
  }
  CDBError("GetDimID: invalid id");
  return -1;
}

int CADAGUC_R_API::SetDim(int dDim_ID,int dLevel){
  if(dDim_ID<0||dDim_ID>NumDims){
    CDBError("SetDim: invalid dim id");
    return 1;
  }
  if(dLevel<0||dLevel>Dims[dDim_ID].dLevels){
    CDBError("SetDim: requested level outside of specified dim size");
    return 1;
  }
  Dims[dDim_ID].offset=dLevel;
  return 0;
}

void *CADAGUC_R_API::GetDimPtr(int dDim_ID){
  if(dDim_ID<0||dDim_ID>NumDims){
    CDBError("GetDimPtr: invalid dim id");
    return NULL;
  }
  return Dims[dDim_ID].data;
}

int   CADAGUC_R_API::WriteLine(int dVar_ID,int dY,void *Data){

  if(dVar_ID<0||dVar_ID>NumVars){
    CDBError("WriteLine: invalid var id");
    return 1;
  }

  RasterVar_class * pcVar=&Vars[dVar_ID];
  // If north up
  //pcVar->dimclass_ids[1]->offset=((pcVar->dimclass_ids[1]->dLevels-1)-dY);
  // If south up
  pcVar->dimclass_ids[1]->offset=dY;

  //pcVar->dimclass_ids[2]->offset=dT;
  for(int j=0;j<pcVar->ndims;j++){
    start[j]=pcVar->dimclass_ids[pcVar->ndims-j-1]->offset;
    count[j]=1;
    count[pcVar->ndims-1]=dColumns;  
  }
  //for(int j=0;j<pcVar->ndims;j++){
    //printf("Dim %d: start[%d],count[%d]",j,start[j],count[j]);
  //}
  if(pcVar->type == NC_CHAR || pcVar->type == NC_UBYTE || pcVar->type == NC_BYTE){
    unsigned char *data = (unsigned char*)Data;
    status=nc_put_vara_ubyte(destncid,pcVar->nc_var_id,start,count,data);
    ncerror ("nc_put_vara_ubyte",status);if(status!=NC_NOERR)return 1;
    return 0;
  }
  if(pcVar->type == NC_SHORT || pcVar->type == NC_USHORT){
    short *data = (short*)Data;
    status=nc_put_vara_short(destncid,pcVar->nc_var_id,start,count,data);
    ncerror ("nc_put_vara_short",status);if(status!=NC_NOERR)return 1;
    return 0;
  }
  if(pcVar->type == NC_INT || pcVar->type == NC_UINT){
    int *data = (int*)Data;
    status=nc_put_vara_int(destncid,pcVar->nc_var_id,start,count,data);
    ncerror ("nc_put_vara_int",status);if(status!=NC_NOERR)return 1;
    return 0;
  }
  if(pcVar->type == NC_FLOAT){
    float *data = (float*)Data;
    status=nc_put_vara_float(destncid,pcVar->nc_var_id,start,count,data);
    ncerror ("nc_put_vara_float",status);if(status!=NC_NOERR)return 1;
    return 0;
  }
  if(pcVar->type == NC_DOUBLE){
    double *data = (double*)Data;
    status=nc_put_vara_double(destncid,pcVar->nc_var_id,start,count,data);
    ncerror ("nc_put_vara_double",status);if(status!=NC_NOERR)return 1;
    return 0;
  }
  CDBError("WriteLine: Variable type not found...");
  return 1;
}

int CADAGUC_R_API::_WriteDIMS(){
  CDBDebug("[WriteDIMS]",1);
  int j;
//  status = timedim->Write(destncid);
//  if(status != 0){CDBError("timedim->Write");return 1;}
  for(j=0;j<NumDims;j++){
    Dims[j].Write(destncid);
  }
  DimsWritten=1;
  CDBDebug("[/WriteDIMS]",1);
  return 0;
}

int CADAGUC_R_API::Close(){
  if(DimsWritten==0){
    _WriteDIMS();
  }

  int j;
  CDBDebug("Writing attributes...",2);
  char szProductVariables[MAX_STR_LEN+1];
  strncpy(szProductVariables,Vars[0].GetName(),MAX_STR_LEN);
  for(j=1;j<NumVars;j++){
      strncat(szProductVariables,",",1);
      strncat(szProductVariables,Vars[j].GetName(),MAX_STR_LEN);
  }
  NCMLWriter->SetMetaDataAttribute("product"    ,"software_version",RAPI_SOFTWARE_VERSION,"String");
  NCMLWriter->SetMetaDataAttribute("product"    ,"variables"       ,szProductVariables,"String");
  NCMLWriter->SetMetaDataAttribute("product"    ,"long_name"       ,"product"         ,"String");
  NCMLWriter->SetMetaDataAttribute("time"       ,"units"           ,time_unit         ,"String");
  NCMLWriter->SetMetaDataAttribute("time"       ,"long_name"       ,"time"            ,"String");
  NCMLWriter->SetMetaDataAttribute("time"       ,"standard_name"   ,"time"            ,"String");
  if(dLatLonFields == 1)
    NCMLWriter->SetMetaDataAttribute("NC_GLOBAL"  ,"Conventions"     ,"CF-1.4"          ,"String");
  else
    NCMLWriter->SetMetaDataAttribute("NC_GLOBAL"  ,"Conventions"     ,"CF-1.2"          ,"String");
  NCMLWriter->SetMetaDataAttribute("projection" ,"proj4_params"    ,szProjection      ,"String");
  NCMLWriter->SetMetaDataAttribute("projection" ,"long_name"       ,"projection"      ,"String");
  NCMLWriter->SetMetaDataAttribute("iso_dataset","long_name"       ,"iso_dataset"     ,"String");

  // Attach creation date
  CADAGUC_time * ADAGUC_time=new CADAGUC_time(time_unit);
  char szCreationDate[MAX_STR_LEN+1];
  ADAGUC_time->GetCurrentDate(szCreationDate,MAX_STR_LEN);
  NCMLWriter->SetMetaDataAttribute("product",    "creation_date"    ,szCreationDate   ,"String");
  if(ADAGUC_time!=NULL)delete ADAGUC_time;
  if(NCMLWriter->AttachNCML()!=0)return 1;
  nc_close(destncid);
  CDBDebug("Finished writing NetCDF4 file",2);
  CDBDebug("[/Close]",1);

  return 0;
}

int CADAGUC_R_API::SetTime(int dLevel,int year,int month,int day,int hour, int minute,float second){
  if(ADAGUC_time==NULL){
    CDBError("SetTime: time unit not set");
    return 1;
  }
/*  if(dLevel<0||dLevel>timedim->dLevels){
    CDBError("SetTime: Requested time level outside dimension size");
    return 1;
}*/
  double offset;
  status = ADAGUC_time->TimeToOffset(offset,year, month, day, hour, minute, second);
//  double *data = (double*)timedim->data;
//  data[dLevel]=offset;
  if(status != 0)return 1;
  return 0;
}

int CADAGUC_R_API::SetTime(int dLevel,const char *pszTime){
  if(ADAGUC_time==NULL){
    CDBError("SetTime: time unit not set");
    return 1;
  }
/*  if(dLevel<0||dLevel>timedim->dLevels){
    CDBError("SetTime: Requested time level outside dimension size");
    return 1;
  }
  double offset;
  status = ADAGUC_time->YYYYMMDDTHHMMSSTimeToOffset(offset,pszTime);
  time[dLevel]=offset;*/
  if(status != 0)return 1;
  return 0;
}


int CADAGUC_R_API::SetProjection(const char * _pszProjection){
  strncpy(szProjection,_pszProjection,MAX_STR_LEN);szProjection[MAX_STR_LEN]='\0';
  int status;
  status = primedim_x->Init(destncid,"primedim_x",dColumns,NC_DOUBLE);
  if(status!=0){
    CDBError("primedim_x->Init");
    return 1;
  }
  status = primedim_y->Init(destncid,"primedim_y",dRows   ,NC_DOUBLE);
  if(status!=0){
    CDBError("primedim_y->Init");
    return 1;
  }
  /*timedim   ->Init(destncid,"time"      ,dNTimes ,NC_DOUBLE);
  if(status!=0){
    CDBError("timedim->Init");
    return 1;
}
  time=(double*)timedim->data;*/
  double dfW,dfH,dfResX,dfResY;
  int dX,dY;
  
  dfW=dColumns;
  dfH=dRows;
  dfResX=((dfBBOX[2]-dfBBOX[0])/dfW);
  dfResY=((dfBBOX[3]-dfBBOX[1])/dfH);

  double *dfXDimData=(double*)primedim_x->data;
  double *dfYDimData=(double*)primedim_y->data;
  for(dX=0;dX<dfW;dX++)
    dfXDimData[dX]=dfBBOX[0]+dfResX/2.0f+dX*dfResX;
  for(dY=0;dY<dfH;dY++)
    dfYDimData[dY]=dfBBOX[1]+dfResY/2.0f+dY*dfResY;

  projUV p,pout;
  projPJ pj;

  if (!(pj = pj_init_plus(szProjection))){
    char szTemp[MAX_STR_LEN+1];
    snprintf(szTemp,MAX_STR_LEN,"SetProjection: Invalid projection: %s",szProjection);
    CDBError(szTemp);
    return 1;
  }

  // Check if we have a projected coordinate system
  isProjected=0;
  p.v=52; p.u=5;
  p.u *= DEG_TO_RAD;p.v *= DEG_TO_RAD;
  p = pj_fwd(p, pj);
  p.u /= DEG_TO_RAD;p.v /= DEG_TO_RAD;
  if(p.v+0.001<52||p.v-0.001>52||
     p.u+0.001<5||p.u-0.001>5)isProjected=1;

  if(isProjected==0){
    primedim_x->SetName("lon");  
    primedim_y->SetName("lat");
    status = primedim_x->Write(destncid);if(status != 0){CDBError("primedim_x->Write");return 1;}
    status = primedim_y->Write(destncid);if(status != 0){CDBError("primedim_y->Write");return 1;}
  }

  if(isProjected==1){
    primedim_x->SetName("x");  
    primedim_y->SetName("y");
    status = primedim_x->Write(destncid);if(status != 0){CDBError("primedim_x->Write");return 1;}
    status = primedim_y->Write(destncid);if(status != 0){CDBError("primedim_y->Write");return 1;}
    if(dLatLonFields==1){
      CDBDebug("Data is projected, creating longitude and latitude values...",2);
      size_t RasterSize=dColumns*dRows;
      float *londim=new float[RasterSize];
      float *latdim=new float[RasterSize];
      int dims[2],lonvar_id,latvar_id;

      for(dY=0;dY<dfH;dY++){
        for(dX=0;dX<dfW;dX++){
            p.u=dfXDimData[dX];
            p.v=dfYDimData[dY];
            pout.u = p.u;
            pout.v = p.v;
            pout = pj_inv(pout, pj);
            pout.u /= DEG_TO_RAD;
            pout.v /= DEG_TO_RAD;
            londim[dX+dY*dColumns]=pout.u;
            latdim[dX+dY*dColumns]=pout.v;
        }
      }

      dims[1]=primedim_x->nc_dim_id;
      dims[0]=primedim_y->nc_dim_id;
      status = nc_redef(destncid);
      status = nc_def_var(destncid, "lon"    , NC_FLOAT,2,dims,&lonvar_id);
      ncerror ("nc_def_var",status);if(status!=NC_NOERR)return 1;
      status = nc_def_var(destncid, "lat"    , NC_FLOAT,2,dims,&latvar_id);
      ncerror ("nc_def_var",status);if(status!=NC_NOERR)return 1;

      //deflate
      if(isNetCDF3==0&&enableChunkDeflate==1){
        /*status = nc_def_var_deflate(destncid,lonvar_id,shuffle ,deflate, deflate_level);
        ncerror ("nc_def_var_deflate",status);if(status!=NC_NOERR)return 1;
        status = nc_def_var_deflate(destncid,latvar_id,shuffle ,deflate, deflate_level);
        ncerror ("nc_def_var_deflate",status);if(status!=NC_NOERR)return 1;
*/
        size_t chunksizes[2];
        chunksizes[0]=dRows;
        chunksizes[1]=dColumns;

        status = nc_def_var_chunking(destncid,lonvar_id,0 ,chunksizes);
        ncerror ("nc_def_var_chunking",status);if(status!=NC_NOERR)return 1;
        status = nc_def_var_chunking(destncid,latvar_id,0 ,chunksizes);
        ncerror ("nc_def_var_chunking",status);if(status!=NC_NOERR)return 1;
      }
      if(isNetCDF3==1){
        status = nc_enddef (destncid);
        ncerror ("SetProjection: nc_enddef",status);if(status!=NC_NOERR)return 1;
      }
      ncerror ("SetProjection: nc_enddef",status);if(status!=NC_NOERR)return 1;
      CDBDebug("NC_FLOAT\tlon",2);
      status=  nc_put_var_float  (destncid, lonvar_id,londim); 
      ncerror ("nc_put_var_float",status);if(status!=NC_NOERR)return 1;
      CDBDebug("NC_FLOAT\tlat",2);
      status=  nc_put_var_float  (destncid, latvar_id,latdim); 
      ncerror ("nc_put_var_float",status);if(status!=NC_NOERR)return 1;
      delete londim;
      delete latdim;
    }
    NCMLWriter->SetMetaDataAttribute("x","long_name"    ,"x coordinate of projection"    ,"String");
    NCMLWriter->SetMetaDataAttribute("x","standard_name","projection_x_coordinate"    ,"String");
    NCMLWriter->SetMetaDataAttribute("x","units"        ,"m" ,"String");
    NCMLWriter->SetMetaDataAttribute("y","long_name"    ,"y coordinate of projection"    ,"String");
    NCMLWriter->SetMetaDataAttribute("y","standard_name","projection_y_coordinate"    ,"String");
    NCMLWriter->SetMetaDataAttribute("y","units"        ,"m" ,"String");

  }
  pj_free(pj);
  if((dLatLonFields==1&&isProjected==1)||isProjected==0){
    NCMLWriter->SetMetaDataAttribute("lon","long_name","longitude"     ,"String");
    NCMLWriter->SetMetaDataAttribute("lon","units"    ,"degrees_east"  ,"String");
    NCMLWriter->SetMetaDataAttribute("lat","long_name","latitude"      ,"String");
    NCMLWriter->SetMetaDataAttribute("lat","units"    ,"degrees_north" ,"String");
  }
  return 0;
}


void CADAGUC_R_API::ncerror(const char * msg,int e) 
{
  if(e==NC_NOERR)return;
  printf("nc_strerror: %s:\t%s\n", msg,nc_strerror(e));
}

/*************************************************/
/*  CNC4DataWriter class                         */
/*************************************************/

void CNC4DataWriter::ncerror(const char * msg,int e) 
{
  if(e==NC_NOERR)return;
  printf("nc_strerror: %s:\t%s\n", msg,nc_strerror(e));
}

int CNC4DataWriter::AllocateData(void **_data,size_t len,nc_type type){
  void *data=&_data;
  data=NULL;
  
  if(type==NC_CHAR||type==NC_BYTE||type==NC_UBYTE)data=(char*)malloc(sizeof(char)*len);
  if(type==NC_SHORT||type==NC_USHORT)data=(short*)malloc(sizeof(short)*len);
  if(type==NC_INT||type==NC_UINT)data=(int*)malloc(sizeof(int)*len);
  if(type==NC_FLOAT)data=(float*)malloc(sizeof(float)*len);
  if(type==NC_DOUBLE)data=(double*)malloc(sizeof(double)*len); 
  
  if(data==NULL){
    CDBError("Initialize: type not found");
    return 1;
  }
  *_data=data;
  return 0;
}

int CNC4DataWriter::WriteNC4Data(int destncid, int var_id,const char *name, void *_data,nc_type type){
  if(type == NC_CHAR||type == NC_UBYTE||type == NC_BYTE){
    snprintf(szDebug,MAX_STR_LEN,"NC_CHAR\t\t%s",name);CDBDebug(szDebug,2);
    unsigned char *data=(unsigned char*)_data;
    status=  nc_put_var_uchar  (destncid, var_id,data); 
    ncerror("nc_put_var_uchar",status); if(status != NC_NOERR)return 1;
    return 0;
  }
  if(type == NC_SHORT||type == NC_USHORT){
    snprintf(szDebug,MAX_STR_LEN,"NC_SHORT\t\t%s",name);CDBDebug(szDebug,2);
    short *data=(short*)_data;
    status=  nc_put_var_short  (destncid, var_id,data); 
    ncerror("nc_put_var_short",status); if(status != NC_NOERR)return 1;
    return 0;
  }
  if(type == NC_INT||type == NC_UINT){
    snprintf(szDebug,MAX_STR_LEN,"NC_INT\t\t%s",name);CDBDebug(szDebug,2);
    int *data=(int*)_data;
    status=  nc_put_var_int  (destncid, var_id,data); 
    ncerror("nc_put_var_int",status); if(status != NC_NOERR)return 1;
    return 0;
  }
  if(type == NC_FLOAT){
    snprintf(szDebug,MAX_STR_LEN,"NC_FLOAT\t%s",name);CDBDebug(szDebug,2);
    float *data=(float*)_data;
    status=  nc_put_var_float  (destncid, var_id,data); 
    ncerror("nc_put_var_float",status); if(status != NC_NOERR)return 1;
    return 0;
  }
  if(type == NC_DOUBLE){
    snprintf(szDebug,MAX_STR_LEN,"NC_DOUBLE\t%s",name);CDBDebug(szDebug,2);
    double *data=(double*)_data;
    status=  nc_put_var_double  (destncid, var_id,data); 
    ncerror("nc_put_var_double",status); if(status != NC_NOERR)return 1;
    return 0;
  }
  CDBError("WriteNC4Data: Invalid data type");
  return 1;
}

/*************************************************/
/*  Dimension_class class                        */
/*************************************************/

Dimension_class::Dimension_class(){
  name=new char[NC_MAX_NAME+1];
  data=NULL;
  Written=0;
  nc_dim_id=0;
  nc_var_id=0;
  dLevels=0;
  NC4Writer = new CNC4DataWriter();
}

Dimension_class::~Dimension_class(){
  delete[] name;
  if(data!=NULL)free(data);
  delete NC4Writer;
}

int Dimension_class::Init(int _destncid,const char *_name, int _dLevels, nc_type _type){
  if(Written==1)return 1;
  destncid=_destncid;
  strncpy(name,_name,NC_MAX_NAME);name[NC_MAX_NAME]='\0';
  type=_type;
  dLevels=_dLevels;
  offset=0;
  status = NC4Writer->AllocateData(&data,dLevels,type);
  return 0;
}

void Dimension_class::SetName(const char *_name){
  strncpy(name,_name,NC_MAX_NAME);name[NC_MAX_NAME]='\0';
}

char* Dimension_class::GetName(){
  return name;
}

int Dimension_class::Write(int destncid){
  if(Written==1)return 1;
  Written=1;
  if(status!=0)return 1;
  status = nc_redef(destncid);
  status = nc_def_dim(destncid, name    , dLevels, &nc_dim_id);
  NC4Writer->ncerror ("nc_def_dim",status);if(status!=NC_NOERR)return 1;
  status = nc_def_var(destncid, name    , NC_DOUBLE,1, &nc_dim_id,&nc_var_id);
  NC4Writer->ncerror ("nc_def_var",status);if(status!=NC_NOERR)return 1;
  
  //deflate
  if(isNetCDF3==0&&enableChunkDeflate==1){
    status = nc_def_var_deflate(destncid,nc_var_id,shuffle ,deflate, deflate_level);
    NC4Writer->ncerror ("nc_def_var_deflate",status);if(status!=NC_NOERR)return 1;
    size_t chunksizes[1];
    chunksizes[0]=dLevels;
    status = nc_def_var_chunking(destncid,nc_var_id,0 ,chunksizes);
    NC4Writer->ncerror ("nc_def_var_chunking",status);if(status!=NC_NOERR)return 1;
  }

  if(isNetCDF3==1){
    status = nc_enddef (destncid);
    NC4Writer->ncerror ("Write DIM: nc_enddef",status);if(status!=NC_NOERR)return 1;
  }

  status = NC4Writer->WriteNC4Data(destncid,nc_var_id,name,data,type);
  if(status!=0)return 1;
  return 0;
}


/*************************************************/
/*  RasterVar_class class                         */
/*************************************************/

RasterVar_class::RasterVar_class (){
  name=new char[NC_MAX_NAME+1];
  Written=0;
  ndims=0;
  nc_var_id=0;
  NC4Writer = new CNC4DataWriter();
}

RasterVar_class::~RasterVar_class (){
  delete[] name;
  delete NC4Writer;
}

int RasterVar_class::Init(int destncid,const char *_name, Dimension_class** _dim_ids,int _ndims, nc_type _type){
  if(Written==1)return 1;
  Written=1;
  strncpy(name,_name,NC_MAX_NAME);name[NC_MAX_NAME]='\0';
  type=_type;
  ndims=_ndims;
  for(int j=0;j<ndims;j++){
    dimclass_ids[j]=(Dimension_class*)_dim_ids[j];
  }

  int dim_id[MAX_DIMS];
  for(int j=0;j<ndims;j++){
    dim_id[j]=dimclass_ids[ndims-j-1]->nc_dim_id;
  }
  status = nc_redef(destncid);
  NC4Writer->ncerror ("nc_redef",status);if(status!=NC_NOERR)return 1;
  status = nc_def_var(destncid, name    , type,ndims, dim_id,&nc_var_id);
  NC4Writer->ncerror ("RasterVar_class::Init :nc_def_var",status);if(status!=NC_NOERR)return 1;

  //deflate
  if(isNetCDF3==0&&enableChunkDeflate==1){
    status = nc_def_var_deflate(destncid,nc_var_id,shuffle ,deflate, deflate_level);
    NC4Writer->ncerror ("nc_def_var_deflate",status);if(status!=NC_NOERR)return 1;
    //Define chunksizes
    size_t *pdchunksizes=new size_t[ndims];
    for(int j=0;j<ndims;j++)pdchunksizes[j]=1;
    pdchunksizes[ndims-1]=dimclass_ids[0]->dLevels;
    size_t yDimLevels=dimclass_ids[1]->dLevels;
    size_t yChunksize=1;
    for(int j=8;j<16;j++){
      yChunksize=yDimLevels/j;
     // printf("***** %d == %d\n",yChunksize,yDimLevels);
      if((yChunksize*j)==yDimLevels)break;else yChunksize=1;
    }
    yChunksize=1;
    pdchunksizes[ndims-2]=yChunksize;
    status = nc_def_var_chunking(destncid,nc_var_id,0 ,pdchunksizes);
    delete pdchunksizes;
    NC4Writer->ncerror ("nc_def_var_chunking",status);if(status!=NC_NOERR)return 1;
  }
  if(isNetCDF3==1){
    status = nc_enddef (destncid);
    NC4Writer->ncerror ("Init Variable: nc_enddef",status);if(status!=NC_NOERR)return 1;
  }
  return 0;
}

void RasterVar_class::SetName(const char *_name){
  strncpy(name,_name,NC_MAX_NAME);name[NC_MAX_NAME]='\0';
}

char *RasterVar_class::GetName(){
  return name;
}


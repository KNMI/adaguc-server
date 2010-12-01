#include "CADAGUC_V_API.h" 
const char *CADAGUC_V_API::className="CADAGUC_V_API";

void SortGetIndex(double* Variable,int nr,int *index){
  //printf("Sort\n");
  int j,nrnodes=0;
  double cmpval;
  node_struct *nodes=new node_struct[nr];
  node_struct *node,*firstnode;
  for(j=0;j<nr;j++){
    nodes[j].next=NULL;
    nodes[j].prev=NULL; 
  }
  nodes[0].next=NULL;//node[1];
  nodes[0].prev=NULL;
  nodes[0].val=Variable[0];
  nodes[0].index=0;
  firstnode=&nodes[0];
  nrnodes++;
  node=firstnode;
  int iterations=0;
  while(node!=NULL&&nrnodes<nr){
    iterations++;
    cmpval=Variable[nrnodes];
    //printf("%f - %d\n",cmpval,nrnodes);
    if(cmpval>node->val&&node->next==NULL){
      node_struct *thisnode=&nodes[nrnodes];
      thisnode->prev=node;
      thisnode->next=NULL;
      thisnode->index=nrnodes;
      thisnode->val=cmpval;
      node->next=thisnode;
      nrnodes++;
      node=thisnode;
    }
    else if(cmpval<node->val&&node->prev==NULL){
      firstnode=&nodes[nrnodes];
      node->prev=firstnode;
      firstnode->next=node;
      firstnode->prev=NULL;
      firstnode->val=cmpval;
      firstnode->index=nrnodes;
      nrnodes++;
      node=firstnode;
    }else if(cmpval>=node->val&&cmpval<=node->next->val){
      node_struct *thisnode=&nodes[nrnodes];
      thisnode->prev=node;
      node->next->prev=thisnode;
      thisnode->next=node->next;
      node->next=thisnode;
      thisnode->prev=node;
      thisnode->val=cmpval;
      thisnode->index=nrnodes;
      nrnodes++;
      node=thisnode;
    }else if(cmpval<node->val)node=node->prev;else node=node->next;
  }
  
  node=firstnode;
  //printf("%d iterations\n",iterations);
  nrnodes=0;
  while(node!=NULL&&nrnodes<nr){
    //printf("%d %d - %f\n",nrnodes,node->index,node->val);
    index[nrnodes]=node->index;
    node=node->next;
    nrnodes++;
  }
  
  delete[] node;
}

void SortByIndex(unsigned char* Variable,int nr,int *index){
  int j;
  unsigned char  *temp=new unsigned char[nr];
  for(j=0;j<nr;j++)temp[j]=Variable[index[j]];
  for(j=0;j<nr;j++)Variable[j]=temp[j];
  delete[] temp;

}


void SortByIndex(char* Variable,int nr,int *index){
  int j;
  char  *temp=new char[nr];
  for(j=0;j<nr;j++)temp[j]=Variable[index[j]];
  for(j=0;j<nr;j++)Variable[j]=temp[j];
  delete[] temp;

}

void SortByIndex(short* Variable,int nr,int *index){
  int j;
  short  *temp=new short[nr];
  for(j=0;j<nr;j++)temp[j]=Variable[index[j]];
  for(j=0;j<nr;j++)Variable[j]=temp[j];
  delete[] temp;

}
void SortByIndex(int* Variable,int nr,int *index){
  int j;
  int  *temp=new int[nr];
  for(j=0;j<nr;j++)temp[j]=Variable[index[j]];
  for(j=0;j<nr;j++)Variable[j]=temp[j];
  delete[] temp;

}
void SortByIndex(float * Variable,int nr,int *index){
  int j;
  float  *temp=new float[nr];
  for(j=0;j<nr;j++)temp[j]=Variable[index[j]];
  for(j=0;j<nr;j++)Variable[j]=temp[j];
  delete[] temp;
}
void SortByIndex(double* Variable,int nr,int *index){
  int j;
  double *temp=new double[nr];
  for(j=0;j<nr;j++)temp[j]=Variable[index[j]];
  for(j=0;j<nr;j++)Variable[j]=temp[j];
  delete[] temp;
}

CADAGUC_V_API::CADAGUC_V_API(){
 NCMLWriter=new CADAGUCNCMLWriter;
 lon_bnds    = NULL;
 lat_bnds    = NULL;
 lon         = NULL;
 lat         = NULL;
 time        = NULL;
 Vars        = NULL;
 NumVars     = 0;
 Allocated   = 0;
 time_unit[0]='\0';
 shuffle=1;
 deflate=1;
 deflate_level=2;
 
}

int CADAGUC_V_API::Initialize(const char *_pszFileName, size_t _nMaxRecords){
  //printf("*** VAPI INIT\n");
  if(Allocated==1){
    CDBError("CADAGUC_V_API already Initialized");
    return 1;
  }
  // Check type sizes 
  if(sizeof(char)!=1){CDBError("The size of char is unequeal to 8 Bits");return 1;}
  if(sizeof(short)!=2){CDBError("The size of short is unequeal to 16 Bits");return 1;}
  if(sizeof(int)!=4){CDBError("The size of int is unequeal to 32 Bits");return 1;}
  if(sizeof(float)!=4){CDBError("The size of float is unequeal to 32 Bits");return 1;}
  if(sizeof(double)!=8){CDBError("The size of double is unequeal to 64 Bits");return 1;}
  strncpy(szFileName,_pszFileName,MAX_STR_LEN);szFileName[MAX_STR_LEN]='\0';
  nRecords=_nMaxRecords;
  nrOfValuesOut = nRecords;
  Allocated=1;
  NumVars = 0; 
  lon_bnds    = new lon_bnds_struct[nRecords];
  lat_bnds    = new lat_bnds_struct[nRecords];
  lon         = new float[nRecords];
  lat         = new float[nRecords];
  time        = new double[nRecords];
  Vars        = new Variable_class[MAX_VARS];
  //nrOfValuesOut   =-1;
  return 0;
}

CADAGUC_V_API::~CADAGUC_V_API(){
  //printf("*** VAPI DESTROYED\n");
  if(Allocated==0)return;
  delete NCMLWriter;
  delete[] lon_bnds ;
  delete[] lat_bnds ;
  delete[] lon      ;
  delete[] lat      ;
  delete[] time     ;
  delete[] Vars     ;
}


void ncerror(const char * msg,int e) 
{
  if(e==NC_NOERR)return;
  printf("Error %s: %s\n", msg,nc_strerror(e));
}

void CADAGUC_V_API::SetTimeUnit(const char *_unit){
  strncpy(time_unit,_unit,NC_MAX_NAME);
  time_unit[NC_MAX_NAME]='\0';
}

void CADAGUC_V_API::SetNrOfRecords(size_t _nr){
  if(_nr>nRecords){
    CDBError("SetNrOfRecords: NrOfRecords>MaxRecords");
    return;
  }
  nrOfValuesOut=_nr;
}

void * CADAGUC_V_API::AddVariable(const char *name,nc_type type){
  int id;
  id=NumVars;
  Vars[id].type=type;
  Vars[id].width=1;
  snprintf(Vars[id].name,NC_MAX_NAME,"%s",name);
  if(Vars[id].data!=NULL){
    CDBError("AddVariable: Vars[id].data!=NULL");
    throw(1);
  }
  Vars[id].data=NULL;
  if(type==NC_CHAR||type==NC_BYTE||type==NC_UBYTE)Vars[id].data=(char*)malloc(sizeof(char)*nRecords);
  if(type==NC_SHORT||type==NC_USHORT)Vars[id].data=(short*)malloc(sizeof(short)*nRecords);
  if(type==NC_INT||type==NC_UINT)Vars[id].data=(int*)malloc(sizeof(int)*nRecords);
  if(type==NC_FLOAT)Vars[id].data=(float*)malloc(sizeof(float)*nRecords);
  if(type==NC_DOUBLE)Vars[id].data=(double*)malloc(sizeof(double)*nRecords);
  if(Vars[id].data==NULL){
    CDBError("AddVariable: type not found");
    throw(1);
  }
  NumVars++; 
  return Vars[id].data;
}

void * CADAGUC_V_API::AddFieldVariable(const char *name,nc_type type,int _width){
  int id;
  id=NumVars;
  size_t width=_width;
  Vars[id].type=type;
  Vars[id].width=width;
  if(width<=0)width=1;
  snprintf(Vars[id].name,NC_MAX_NAME,"%s",name);
  if(Vars[id].data!=NULL){
    CDBError("AddFieldVariable: Vars[id].data!=NULL");
    throw(1);
  }
  Vars[id].data=NULL;
  if(type==NC_CHAR||type==NC_BYTE||type==NC_UBYTE){
    Vars[id].typeSize=sizeof(char);
    Vars[id].data=(char*)malloc(sizeof(char)*nRecords*width);
  }
  if(type==NC_SHORT||type==NC_USHORT){
    Vars[id].typeSize=sizeof(short);
    Vars[id].data=(short*)malloc(sizeof(short)*nRecords*width);
  }
  if(type==NC_INT||type==NC_UINT){
    Vars[id].typeSize=sizeof(int);
    Vars[id].data=(int*)malloc(sizeof(int)*nRecords*width);
  }
  if(type==NC_FLOAT){
    Vars[id].typeSize=sizeof(float);
    Vars[id].data=(float*)malloc(sizeof(float)*nRecords*width);
  }
  if(type==NC_DOUBLE){
    Vars[id].typeSize=sizeof(double);
    Vars[id].data=(double*)malloc(sizeof(double)*nRecords*width);
  }
  if(Vars[id].data==NULL){
    CDBError("AddFieldVariable: type not found");
    throw(1);
  }
  NumVars++; 
  return Vars[id].data;
}

void *CADAGUC_V_API::GetVarPtr(int id){
  if(id<0||id>NumVars){
    CDBError("GetVarPtr: invalid id");
    return NULL;
  }
  return Vars[id].data;
}

int   CADAGUC_V_API::GetVarClass(const int id, Variable_class **Variable){
  if(id<0||id>NumVars){
    CDBError("GetVarName: invalid id");
    return 1;
  }
  *Variable = &Vars[id];
  return 0;
}

int CADAGUC_V_API::GetVarName(const int id, char *pszName){
  if(id<0||id>NumVars){
    CDBError("GetVarName: invalid id");
    return 1;
  }
  strncpy(pszName,Vars[id].name,MAX_STR_LEN);
  pszName[MAX_STR_LEN]='\0';
  return 0;
}

int CADAGUC_V_API::GetVarType(const int id, nc_type &type){
  if(id<0||id>NumVars){
    CDBError("GetVarType: invalid id");
    return 1;
  }
  type=Vars[id].type;
  return 0;
}
int CADAGUC_V_API::GetVarTypeSize(const int id){
  if(id<0||id>NumVars){
    CDBError("GetVarTypeSize: invalid id");
    return 0;
  }
  return Vars[id].typeSize;
}

int   CADAGUC_V_API::GetVarWidth(const int id){
  if(id<0||id>NumVars){
    CDBError("GetVarWidth: invalid id");
    return 0;
  }
  return Vars[id].width;
}


int CADAGUC_V_API::GetVarID(const char *name){
  int j,s=strlen(name);
  for(j=0;j<NumVars;j++){
    int v_sl=strlen(Vars[j].name);
    if(v_sl==s)if(strncmp(Vars[j].name,name,s)==0)return j;
  }
  char szTemp[MAX_STR_LEN+1];
  snprintf(szTemp,MAX_STR_LEN,"GetVarID: invalid variable name [%s]",name);
  szTemp[MAX_STR_LEN]='\0';
  CDBError(szTemp);
  return -1;
}

int CADAGUC_V_API::Close(){
  CDBDebug("[WriteNC4]",1);
  
  /*if(nrOfValuesOut==-1){
    //CDBError("Number of values to write not specified");
    //return 1;
    nrOfValuesOut=nRecords;
}*/
  if(nrOfValuesOut==0){
    CDBError("No Values in the data. Check your -start and -stop times");
    //return 1;
  }
  if(nrOfValuesOut>nRecords){
    CDBError("Internal error:nrOfValuesOut>nRecords");
    return 1;
  }
  if(time_unit[0]=='\0'){
    CDBError("time unit not specified");
    return 1;
  }

  int DIM_time,DIM_nv,DIM_time_bnds[2];
  int VAR_time,VAR_lat_bnds,VAR_lon_bnds,VAR_lat,VAR_lon,VAR_nv;
  char szTemp[MAX_STR_LEN+1];
  char szProductVariables[MAX_STR_LEN+1];
  szProductVariables[0]='\0';
  int nv[]={0,1,2,3};
  status = nc_create(szFileName , NC_NETCDF4|NC_CLOBBER, &destncid);ncerror("nc_create",status);
 // snprintf(szDebug,MAX_STR_LEN,
   //        "Records read: [%d]\tRecords within time range:[%d]",
     //      nRecords,nrOfValuesOut);
  //CDBDebug(szDebug,1);
  int *index=new int[nrOfValuesOut];
  for(size_t j=0;j<nrOfValuesOut;j++)index[j]=j;
  if(1==0)SortGetIndex(time,nrOfValuesOut,index);
  
  lon_bnds_struct *temp=new lon_bnds_struct[nrOfValuesOut];
  for(size_t j=0;j<nrOfValuesOut;j++)temp[j]=lon_bnds[index[j]];
  for(size_t j=0;j<nrOfValuesOut;j++)lon_bnds[j]=temp[j];
  delete[] temp;
  lat_bnds_struct *temp2=new lat_bnds_struct[nrOfValuesOut];
  for(size_t j=0;j<nrOfValuesOut;j++)temp2[j]=lat_bnds[index[j]];
  for(size_t j=0;j<nrOfValuesOut;j++)lat_bnds[j]=temp2[j];
  delete[] temp2;
  SortByIndex(time,nrOfValuesOut,index);
  SortByIndex(lon,nrOfValuesOut,index);
  SortByIndex(lat,nrOfValuesOut,index);
  
  if(status != NC_NOERR)return 1;

  status = nc_def_dim	      (destncid, "time"    , nrOfValuesOut, &DIM_time);
  ncerror("nc_def_dim",status); if(status != NC_NOERR)return 1;
  status = nc_def_dim	      (destncid, "nv"      , 4, &DIM_nv);
  DIM_time_bnds[0]=DIM_time;
  DIM_time_bnds[1]=DIM_nv;

  status = nc_def_var	      (destncid, "time"    , NC_DOUBLE,1, &DIM_time,&VAR_time);
  ncerror("nc_def_var",status); if(status != NC_NOERR)return 1;
  status = nc_def_var	      (destncid, "nv"      , NC_INT   ,1, &DIM_nv  ,&VAR_nv);
  status = nc_def_var	      (destncid, "lon_bnds", NC_FLOAT, 2, DIM_time_bnds,&VAR_lon_bnds);
  status = nc_def_var	      (destncid, "lat_bnds", NC_FLOAT, 2, DIM_time_bnds,&VAR_lat_bnds);
  status = nc_def_var	      (destncid, "lon"     , NC_FLOAT, 1, &DIM_time,&VAR_lon);
  status = nc_def_var	      (destncid, "lat"     , NC_FLOAT, 1, &DIM_time,&VAR_lat);
  
  //Chunking
/*  chunking_level[0]=1;
  chunking_level[0]=1;
  status = nc_def_var_chunking(destncid,VAR_time      ,0, chunking_level);
  ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_chunking(destncid,VAR_lon_bnds  ,0, chunking_level);
  ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_chunking(destncid,VAR_lat_bnds  ,0, chunking_level);
  ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_chunking(destncid,VAR_lon       ,0, chunking_level);
  ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_chunking(destncid,VAR_lat       ,0, chunking_level);
  ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;
  */
  //Deflate
  status = nc_def_var_deflate(destncid,VAR_time     ,shuffle ,deflate, deflate_level);
  ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_deflate(destncid,VAR_lon_bnds ,shuffle ,deflate, deflate_level);
  ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_deflate(destncid,VAR_lat_bnds ,shuffle ,deflate, deflate_level);
  ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_deflate(destncid,VAR_lon      ,shuffle ,deflate, deflate_level);
  ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;
  status = nc_def_var_deflate(destncid,VAR_lat      ,shuffle ,deflate, deflate_level);
  ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;

  status = nc_enddef(destncid);
  
  nc_put_att_text(destncid, VAR_time, "units", strlen(time_unit),time_unit);
  
  status=  nc_put_var_int  (destncid, VAR_nv,nv); 
  ncerror("nc_put_vara_int",status); if(status != NC_NOERR)return 1;
  

  status=  nc_put_var_double  (destncid, VAR_time,time); 
  ncerror("nc_put_vara_double", status);
  
  status=  nc_put_var_float  (destncid, VAR_lon_bnds,(float*)lon_bnds);
  ncerror("nc_put_vara_float", status);
  status=  nc_put_var_float  (destncid, VAR_lat_bnds,(float*)lat_bnds);
  ncerror("nc_put_vara_float", status);
  
  status=  nc_put_var_float  (destncid, VAR_lat,lat); 
  ncerror("nc_put_vara_float", status);
  
  status=  nc_put_var_float  (destncid, VAR_lon,lon); 
  ncerror("nc_put_vara_float", status);

  /*********************/
  //Put variables
  int Variable,j;
  CDBDebug("Write variables:",2);
  strncpy(szProductVariables,Vars[0].name,MAX_STR_LEN);
  for(j=0;j<NumVars;j++){
    if(j>0){
      strncat(szProductVariables,",",1);
      strncat(szProductVariables,Vars[j].name,MAX_STR_LEN);
    }
    status = nc_redef(destncid);
    if(Vars[j].width>1){
      int dims[2];
      int DIM_width,VAR_width,*VAR_values;
      snprintf(szTemp,MAX_STR_LEN,"%s_dim",Vars[j].name);
      status = nc_def_dim(destncid, szTemp,Vars[j].width,&DIM_width);
      ncerror("nc_def_dim variable_width dim",status); if(status != NC_NOERR)return 1;
      status = nc_def_var	      (destncid, szTemp, NC_INT   ,1, &DIM_width  ,&VAR_width);
      ncerror("nc_def_var variable_width dim",status); if(status != NC_NOERR)return 1;
      dims[0] = DIM_time;
      dims[1] = DIM_width;
      status = nc_def_var(destncid,Vars[j].name , Vars[j].type ,2, dims,&Variable);
      ncerror("nc_def_var",status); if(status != NC_NOERR)return 1;

      //Deflate
      status = nc_def_var_deflate(destncid,Variable ,shuffle ,deflate, deflate_level);
      ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;

      //Chunking
    //  chunking_level[0]=1;
    //  chunking_level[1]=DIM_width;
   //   status = nc_def_var_chunking(destncid,Variable ,0, chunking_level);
 //     ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;

      status = nc_enddef(destncid);
      VAR_values = new int[Vars[j].width];
      for(int k=0;k<Vars[j].width;k++)VAR_values[k]=k;
      status=  nc_put_var_int  (destncid, VAR_width,VAR_values); 
      delete VAR_values;
      ncerror("nc_put_vara_int",status); if(status != NC_NOERR)return 1;
    }else{
      status = nc_def_var(destncid,Vars[j].name , Vars[j].type ,1, &DIM_time,&Variable);
      ncerror("nc_def_var",status); if(status != NC_NOERR)return 1;
      //Chunking
      //status = nc_def_var_chunking(destncid,Variable ,0, chunking_level);
      //ncerror("nc_def_var_chunking",status); if(status != NC_NOERR)return 1;

      //Deflate
      status = nc_def_var_deflate(destncid,Variable ,shuffle ,deflate, deflate_level);
      ncerror("nc_def_var_deflate",status); if(status != NC_NOERR)return 1;
    }
    
    status = nc_enddef(destncid);
    if(Vars[j].type == NC_CHAR||Vars[j].type == NC_BYTE){
      snprintf(szDebug,MAX_STR_LEN,"NC_CHAR\t\t%s",Vars[j].name);CDBDebug(szDebug,2);
      signed char *data=(signed char*)Vars[j].data;
      SortByIndex((char*)data,nrOfValuesOut,index);
      status=  nc_put_var_schar  (destncid, Variable,data); 
      ncerror("nc_put_var_schar",status); if(status != NC_NOERR)return 1;
    }
    if(Vars[j].type == NC_UBYTE){
      snprintf(szDebug,MAX_STR_LEN,"NC_CHAR\t\t%s",Vars[j].name);CDBDebug(szDebug,2);
      unsigned char *data=(unsigned char*)Vars[j].data;
      SortByIndex(data,nrOfValuesOut,index);
      status=  nc_put_var_uchar  (destncid, Variable,data); 
      ncerror("nc_put_var_uchar",status); if(status != NC_NOERR)return 1;
    }
    if(Vars[j].type == NC_SHORT||Vars[j].type == NC_USHORT){
      snprintf(szDebug,MAX_STR_LEN,"NC_SHORT\t\t%s",Vars[j].name);CDBDebug(szDebug,2);
      short *data=(short*)Vars[j].data;
      SortByIndex(data,nrOfValuesOut,index);
      status=  nc_put_var_short  (destncid, Variable,data); 
      ncerror("nc_put_var_short",status); if(status != NC_NOERR)return 1;
    }
    if(Vars[j].type == NC_INT||Vars[j].type == NC_UINT){
      snprintf(szDebug,MAX_STR_LEN,"NC_INT\t\t%s",Vars[j].name);CDBDebug(szDebug,2);
      int *data=(int*)Vars[j].data;
      SortByIndex(data,nrOfValuesOut,index);
      status=  nc_put_var_int  (destncid, Variable,data); 
      ncerror("nc_put_var_int",status); if(status != NC_NOERR)return 1;
    }
    if(Vars[j].type == NC_FLOAT){
      snprintf(szDebug,MAX_STR_LEN,"NC_FLOAT\t%s",Vars[j].name);CDBDebug(szDebug,2);
      float *data=(float*)Vars[j].data;
      SortByIndex(data,nrOfValuesOut,index);
      status=  nc_put_var_float  (destncid, Variable,data); 
      ncerror("nc_put_var_float",status); if(status != NC_NOERR)return 1;
    }
    if(Vars[j].type == NC_DOUBLE){
      snprintf(szDebug,MAX_STR_LEN,"NC_DOUBLE\t%s",Vars[j].name);CDBDebug(szDebug,2);
      double *data=(double*)Vars[j].data;
      SortByIndex(data,nrOfValuesOut,index);
      status=  nc_put_var_double  (destncid, Variable,data); 
      ncerror("nc_put_var_double",status); if(status != NC_NOERR)return 1;
    }

  }
  NCMLWriter->SetNetCDFId(destncid);
  
  // Attach specific product attributes:
  NCMLWriter->SetMetaDataAttribute("product"  ,"software_version",SOFTWARE_VERSION  ,"String");
  NCMLWriter->SetMetaDataAttribute("product"  ,"variables"       ,szProductVariables,"String");
  NCMLWriter->SetMetaDataAttribute("time"     ,"units"           ,time_unit         ,"String");
  NCMLWriter->SetMetaDataAttribute("NC_GLOBAL","Conventions"     ,"CF-1.4"          ,"String");
  // Attach creation date
  CADAGUC_time * ADAGUC_time=new CADAGUC_time(time_unit);
  char szCreationDate[MAX_STR_LEN+1];
  ADAGUC_time->GetCurrentDate(szCreationDate,MAX_STR_LEN);
  NCMLWriter->SetMetaDataAttribute("product","creation_date"    ,szCreationDate,"String");
  delete ADAGUC_time;
  if(NCMLWriter->AttachNCML()!=0)return 1;
  /*********************/
  nc_close(destncid);
  CDBDebug("Finished writing NetCDF4 file",2);
  CDBDebug("[/WriteNC4]",1);
  delete[] index;
  return 0;
}

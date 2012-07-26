#ifndef CImageWarper_H
#define CImageWarper_H
#include "CServerParams.h"
#include "CDataReader.h"
#include "CDrawImage.h"
#include <proj_api.h>
#include <math.h>
#include "CDebugger.h"
#include "CStopWatch.h"

#define LATLONPROJECTION "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
void floatToString(char * string,size_t maxlen,float number);
void floatToString(char * string,size_t maxlen,int numdigits,float number);

class CImageWarper{
//  CNetCDFReader reader;
  private:

    double dfMaxExtent[4];
    int dMaxExtentDefined;
    bool convertRadiansDegreesDst,convertRadiansDegreesSrc,requireReprojection;
    DEF_ERRORFUNCTION();
    unsigned char pixel;
    CDataSource * _dataSource;
    CGeoParams * _geoDest;
    CT::string destinationCRS;
    int _decodeCRS(CT::string *CRS);
    std::vector <CServerConfig::XMLE_Projection*> *prj;
    bool initialized;
  public:
    CImageWarper(){
      prj=NULL;
      sourcepj=NULL;
      destpj=NULL;
      latlonpj=NULL;
      initialized =false;
    }
    projPJ sourcepj,destpj,latlonpj;
    int initreproj(CDataSource *dataSource,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *prj);
    int initreproj(const char * projString,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj);
    int closereproj();
    int reprojpoint(double &dfx,double &dfy);
    int reprojpoint(CPoint &p);
    int reprojpoint_inv(double &dfx,double &dfy);
    int reprojpoint_inv(CPoint &p);
    int reprojModelToLatLon(double &dfx,double &dfy);
    int reprojModelFromLatLon(double &dfx,double &dfy);
    void reprojBBOX(double *df4PixelExtent);
    int reprojfromLatLon(double &dfx,double &dfy);
    int reprojToLatLon(double &dfx,double &dfy);
    int decodeCRS(CT::string *outputCRS, CT::string *inputCRS);
    int decodeCRS(CT::string *outputCRS, CT::string *inputCRS,std::vector <CServerConfig::XMLE_Projection*> *prj);
    int findExtent(CDataSource *dataSource,double * dfBBOX);
    bool isProjectionRequired(){return requireReprojection;}
  };
  

#endif

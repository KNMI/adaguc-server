#ifndef COpenDAPHandler_H
#define COpenDAPHandler_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CIBaseDataWriterInterface.h"
#include "CImgWarpNearestNeighbour.h"
#include "CImgWarpNearestRGBA.h"
#include "CImgWarpBilinear.h"
#include "CImgWarpBoolean.h"
#include "CImgRenderPoints.h"
#include "CStyleConfiguration.h"
#include "CMyCURL.h"
#include "CXMLParser.h"
#include "CTime.h"
#include "CDebugger.h"

class COpenDAPHandler{
private:
  /**
    * This class contains all settings put in the REST OpenDAP URL, e.g. selected variables and their dimensions (start, stride, count).
    */
    class VarInfo{
      public:
        class Dim{
        public:
          Dim(const char *name,size_t start,size_t count, ptrdiff_t stride){
            this->name = name;
            this->start = start;
            this->count = count;
            this->stride = stride;
          }
          CT::string name;
          size_t start;
          size_t count;
          ptrdiff_t stride;
        };
      VarInfo(const char *name){
        this->name = name;
      }
      CT::string name;
      std::vector<Dim> dimInfo;
      
    };
  static CT::string VarInfoToString(std::vector <VarInfo> selectedVariables);
  static int putVariableData(CDF::Variable *v,CDFType type);
  static CT::string createDDSHeader(CT::string layerName, CDFObject *cdfObject ,std::vector <VarInfo> selectedVariables);
  static int getDimSize(CDataSource *dataSource, const char *name);
public:
  DEF_ERRORFUNCTION();
  static int HandleOpenDAPRequest(const char *path,const char *query,CServerParams *srvParams);
};

#endif
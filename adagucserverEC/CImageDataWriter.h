#ifndef CImageDataWriter_H
#define CImageDataWriter_H
#include "Definitions.h"
#include "CStopWatch.h"
#include "CIBaseDataWriterInterface.h"
#include "CImgWarpNearestNeighbour.h"
#include "CImgWarpBilinear.h"
#include "CImgWarpBoolean.h"
#include "CMyCURL.h"

class CImageDataWriter: public CBaseDataWriterInterface{
  private:
    CImageWarper imageWarper;
    int requestType;
    int status;
    int mode;
    int animation;
    int nrImagesAdded;
    //CT::string getFeatureInfoResult;
    //CT::string getFeatureInfoHeader;

    
    class GetFeatureInfoResult{
      public:
        ~GetFeatureInfoResult(){
          for(size_t j=0;j<elements.size();j++)delete elements[j];
        }
        
        int x_imagePixel,y_imagePixel;
        double x_imageCoordinate,y_imageCoordinate;
        double x_rasterCoordinate,y_rasterCoordinate;
        int x_rasterIndex,y_rasterIndex;
        double lat_coordinate,lon_coordinate;

        CT::string layerName;

        class Element{
        public:
          CT::string value;
          CT::string units;
          CT::string standard_name;
          CT::string feature_name;
          CT::string long_name;
          CT::string var_name;
          CT::string time;
          CDF::Variable *variable;
        };
        std::vector <Element*>elements;
    };
    
    
    std::vector<GetFeatureInfoResult*> getFeatureInfoResultList;
    
    DEF_ERRORFUNCTION();
    int createLegend(CDataSource *sourceImage,CDrawImage *drawImage);
    int warpImage(CDataSource *sourceImage,CDrawImage *drawImage);
    CDrawImage drawImage;
    CServerParams *srvParam;
    enum RenderMethodEnum { nearest, bilinear, contour, vector, barb, barbcontour, shaded,contourshaded,vectorcontour,vectorcontourshaded,nearestcontour,bilinearcontour};
    
    float shadeInterval,contourIntervalL,contourIntervalH;
    float textScaleFactor,textOffsetFactor;//To display pressure in pa to hpa etc...
    int smoothingFilter;
    RenderMethodEnum renderMethod;
    RenderMethodEnum getRenderMethodFromString(CT::string *renderMethodString);
    double convertValue(CDFType type,void *data,size_t p);
    void setValue(CDFType type,void *data,size_t ptr,double pixel);
    int _setTransparencyAndBGColor(CServerParams *srvParam,CDrawImage* drawImage);
    float getValueForColorIndex(CDataSource *dataSource,int index);
    int getColorIndexForValue(CDataSource *dataSource,float value);
    int drawCascadedWMS(const char *service,const char *layers,bool transparent);
  public:
    static const char *RenderMethodStringList;
    CImageDataWriter();
    ~CImageDataWriter(){
      for(size_t j=0;j<getFeatureInfoResultList.size();j++){delete getFeatureInfoResultList[j];getFeatureInfoResultList[j]=NULL; } getFeatureInfoResultList.clear();
    }
    
    int createLegend(CDataSource *sourceImage);
    int getFeatureInfo(CDataSource *sourceImage,int dX,int dY);
    int createAnimation();
    void setDate(const char *date);
    int calculateData(std::vector <CDataSource*> &dataSources);
    
    // Virtual functions
    int init(CServerParams *srvParam, CDataSource *dataSource,int nrOfBands);
    int addData(std::vector <CDataSource*> &dataSources);
    int end();
    int initializeLegend(CServerParams *srvParam,CDataSource *dataSource);
};

#endif

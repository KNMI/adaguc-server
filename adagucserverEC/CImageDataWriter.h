#ifndef CImageDataWriter_H
#define CImageDataWriter_H
#include <string>
#include <map>

#include "Definitions.h"
#include "CStopWatch.h"
#include "CIBaseDataWriterInterface.h"
#include "CImgWarpNearestNeighbour.h"
#include "CImgWarpBilinear.h"
#include "CImgWarpBoolean.h"
#include "CImgRenderPoints.h"
#include "CMyCURL.h"

class CImageDataWriter: public CBaseDataWriterInterface{
  private:
    
    class ProjCacheInfo{
    public:
      double CoordX,CoordY;
      double nativeCoordX,nativeCoordY;
      double lonX,lonY;
      int imx,imy;
    };
    
    static std::map<std::string,CImageDataWriter::ProjCacheInfo> projCacheMap;
    static std::map<std::string,CImageDataWriter::ProjCacheInfo>::iterator projCacheIter;
  
    
    CImageWarper imageWarper;
  
    
    CDataSource *currentDataSource;
    
    int requestType;
    int status;
   
    int animation;
    int nrImagesAdded;
    //CT::string getFeatureInfoResult;
    //CT::string getFeatureInfoHeader;
    enum RenderMethodEnum { undefined,nearest, bilinear, contour, 
    barb, barbshaded, barbcontour, barbcontourshaded, thinbarb, thinbarbshaded, thinbarbcontour, thinbarbcontourshaded, 
    shaded,shadedcontour, 
    point,
    vector, vectorshaded, vectorcontour, vectorcontourshaded, thinvector, thinvectorshaded, thinvectorcontour, thinvectorcontourshaded,
    nearestcontour,bilinearcontour};
    static void calculateScaleAndOffsetFromMinMax(float &scale, float &offset,float min,float max,float log);
public:
  class StyleConfiguration {
  public:
    
    float shadeInterval,contourIntervalL,contourIntervalH;
    float legendScale,legendOffset,legendLog;
    
    //Values in which the legend is valid
    
    float legendLowerRange,legendUpperRange;
    int smoothingFilter;
    bool hasLegendValueRange;
    bool hasError;
    RenderMethodEnum renderMethod;
    int legendIndex;
    int styleIndex;
    CT::string styleCompositionName;
    void printStyleConfig(CT::string *data){
      data->print("shadeInterval = %f\n",shadeInterval);
      data->printconcat("contourIntervalL = %f\n",contourIntervalL);
      data->printconcat("contourIntervalH = %f\n",contourIntervalH);
      data->printconcat("legendScale = %f\n",legendScale);
      data->printconcat("legendOffset = %f\n",legendOffset);
      data->printconcat("legendLog = %f\n",legendLog);
      data->printconcat("hasLegendValueRange = %d\n",hasLegendValueRange);
      data->printconcat("legendLowerRange = %f\n",legendLowerRange);
      data->printconcat("legendUpperRange = %f\n",legendUpperRange);
      data->printconcat("smoothingFilter = %d\n",smoothingFilter);
      CT::string rMethodString;
      getRenderMethodAsString(&rMethodString,renderMethod);
      data->printconcat("renderMethod = %s",rMethodString.c_str());
    }
  };
private:
    
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
        int dataSourceIndex;
        
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
          CDataSource *dataSource;
        };
        std::vector <Element*>elements;
    };


private:
    static int getTextForValue(CT::string *tv,float v,StyleConfiguration *currentStyleConfiguration);
    std::vector<GetFeatureInfoResult*> getFeatureInfoResultList;
    
    DEF_ERRORFUNCTION();

    int warpImage(CDataSource *sourceImage,CDrawImage *drawImage);
  
    CServerParams *srvParam;
    
    enum ImageDataWriterStatus { uninitialized, initialized, finished};
    ImageDataWriterStatus writerStatus;
    //float shadeInterval,contourIntervalL,contourIntervalH;

    //int smoothingFilter;
    //RenderMethodEnum renderMethod;
    static RenderMethodEnum getRenderMethodFromString(CT::string *renderMethodString);
    static void getRenderMethodAsString(CT::string *renderMethodString, RenderMethodEnum renderMethod);
    double convertValue(CDFType type,void *data,size_t p);
    void setValue(CDFType type,void *data,size_t ptr,double pixel);
    int _setTransparencyAndBGColor(CServerParams *srvParam,CDrawImage* drawImage);
    float getValueForColorIndex(CDataSource *dataSource,int index);
    int getColorIndexForValue(CDataSource *dataSource,float value);
    int drawCascadedWMS(CDataSource *dataSource,const char *service,const char *layers,bool transparent);
    
    static CT::PointerList<CT::string*> *getRenderMethodListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style);      
    static CT::PointerList<CT::string*> *getLegendListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style);
    static CT::PointerList<CT::string*> *getLegendNames(std::vector <CServerConfig::XMLE_Legend*> Legend);
    static CT::PointerList<CT::string*> *getStyleNames(std::vector <CServerConfig::XMLE_Styles*> Styles);
    static CT::PointerList<CT::string*> *getStyleListForDataSource(CDataSource *dataSource,StyleConfiguration *styleConfig);
  public:
    CDrawImage drawImage;
    CImageDataWriter::StyleConfiguration * currentStyleConfiguration;
    static int  getServerLegendIndexByName(const char * legendName,std::vector <CServerConfig::XMLE_Legend*> serverLegends);
    static int  getServerStyleIndexByName(const char * styleName,std::vector <CServerConfig::XMLE_Style*> serverStyles);
    static CT::PointerList<CT::string*> *getStyleListForDataSource(CDataSource *dataSource);
    static int makeStyleConfig(StyleConfiguration *styleConfig,CDataSource *dataSource,const char *styleName,const char *legendName,const char *renderMethod);
    static StyleConfiguration *getStyleConfigurationByName(const char *styleName,CDataSource *dataSource);

    static const char *RenderMethodStringList;
    CImageDataWriter();
    ~CImageDataWriter(){
      for(size_t j=0;j<getFeatureInfoResultList.size();j++){delete getFeatureInfoResultList[j];getFeatureInfoResultList[j]=NULL; } getFeatureInfoResultList.clear();
      delete currentStyleConfiguration;currentStyleConfiguration = NULL;
    }
    
    int createLegend(CDataSource *sourceImage,CDrawImage *legendImage);
    int getFeatureInfo(std::vector<CDataSource *>dataSources,int dataSourceIndex,int dX,int dY);
    int createAnimation();
    void setDate(const char *date);
    int calculateData(std::vector <CDataSource*> &dataSources);
    
    // Virtual functions
    int init(CServerParams *srvParam, CDataSource *dataSource,int nrOfBands);
    int addData(std::vector <CDataSource*> &dataSources);
    int end();
    int initializeLegend(CServerParams *srvParam,CDataSource *dataSource);
    int drawText(int x,int y,const char *fontfile,float size, float angle,const char *text,unsigned char colorIndex);
};

#endif

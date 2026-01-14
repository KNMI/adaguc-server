#include "CCreateLegend.h"
#include "CCreateLegendRenderDiscreteLegend.cpp"
#include "CCreateLegendRenderContinuousLegend.cpp"
#include "CDataReader.h"
#include "CImageDataWriter.h"

const char *CCreateLegend::className = "CCreateLegend";

int CCreateLegend::createLegend(CDataSource *dataSource, CDrawImage *legendImage) {
  createLegend(dataSource, legendImage, false);
  return 0;
}

int CCreateLegend::createLegend(CDataSource *dataSource, CDrawImage *legendImage, bool rotate) {
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("createLegend");
#endif

  if (dataSource->cfgLayer != NULL) {
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if (styleConfiguration != NULL) {

      auto drawSettings = getDrawFunctionSettings(dataSource, legendImage, styleConfiguration);
      if (drawSettings.drawgrid == false) {
        legendImage->crop(4);
        return 0;
      }
      if (styleConfiguration->legendGraphic.attr.value.empty() == false) {
        const char *fileName = styleConfiguration->legendGraphic.attr.value.c_str();
        legendImage->destroyImage();
        legendImage->createImage(fileName);
        return 0;
      }
    }
  }

  int status = 0;
  enum LegendType { undefined, continous, discrete, statusflag, cascaded };
  LegendType legendType = undefined;
  bool estimateMinMax = false;

  float legendHeight = legendImage->geoParams.height;

  int pLeft = 4;
  int pTop = (int)(legendImage->geoParams.height - legendHeight);

  if (dataSource->dLayerType == CConfigReaderLayerTypeCascaded) {
    legendType = cascaded;
    CDBDebug("GetLegendGraphic for cascaded WMS is not yet supported");
    legendImage->crop(4);
    return 0;
  }

  CDataReader reader;
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration == NULL) {
    CDBError("No style configuration");
    return 1;
  }
  auto renderMethod = styleConfiguration->renderMethod;

  if (renderMethod & RM_RGBA) {

    // legendImage->setText("",5,0,0,248,-1);
    legendImage->crop(4);
    return 0;
  }
  // legendImage->enableTransparency(true);
  // legendImage->rectangle(0,0,20,20,CColor(255,255,255,255),CColor(255,255,255,255));
  // legendImage->setText("RGBA",5,0,0,255,-1);
  // legendImage->crop(40,40);
  // return 0;
  if (legendType == cascaded) {
    legendImage->crop(4);
    return 0;
  }
  // If no min or mas is set, detect it. Also detect it if the legend has no fixed/min/max
  if (styleConfiguration->legendScale == 0.0f || styleConfiguration->legendHasFixedMinMax == false) {
    estimateMinMax = true;
  } else {
    estimateMinMax = false;
  }

  if (dataSource->srvParams->wmsExtensions.colorScaleRangeSet && dataSource->srvParams->wmsExtensions.numColorBandsSet) {
    estimateMinMax = false;
  }

  if (dataSource->getFileName() != NULL && strlen(dataSource->getFileName()) > 0) {
    if (estimateMinMax) {
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Opening CNETCDFREADER_MODE_OPEN_ALL");
#endif
      status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL);
    } else {
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Opening CNETCDFREADER_MODE_OPEN_HEADER");
#endif
      status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
    }

    if (status != 0) {
      CDBError("Unable to open file");
      return 1;
    }
  } else {
    estimateMinMax = false;
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("createLegend without any file information");
#endif
  }
//}
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("Determine legendtype");
#endif

  // Determine legendtype.
  if (dataSource->getDataObject(0)->hasStatusFlag) {
    legendType = statusflag;
  } else if (renderMethod & RM_POINT) {
    if (styleConfiguration != NULL && styleConfiguration->shadeIntervals.size() > 0) {
      legendType = discrete;
    } else {
      legendType = continous;
    }
  } else if (!(renderMethod & RM_SHADED || renderMethod & RM_CONTOUR)) {
    legendType = continous;
  } else {
    if (!(renderMethod & RM_SHADED)) {
      if (renderMethod & RM_NEAREST || renderMethod & RM_BILINEAR) {
        legendType = continous;
      } else {
        legendType = cascaded;
      }
    } else {
      legendType = discrete;
    }
  }

  if (styleConfiguration->renderMethod == RM_GENERIC && styleConfiguration->shadeIntervals.size() > 0) {
    legendType = discrete;
  }

  if (styleConfiguration->featureIntervals.size() > 0) {
    legendType = discrete;
  }

  for (auto renderSetting : styleConfiguration->renderSettings) {
    /* When using the nearest or bilinear rendermethod, discrete classes defined by ShadeInterval can be used if the renderhint is set to RENDERHINT_DISCRETECLASSES */
    if (renderSetting->attr.renderhint.equals(RENDERHINT_DISCRETECLASSES)) {
      legendType = discrete;
    }
  }

  /*
   * if(legendType==continous){
   *   if(legendHeight>280)legendHeight=280;
}*/

  // Create a legend based on status flags.
  if (legendType == statusflag) {
#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("legendtype statusflag");
#endif
    int dH = 30;
    // cbW=LEGEND_WIDTH/3;cbW/=3;cbW*=3;cbW+=3;
    float cbW = 20; // legendWidth/8;
    float cbH = legendHeight - 13 - 13 - 30;

    bool useShadeIntervals = false;
    if (styleConfiguration != NULL && styleConfiguration->shadeIntervals.size() > 0) {
      useShadeIntervals = true;
    }

    int blockHeight = 18;
    int blockDistance = 20;

    size_t numFlags = dataSource->getDataObject(0)->statusFlagList.size();
    while (numFlags * blockDistance > legendHeight - 14 && blockDistance > 5) {
      blockDistance--;
      if (blockHeight > 5) {
        blockHeight--;
      }
    }

    CColor black(0, 0, 0, 255);

    for (size_t j = 0; j < numFlags; j++) {
      float y = j * blockDistance + (cbH - numFlags * blockDistance + 8);
      double value = dataSource->getDataObject(0)->statusFlagList[j].value;
      if (useShadeIntervals) {
        CColor col = CImageDataWriter::getPixelColorForValue(dataSource, value);
        legendImage->rectangle(1 + pLeft, int(2 + dH + y) + pTop, (int)cbW + 9 + pLeft, (int)y + 2 + dH + blockHeight + pTop, col, black);
      } else {
        int c = CImageDataWriter::getColorIndexForValue(dataSource, value);
        legendImage->rectangle(1 + pLeft, int(2 + dH + y) + pTop, (int)cbW + 9 + pLeft, (int)y + 2 + dH + blockHeight + pTop, c, 248);
      }

      CT::string flagMeaning;
      CDataSource::getFlagMeaningHumanReadable(&flagMeaning, &dataSource->getDataObject(0)->statusFlagList, value);
      CT::string legendMessage;
      legendMessage.print("%d) %s", (int)value, flagMeaning.c_str());
      legendImage->setText(legendMessage.c_str(), legendMessage.length(), (int)cbW + 15 + pLeft, (int)y + dH + 2 + pTop, 248, -1);
    }
    //     CT::string units="status flag";
    //     legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    // legendImage->crop(4,4);
  }

  if (legendType == continous) {
    if (renderContinuousLegend(dataSource, legendImage, styleConfiguration, rotate, estimateMinMax) != 0) {
      CDBError("renderContinuousLegend did not succeed.");
      return 1;
    }
  }
  // Draw legend with fixed intervals
  if (legendType == discrete) {
    if (renderDiscreteLegend(dataSource, legendImage, styleConfiguration, rotate, estimateMinMax) != 0) {
      CDBError("renderDiscreteLegend did not succeed.");
      return 1;
    }
  }

  reader.close();

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("cropping");
#endif

  double scaling = dataSource->getScaling();

  legendImage->crop(4 * scaling);
  if (rotate) {
    CDBDebug("rotate");
    legendImage->rotate();
  }
  return 0;
}

/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

class PlotObject {
public:
  PlotObject() {
    elements = NULL;
    values = NULL;
    length = 0;
  }
  ~PlotObject() { freePoints(); }

  CImageDataWriter::GetFeatureInfoResult::Element **elements;
  size_t length;
  CT::string name;
  CT::string units;

  double minValue, maxValue, *values;

  void freePoints() {
    // First remove pointers, otherwise the elements are also destructed.
    if (elements == NULL) return;
    for (size_t j = 0; j < length; j++) {
      elements[j] = NULL;
    }
    delete[] elements;
    delete[] values;
    elements = NULL;
    values = NULL;
  }

  void allocateLength(size_t numPoints) {
    length = numPoints;
    freePoints();
    elements = new CImageDataWriter::GetFeatureInfoResult::Element *[numPoints];
    values = new double[numPoints];
  }
};

class CTimeSeriesPlot {
public:
  static int createTimeSeriesPlot() {
#ifdef MEASURETIME
    StopWatch_Stop("Start creating image");
#endif

    if (getFeatureInfoResultList.size() == 0) {
      CDBError("Query returned no results");
      return 1;
    }

#ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("GetFeatureInfo Format image/png");
#endif
    float width = srvParam->geoParams.dWidth, height = srvParam->geoParams.dHeight;
    if (srvParam->figWidth > 1) width = srvParam->figWidth;
    if (srvParam->figHeight > 1) height = srvParam->figHeight;

    float plotOffsetX = (width * 0.08);
    float plotOffsetY = (height * 0.1);
    if (plotOffsetX < 80) plotOffsetX = 80;
    if (plotOffsetY < 35) plotOffsetY = 35;
    plotOffsetY = 35;

    float plotHeight = ((height - plotOffsetY - 34));
    float plotWidth = ((width - plotOffsetX) * 0.98);

    // Set font location
    const char *fontLocation = NULL;
    if (srvParam->cfg->WMS[0]->ContourFont.size() != 0) {
      if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.empty() == false) {
        fontLocation = srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();
      } else {
        CDBError("In <Font>, attribute \"location\" missing");
        return 1;
      }
    }

    // Initialization of whole canvas
    CDrawImage plotCanvas;
    CDrawImage lineCanvas;
    if (resultFormat == imagepng) {
      plotCanvas.setTrueColor(true);
      lineCanvas.setTrueColor(true);
    }
    if (resultFormat == imagegif) {
      plotCanvas.setTrueColor(false);
      plotCanvas.setBGColor(255, 255, 255);
      lineCanvas.setTrueColor(false);
      lineCanvas.enableTransparency(true);
    }
    plotCanvas.createImage(int(width), int(height));
    plotCanvas.create685Palette();
    lineCanvas.createImage(plotWidth, plotHeight);
    lineCanvas.create685Palette();
    plotCanvas.rectangle(0, 0, int(width - 1), int(height - 1), CColor(255, 255, 255, 255), CColor(0, 0, 0, 255));

    size_t nrOfLayers = getFeatureInfoResultList.size();
    //      size_t nrOfElements = getFeatureInfoResultList[0]->elements.size();

    std::vector<PlotObject *> plotObjects;

    std::vector<CT::string> features[nrOfLayers];
    std::vector<int> numDims[nrOfLayers];
    // Find number of features per layer
    for (size_t layerNr = 0; layerNr < nrOfLayers; layerNr++) {
      size_t nrOfElements = getFeatureInfoResultList[layerNr]->elements.size();

      for (size_t elNr = 0; elNr < nrOfElements; elNr++) {
        GetFeatureInfoResult::Element *element = getFeatureInfoResultList[layerNr]->elements[elNr];
        bool featureNameFound = false;
        for (size_t j = 0; j < features[layerNr].size(); j++) {
          if (features[layerNr][j].equals(&element->feature_name)) {
            featureNameFound = true;
            break;
          }
        }
        if (!featureNameFound) {
          features[layerNr].push_back(element->feature_name.c_str());
          numDims[layerNr].push_back(element->cdfDims.dimensions.size());
        } else {
          for (size_t j = 0; j < features[layerNr].size(); j++) {
            CDBDebug("%d %d %s\tDims:%d", layerNr, j, features[layerNr][j].c_str(), numDims[layerNr][j]);
          }

          break;
        }
      }
    }
    for (size_t layerNr = 0; layerNr < nrOfLayers; layerNr++) {
      if (numDims[layerNr].size() > 0) {
        CDataSource *ds = getFeatureInfoResultList[layerNr]->elements[0]->dataSource;
        size_t nrOfElements = getFeatureInfoResultList[layerNr]->elements.size();
        size_t nrOfFeatures = features[layerNr].size();
        size_t nrOfElementSteps = nrOfElements / (nrOfFeatures);

        size_t numDimStepsPerTime = 1;
        CT::string dimname = "";
        for (size_t j = 1; j < ds->requiredDims.size(); j++) {
          numDimStepsPerTime *= ds->requiredDims[j]->allValues.size();
        }
        nrOfElementSteps = nrOfElementSteps / numDimStepsPerTime;

        for (size_t featureNr = 0; featureNr < nrOfFeatures; featureNr++) {
          for (size_t dimIter = 0; dimIter < numDimStepsPerTime; dimIter++) {
            PlotObject *plotObject = new PlotObject();
            plotObjects.push_back(plotObject);
            plotObject->allocateLength(nrOfElementSteps);

            size_t elNr = dimIter * nrOfFeatures + featureNr;
            GetFeatureInfoResult::Element *element = getFeatureInfoResultList[layerNr]->elements[elNr];
            plotObject->name.copy(getFeatureInfoResultList[layerNr]->layerName.c_str());
            plotObject->name.concat("/");
            plotObject->name.concat(features[layerNr][featureNr].c_str());
            for (size_t j = 1; j < element->cdfDims.dimensions.size(); j++) {
              plotObject->name.concat(" @");
              plotObject->name.concat(element->cdfDims.dimensions[j]->value.c_str());
            }

            plotObject->units = &element->units;
            for (size_t elStep = 0; elStep < nrOfElementSteps; elStep++) {

              // CDBDebug("Iterating %s",ds->requiredDims[j]->allValues[

              size_t elNr = elStep * nrOfFeatures * numDimStepsPerTime + featureNr + dimIter * nrOfFeatures;

              // CDBDebug("%d = %d/%d %d/%d - %d/%d - %d/%d",elNr,layerNr,nrOfLayers,featureNr,nrOfFeatures,dimIter,numDimStepsPerTime,elStep,nrOfElementSteps);

              GetFeatureInfoResult::Element *element = getFeatureInfoResultList[layerNr]->elements[elNr];
              plotObject->elements[elStep] = element;

              /*
              CT::string dims = "";
              for(size_t d=1;d<element->cdfDims.dimensions.size();d++){
                dims.printconcat("%s ",element->cdfDims.dimensions[d]->value.c_str());
              }
              CDBDebug("%s %s",features[layerNr][featureNr].c_str(),dims.c_str());
            */
            }
          }
        }
      }
    }

    // Find min max for values and time

    CTime *ctime = new CTime();
    ctime->init("seconds since 1950");

    double startTimeValue = 0;
    double stopTimeValue = 0;
    bool firstDateDone = false;

    for (size_t j = 0; j < plotObjects.size(); j++) {
      PlotObject *plotObject = plotObjects[j];
      CDBDebug("%d) %s in %s", j, plotObject->name.c_str(), plotObject->units.c_str());

      // Find min and max dates
      double minDate;
      double maxDate;
      try {
        minDate = ctime->ISOStringToDate(plotObject->elements[0]->time.c_str()).offset;
      } catch (int e) {
        CDBError("Time startTimeValue error %s", plotObject->elements[0]->time.c_str());
      }

      try {
        maxDate = ctime->ISOStringToDate(plotObject->elements[plotObject->length - 1]->time.c_str()).offset;
      } catch (int e) {
        CDBError("Time stopTimeValue error %s", plotObject->elements[plotObject->length - 1]->time.c_str());
      }

      if (!firstDateDone) {
        startTimeValue = minDate;
        stopTimeValue = maxDate;
        firstDateDone = true;
      } else {
        if (startTimeValue > minDate) startTimeValue = minDate;
        if (stopTimeValue < maxDate) stopTimeValue = maxDate;
      }

      bool firstDone = false;
      plotObject->minValue = 0;
      plotObject->maxValue = 1;

      // Find min and max values
      for (size_t i = 0; i < plotObject->length; i++) {
        GetFeatureInfoResult::Element *element = plotObject->elements[i];
        double value = element->value.toFloat();
        if (element->value.c_str()[0] > 60) value = NAN;
        ;
        if (element->value.equals("nodata")) value = NAN;
        plotObject->values[i] = value;
        // CDBDebug("%f",plotObject->values[i]);
        if (value == value) {
          if (firstDone == false) {
            plotObject->minValue = value;
            plotObject->maxValue = value;
          }
          firstDone = true;
          if (plotObject->minValue > value) plotObject->minValue = value;
          if (plotObject->maxValue < value) plotObject->maxValue = value;
        }
      }

      // Minmax is fixed by layer settings:
      if (plotObject->elements[0]->dataSource != NULL) {
        if (!plotObject->elements[0]->dataSource->stretchMinMax) {
          // Determine min max based on given datasource settings (scale/offset/log or min/max/log in config file)
          plotObject->minValue = getValueForColorIndex(plotObject->elements[0]->dataSource, 0);
          plotObject->maxValue = getValueForColorIndex(plotObject->elements[0]->dataSource, 240);
        }
      }

      // Increase minmax if they are the same.
      if (plotObject->minValue == plotObject->maxValue) {
        plotObject->minValue = plotObject->minValue + 0.01;
        plotObject->maxValue = plotObject->maxValue - 0.01;
      }

      CDBDebug("%f %f", plotObject->minValue, plotObject->maxValue);
    }

    CT::string startDateString = ctime->dateToISOString(ctime->getDate(startTimeValue));
    CT::string stopDateString = ctime->dateToISOString(ctime->getDate(stopTimeValue));
    startDateString.setChar(19, 'Z');
    startDateString.setSize(20);
    stopDateString.setChar(19, 'Z');
    stopDateString.setSize(20);
    CDBDebug("Dates: %s/%s", startDateString.c_str(), stopDateString.c_str());

    float classes = 6;
    int tickRound = 0;

    if (currentStyleConfiguration->legendTickInterval > 0.0f) {
      classes = (plotObjects[0]->minValue - plotObjects[0]->maxValue) / currentStyleConfiguration->legendTickInterval;
    }
    if (currentStyleConfiguration->legendTickRound > 0) {
      tickRound = int(round(log10(currentStyleConfiguration->legendTickRound)) + 3);
    }

    // TODO
    plotCanvas.rectangle(int(plotOffsetX), int(plotOffsetY), int(plotWidth + plotOffsetX), int(plotHeight + plotOffsetY), CColor(240, 240, 240, 255), CColor(0, 0, 0, 255));
    CDataSource *dataSource = getFeatureInfoResultList[0]->elements[0]->dataSource;

    for (int j = 0; j <= classes; j++) {
      char szTemp[256];
      float c = ((float(classes - j) / classes)) * (plotHeight);
      float v = ((float(j) / classes)) * (240.0f);
      v -= dataSource->legendOffset;
      v /= dataSource->legendScale;
      if (dataSource->legendLog != 0) {
        v = pow(dataSource->legendLog, v);
      }

      if (j != 0) plotCanvas.line(plotOffsetX, (int)c + plotOffsetY, plotOffsetX + plotWidth, (int)c + plotOffsetY, 0.5, CColor(0, 0, 0, 128));
      if (tickRound == 0) {
        floatToString(szTemp, 255, v);
      } else {
        floatToString(szTemp, 255, tickRound, v);
      }
      plotCanvas.drawText(4, int(c + plotOffsetY + 3), fontLocation, 8, 0, szTemp, CColor(0, 0, 0, 255), CColor(255, 255, 255, 0));
    }

    for (size_t plotNr = 0; plotNr < plotObjects.size(); plotNr++) {
      PlotObject *plotObject = plotObjects[plotNr];
      CColor color = CColor(255, 255, 255, 255);
      if (plotNr == 0) {
        color = CColor(0, 0, 255, 255);
      }
      if (plotNr == 1) {
        color = CColor(0, 255, 0, 255);
      }
      if (plotNr == 2) {
        color = CColor(255, 0, 0, 255);
      }
      if (plotNr == 3) {
        color = CColor(255, 128, 0, 255);
      }
      if (plotNr == 4) {
        color = CColor(0, 255, 128, 255);
      }
      if (plotNr == 5) {
        color = CColor(255, 0, 128, 255);
      }
      if (plotNr == 6) {
        color = CColor(0, 0, 128, 255);
      }
      if (plotNr == 7) {
        color = CColor(128, 0, 0, 255);
      }
      if (plotNr == 8) {
        color = CColor(0, 128, 0, 255);
      }

      if (plotNr == 9) {
        color = CColor(0, 128, 0, 255);
      }
      if (plotNr == 10) {
        color = CColor(0, 128, 128, 255);
      }
      if (plotNr == 11) {
        color = CColor(128, 128, 0, 255);
      }

      float stepX = float(plotWidth);
      if (stopTimeValue - startTimeValue > 0) {
        stepX = float(plotWidth) / ((stopTimeValue - startTimeValue));
      }
      double timeWidth = (stopTimeValue - startTimeValue);
      for (size_t i = 0; i < plotObject->length - 1; i++) {
        CTime::Date timePos1 = ctime->ISOStringToDate(plotObject->elements[i]->time.c_str());
        CTime::Date timePos2 = ctime->ISOStringToDate(plotObject->elements[i + 1]->time.c_str());
        double x1 = ((timePos1.offset - startTimeValue) / timeWidth) * plotWidth;
        double x2 = ((timePos2.offset - startTimeValue) / timeWidth) * plotWidth;

        if (plotNr == 0) {
          if (timePos1.hour == 0 && timePos1.minute == 0 && timePos1.second == 0) {
            plotCanvas.line(x1 + plotOffsetX, plotOffsetY, x1 + plotOffsetX, plotOffsetY + plotHeight, 1, CColor(0, 0, 0, 128));
            char szTemp[256];
            snprintf(szTemp, 255, "%d", timePos1.day);
            plotCanvas.drawText(x1 - strlen(szTemp) * 3 + plotOffsetX, int(plotOffsetY + plotHeight + 10), fontLocation, 6, 0, szTemp, CColor(0, 0, 0, 255), CColor(255, 255, 255, 0));
          } else {
            plotCanvas.line(x1 + plotOffsetX, plotOffsetY, x1 + plotOffsetX, plotOffsetY + plotHeight, 0.5, CColor(128, 128, 128, 128));
          }
        }

        float v1 = plotObject->values[i];
        float v2 = plotObject->values[i + 1];
        if (v1 == v1 && v2 == v2) {
          // if(v1>minValue[elNr]&&v1<maxValue[elNr]&&v2>minValue[elNr]&&v2<maxValue[elNr]){
          // }

          float v1l = v1;
          float v2l = v2;
          bool noData = false;
          if (dataSource->legendLog != 0) {
            if ((v1 > 0) && (v2 > 0)) {
              v1l = log10(v1l) / log10(dataSource->legendLog);
              v2l = log10(v2l) / log10(dataSource->legendLog);
            } else {
              noData = true;
            }
          }

          v1l *= dataSource->legendScale;
          v1l += dataSource->legendOffset;
          v1l /= 240.0;
          v2l *= dataSource->legendScale;
          v2l += dataSource->legendOffset;
          v2l /= 240.0;
          int y1 = int((1 - v1l) * plotHeight);
          int y2 = int((1 - v2l) * plotHeight);

          if (!noData) {
            lineCanvas.line(x1, y1, x2, y2, 2, color);
          }
        }
      }
    }

    delete ctime;
    CT::string title;
    // GetFeatureInfoResult::Element * e=getFeatureInfoResultList[0]->elements[0];
    // title.print("%s - %s (%s)",e->var_name.c_str(),e->feature_name.c_str(),e->units.c_str());
    // plotCanvas.drawText(int(plotWidth/2-float(title.length())*2.5),22,fontLocation,10,0,title.c_str(),CColor(0,0,0,255),CColor(255,255,255,0));
    for (size_t j = 0; j < plotObjects.size(); j++) {
      CT::string title = plotObjects[j]->name.c_str();
      plotCanvas.drawText(int(plotWidth / 2 - float(title.length()) * 2.5), 15 + j * 10, fontLocation, 8, 0, title.c_str(), CColor(0, 0, 0, 255), CColor(255, 255, 255, 0));
    }

    // GetFeatureInfoResult::Element * e2=getFeatureInfoResultList[getFeatureInfoResultList.size()-1]->elements[0];
    title.print("(%s / %s)", startDateString.c_str(), stopDateString.c_str());
    plotCanvas.drawText(int(plotWidth / 2 - float(title.length()) * 2.5), int(25 + plotHeight + plotOffsetY), fontLocation, 8, 0, title.c_str(), CColor(0, 0, 0, 255), CColor(255, 255, 255, 0));
    plotCanvas.draw(plotOffsetX, plotOffsetY, 0, 0, &lineCanvas);
    if (resultFormat == imagepng) {
      printf("%s%c%c\n", "Content-Type:image/png", 13, 10);
      plotCanvas.printImagePng();
    }
    if (resultFormat == imagegif) {
      printf("%s%c%c\n", "Content-Type:image/gif", 13, 10);
      plotCanvas.printImageGif();
    }
#ifdef MEASURETIME
    StopWatch_Stop("/Start creating image");
#endif

    for (size_t j = 0; j < plotObjects.size(); j++) delete plotObjects[j];
    plotObjects.clear();
    // CDBDebug("Done!");
  }
};
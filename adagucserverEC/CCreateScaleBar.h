#ifndef CCreateScaleBar_H
#define CCreateScaleBar_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CDebugger.h"

class CCreateScaleBar {

public:
  /**
   * Create scalebar creates a scalebar image for given geoParams.
   * @param scaleBarImage The CDrawImage object to write the scalebar to
   * @param geoParams The projection information to create the scalebar from, uses boundingbox and CRS.
   * @param scaling The scaling of the scalebar, 1.0 is natural, 2.0 is twice as big.
   * @return 0 on success nonzero on failure.
   */
  static int createScaleBar(CDrawImage *scaleBarImage, GeoParameters &geoParams, float scaling);

private:
  class Props {
  public:
    int width;
    double mapunits;
  };

  static Props getScaleBarProperties(GeoParameters &geoParams, float scaling);
};
#endif

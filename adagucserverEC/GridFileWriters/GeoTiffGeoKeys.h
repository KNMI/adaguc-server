#ifndef GEOTIFFGEOKEYS_H
#define GEOTIFFGEOKEYS_H

#include <string>
#include <geotiffio.h>

/**
 * Returns true if PROJ understands the CRS string (an authority code like EPSG:4326 or a proj string).
 */
bool isCRSKnownToPROJ(const std::string &crs);

/**
 * Sets the GeoTIFF GeoKeys that describe crs, an authority code like EPSG:4326 or a proj string, in the same way as GDAL does:
 * - CRSs with an EPSG code are written as that EPSG code.
 * - Other geographic and projected CRSs are written as user defined CRS, with datum, ellipsoid, TOWGS84 and projection parameters,
 *   for the projection methods that GeoTIFF can describe.
 * - For other projection methods (for example geostationary) the CRS is written as WKT in the PCSCitationGeoKey (as "ESRI PE String"), which GDAL reads.
 * - If that is not possible either (for example rotated pole), no GeoKeys are written and a warning is logged.
 * Returns 0 on success, 1 if PROJ cannot parse the CRS.
 */
int setGeoKeysFromCRS(GTIF *gtif, const std::string &crs);

#endif

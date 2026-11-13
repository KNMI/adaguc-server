#include "GeoTiffGeoKeys.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <numbers>
#include <vector>
#include <geovalues.h>
#include <proj.h>
#include "CDebugger.h"
#include "CTString.h"

namespace {
  struct PJDeleter {
    void operator()(PJ *pj) const { proj_destroy(pj); }
  };
  typedef std::unique_ptr<PJ, PJDeleter> PJPtr;

  const int EPSG_ANGULAR_UNIT_DEGREE = 9102;
  const double RADIANS_PER_DEGREE = std::numbers::pi / 180.0;

  // GeoTIFF keys for the parameters of a projection method, the parameters are identified by their EPSG parameter code
  struct ProjectionParameterKey {
    int epsgParameterCode;
    geokey_t geoKey;
  };

  struct ProjectionMethod {
    int epsgMethodCode;
    int coordinateTransformation;
    std::vector<ProjectionParameterKey> parameters;
  };

  // EPSG parameter codes
  const int LAT_NATURAL_ORIGIN = 8801, LON_NATURAL_ORIGIN = 8802, SCALE_NATURAL_ORIGIN = 8805, FALSE_EASTING = 8806, FALSE_NORTHING = 8807;
  const int LAT_FALSE_ORIGIN = 8821, LON_FALSE_ORIGIN = 8822, LAT_1ST_PARALLEL = 8823, LAT_2ND_PARALLEL = 8824, EASTING_FALSE_ORIGIN = 8826, NORTHING_FALSE_ORIGIN = 8827;
  const int LAT_STANDARD_PARALLEL = 8832, LON_ORIGIN = 8833;

  // The projection methods GeoTIFF can describe, with the same keys as GDAL writes them
  const std::vector<ProjectionMethod> &projectionMethods() {
    const std::vector<ProjectionParameterKey> naturalOrigin = {{LAT_NATURAL_ORIGIN, ProjNatOriginLatGeoKey},
                                                               {LON_NATURAL_ORIGIN, ProjNatOriginLongGeoKey},
                                                               {SCALE_NATURAL_ORIGIN, ProjScaleAtNatOriginGeoKey},
                                                               {FALSE_EASTING, ProjFalseEastingGeoKey},
                                                               {FALSE_NORTHING, ProjFalseNorthingGeoKey}};
    const std::vector<ProjectionParameterKey> center = {
        {LAT_NATURAL_ORIGIN, ProjCenterLatGeoKey}, {LON_NATURAL_ORIGIN, ProjCenterLongGeoKey}, {FALSE_EASTING, ProjFalseEastingGeoKey}, {FALSE_NORTHING, ProjFalseNorthingGeoKey}};
    static const std::vector<ProjectionMethod> methods = {
        {9807, CT_TransverseMercator, naturalOrigin},
        {9809, CT_ObliqueStereographic, naturalOrigin},
        {9801, CT_LambertConfConic_1SP, naturalOrigin},
        {9804, CT_Mercator, naturalOrigin},
        {9810,
         CT_PolarStereographic,
         {{LAT_NATURAL_ORIGIN, ProjNatOriginLatGeoKey},
          {LON_NATURAL_ORIGIN, ProjStraightVertPoleLongGeoKey},
          {SCALE_NATURAL_ORIGIN, ProjScaleAtNatOriginGeoKey},
          {FALSE_EASTING, ProjFalseEastingGeoKey},
          {FALSE_NORTHING, ProjFalseNorthingGeoKey}}},
        // Polar stereographic variant B: GDAL writes the latitude of the standard parallel as latitude of natural origin
        {9829,
         CT_PolarStereographic,
         {{LAT_STANDARD_PARALLEL, ProjNatOriginLatGeoKey}, {LON_ORIGIN, ProjStraightVertPoleLongGeoKey}, {FALSE_EASTING, ProjFalseEastingGeoKey}, {FALSE_NORTHING, ProjFalseNorthingGeoKey}}},
        {9805,
         CT_Mercator,
         {{LAT_1ST_PARALLEL, ProjStdParallel1GeoKey}, {LON_NATURAL_ORIGIN, ProjNatOriginLongGeoKey}, {FALSE_EASTING, ProjFalseEastingGeoKey}, {FALSE_NORTHING, ProjFalseNorthingGeoKey}}},
        {9802,
         CT_LambertConfConic_2SP,
         {{LAT_FALSE_ORIGIN, ProjFalseOriginLatGeoKey},
          {LON_FALSE_ORIGIN, ProjFalseOriginLongGeoKey},
          {LAT_1ST_PARALLEL, ProjStdParallel1GeoKey},
          {LAT_2ND_PARALLEL, ProjStdParallel2GeoKey},
          {EASTING_FALSE_ORIGIN, ProjFalseOriginEastingGeoKey},
          {NORTHING_FALSE_ORIGIN, ProjFalseOriginNorthingGeoKey}}},
        {9822,
         CT_AlbersEqualArea,
         {{LAT_FALSE_ORIGIN, ProjNatOriginLatGeoKey},
          {LON_FALSE_ORIGIN, ProjNatOriginLongGeoKey},
          {LAT_1ST_PARALLEL, ProjStdParallel1GeoKey},
          {LAT_2ND_PARALLEL, ProjStdParallel2GeoKey},
          {EASTING_FALSE_ORIGIN, ProjFalseEastingGeoKey},
          {NORTHING_FALSE_ORIGIN, ProjFalseNorthingGeoKey}}},
        {9820, CT_LambertAzimEqualArea, center},
        {1027, CT_LambertAzimEqualArea, center},
        {9840, CT_Orthographic, center},
        {1028,
         CT_Equirectangular,
         {{LAT_1ST_PARALLEL, ProjStdParallel1GeoKey}, {LON_NATURAL_ORIGIN, ProjCenterLongGeoKey}, {FALSE_EASTING, ProjFalseEastingGeoKey}, {FALSE_NORTHING, ProjFalseNorthingGeoKey}}},
    };
    return methods;
  }

  const ProjectionMethod *findProjectionMethod(int epsgMethodCode) {
    for (const auto &method: projectionMethods()) {
      if (method.epsgMethodCode == epsgMethodCode) return &method;
    }
    return nullptr;
  }

  // Returns the EPSG code, or 0 if it is not an EPSG code that fits in a GeoKey (codes from 32767 on are user defined or private)
  int toEPSGCode(const char *authority, const char *code) {
    if (authority == nullptr || code == nullptr || strcmp(authority, "EPSG") != 0) return 0;
    int value = atoi(code);
    return value > 0 && value < KvUserDefined ? value : 0;
  }

  // Returns the EPSG code of a PROJ object, or 0 if it has none
  int getEPSGCode(const PJ *obj) { return toEPSGCode(proj_get_id_auth_name(obj, 0), proj_get_id_code(obj, 0)); }

  std::string getName(const PJ *obj) {
    const char *name = obj == nullptr ? nullptr : proj_get_name(obj);
    return name == nullptr ? "unknown" : name;
  }

  // Citation of a user defined geographic CRS in the format of GDAL, which GDAL uses to restore the names
  std::string getGDALGeographicCitation(const PJ *geodeticCRS, const PJ *datum) {
    PJPtr ellipsoid(proj_get_ellipsoid(PJ_DEFAULT_CTX, geodeticCRS));
    PJPtr primeMeridian(proj_get_prime_meridian(PJ_DEFAULT_CTX, geodeticCRS));
    // Like GDAL, leave out the name of an EPSG ellipsoid: GeogEllipsoidGeoKey has its code
    std::string ellipsoidName = ellipsoid != nullptr && getEPSGCode(ellipsoid.get()) != 0 ? "" : "|Ellipsoid = " + getName(ellipsoid.get());
    return "GCS Name = " + getName(geodeticCRS) + "|Datum = " + getName(datum) + ellipsoidName + "|Primem = " + getName(primeMeridian.get()) + "|";
  }

  PJPtr createCRS(const std::string &crs) {
    std::string definition = CT::trim(crs);
    if (CT::startsWith(definition, "+") && definition.find("+type=crs") == std::string::npos) {
      definition += " +type=crs";
    }
    PJPtr pj(proj_create(PJ_DEFAULT_CTX, definition.c_str()));
    if (pj == nullptr || !proj_is_crs(pj.get())) return nullptr;
    return pj;
  }

  void setShortKey(GTIF *gtif, geokey_t key, int value) { GTIFKeySet(gtif, key, TYPE_SHORT, 1, value); }
  void setDoubleKey(GTIF *gtif, geokey_t key, double value) { GTIFKeySet(gtif, key, TYPE_DOUBLE, 1, value); }
  void setAsciiKey(GTIF *gtif, geokey_t key, const std::string &value) { GTIFKeySet(gtif, key, TYPE_ASCII, 0, value.c_str()); }

  void setEllipsoidKeys(GTIF *gtif, const PJ *geodeticCRS, bool userDefined) {
    PJPtr ellipsoid(proj_get_ellipsoid(PJ_DEFAULT_CTX, geodeticCRS));
    if (ellipsoid == nullptr) return;
    double semiMajor = 0, semiMinor = 0, inverseFlattening = 0;
    int semiMinorComputed = 0;
    proj_ellipsoid_get_parameters(PJ_DEFAULT_CTX, ellipsoid.get(), &semiMajor, &semiMinor, &semiMinorComputed, &inverseFlattening);
    if (userDefined) {
      int code = getEPSGCode(ellipsoid.get());
      setShortKey(gtif, GeogEllipsoidGeoKey, code != 0 ? code : KvUserDefined);
    }
    setDoubleKey(gtif, GeogSemiMajorAxisGeoKey, semiMajor);
    if (inverseFlattening == 0) {
      setDoubleKey(gtif, GeogSemiMinorAxisGeoKey, semiMinor); // A sphere
    } else {
      setDoubleKey(gtif, GeogInvFlatteningGeoKey, inverseFlattening);
    }
  }

  double getPrimeMeridianInDegrees(const PJ *geodeticCRS) {
    PJPtr primeMeridian(proj_get_prime_meridian(PJ_DEFAULT_CTX, geodeticCRS));
    double longitude = 0, unitToRadians = 0;
    if (primeMeridian == nullptr || !proj_prime_meridian_get_parameters(PJ_DEFAULT_CTX, primeMeridian.get(), &longitude, &unitToRadians, nullptr)) {
      return 0;
    }
    return longitude * (unitToRadians / RADIANS_PER_DEGREE);
  }

  // The geographic part of the CRS: an EPSG geographic CRS, or a user defined one with datum, ellipsoid, prime meridian and TOWGS84
  void setGeographicKeys(GTIF *gtif, const PJ *geodeticCRS, const std::vector<double> &toWGS84) {
    int code = getEPSGCode(geodeticCRS);
    bool userDefined = code == 0 || !toWGS84.empty();
    if (userDefined) {
      setShortKey(gtif, GeographicTypeGeoKey, KvUserDefined);
      PJPtr datum(proj_crs_get_datum_forced(PJ_DEFAULT_CTX, geodeticCRS));
      int datumCode = datum == nullptr ? 0 : getEPSGCode(datum.get());
      setShortKey(gtif, GeogGeodeticDatumGeoKey, datumCode != 0 ? datumCode : KvUserDefined);
      setAsciiKey(gtif, GeogCitationGeoKey, getGDALGeographicCitation(geodeticCRS, datum.get()));
    } else {
      setShortKey(gtif, GeographicTypeGeoKey, code);
      setAsciiKey(gtif, GeogCitationGeoKey, getName(geodeticCRS));
    }
    setShortKey(gtif, GeogAngularUnitsGeoKey, EPSG_ANGULAR_UNIT_DEGREE);
    setEllipsoidKeys(gtif, geodeticCRS, userDefined);
    if (userDefined) {
      setDoubleKey(gtif, GeogPrimeMeridianLongGeoKey, getPrimeMeridianInDegrees(geodeticCRS));
    }
    if (!toWGS84.empty()) {
      GTIFKeySet(gtif, GeogTOWGS84GeoKey, TYPE_DOUBLE, static_cast<int>(toWGS84.size()), toWGS84.data());
    }
  }

  // Returns the size of the linear unit of the projected CRS in metres, and sets ProjLinearUnitsGeoKey
  double setLinearUnitKeys(GTIF *gtif, const PJ *projectedCRS) {
    PJPtr coordinateSystem(proj_crs_get_coordinate_system(PJ_DEFAULT_CTX, projectedCRS));
    double unitInMetres = 1;
    const char *unitAuthority = nullptr, *unitCode = nullptr;
    if (coordinateSystem == nullptr || !proj_cs_get_axis_info(PJ_DEFAULT_CTX, coordinateSystem.get(), 0, nullptr, nullptr, nullptr, &unitInMetres, nullptr, &unitAuthority, &unitCode)) {
      setShortKey(gtif, ProjLinearUnitsGeoKey, Linear_Meter);
      return 1;
    }
    int code = toEPSGCode(unitAuthority, unitCode);
    if (code != 0) {
      setShortKey(gtif, ProjLinearUnitsGeoKey, code);
    } else {
      setShortKey(gtif, ProjLinearUnitsGeoKey, KvUserDefined);
      setDoubleKey(gtif, ProjLinearUnitSizeGeoKey, unitInMetres);
    }
    return unitInMetres;
  }

  // Sets the projection parameter keys. Angles are written in degrees and lengths in the linear unit of the CRS.
  void setProjectionParameterKeys(GTIF *gtif, const PJ *conversion, const ProjectionMethod &method, double linearUnitInMetres) {
    int count = proj_coordoperation_get_param_count(PJ_DEFAULT_CTX, conversion);
    for (int j = 0; j < count; j++) {
      const char *authority = nullptr, *code = nullptr, *category = nullptr;
      double value = 0, unitConversionFactor = 1;
      proj_coordoperation_get_param(PJ_DEFAULT_CTX, conversion, j, nullptr, &authority, &code, &value, nullptr, &unitConversionFactor, nullptr, nullptr, nullptr, &category);
      int parameterCode = toEPSGCode(authority, code);
      for (const auto &parameter: method.parameters) {
        if (parameter.epsgParameterCode != parameterCode) continue;
        // Divide the conversion factors first, so values in degrees and metres stay exactly the same
        double factor = unitConversionFactor;
        if (category != nullptr && strcmp(category, "angular") == 0) factor = unitConversionFactor / RADIANS_PER_DEGREE;
        if (category != nullptr && strcmp(category, "linear") == 0) factor = unitConversionFactor / linearUnitInMetres;
        setDoubleKey(gtif, parameter.geoKey, value * factor);
      }
    }
    if (method.epsgMethodCode == 9829) {
      setDoubleKey(gtif, ProjScaleAtNatOriginGeoKey, 1.0); // Same as GDAL
    }
  }

  // Writes the CRS as WKT in the PCSCitationGeoKey, which is how GDAL writes CRSs that GeoTIFF cannot describe. Returns false if PROJ cannot export the CRS as WKT1.
  bool setWKTCitationKeys(GTIF *gtif, const PJ *crs, const PJ *geodeticCRS, const std::vector<double> &toWGS84) {
    const char *const options[] = {"MULTILINE=NO", nullptr};
    const char *wkt = proj_as_wkt(PJ_DEFAULT_CTX, crs, PJ_WKT1_GDAL, options);
    if (wkt == nullptr) return false;
    setShortKey(gtif, GTModelTypeGeoKey, KvUserDefined);
    setShortKey(gtif, GTRasterTypeGeoKey, RasterPixelIsArea);
    setAsciiKey(gtif, GTCitationGeoKey, getName(crs));
    if (geodeticCRS != nullptr) setGeographicKeys(gtif, geodeticCRS, toWGS84);
    // GDAL uses this prefix for any WKT1 in the citation
    setAsciiKey(gtif, PCSCitationGeoKey, std::string("ESRI PE String = ") + wkt);
    return true;
  }

  void setEPSGKeys(GTIF *gtif, const PJ *crs, int code, const PJ *geodeticCRS) {
    setShortKey(gtif, GTRasterTypeGeoKey, RasterPixelIsArea);
    if (proj_get_type(crs) == PJ_TYPE_PROJECTED_CRS) {
      setShortKey(gtif, GTModelTypeGeoKey, ModelTypeProjected);
      setAsciiKey(gtif, GTCitationGeoKey, getName(crs));
      if (geodeticCRS != nullptr) setAsciiKey(gtif, GeogCitationGeoKey, getName(geodeticCRS));
      setShortKey(gtif, GeogAngularUnitsGeoKey, EPSG_ANGULAR_UNIT_DEGREE);
      setShortKey(gtif, ProjectedCSTypeGeoKey, code);
      setLinearUnitKeys(gtif, crs);
    } else {
      setShortKey(gtif, GTModelTypeGeoKey, ModelTypeGeographic);
      setGeographicKeys(gtif, crs, {});
    }
  }

  // Returns false if the projection method cannot be described with GeoTIFF keys
  bool setUserDefinedProjectedKeys(GTIF *gtif, const PJ *crs, const PJ *geodeticCRS, const std::vector<double> &toWGS84) {
    PJPtr conversion(proj_crs_get_coordoperation(PJ_DEFAULT_CTX, crs));
    if (conversion == nullptr) return false;
    const char *methodAuthority = nullptr, *methodCode = nullptr;
    proj_coordoperation_get_method_info(PJ_DEFAULT_CTX, conversion.get(), nullptr, &methodAuthority, &methodCode);
    const ProjectionMethod *method = findProjectionMethod(toEPSGCode(methodAuthority, methodCode));
    if (method == nullptr || geodeticCRS == nullptr) return false;
    setShortKey(gtif, GTModelTypeGeoKey, ModelTypeProjected);
    setShortKey(gtif, GTRasterTypeGeoKey, RasterPixelIsArea);
    setAsciiKey(gtif, GTCitationGeoKey, getName(crs));
    setGeographicKeys(gtif, geodeticCRS, toWGS84);
    setShortKey(gtif, ProjectedCSTypeGeoKey, KvUserDefined);
    setShortKey(gtif, ProjectionGeoKey, KvUserDefined);
    setShortKey(gtif, ProjCoordTransGeoKey, method->coordinateTransformation);
    double linearUnitInMetres = setLinearUnitKeys(gtif, crs);
    setProjectionParameterKeys(gtif, conversion.get(), *method, linearUnitInMetres);
    return true;
  }

  // A proj string with +towgs84 gives a BoundCRS: split it into the source CRS and the TOWGS84 parameters
  PJPtr unwrapBoundCRS(PJPtr crs, std::vector<double> &toWGS84) {
    if (proj_get_type(crs.get()) != PJ_TYPE_BOUND_CRS) return crs;
    PJPtr transformation(proj_crs_get_coordoperation(PJ_DEFAULT_CTX, crs.get()));
    double values[7];
    if (transformation != nullptr && proj_coordoperation_get_towgs84_values(PJ_DEFAULT_CTX, transformation.get(), values, 7, false)) {
      toWGS84.assign(values, values + 7);
    }
    PJPtr source(proj_get_source_crs(PJ_DEFAULT_CTX, crs.get()));
    return source != nullptr ? std::move(source) : std::move(crs);
  }
} // namespace

bool isCRSKnownToPROJ(const std::string &crs) { return createCRS(crs) != nullptr; }

int setGeoKeysFromCRS(GTIF *gtif, const std::string &crsString) {
  PJPtr crs = createCRS(crsString);
  if (crs == nullptr) {
    CDBError("Unable to parse CRS [%s]", crsString.c_str());
    return 1;
  }
  std::vector<double> toWGS84;
  crs = unwrapBoundCRS(std::move(crs), toWGS84);
  PJ_TYPE type = proj_get_type(crs.get());
  // A derived geographic CRS, like rotated pole, is not a plain geographic CRS
  bool isGeographic = (type == PJ_TYPE_GEOGRAPHIC_2D_CRS || type == PJ_TYPE_GEOGRAPHIC_3D_CRS) && !proj_is_derived_crs(PJ_DEFAULT_CTX, crs.get());
  PJPtr geodeticCRS(proj_crs_get_geodetic_crs(PJ_DEFAULT_CTX, crs.get()));

  int code = getEPSGCode(crs.get());
  if (code != 0 && toWGS84.empty() && (isGeographic || type == PJ_TYPE_PROJECTED_CRS)) {
    setEPSGKeys(gtif, crs.get(), code, geodeticCRS.get());
    return 0;
  }
  if (isGeographic) {
    setShortKey(gtif, GTModelTypeGeoKey, ModelTypeGeographic);
    setShortKey(gtif, GTRasterTypeGeoKey, RasterPixelIsArea);
    setGeographicKeys(gtif, crs.get(), toWGS84);
    return 0;
  }
  if (type == PJ_TYPE_PROJECTED_CRS && setUserDefinedProjectedKeys(gtif, crs.get(), geodeticCRS.get(), toWGS84)) {
    return 0;
  }
  if (setWKTCitationKeys(gtif, crs.get(), geodeticCRS.get(), toWGS84)) {
    return 0;
  }
  CDBWarning("The CRS [%s] cannot be described in GeoTIFF, the file has no CRS", crsString.c_str());
  return 0;
}

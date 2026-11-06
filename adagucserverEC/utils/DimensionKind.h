#ifndef DIMENSION_KIND_H
#define DIMENSION_KIND_H

#include <string>
#include "CServerConfig_CPPXSD.h"
#include "Types/LayerMetadataType.h"

enum class DimKind { Normal, Time, ReferenceTime };

// The kind of a dimension by its service name only: time, or a name ending with reference_time (like forecast_reference_time).
// Used for requested dimensions, where only the service name is known.
DimKind getDimKindForServiceName(const std::string &serviceName);

// The kind of a configured dimension. A service name for time needs ISO8601 units, otherwise the type attribute and the NetCDF name decide.
// Time dimensions have their values stored as timestamps, so they are ordered by value and advertised as ISO8601.
DimKind getDimKind(const std::string &serviceName, const std::string &cdfName, const std::string &type, const std::string &units);
DimKind getDimKind(const CServerConfig::XMLE_Dimension &cfgDim);
DimKind getDimKind(const LayerMetadataDim &dim);

#endif

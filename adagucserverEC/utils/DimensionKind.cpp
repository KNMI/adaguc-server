#include "DimensionKind.h"

DimKind getDimKindForServiceName(const std::string &serviceName) {
  if (serviceName == "time") return DimKind::Time;
  if (serviceName.ends_with("reference_time")) return DimKind::ReferenceTime;
  return DimKind::Normal;
}

DimKind getDimKind(const std::string &serviceName, const std::string &cdfName, const std::string &type, const std::string &units) {
  const bool isIso8601 = units == "ISO8601";
  const DimKind kindByServiceName = getDimKindForServiceName(serviceName);
  if (kindByServiceName != DimKind::Normal && isIso8601) return kindByServiceName;
  if (type == "dimtype_reference_time" || cdfName.ends_with("reference_time")) return DimKind::ReferenceTime;
  if (type == "dimtype_time" || cdfName == "time") return DimKind::Time;
  // Older configurations name a time dimension anything with time in it, with ISO8601 units
  if (cdfName.find("time") != std::string::npos && isIso8601) return DimKind::Time;
  return DimKind::Normal;
}

DimKind getDimKind(const CServerConfig::XMLE_Dimension &cfgDim) { return getDimKind(cfgDim.elementValue, cfgDim.attr.name, cfgDim.attr.type, cfgDim.attr.units); }

DimKind getDimKind(const LayerMetadataDim &dim) { return getDimKind(dim.serviceName, dim.cdfName, dim.type, dim.units); }

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <charconv>
#include <cmath>
#include <optional>
#include <vector>
#include "CDebugger.h"
#include "CTString.h"
#include "SldHandler.h"

namespace {
  // SLD 1.0.0 has all elements in the SLD namespace. SLD 1.1.0 has the styling elements (Name, Rule, RasterSymbolizer, ...) in the SE namespace.
  struct Namespaces {
    const char *sld;
    const char *se;
  };

  // ShadeIntervals need finite bounds, this stands for minus and plus infinity of an open ended class
  constexpr double OPEN_BOUND = 1e30;
  // The palette of a colorRange Legend runs from index 0 (Min) to 240 (Max)
  constexpr int PALETTE_MAX_INDEX = 240;
  // Names of the generated Legend and Style
  constexpr const char *GENERATED_NAME = "sld_colormap";

  std::string_view nameOf(const xmlNode *node) { return node->name != nullptr ? (const char *)node->name : ""; }

  std::string_view namespaceOf(const xmlNode *node) { return node->ns != nullptr && node->ns->href != nullptr ? (const char *)node->ns->href : ""; }

  bool is(const xmlNode *node, const char *nameSpace, std::string_view name) { return nameOf(node) == name && namespaceOf(node) == nameSpace; }

  // Name, Title, Abstract (SLD 1.0) and Description (SE 1.1) only describe, they do not change the style
  bool isDescription(const xmlNode *node, const Namespaces &ns) {
    std::string_view name = nameOf(node);
    return (namespaceOf(node) == ns.se || namespaceOf(node) == ns.sld) && (name == "Name" || name == "Title" || name == "Abstract" || name == "Description");
  }

  std::vector<const xmlNode *> childElements(const xmlNode *node) {
    std::vector<const xmlNode *> result;
    for (const xmlNode *child = node->children; child != nullptr; child = child->next) {
      if (child->type == XML_ELEMENT_NODE) result.push_back(child);
    }
    return result;
  }

  const xmlNode *childElement(const xmlNode *node, const char *nameSpace, std::string_view name) {
    for (const xmlNode *child: childElements(node)) {
      if (is(child, nameSpace, name)) return child;
    }
    return nullptr;
  }

  std::string textOf(const xmlNode *node) {
    if (node == nullptr) return "";
    xmlChar *content = xmlNodeGetContent(node);
    std::string text = content != nullptr ? CT::trim((const char *)content) : "";
    xmlFree(content);
    return text;
  }

  std::string attributeOf(const xmlNode *node, const char *name) {
    xmlChar *value = xmlGetProp(node, (const xmlChar *)name);
    std::string text = value != nullptr ? CT::trim((const char *)value) : "";
    xmlFree(value);
    return text;
  }

  std::string nameElementOf(const xmlNode *node, const Namespaces &ns) { return textOf(childElement(node, ns.se, "Name")); }

  std::string escape(std::string_view text) {
    std::string result;
    for (char c: text) {
      switch (c) {
      case '&': result += "&amp;"; break;
      case '<': result += "&lt;"; break;
      case '>': result += "&gt;"; break;
      case '"': result += "&quot;"; break;
      default: result += c;
      }
    }
    return result;
  }

  // Shortest text that reads back as the same double
  std::string number(double value) {
    char buffer[32];
    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, end);
  }

  std::string shortNumber(double value) { return CT::printf("%g", value); }

  bool isTrue(const std::string &text) { return text == "1" || text == "true"; }

  // ---- AdagucSymbolizer: ADAGUC style XML ----

  // Writes an element of the AdagucSymbolizer without namespaces, as STYLESHEET XML. Comments are left out.
  void writeWithoutNamespaces(const xmlNode *node, std::string &xml) {
    if (node->type == XML_TEXT_NODE || node->type == XML_CDATA_SECTION_NODE) {
      xml += escape(node->content != nullptr ? (const char *)node->content : "");
      return;
    }
    if (node->type != XML_ELEMENT_NODE) return;
    xml += "<" + std::string(nameOf(node));
    for (const xmlAttr *attribute = node->properties; attribute != nullptr; attribute = attribute->next) {
      xmlChar *value = xmlNodeListGetString(node->doc, attribute->children, 1);
      xml += CT::printf(R"( %s="%s")", (const char *)attribute->name, escape(value != nullptr ? (const char *)value : "").c_str());
      xmlFree(value);
    }
    xml += ">";
    for (const xmlNode *child = node->children; child != nullptr; child = child->next) writeWithoutNamespaces(child, xml);
    xml += "</" + std::string(nameOf(node)) + ">";
  }

  void adagucSymbolizerToStyleSheet(const xmlNode *symbolizer, const Namespaces &ns, std::string &xml) {
    xml.clear();
    for (const xmlNode *node: childElements(symbolizer)) {
      if (!isDescription(node, ns)) writeWithoutNamespaces(node, xml);
    }
  }

  // ---- RasterSymbolizer: a ColorMap, as QGIS and GeoServer make it ----

  struct Color {
    int red, green, blue;
    double opacity;
  };

  struct Stop {
    double value;
    Color color;
    std::string label;
  };

  struct Class {
    double min, max;
    Color color;
    std::string label;
  };

  std::optional<double> parseNumber(std::string_view text) {
    auto value = CT::parseDouble(text);
    if (!value.has_value() || !std::isfinite(*value)) return std::nullopt;
    return value;
  }

  std::optional<double> parseOpacity(const std::string &text) {
    if (text.empty()) return 1.0;
    auto value = parseNumber(text);
    if (!value.has_value() || *value < 0 || *value > 1) return std::nullopt;
    return value;
  }

  // #RRGGBB, as SLD and SE define colours
  std::optional<Color> parseColor(std::string_view text, double opacity) {
    if (text.size() != 7 || text[0] != '#') return std::nullopt;
    int rgb[3];
    for (int i = 0; i < 3; i++) {
      const char *begin = text.data() + 1 + i * 2;
      auto [end, error] = std::from_chars(begin, begin + 2, rgb[i], 16);
      if (error != std::errc() || end != begin + 2) return std::nullopt;
    }
    return Color{rgb[0], rgb[1], rgb[2], opacity};
  }

  int alphaOf(const Color &color) { return (int)std::lround(color.opacity * 255); }

  // A palette from the first to the last value. Values outside get the colour of the nearest end, as in GeoServer.
  bool rampToStyleSheet(const std::vector<Stop> &stops, std::string &xml) {
    if (stops.size() < 2 || !(stops.back().value > stops.front().value)) {
      CDBError("SLD_BODY: a colour ramp needs at least two increasing values");
      return false;
    }
    double first = stops.front().value, last = stops.back().value;
    std::string palette;
    int previousIndex = -1;
    for (size_t i = 0; i < stops.size(); i++) {
      if (i > 0 && stops[i].value < stops[i - 1].value) {
        CDBError("SLD_BODY: colour ramp values must be increasing");
        return false;
      }
      // The palette has 241 steps between the first and the last value. Two values at the same step: the last one wins.
      int index = (int)std::lround((stops[i].value - first) / (last - first) * PALETTE_MAX_INDEX);
      if (index == previousIndex) palette.erase(palette.rfind("<palette"));
      const Color &color = stops[i].color;
      palette += CT::printf(R"(<palette index="%d" red="%d" green="%d" blue="%d" alpha="%d"/>)", index, color.red, color.green, color.blue, alphaOf(color));
      previousIndex = index;
    }
    xml = CT::printf(R"(<Legend name="%s" type="colorRange">%s</Legend><Style name="%s"><Legend>%s</Legend><Min>%s</Min><Max>%s</Max></Style>)", GENERATED_NAME, palette.c_str(),
                     GENERATED_NAME, GENERATED_NAME, number(first).c_str(), number(last).c_str());
    return true;
  }

  // ShadeIntervals, which include their min and exclude their max
  bool classesToStyleSheet(const std::vector<Class> &classes, std::string &xml) {
    std::string intervals;
    for (const auto &c: classes) {
      if (!(c.min < c.max)) {
        CDBError("SLD_BODY: colour map values must be increasing");
        return false;
      }
      intervals += CT::printf(R"(<ShadeInterval min="%s" max="%s" label="%s" fillcolor="#%02X%02X%02X%02X"/>)", number(c.min).c_str(), number(c.max).c_str(), escape(c.label).c_str(), c.color.red,
                              c.color.green, c.color.blue, alphaOf(c.color));
    }
    xml = CT::printf(R"(<Style name="%s">%s<RenderSettings renderhint="discreteclasses"/></Style>)", GENERATED_NAME, intervals.c_str());
    return true;
  }

  // SLD 1.0 ColorMapEntry elements, which QGIS also writes in SLD 1.1, with the GeoServer meaning of type:
  // ramp interpolates, intervals colours the values below an entry's quantity down to the previous quantity
  // (the first entry from minus infinity, nothing from the last quantity up), values colours exactly the quantities.
  bool colorMapEntriesToStyleSheet(const xmlNode *colorMap, const Namespaces &ns, double opacity, std::string &xml) {
    std::vector<Stop> stops;
    for (const xmlNode *entry: childElements(colorMap)) {
      if (!is(entry, ns.se, "ColorMapEntry")) continue;
      auto value = parseNumber(attributeOf(entry, "quantity"));
      auto entryOpacity = parseOpacity(attributeOf(entry, "opacity"));
      // The opacity of an entry overrides the Opacity of the RasterSymbolizer
      auto color = entryOpacity.has_value() ? parseColor(attributeOf(entry, "color"), attributeOf(entry, "opacity").empty() ? opacity : *entryOpacity) : std::nullopt;
      if (!value.has_value() || !color.has_value()) {
        CDBError("SLD_BODY: ColorMapEntry needs a number quantity, a colour #RRGGBB and an opacity from 0 to 1");
        return false;
      }
      stops.push_back({*value, *color, attributeOf(entry, "label")});
    }
    std::string type = attributeOf(colorMap, "type");
    if (type.empty() || type == "ramp") return rampToStyleSheet(stops, xml);
    std::vector<Class> classes;
    if (type == "intervals") {
      for (size_t i = 0; i < stops.size(); i++) {
        double min = i == 0 ? -OPEN_BOUND : stops[i - 1].value;
        std::string label = !stops[i].label.empty() ? stops[i].label
                            : i == 0                ? "< " + shortNumber(stops[i].value)
                                                    : shortNumber(min) + " - " + shortNumber(stops[i].value);
        classes.push_back({min, stops[i].value, stops[i].color, label});
      }
    } else if (type == "values") {
      // A class of one value: from the value to the next larger double
      for (const auto &stop: stops) {
        classes.push_back({stop.value, std::nextafter(stop.value, INFINITY), stop.color, !stop.label.empty() ? stop.label : shortNumber(stop.value)});
      }
    } else {
      CDBError("SLD_BODY: ColorMap type [%s] is not supported, use ramp, intervals or values", type.c_str());
      return false;
    }
    return classesToStyleSheet(classes, xml);
  }

  // SE 1.1 Categorize: Value (Threshold Value)*. By default a threshold belongs to the class above it, as in a ShadeInterval.
  bool categorizeToStyleSheet(const xmlNode *categorize, const Namespaces &ns, double opacity, std::string &xml) {
    std::vector<Color> colors;
    std::vector<double> thresholds;
    for (const xmlNode *node: childElements(categorize)) {
      if (is(node, ns.se, "Value")) {
        auto color = parseColor(textOf(node), opacity);
        if (!color.has_value()) {
          CDBError("SLD_BODY: Categorize Value must be a colour #RRGGBB");
          return false;
        }
        colors.push_back(*color);
      } else if (is(node, ns.se, "Threshold")) {
        auto threshold = parseNumber(textOf(node));
        if (!threshold.has_value()) {
          CDBError("SLD_BODY: Categorize Threshold must be a number");
          return false;
        }
        thresholds.push_back(*threshold);
      }
    }
    if (colors.size() != thresholds.size() + 1) {
      CDBError("SLD_BODY: Categorize needs one Value more than Thresholds");
      return false;
    }
    // threshholdsBelongTo is the spelling of SE 1.1
    bool preceding = attributeOf(categorize, "threshholdsBelongTo") == "preceding" || attributeOf(categorize, "thresholdsBelongTo") == "preceding";
    // A threshold that belongs to the class below moves up by the smallest step, as ShadeIntervals include their min
    auto bound = [&](double threshold) { return preceding ? std::nextafter(threshold, INFINITY) : threshold; };
    std::string below = preceding ? "<= " : "< ", above = preceding ? "> " : ">= ";
    std::vector<Class> classes;
    for (size_t i = 0; i < colors.size(); i++) {
      double min = i == 0 ? -OPEN_BOUND : bound(thresholds[i - 1]);
      double max = i == thresholds.size() ? OPEN_BOUND : bound(thresholds[i]);
      std::string label = thresholds.empty()       ? "all"
                          : i == 0                 ? below + shortNumber(thresholds[0])
                          : i == thresholds.size() ? above + shortNumber(thresholds.back())
                                                   : shortNumber(thresholds[i - 1]) + " - " + shortNumber(thresholds[i]);
      classes.push_back({min, max, colors[i], label});
    }
    return classesToStyleSheet(classes, xml);
  }

  // SE 1.1 Interpolate: InterpolationPoints with Data and Value
  bool interpolateToStyleSheet(const xmlNode *interpolate, const Namespaces &ns, double opacity, std::string &xml) {
    if (attributeOf(interpolate, "method") == "numeric") {
      CDBError("SLD_BODY: Interpolate method numeric is not supported, use color");
      return false;
    }
    std::vector<Stop> stops;
    for (const xmlNode *point: childElements(interpolate)) {
      if (!is(point, ns.se, "InterpolationPoint")) continue;
      auto value = parseNumber(textOf(childElement(point, ns.se, "Data")));
      auto color = parseColor(textOf(childElement(point, ns.se, "Value")), opacity);
      if (!value.has_value() || !color.has_value()) {
        CDBError("SLD_BODY: InterpolationPoint needs a number Data and a colour #RRGGBB Value");
        return false;
      }
      stops.push_back({*value, *color, ""});
    }
    return rampToStyleSheet(stops, xml);
  }

  // Only Opacity and ColorMap are used. ChannelSelection and ContrastEnhancement are ignored: ADAGUC draws one variable
  // with the colour map. ShadedRelief, OverlapBehavior and ImageOutline are ignored, as in GeoServer.
  bool rasterSymbolizerToStyleSheet(const xmlNode *symbolizer, const Namespaces &ns, std::string &xml) {
    auto opacity = parseOpacity(textOf(childElement(symbolizer, ns.se, "Opacity")));
    if (!opacity.has_value()) {
      CDBError("SLD_BODY: Opacity must be a number from 0 to 1");
      return false;
    }
    const xmlNode *colorMap = childElement(symbolizer, ns.se, "ColorMap");
    if (colorMap == nullptr) {
      CDBError("SLD_BODY: RasterSymbolizer needs a ColorMap");
      return false;
    }
    if (const xmlNode *categorize = childElement(colorMap, ns.se, "Categorize")) return categorizeToStyleSheet(categorize, ns, *opacity, xml);
    if (const xmlNode *interpolate = childElement(colorMap, ns.se, "Interpolate")) return interpolateToStyleSheet(interpolate, ns, *opacity, xml);
    if (childElement(colorMap, ns.se, "ColorMapEntry") != nullptr) return colorMapEntriesToStyleSheet(colorMap, ns, *opacity, xml);
    CDBError("SLD_BODY: ColorMap needs ColorMapEntry, Categorize or Interpolate elements");
    return false;
  }

  // ---- Layers and styles ----

  // A NamedStyle refers to a configured style
  bool namedStyleToStyleSheet(const xmlNode *namedStyle, const Namespaces &ns, std::string &xml) {
    std::string name = nameElementOf(namedStyle, ns);
    if (name.empty()) {
      CDBError("SLD_BODY: NamedStyle needs a Name");
      return false;
    }
    xml = CT::printf(R"(<Style name="%s"><IncludeStyle name="%s"/></Style>)", escape(name).c_str(), escape(name).c_str());
    return true;
  }

  // A UserStyle with exactly one RasterSymbolizer or AdagucSymbolizer, in a Rule of a CoverageStyle or FeatureTypeStyle
  bool userStyleToStyleSheet(const xmlNode *userStyle, const Namespaces &ns, std::string &xml) {
    std::vector<const xmlNode *> symbolizers;
    for (const xmlNode *featureStyle: childElements(userStyle)) {
      if (isDescription(featureStyle, ns) || is(featureStyle, ns.sld, "IsDefault")) continue;
      if (!is(featureStyle, ns.se, "CoverageStyle") && !is(featureStyle, ns.se, "FeatureTypeStyle")) {
        CDBError("SLD_BODY: [%s] in UserStyle is not supported, use CoverageStyle or FeatureTypeStyle", (const char *)featureStyle->name);
        return false;
      }
      for (const xmlNode *rule: childElements(featureStyle)) {
        if (isDescription(rule, ns) || is(rule, ns.se, "FeatureTypeName") || is(rule, ns.se, "CoverageName") || is(rule, ns.se, "SemanticTypeIdentifier")) continue;
        if (!is(rule, ns.se, "Rule")) {
          CDBError("SLD_BODY: [%s] in %s is not supported", (const char *)rule->name, (const char *)featureStyle->name);
          return false;
        }
        for (const xmlNode *node: childElements(rule)) {
          if (isDescription(node, ns)) continue;
          // Filters, scale denominators and legend graphics would change what is drawn: they are refused, not ignored
          if (!is(node, ns.se, "RasterSymbolizer") && !is(node, ADAGUC_SLD_NAMESPACE, "AdagucSymbolizer")) {
            CDBError("SLD_BODY: [%s] in Rule is not supported, use RasterSymbolizer or AdagucSymbolizer (namespace %s)", (const char *)node->name, ADAGUC_SLD_NAMESPACE);
            return false;
          }
          symbolizers.push_back(node);
        }
      }
    }
    if (symbolizers.size() != 1) {
      CDBError("SLD_BODY: a UserStyle needs exactly one RasterSymbolizer or AdagucSymbolizer, got %zu", symbolizers.size());
      return false;
    }
    const xmlNode *symbolizer = symbolizers.front();
    if (nameOf(symbolizer) == "RasterSymbolizer") return rasterSymbolizerToStyleSheet(symbolizer, ns, xml);
    adagucSymbolizerToStyleSheet(symbolizer, ns, xml);
    return true;
  }

  SldResult readStyledLayerDescriptor(const xmlNode *root, std::string_view sldVersion, std::string_view layerName, std::string_view styleName, SldStyle &sldStyle) {
    if (root == nullptr || !is(root, SLD_NAMESPACE, "StyledLayerDescriptor")) {
      CDBError("SLD_BODY: root element must be StyledLayerDescriptor in namespace %s", SLD_NAMESPACE);
      return SldResult::invalid;
    }
    std::string version = attributeOf(root, "version");
    if (version != "1.0.0" && version != "1.1.0") {
      CDBError("SLD_BODY: StyledLayerDescriptor version must be 1.0.0 or 1.1.0");
      return SldResult::invalid;
    }
    // SLD_VERSION is mandatory for SLD 1.1.0 (05-078r4 table 7). The WMS profile of SLD 1.0.0 has no SLD_VERSION.
    if (sldVersion.empty() ? version == "1.1.0" : sldVersion != version) {
      CDBError("SLD_BODY: SLD_VERSION must be %s, the version of the StyledLayerDescriptor", version.c_str());
      return SldResult::invalid;
    }
    Namespaces ns = {SLD_NAMESPACE, version == "1.0.0" ? SLD_NAMESPACE : SE_NAMESPACE};

    std::vector<const xmlNode *> namedLayers;
    for (const xmlNode *node: childElements(root)) {
      if (isDescription(node, ns)) continue;
      if (!is(node, ns.sld, "NamedLayer")) {
        CDBError("SLD_BODY: [%s] is not supported, use NamedLayer", (const char *)node->name);
        return SldResult::invalid;
      }
      namedLayers.push_back(node);
    }

    // Library mode: the NamedLayer of the requested layer. Literal mode: the only NamedLayer.
    const xmlNode *namedLayer = nullptr;
    if (layerName.empty()) {
      if (namedLayers.size() != 1) {
        CDBError("SLD_BODY without LAYERS needs exactly one NamedLayer, got %zu", namedLayers.size());
        return SldResult::invalid;
      }
      namedLayer = namedLayers.front();
    } else {
      for (const xmlNode *candidate: namedLayers) {
        if (nameElementOf(candidate, ns) == layerName) namedLayer = candidate;
      }
      if (namedLayer == nullptr) return SldResult::notForLayer;
    }
    sldStyle.layerName = nameElementOf(namedLayer, ns);
    if (sldStyle.layerName.empty()) {
      CDBError("SLD_BODY: NamedLayer needs a Name");
      return SldResult::invalid;
    }

    std::vector<const xmlNode *> styles;
    for (const xmlNode *node: childElements(namedLayer)) {
      if (is(node, ns.sld, "NamedStyle") || is(node, ns.sld, "UserStyle")) styles.push_back(node);
      if (is(node, ns.sld, "LayerFeatureConstraints")) {
        CDBError("SLD_BODY: LayerFeatureConstraints is not supported");
        return SldResult::invalid;
      }
    }

    const xmlNode *style = nullptr;
    if (layerName.empty()) {
      // Literal mode draws the layer with all its styles; one is supported. Without a style the layer's default style is used.
      if (styles.size() > 1) {
        CDBError("SLD_BODY: one style per NamedLayer is supported, got %zu", styles.size());
        return SldResult::invalid;
      }
      if (styles.empty()) {
        sldStyle.styleSheet.clear();
        return SldResult::ok;
      }
      style = styles.front();
    } else {
      // Library mode: the style named in STYLES, or else the one with IsDefault. Otherwise the server's own style is used.
      for (const xmlNode *candidate: styles) {
        bool matches = styleName.empty() ? isTrue(textOf(childElement(candidate, ns.sld, "IsDefault"))) : nameElementOf(candidate, ns) == styleName;
        if (matches && style == nullptr) style = candidate;
      }
      if (style == nullptr) return SldResult::notForLayer;
    }

    bool translated = is(style, ns.sld, "NamedStyle") ? namedStyleToStyleSheet(style, ns, sldStyle.styleSheet) : userStyleToStyleSheet(style, ns, sldStyle.styleSheet);
    return translated ? SldResult::ok : SldResult::invalid;
  }
} // namespace

SldResult readSldBody(const std::string &sld, std::string_view sldVersion, std::string_view layerName, std::string_view styleName, SldStyle &sldStyle) {
  xmlDoc *doc = xmlReadMemory(sld.c_str(), sld.length(), nullptr, nullptr, XML_PARSE_NONET);
  if (doc == nullptr) {
    CDBError("SLD_BODY is not valid XML");
    return SldResult::invalid;
  }
  SldResult result = SldResult::invalid;
  // A DOCTYPE could declare entities
  if (doc->intSubset != nullptr || doc->extSubset != nullptr) {
    CDBError("SLD_BODY: DOCTYPE is not allowed");
  } else {
    result = readStyledLayerDescriptor(xmlDocGetRootElement(doc), sldVersion, layerName, styleName, sldStyle);
  }
  xmlFreeDoc(doc);
  return result;
}

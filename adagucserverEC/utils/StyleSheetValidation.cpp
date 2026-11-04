#include <libxml/parser.h>
#include <libxml/tree.h>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include "CDebugger.h"
#include "CTString.h"
#include "StyleSheetValidation.h"

namespace {

  enum class ValueType {
    Text,        // Free text, length limited
    Number,      // Finite number within [min, max]
    NumberList,  // Comma separated Numbers
    Color,       // #RRGGBB or #RRGGBBAA
    FloatFormat, // printf format, length limited. Every renderer prints it with CT::formatFloat, which falls back to %g for unsafe formats
    PngFile,     // Checked and resolved by CServerParams::applyStyleSheet
    Pattern,     // FeatureInterval regex, without backslashes (no back references or bounds)
    Algorithm,   // DataPostProc algorithm without file or layer access
  };

  struct ValueRule {
    ValueType type;
    double min = -std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::max();
  };

  struct ElementRule {
    std::map<std::string, ValueRule, std::less<>> attributes = {};
    std::optional<ValueRule> text = std::nullopt; // Rule for the element text; other element text is ignored
    std::set<std::string, std::less<>> children = {};
  };

  const double LARGE = std::numeric_limits<double>::max();
  const size_t MAX_TEXT_LENGTH = 1024;
  const size_t MAX_LIST_LENGTH = 1000;
  const ValueRule TEXT{ValueType::Text};
  const ValueRule NUMBER{ValueType::Number};
  const ValueRule COLOR{ValueType::Color};
  const ValueRule FORMAT{ValueType::FloatFormat};
  const ValueRule PNG_FILE{ValueType::PngFile};
  const ValueRule PATTERN{ValueType::Pattern};

  ValueRule range(double min, double max) { return {ValueType::Number, min, max}; }

  // Every element and attribute a client may post. Missing on purpose: file and font attributes, symbol,
  // labelpropertyformat (applied to strings and integers too) and most DataPostProc attributes.
  // IncludeStyle may only name a configured style, see CServerParams::applyStyleSheet.
  const std::map<std::string, ElementRule, std::less<>> &elementRules() {
    static const std::map<std::string, ElementRule, std::less<>> rules = {
        {"Configuration", {.children = {"Style", "Legend"}}},
        {"Style",
         {.attributes = {{"name", TEXT}, {"title", TEXT}, {"abstract", TEXT}},
          .children = {"Thinning", "Point", "Vector", "FilterPoints", "Legend", "Scale", "Offset", "Min", "Max", "ContourIntervalL", "ContourIntervalH", "Log", "ValueRange", "RenderMethod",
                       "ShadeInterval", "SymbolInterval", "ContourLine", "NameMapping", "SmoothingFilter", "StandardNames", "LegendGraphic", "FeatureInterval", "Stippling", "RenderSettings",
                       "DataPostProc", "IncludeStyle"}}},
        {"Legend",
         {.attributes = {{"name", TEXT}, {"type", TEXT}, {"tickround", NUMBER}, {"tickinterval", range(0, LARGE)}, {"fixedclasses", TEXT}, {"fixed", TEXT}, {"textformatting", FORMAT}},
          .text = TEXT,
          .children = {"palette"}}},
        {"palette",
         {.attributes = {{"min", range(0, 255)},
                         {"max", range(0, 255)},
                         {"index", range(0, 255)},
                         {"red", range(0, 255)},
                         {"green", range(0, 255)},
                         {"blue", range(0, 255)},
                         {"alpha", range(-256, 255)}, // Negative means overwrite, see CStyleConfiguration::drawPixelFromPaletteIndex
                         {"color", COLOR}}}},
        {"Thinning", {.attributes = {{"radius", range(0, 1000)}}}},
        {"Point",
         {.attributes = {{"fillcolor", COLOR},
                         {"linecolor", COLOR},
                         {"textcolor", COLOR},
                         {"textoutlinecolor", COLOR},
                         {"fontsize", range(0, 200)},
                         {"discradius", range(0, 1000)},
                         {"textradius", range(0, 1000)},
                         {"dot", TEXT},
                         {"textformat", FORMAT},
                         {"plotstationid", TEXT},
                         {"pointstyle", TEXT},
                         {"min", NUMBER},
                         {"max", NUMBER},
                         {"maxpointspercell", range(0, 1000)},
                         {"maxpointcellsize", range(0, 1000)}}}},
        {"Vector",
         {.attributes = {{"linecolor", COLOR},
                         {"linewidth", range(0, 100)},
                         {"scale", NUMBER},
                         {"discradius", range(0, 1000)},
                         {"vectorstyle", TEXT},
                         {"plotstationid", TEXT},
                         {"textformat", FORMAT},
                         {"plotvalue", TEXT},
                         {"min", NUMBER},
                         {"max", NUMBER},
                         {"fontsize", range(0, 200)},
                         {"outlinewidth", range(0, 100)},
                         {"outlinecolor", COLOR},
                         {"textcolor", COLOR},
                         {"fillcolor", COLOR}}}},
        {"FilterPoints", {.attributes = {{"use", TEXT}}}},
        {"Scale", {.text = NUMBER}},
        {"Offset", {.text = NUMBER}},
        {"Min", {.text = NUMBER}},
        {"Max", {.text = NUMBER}},
        {"Log", {.text = NUMBER}},
        {"ContourIntervalL", {.text = range(0, LARGE)}},
        {"ContourIntervalH", {.text = range(0, LARGE)}},
        {"SmoothingFilter", {.text = range(0, 20)}},
        {"RenderMethod", {.text = TEXT}},
        {"ValueRange", {.attributes = {{"min", NUMBER}, {"max", NUMBER}}}},
        {"ShadeInterval", {.attributes = {{"min", NUMBER}, {"max", NUMBER}, {"label", TEXT}, {"fillcolor", COLOR}, {"bgcolor", COLOR}, {"showinlegend", TEXT}}}},
        {"SymbolInterval",
         {.attributes = {{"min", NUMBER}, {"max", NUMBER}, {"file", PNG_FILE}, {"binary_and", range(0, 2147483647)}, {"offset_x", range(-1000, 1000)}, {"offset_y", range(-1000, 1000)}}}},
        {"ContourLine",
         {.attributes = {{"width", range(0, 100)},
                         {"dashing", {ValueType::NumberList, 0, 1000}},
                         {"linecolor", COLOR},
                         {"textsize", range(0, 200)},
                         {"textcolor", COLOR},
                         {"textstrokecolor", COLOR},
                         {"classes", {ValueType::NumberList}},
                         {"interval", range(std::numeric_limits<double>::min(), LARGE)},
                         {"textformatting", FORMAT},
                         {"textstrokewidth", range(0, 100)},
                         {"textdistance", range(1, 1000)},
                         {"textgap", TEXT},
                         {"textsofthalo", TEXT},
                         {"boldvalues", {ValueType::NumberList}},
                         {"boldwidth", range(0, 100)}}}},
        {"NameMapping", {.attributes = {{"name", TEXT}, {"title", TEXT}, {"abstract", TEXT}}}},
        {"StandardNames", {.attributes = {{"units", TEXT}, {"standard_name", TEXT}, {"variable_name", TEXT}}}},
        {"LegendGraphic", {.attributes = {{"value", PNG_FILE}}}},
        {"FeatureInterval",
         {.attributes = {{"match", PATTERN},
                         {"matchid", PATTERN},
                         {"label", TEXT},
                         {"min", NUMBER},
                         {"max", NUMBER},
                         {"fillcolor", COLOR},
                         {"bgcolor", COLOR},
                         {"bordercolor", COLOR},
                         {"linecolor", COLOR},
                         {"labelcolor", COLOR},
                         {"borderwidth", range(0, 100)},
                         {"linewidth", range(0, 100)},
                         {"labelfontsize", range(0, 200)},
                         {"labelpropertyname", TEXT},
                         {"labelangle", range(-360, 360)},
                         {"labelpadding", range(0, 1000)}}}},
        {"Stippling", {.attributes = {{"distancex", range(0, 1000)}, {"distancey", range(0, 1000)}, {"discradius", range(0, 1000)}, {"mode", TEXT}, {"color", COLOR}}}},
        {"RenderSettings",
         {.attributes = {{"settings", TEXT},
                         {"striding", range(1, 100)},
                         {"renderhint", TEXT},
                         {"scalewidth", range(0, 1000)},
                         {"scalecontours", TEXT},
                         {"randomizefeatures", TEXT},
                         {"featuresoverlap", TEXT},
                         {"rendertextforvectors", TEXT},
                         {"cliplegend", TEXT},
                         {"interpolationmethod", TEXT},
                         {"drawgridboxoutline", TEXT},
                         {"drawgrid", TEXT}}}},
        {"IncludeStyle", {.attributes = {{"name", TEXT}}}},
        {"DataPostProc", {.attributes = {{"algorithm", {ValueType::Algorithm}}, {"a", NUMBER}, {"b", NUMBER}, {"select", TEXT}, {"units", TEXT}}}},
    };
    return rules;
  }

  bool parseNumber(std::string_view text, double &value) {
    std::string trimmed = CT::trim(text);
    if (trimmed.empty()) return false;
    char *end = nullptr;
    value = strtod(trimmed.c_str(), &end);
    return end == trimmed.c_str() + trimmed.size() && std::isfinite(value);
  }

  bool isValidValue(std::string_view value, const ValueRule &rule) {
    if (value.size() > MAX_TEXT_LENGTH) return false;
    switch (rule.type) {
    case ValueType::Text:
    case ValueType::PngFile:
      return true;
    case ValueType::Number: {
      double number;
      return parseNumber(value, number) && number >= rule.min && number <= rule.max;
    }
    case ValueType::NumberList: {
      auto items = CT::split(value, ",");
      if (items.size() > MAX_LIST_LENGTH) return false;
      for (const auto &item: items) {
        if (!isValidValue(item, {ValueType::Number, rule.min, rule.max})) return false;
      }
      return true;
    }
    case ValueType::Color:
      return (value.size() == 7 || value.size() == 9) && value[0] == '#' && value.find_first_not_of("0123456789abcdefABCDEF", 1) == std::string_view::npos;
    case ValueType::FloatFormat:
      return value.size() <= 64;
    case ValueType::Pattern:
      return value.size() <= 256 && value.find('\\') == std::string_view::npos;
    case ValueType::Algorithm:
      return value == "pointsfromgrid" || value == "beaufort" || value == "clipminmax";
    }
    return false;
  }

  std::string textContent(xmlNode *node) {
    std::string text;
    for (xmlNode *child = node->children; child != nullptr; child = child->next) {
      if ((child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) && child->content != nullptr) text += (const char *)child->content;
    }
    return text;
  }

  bool isAllowedElement(xmlNode *node) {
    std::string_view name = (const char *)node->name;
    auto ruleIt = elementRules().find(name);
    if (ruleIt == elementRules().end()) {
      CDBError("STYLESHEET: element [%s] is not allowed", (const char *)node->name);
      return false;
    }
    const ElementRule &rule = ruleIt->second;
    for (xmlAttr *attribute = node->properties; attribute != nullptr; attribute = attribute->next) {
      auto attributeIt = rule.attributes.find(std::string_view((const char *)attribute->name));
      if (attributeIt == rule.attributes.end()) {
        CDBError("STYLESHEET: attribute [%s] in [%s] is not allowed", (const char *)attribute->name, (const char *)node->name);
        return false;
      }
      const char *value = attribute->children != nullptr && attribute->children->content != nullptr ? (const char *)attribute->children->content : "";
      if (!isValidValue(value, attributeIt->second)) {
        CDBError("STYLESHEET: invalid value for attribute [%s] in [%s]", (const char *)attribute->name, (const char *)node->name);
        return false;
      }
    }
    if (rule.text.has_value()) {
      std::string text = CT::trim(textContent(node));
      if (!text.empty() && !isValidValue(text, *rule.text)) {
        CDBError("STYLESHEET: invalid value for [%s]", (const char *)node->name);
        return false;
      }
    }
    for (xmlNode *child = node->children; child != nullptr; child = child->next) {
      if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE || child->type == XML_COMMENT_NODE) continue;
      if (child->type != XML_ELEMENT_NODE || !rule.children.contains(std::string_view((const char *)child->name))) {
        CDBError("STYLESHEET: [%s] is not allowed in [%s]", child->name != nullptr ? (const char *)child->name : "?", (const char *)node->name);
        return false;
      }
      if (!isAllowedElement(child)) return false;
    }
    return true;
  }

} // namespace

bool isAllowedStyleSheet(const std::string &xml) {
  xmlDoc *doc = xmlReadMemory(xml.c_str(), xml.length(), nullptr, nullptr, XML_PARSE_NONET);
  if (doc == nullptr) {
    CDBError("STYLESHEET is not valid XML");
    return false;
  }
  bool allowed = true;
  // A DOCTYPE could declare entities
  if (doc->intSubset != nullptr || doc->extSubset != nullptr) {
    CDBError("STYLESHEET: DOCTYPE is not allowed");
    allowed = false;
  }
  xmlNode *root = xmlDocGetRootElement(doc);
  if (allowed && (root == nullptr || std::string_view((const char *)root->name) != "Configuration")) {
    CDBError("STYLESHEET: root element must be Configuration");
    allowed = false;
  }
  allowed = allowed && isAllowedElement(root);
  xmlFreeDoc(doc);
  return allowed;
}

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include "CColor.h"
#include "CDebugger.h"
#include "CTString.h"
#include "json.hpp"
#include "StyleSheetValidation.h"

namespace {

  enum class ValueType {
    Text,        // Free text, length limited
    Number,      // Finite number within [min, max]
    Integer,     // Whole number within [min, max], for values read with atoi: "100000e-3" is 100 as a number but 100000 for atoi
    NumberList,  // Comma separated Numbers
    Color,       // #RRGGBB or #RRGGBBAA
    FloatFormat, // printf format, length limited. Every renderer prints it with CT::formatFloat, which falls back to %g for unsafe formats
    PngFile,     // Checked and resolved by CServerParams::applyStyleSheet
    Pattern,     // FeatureInterval regex, without backslashes (no back references or bounds)
    Algorithm,   // DataPostProc algorithm without file or layer access. CServerParams::applyStyleSheet allows name only for add_dataobject
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
  ValueRule intRange(double min, double max) { return {ValueType::Integer, min, max}; }

  // Every element and attribute a client may post. Missing on purpose: file and font attributes, symbol,
  // labelpropertyformat (applied to strings and integers too) and most DataPostProc algorithms and attributes.
  // IncludeStyle may only name a configured style, see CServerParams::applyStyleSheet.
  // Element and attribute names follow CServerConfig_CPPXSD.h. An attribute added there is refused for STYLESHEET until it is added here.
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
         {.attributes = {{"min", intRange(0, 255)},
                         {"max", intRange(0, 255)},
                         {"index", intRange(0, 255)},
                         {"red", intRange(0, 255)},
                         {"green", intRange(0, 255)},
                         {"blue", intRange(0, 255)},
                         {"alpha", intRange(-256, 255)}, // Negative means overwrite, see CStyleConfiguration::drawPixelFromPaletteIndex
                         {"color", COLOR}}}},
        {"Thinning", {.attributes = {{"radius", intRange(0, 1000)}}}},
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
                         {"maxpointspercell", intRange(0, 1000)},
                         {"maxpointcellsize", intRange(0, 1000)}}}},
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
        {"Log", {.text = range(0, LARGE)}},
        {"ContourIntervalL", {.text = range(0, LARGE)}},
        {"ContourIntervalH", {.text = range(0, LARGE)}},
        {"SmoothingFilter", {.text = intRange(0, 20)}},
        {"RenderMethod", {.text = TEXT}},
        {"ValueRange", {.attributes = {{"min", NUMBER}, {"max", NUMBER}}}},
        // The element text is the interval of a discrete legend
        {"ShadeInterval", {.attributes = {{"min", NUMBER}, {"max", NUMBER}, {"label", TEXT}, {"fillcolor", COLOR}, {"bgcolor", COLOR}, {"showinlegend", TEXT}}, .text = range(0, LARGE)}},
        {"SymbolInterval",
         {.attributes = {{"min", NUMBER}, {"max", NUMBER}, {"file", PNG_FILE}, {"binary_and", intRange(0, 2147483647)}, {"offset_x", intRange(-1000, 1000)}, {"offset_y", intRange(-1000, 1000)}}}},
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
                         {"textstrokewidth", range(0, 100)}}}},
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
        {"Stippling", {.attributes = {{"distancex", intRange(0, 1000)}, {"distancey", intRange(0, 1000)}, {"discradius", intRange(0, 1000)}, {"mode", TEXT}, {"color", COLOR}}}},
        {"RenderSettings",
         {.attributes = {{"settings", TEXT},
                         {"striding", intRange(1, 100)},
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
        {"DataPostProc", {.attributes = {{"algorithm", {ValueType::Algorithm}}, {"a", NUMBER}, {"b", NUMBER}, {"select", TEXT}, {"units", TEXT}, {"from_units", TEXT}, {"name", TEXT}}}},
    };
    return rules;
  }

  // The text as parseConfig reads it (CXMLSerializerInterface.cpp): only a first child that is text. Text after a comment
  // or CDATA is not read, so it must not be validated either: <Max>1<!---->e9</Max> would pass as 1e9 and be used as 1.
  std::string textContent(xmlNode *node) {
    xmlNode *child = node->children;
    return child != nullptr && child->type == XML_TEXT_NODE && child->content != nullptr ? (const char *)child->content : "";
  }

  // Walks the Style and Legend elements. A posted STYLESHEET is untrusted: everything must be allowed, and the walk stops at the first issue.
  // Configured styles are trusted: only the values of known elements and attributes are checked, and every issue is reported.
  struct StyleChecker {
    bool configured;
    std::string datasetName = ""; // Configured only, for the lint messages
    std::string context = "";     // The Style or Legend being checked
    int issues = 0;

    bool done() const { return !configured && issues > 0; }

    void report(const std::string &message) {
      issues++;
      if (configured) {
        CDBLint("ERROR: In dataset \"%s\", %s: %s", datasetName.c_str(), context.c_str(), message.c_str());
      } else {
        CDBError("STYLESHEET: %s", message.c_str());
      }
    }

    bool isValidValue(std::string_view value, const ValueRule &rule) const {
      if (!configured && value.size() > MAX_TEXT_LENGTH) return false;
      switch (rule.type) {
      case ValueType::Text:
      case ValueType::PngFile:
        return true;
      case ValueType::Number: {
        auto number = CT::parseDouble(value);
        if (!number.has_value()) return false;
        // Configurations may use -INF and INF for open intervals
        if (std::isinf(*number)) return configured && rule.min == -LARGE && rule.max == LARGE;
        return *number >= rule.min && *number <= rule.max;
      }
      case ValueType::Integer: {
        // Only whole numbers, so atoi reads the same value
        auto number = CT::parseInt(value);
        return number.has_value() && *number >= rule.min && *number <= rule.max;
      }
      case ValueType::NumberList: {
        auto items = CT::split(value, ",");
        if (!configured && items.size() > MAX_LIST_LENGTH) return false;
        for (const auto &item: items) {
          if (!isValidValue(item, {ValueType::Number, rule.min, rule.max})) return false;
        }
        return true;
      }
      case ValueType::Color:
        return CColor::isValid(value);
      case ValueType::FloatFormat:
        // Posted formats are accepted, CT::formatFloat falls back to %g. A configured one should print as intended.
        return configured ? CT::isSafeFloatFormat(value) : value.size() <= 64;
      case ValueType::Pattern:
        return configured || (value.size() <= 256 && value.find('\\') == std::string_view::npos);
      case ValueType::Algorithm:
        return configured || value == "pointsfromgrid" || value == "beaufort" || value == "clipminmax" || value == "convert_units" || value == "ax+b" || value == "toknots" ||
               value == "windspeed_knots_to_ms" || value == "add_dataobject" || value == "dbztorr" || value == "filter_dataobjects";
      }
      return false;
    }

    void checkElement(xmlNode *node) {
      std::string_view name = (const char *)node->name;
      auto ruleIt = elementRules().find(name);
      if (ruleIt == elementRules().end()) {
        if (!configured) report(CT::printf("element [%s] is not allowed", (const char *)node->name));
        return;
      }
      const ElementRule &rule = ruleIt->second;
      for (xmlAttr *attribute = node->properties; attribute != nullptr && !done(); attribute = attribute->next) {
        auto attributeIt = rule.attributes.find(std::string_view((const char *)attribute->name));
        if (attributeIt == rule.attributes.end()) {
          if (!configured) report(CT::printf("attribute [%s] in [%s] is not allowed", (const char *)attribute->name, (const char *)node->name));
          continue;
        }
        const char *value = attribute->children != nullptr && attribute->children->content != nullptr ? (const char *)attribute->children->content : "";
        if (isValidValue(value, attributeIt->second)) continue;
        if (!configured) {
          report(CT::printf("invalid value for attribute [%s] in [%s]", (const char *)attribute->name, (const char *)node->name));
        } else if (attributeIt->second.type == ValueType::FloatFormat) {
          report(CT::printf("unsupported format [%s] for attribute [%s] in [%s], %%g is used instead", value, (const char *)attribute->name, (const char *)node->name));
        } else {
          report(CT::printf("invalid value [%s] for attribute [%s] in [%s]", value, (const char *)attribute->name, (const char *)node->name));
        }
      }
      if (rule.text.has_value() && !done()) {
        std::string text = CT::trim(textContent(node));
        if (!text.empty() && !isValidValue(text, *rule.text)) {
          report(configured ? CT::printf("invalid value [%s] for [%s]", text.c_str(), (const char *)node->name) : CT::printf("invalid value for [%s]", (const char *)node->name));
        }
      }
      for (xmlNode *child = node->children; child != nullptr && !done(); child = child->next) {
        if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE || child->type == XML_COMMENT_NODE) continue;
        if (child->type != XML_ELEMENT_NODE || !rule.children.contains(std::string_view((const char *)child->name))) {
          if (!configured) report(CT::printf("[%s] is not allowed in [%s]", child->name != nullptr ? (const char *)child->name : "?", (const char *)node->name));
          continue;
        }
        if (node->parent != nullptr && node->parent->type == XML_DOCUMENT_NODE) {
          xmlChar *childName = xmlGetProp(child, (const xmlChar *)"name");
          context = CT::printf("%s [%s]", (const char *)child->name, childName != nullptr ? (const char *)childName : "");
          xmlFree(childName);
        }
        checkElement(child);
      }
    }
  };

  // A posted STYLESHEET is at most root, Style, an array of elements and an element; deeper JSON is refused while it is parsed
  const size_t MAX_JSON_DEPTH = 8;
  const size_t MAX_JSON_BYTES = 64 * 1024;

  // A name from the request as it may appear in a log message: printable ASCII, at most 64 characters
  std::string printable(std::string_view name) {
    std::string result;
    for (char c: name.substr(0, 64)) result += (c >= 0x20 && c < 0x7f) ? c : '?';
    return result;
  }

  bool hasControlCharacters(std::string_view text) {
    return std::any_of(text.begin(), text.end(), [](char c) { return (unsigned char)c < 0x20 && c != '\t' && c != '\n' && c != '\r'; });
  }

  // A parsed JSON value. Strings, numbers and booleans are text, numbers exactly as written: 1e9 stays 1e9.
  struct JsonValue {
    enum class Type { Text, Object, Array } type = Type::Text;
    std::string text;
    std::vector<std::pair<std::string, JsonValue>> members; // In the order of the JSON
    std::vector<JsonValue> items;
  };

  // Parses JSON into a JsonValue. Refuses null, duplicate keys (a repeated element is an array), control characters and
  // nesting deeper than MAX_JSON_DEPTH while parsing.
  class JsonParser : public nlohmann::json_sax<nlohmann::json> {
  public:
    JsonValue root;
    std::string error;

    bool null() override { return fail("null is not allowed"); }
    bool boolean(bool value) override { return addText(value ? "true" : "false"); }
    bool number_integer(number_integer_t value) override { return addText(std::to_string(value)); }
    bool number_unsigned(number_unsigned_t value) override { return addText(std::to_string(value)); }
    bool number_float(number_float_t, const string_t &text) override { return addText(text); }
    bool string(string_t &value) override { return hasControlCharacters(value) ? fail("control characters are not allowed") : addText(value); }
    bool binary(binary_t &) override { return fail("binary values are not allowed"); }
    bool start_object(std::size_t) override { return open(JsonValue::Type::Object); }
    bool end_object() override { return close(); }
    bool start_array(std::size_t) override { return open(JsonValue::Type::Array); }
    bool end_array() override { return close(); }
    bool key(string_t &key) override {
      if (hasControlCharacters(key)) return fail("control characters are not allowed");
      const auto &members = open_.back().members;
      if (std::any_of(members.begin(), members.end(), [&key](const auto &member) { return member.first == key; })) {
        return fail("duplicate key [" + printable(key) + "], use an array for repeated elements");
      }
      keys_.back() = key;
      return true;
    }
    bool parse_error(std::size_t position, const std::string &, const nlohmann::detail::exception &) override {
      return fail("not valid JSON at position " + std::to_string(position));
    }

  private:
    std::vector<JsonValue> open_; // Objects and arrays that are being parsed, innermost last
    std::vector<std::string> keys_;

    bool fail(const std::string &reason) {
      if (error.empty()) error = reason;
      return false;
    }
    bool open(JsonValue::Type type) {
      if (open_.size() >= MAX_JSON_DEPTH) return fail("nested too deep");
      open_.emplace_back();
      open_.back().type = type;
      keys_.emplace_back();
      return true;
    }
    bool close() {
      JsonValue value = std::move(open_.back());
      open_.pop_back();
      keys_.pop_back();
      return add(std::move(value));
    }
    bool addText(std::string text) {
      JsonValue value;
      value.text = std::move(text);
      return add(std::move(value));
    }
    bool add(JsonValue value) {
      if (open_.empty()) {
        root = std::move(value);
      } else if (open_.back().type == JsonValue::Type::Object) {
        open_.back().members.emplace_back(keys_.back(), std::move(value));
      } else {
        open_.back().items.push_back(std::move(value));
      }
      return true;
    }
  };

  // Checks a posted STYLESHEET against the rules and adds its elements to a configuration, as parseConfig does for XML:
  // element text (trimmed), handleValue, the attributes, then the children. Nothing is added that the rules do not allow.
  struct PostedStyleSheetBuilder {
    StyleChecker checker{.configured = false};

    bool refuse(const std::string &message) {
      CDBError("STYLESHEET: %s", message.c_str());
      return false;
    }

    // An element, or one element per item for an array (a repeated element)
    bool addElements(CXMLObjectInterface *parent, const std::string &parentName, const std::string &name, const JsonValue &value) {
      auto parentRule = elementRules().find(parentName);
      if (parentRule == elementRules().end() || !parentRule->second.children.contains(name)) {
        return refuse(CT::printf("[%s] is not allowed in [%s]", printable(name).c_str(), parentName.c_str()));
      }
      if (value.type != JsonValue::Type::Array) return addElement(parent, name, value);
      for (const auto &item: value.items) {
        if (item.type == JsonValue::Type::Array) return refuse(CT::printf("[%s] contains an array in an array", name.c_str()));
        if (!addElement(parent, name, item)) return false;
      }
      return true;
    }

    bool addElement(CXMLObjectInterface *parent, const std::string &name, const JsonValue &value) {
      auto ruleIt = elementRules().find(name);
      if (ruleIt == elementRules().end()) return refuse(CT::printf("element [%s] is not allowed", name.c_str()));
      const ElementRule &rule = ruleIt->second;
      static const std::vector<std::pair<std::string, JsonValue>> noMembers;
      const auto &members = value.type == JsonValue::Type::Object ? value.members : noMembers;

      // Check the text and the attributes before anything is added
      std::string text = value.type == JsonValue::Type::Text ? value.text : "";
      for (const auto &[key, member]: members) {
        if (key == "#text") {
          if (member.type != JsonValue::Type::Text) return refuse(CT::printf("#text of [%s] must be a string or number", name.c_str()));
          text = member.text;
        } else if (key.starts_with("@")) {
          auto attributeIt = rule.attributes.find(std::string_view(key).substr(1));
          if (attributeIt == rule.attributes.end()) {
            return refuse(CT::printf("attribute [%s] in [%s] is not allowed", printable(std::string_view(key).substr(1)).c_str(), name.c_str()));
          }
          if (member.type != JsonValue::Type::Text || !checker.isValidValue(member.text, attributeIt->second)) {
            return refuse(CT::printf("invalid value for attribute [%s] in [%s]", attributeIt->first.c_str(), name.c_str()));
          }
        } else if (key.starts_with("#")) {
          return refuse(CT::printf("[%s] is not allowed in [%s]", printable(key).c_str(), name.c_str()));
        }
      }
      text = CT::trim(text);
      if (!text.empty()) {
        if (!rule.text.has_value()) return refuse(CT::printf("[%s] has no text, use an object with attributes", name.c_str()));
        if (!checker.isValidValue(text, *rule.text)) return refuse(CT::printf("invalid value for [%s]", name.c_str()));
      }

      CXMLObjectInterface *element = parent->addElement(name);
      if (element == nullptr) return refuse(CT::printf("element [%s] can not be added", name.c_str()));
      element->elementValue = text;
      element->handleValue();
      for (const auto &[key, member]: members) {
        if (!key.starts_with("@")) continue;
        if (!element->addAttribute({.name = key.substr(1), .value = member.text})) {
          return refuse(CT::printf("attribute [%s] in [%s] can not be added", key.c_str() + 1, name.c_str()));
        }
      }
      for (const auto &[key, member]: members) {
        if (key.starts_with("@") || key.starts_with("#")) continue;
        if (!addElements(element, name, key, member)) return false;
      }
      return true;
    }
  };

} // namespace

bool buildPostedStyleSheet(const std::string &json, CXMLObjectInterface *configuration) {
  if (json.size() > MAX_JSON_BYTES) {
    CDBError("STYLESHEET is larger than %zu bytes", MAX_JSON_BYTES);
    return false;
  }
  JsonParser parser;
  if (!nlohmann::json::sax_parse(json, &parser)) {
    CDBError("STYLESHEET: %s", parser.error.c_str());
    return false;
  }
  if (parser.root.type != JsonValue::Type::Object) {
    CDBError("STYLESHEET must be a JSON object with Style and Legend");
    return false;
  }
  PostedStyleSheetBuilder builder;
  for (const auto &[key, value]: parser.root.members) {
    if (!builder.addElements(configuration, "Configuration", key, value)) return false;
  }
  return true;
}

int lintConfiguredStyles(const std::string &xml, const std::string &datasetName) {
  xmlDoc *doc = xmlReadMemory(xml.c_str(), xml.length(), nullptr, nullptr, XML_PARSE_NONET);
  if (doc == nullptr) return 0; // Reported by parseConfig
  xmlNode *root = xmlDocGetRootElement(doc);
  StyleChecker checker{.configured = true, .datasetName = datasetName};
  if (root != nullptr && std::string_view((const char *)root->name) == "Configuration") {
    checker.checkElement(root);
  }
  xmlFreeDoc(doc);
  return checker.issues;
}

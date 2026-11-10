#include <algorithm>
#include <cctype>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "json.hpp"
#include "CDebugger.h"
#include "CTString.h"
#include "StyleSheetJson.h"

using ordered_json = nlohmann::ordered_json;

namespace {

  // A stylesheet is at most Configuration/Style/Element; deeper JSON is refused before it is converted
  const int MAX_DEPTH = 8;

  bool isName(std::string_view name) {
    if (name.empty() || !(std::isalpha((unsigned char)name[0]) || name[0] == '_')) return false;
    return std::all_of(name.begin(), name.end(), [](char c) { return std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.'; });
  }

  std::string escape(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char c: text) {
      switch (c) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&apos;";
        break;
      default:
        escaped += c;
      }
    }
    return escaped;
  }

  // The text of a string, number or boolean; false for objects, arrays and null
  bool toText(const ordered_json &value, std::string &text) {
    if (value.is_string()) {
      text = value.get<std::string>();
    } else if (value.is_number() || value.is_boolean()) {
      text = value.dump();
    } else {
      return false;
    }
    return true;
  }

  bool appendElements(const std::string &name, const ordered_json &value, std::string &xml, int depth);

  bool appendElement(const std::string &name, const ordered_json &value, std::string &xml, int depth) {
    if (depth > MAX_DEPTH) {
      CDBError("STYLESHEET JSON is nested too deep");
      return false;
    }
    if (!isName(name)) {
      CDBError("STYLESHEET JSON: [%s] is not a valid element name", name.c_str());
      return false;
    }
    std::string text;
    if (toText(value, text)) {
      xml += "<" + name + ">" + escape(text) + "</" + name + ">";
      return true;
    }
    if (!value.is_object()) {
      CDBError("STYLESHEET JSON: element [%s] must be an object, array, string, number or boolean", name.c_str());
      return false;
    }
    xml += "<" + name;
    for (const auto &[key, item]: value.items()) {
      if (!key.starts_with("@")) continue;
      if (!isName(std::string_view(key).substr(1)) || !toText(item, text)) {
        CDBError("STYLESHEET JSON: attribute [%s] of [%s] must have a valid name and a string, number or boolean value", key.c_str(), name.c_str());
        return false;
      }
      xml += " " + key.substr(1) + "=\"" + escape(text) + "\"";
    }
    xml += ">";
    for (const auto &[key, item]: value.items()) {
      if (key.starts_with("@")) continue;
      if (key == "#text") {
        if (!toText(item, text)) {
          CDBError("STYLESHEET JSON: #text of [%s] must be a string, number or boolean", name.c_str());
          return false;
        }
        xml += escape(text);
      } else if (!appendElements(key, item, xml, depth + 1)) {
        return false;
      }
    }
    xml += "</" + name + ">";
    return true;
  }

  // An array is a repeated element, anything else one element
  bool appendElements(const std::string &name, const ordered_json &value, std::string &xml, int depth) {
    if (!value.is_array()) return appendElement(name, value, xml, depth);
    for (const auto &item: value) {
      if (item.is_array()) {
        CDBError("STYLESHEET JSON: [%s] contains an array in an array", name.c_str());
        return false;
      }
      if (!appendElement(name, item, xml, depth)) return false;
    }
    return true;
  }

  ordered_json elementToJson(xmlNode *element) {
    ordered_json object = ordered_json::object();
    for (xmlAttr *attribute = element->properties; attribute != nullptr; attribute = attribute->next) {
      xmlChar *value = xmlNodeGetContent((xmlNode *)attribute);
      object["@" + std::string((const char *)attribute->name)] = value != nullptr ? std::string((const char *)value) : "";
      xmlFree(value);
    }
    std::string text;
    bool hasChildren = false;
    for (xmlNode *child = element->children; child != nullptr; child = child->next) {
      if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
        text += (const char *)child->content;
      } else if (child->type == XML_ELEMENT_NODE) {
        hasChildren = true;
      }
    }
    text = CT::trim(text);
    if (object.empty() && !hasChildren) return text;
    if (!text.empty()) object["#text"] = text;
    for (xmlNode *child = element->children; child != nullptr; child = child->next) {
      if (child->type != XML_ELEMENT_NODE) continue;
      const std::string name = (const char *)child->name;
      ordered_json value = elementToJson(child);
      if (!object.contains(name)) {
        object[name] = std::move(value);
      } else {
        // A repeated element becomes an array
        if (!object[name].is_array()) object[name] = ordered_json::array({std::move(object[name])});
        object[name].push_back(std::move(value));
      }
    }
    return object;
  }

} // namespace

bool styleSheetJsonToXml(const std::string &json, std::string &xml) {
  const ordered_json root = ordered_json::parse(json, nullptr, false);
  if (root.is_discarded() || !root.is_object()) {
    CDBError("STYLESHEET is not a valid JSON object");
    return false;
  }
  xml.clear();
  if (root.size() == 1 && root.contains("Configuration")) {
    return appendElement("Configuration", root["Configuration"], xml, 0);
  }
  xml = "<Configuration>";
  for (const auto &[key, item]: root.items()) {
    if (key.starts_with("@") || key.starts_with("#")) {
      CDBError("STYLESHEET JSON: [%s] is not allowed at the top level", key.c_str());
      return false;
    }
    if (!appendElements(key, item, xml, 1)) return false;
  }
  xml += "</Configuration>";
  return true;
}

bool styleSheetXmlToJson(const std::string &styleSheetXml, std::string &json, int indent) {
  std::string xml = CT::trim(styleSheetXml);
  if (xml.starts_with("<?xml")) {
    size_t declarationEnd = xml.find("?>");
    xml = declarationEnd == std::string::npos ? "" : CT::trim(std::string_view(xml).substr(declarationEnd + 2));
  }
  if (!xml.starts_with("<Configuration")) xml = "<Configuration>" + xml + "</Configuration>";
  xmlDoc *doc = xmlReadMemory(xml.c_str(), xml.length(), nullptr, nullptr, XML_PARSE_NONET);
  if (doc == nullptr) {
    CDBError("STYLESHEET is not valid XML");
    return false;
  }
  ordered_json result = elementToJson(xmlDocGetRootElement(doc));
  xmlFreeDoc(doc);
  // The Configuration wrapper is left out; an empty stylesheet is an empty object
  if (!result.is_object()) result = ordered_json::object();
  json = result.dump(indent);
  return true;
}

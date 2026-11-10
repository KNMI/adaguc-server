#include <algorithm>
#include <set>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "json.hpp"
#include "CDebugger.h"
#include "StyleSheetJson.h"

using ordered_json = nlohmann::ordered_json;

namespace {

  // A stylesheet is at most Configuration/Style/Element in an array; deeper JSON is refused while it is parsed
  const int MAX_DEPTH = 8;
  // Larger than any request adagucserver accepts through the Python server
  const size_t MAX_JSON_BYTES = 64 * 1024;

  bool isLetter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

  // Element and attribute names, ASCII only: no ':' (namespaces), '!' or '?'
  bool isName(std::string_view name) {
    if (name.empty() || !isLetter(name[0])) return false;
    return std::all_of(name.begin(), name.end(), [](char c) { return isLetter(c) || (c >= '0' && c <= '9') || c == '-' || c == '.'; });
  }

  bool hasControlCharacters(std::string_view text) {
    return std::any_of(text.begin(), text.end(), [](char c) { return (unsigned char)c < 0x20 && c != '\t' && c != '\n' && c != '\r'; });
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
    return !hasControlCharacters(text);
  }

  bool addElements(xmlNode *parent, const std::string &name, const ordered_json &value);

  // Adds the attributes, text and children of a JSON object to an element. The XML library escapes the values.
  // The text goes first, wherever #text is in the JSON: adaguc reads only a text node that is the first child.
  bool fillElement(xmlNode *element, const ordered_json &object) {
    std::string text;
    for (const auto &[key, value]: object.items()) {
      if (key.starts_with("@")) {
        if (!isName(std::string_view(key).substr(1)) || !toText(value, text)) {
          CDBError("STYLESHEET JSON: invalid attribute in [%s]", (const char *)element->name);
          return false;
        }
        xmlNewProp(element, BAD_CAST key.c_str() + 1, BAD_CAST text.c_str());
      } else if (key == "#text") {
        if (!toText(value, text)) {
          CDBError("STYLESHEET JSON: invalid #text in [%s]", (const char *)element->name);
          return false;
        }
        xmlNodeAddContent(element, BAD_CAST text.c_str());
      }
    }
    for (const auto &[key, value]: object.items()) {
      if (!key.starts_with("@") && key != "#text" && !addElements(element, key, value)) return false;
    }
    return true;
  }

  // Adds an element for an object or text, or one element per item for an array (a repeated element)
  bool addElements(xmlNode *parent, const std::string &name, const ordered_json &value) {
    if (!isName(name)) {
      CDBError("STYLESHEET JSON: invalid element name in [%s]", (const char *)parent->name);
      return false;
    }
    if (value.is_array()) {
      for (const auto &item: value) {
        if (item.is_array() || !addElements(parent, name, item)) return false;
      }
      return true;
    }
    // Created without content: content passed to xmlNewChild would be parsed for entity references
    xmlNode *element = xmlNewChild(parent, nullptr, BAD_CAST name.c_str(), nullptr);
    std::string text;
    if (toText(value, text)) {
      xmlNodeAddContent(element, BAD_CAST text.c_str());
      return true;
    }
    if (!value.is_object()) {
      CDBError("STYLESHEET JSON: element [%s] must be an object, array, string, number or boolean", name.c_str());
      return false;
    }
    return fillElement(element, value);
  }

} // namespace

bool styleSheetJsonToXml(const std::string &json, std::string &xml) {
  if (json.size() > MAX_JSON_BYTES) {
    CDBError("STYLESHEET JSON is larger than %zu bytes", MAX_JSON_BYTES);
    return false;
  }
  // Refused while parsing: duplicate keys (a repeated element is an array), control characters in keys and deep nesting
  std::vector<std::set<std::string>> keys;
  bool refused = false;
  auto check = [&](int depth, ordered_json::parse_event_t event, ordered_json &parsed) {
    if (depth > MAX_DEPTH) refused = true;
    if (event == ordered_json::parse_event_t::object_start) keys.emplace_back();
    if (event == ordered_json::parse_event_t::object_end) keys.pop_back();
    if (event == ordered_json::parse_event_t::key) {
      const auto &key = parsed.get_ref<const std::string &>();
      if (hasControlCharacters(key) || !keys.back().insert(key).second) refused = true;
    }
    return !refused;
  };
  const ordered_json root = ordered_json::parse(json, check, false);
  if (refused || root.is_discarded() || !root.is_object()) {
    CDBError("STYLESHEET is not a valid JSON object: invalid JSON, duplicate keys, control characters in keys or nested too deep");
    return false;
  }
  // The elements, optionally wrapped in {"Configuration": {...}}
  const ordered_json &content = root.size() == 1 && root.contains("Configuration") ? root["Configuration"] : root;
  if (!content.is_object()) {
    CDBError("STYLESHEET JSON: Configuration must be an object");
    return false;
  }
  for (const auto &[key, value]: content.items()) {
    if (key.starts_with("@") || key.starts_with("#")) {
      CDBError("STYLESHEET JSON: attributes and #text are not allowed at the top level");
      return false;
    }
  }
  xmlDoc *doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNode *configuration = xmlNewNode(nullptr, BAD_CAST "Configuration");
  xmlDocSetRootElement(doc, configuration);
  bool converted = fillElement(configuration, content);
  if (converted) {
    xmlBuffer *buffer = xmlBufferCreate();
    xmlNodeDump(buffer, doc, configuration, 0, 0);
    xml = (const char *)xmlBufferContent(buffer);
    xmlBufferFree(buffer);
  }
  xmlFreeDoc(doc);
  return converted;
}

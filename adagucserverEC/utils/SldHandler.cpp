#include <libxml/parser.h>
#include <libxml/tree.h>
#include <vector>
#include "CDebugger.h"
#include "CTString.h"
#include "SldHandler.h"

namespace {
  std::string_view nameOf(const xmlNode *node) { return node->name != nullptr ? (const char *)node->name : ""; }

  std::string_view namespaceOf(const xmlNode *node) { return node->ns != nullptr && node->ns->href != nullptr ? (const char *)node->ns->href : ""; }

  bool is(const xmlNode *node, const char *nameSpace, std::string_view name) { return nameOf(node) == name && namespaceOf(node) == nameSpace; }

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

  std::string nameElementOf(const xmlNode *node) { return textOf(childElement(node, SE_NAMESPACE, "Name")); }

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

  bool isTrue(const std::string &text) { return text == "1" || text == "true"; }

  // A NamedStyle refers to a configured style
  bool namedStyleToStyleSheet(const xmlNode *namedStyle, std::string &xml) {
    std::string name = nameElementOf(namedStyle);
    if (name.empty()) {
      CDBError("SLD_BODY: NamedStyle needs a se:Name");
      return false;
    }
    xml = CT::printf(R"(<Style name="%s"><IncludeStyle name="%s"/></Style>)", escape(name).c_str(), escape(name).c_str());
    return true;
  }

  // A UserStyle with exactly one AdagucSymbolizer, in a se:Rule of a se:CoverageStyle or se:FeatureTypeStyle
  bool userStyleToStyleSheet(const xmlNode *userStyle, std::string &xml) {
    std::vector<const xmlNode *> symbolizers;
    for (const xmlNode *featureStyle: childElements(userStyle)) {
      if (is(featureStyle, SE_NAMESPACE, "Name") || is(featureStyle, SE_NAMESPACE, "Description") || is(featureStyle, SLD_NAMESPACE, "IsDefault")) continue;
      if (!is(featureStyle, SE_NAMESPACE, "CoverageStyle") && !is(featureStyle, SE_NAMESPACE, "FeatureTypeStyle")) {
        CDBError("SLD_BODY: [%s] in UserStyle is not supported, use se:CoverageStyle", (const char *)featureStyle->name);
        return false;
      }
      for (const xmlNode *rule: childElements(featureStyle)) {
        if (is(rule, SE_NAMESPACE, "Name") || is(rule, SE_NAMESPACE, "Description")) continue;
        if (!is(rule, SE_NAMESPACE, "Rule")) {
          CDBError("SLD_BODY: [%s] in %s is not supported", (const char *)rule->name, (const char *)featureStyle->name);
          return false;
        }
        for (const xmlNode *node: childElements(rule)) {
          if (is(node, SE_NAMESPACE, "Name") || is(node, SE_NAMESPACE, "Description")) continue;
          // Filters, scale denominators and legend graphics would change what is drawn: they are refused, not ignored
          if (!is(node, ADAGUC_SLD_NAMESPACE, "AdagucSymbolizer")) {
            CDBError("SLD_BODY: [%s] in Rule is not supported, use AdagucSymbolizer in namespace %s", (const char *)node->name, ADAGUC_SLD_NAMESPACE);
            return false;
          }
          symbolizers.push_back(node);
        }
      }
    }
    if (symbolizers.size() != 1) {
      CDBError("SLD_BODY: a UserStyle needs exactly one AdagucSymbolizer, got %zu", symbolizers.size());
      return false;
    }
    // The elements of SymbolizerType, then the ADAGUC Legend and Style elements
    xml.clear();
    for (const xmlNode *node: childElements(symbolizers.front())) {
      if (is(node, SE_NAMESPACE, "Name") || is(node, SE_NAMESPACE, "Description")) continue;
      writeWithoutNamespaces(node, xml);
    }
    return true;
  }

  SldResult readStyledLayerDescriptor(const xmlNode *root, std::string_view layerName, std::string_view styleName, SldStyle &sldStyle) {
    if (root == nullptr || !is(root, SLD_NAMESPACE, "StyledLayerDescriptor")) {
      CDBError("SLD_BODY: root element must be StyledLayerDescriptor in namespace %s", SLD_NAMESPACE);
      return SldResult::invalid;
    }
    xmlChar *version = xmlGetProp(root, (const xmlChar *)"version");
    bool isVersion110 = version != nullptr && std::string_view((const char *)version) == "1.1.0";
    xmlFree(version);
    if (!isVersion110) {
      CDBError("SLD_BODY: StyledLayerDescriptor version must be 1.1.0");
      return SldResult::invalid;
    }

    std::vector<const xmlNode *> namedLayers;
    for (const xmlNode *node: childElements(root)) {
      if (is(node, SE_NAMESPACE, "Name") || is(node, SE_NAMESPACE, "Description")) continue;
      if (!is(node, SLD_NAMESPACE, "NamedLayer")) {
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
        if (nameElementOf(candidate) == layerName) namedLayer = candidate;
      }
      if (namedLayer == nullptr) return SldResult::notForLayer;
    }
    sldStyle.layerName = nameElementOf(namedLayer);
    if (sldStyle.layerName.empty()) {
      CDBError("SLD_BODY: NamedLayer needs a se:Name");
      return SldResult::invalid;
    }

    std::vector<const xmlNode *> styles;
    for (const xmlNode *node: childElements(namedLayer)) {
      if (is(node, SLD_NAMESPACE, "NamedStyle") || is(node, SLD_NAMESPACE, "UserStyle")) styles.push_back(node);
      if (is(node, SLD_NAMESPACE, "LayerFeatureConstraints")) {
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
        bool matches = styleName.empty() ? isTrue(textOf(childElement(candidate, SLD_NAMESPACE, "IsDefault"))) : nameElementOf(candidate) == styleName;
        if (matches && style == nullptr) style = candidate;
      }
      if (style == nullptr) return SldResult::notForLayer;
    }

    bool translated = is(style, SLD_NAMESPACE, "NamedStyle") ? namedStyleToStyleSheet(style, sldStyle.styleSheet) : userStyleToStyleSheet(style, sldStyle.styleSheet);
    return translated ? SldResult::ok : SldResult::invalid;
  }
} // namespace

SldResult readSldBody(const std::string &sld, std::string_view layerName, std::string_view styleName, SldStyle &sldStyle) {
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
    result = readStyledLayerDescriptor(xmlDocGetRootElement(doc), layerName, styleName, sldStyle);
  }
  xmlFreeDoc(doc);
  return result;
}

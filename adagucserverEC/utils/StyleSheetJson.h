#ifndef STYLESHEET_JSON_H
#define STYLESHEET_JSON_H

#include <string>

/**
 * A STYLESHEET can be posted as XML or as JSON. Both describe the same elements, one to one:
 *
 * - A key is an element name. Its value is an object, a scalar for an element with only text, or an array for a repeated element.
 * - A key starting with "@" is an attribute, "#text" is the text of an element that also has attributes or children.
 * - Numbers and booleans are written as they appear in JSON, so {"Min": -5} is <Min>-5</Min>.
 *
 * {"Legend": {"@name": "terrain", "@type": "colorRange", "palette": [{"@index": 0, "@color": "#2B83BA"}, {"@index": 240, "@color": "#7A3B10"}]},
 *  "Style": {"@name": "terrain", "Legend": {"@fixedclasses": "true", "#text": "terrain"}, "Min": -5, "Max": 75}}
 *
 * The elements may be wrapped in {"Configuration": {...}}, like the XML in <Configuration>.
 */

/**
 * Converts a STYLESHEET in JSON to XML wrapped in <Configuration>. The XML is then checked and parsed like a posted XML stylesheet.
 * Logs the reason with CDBError.
 * @return false when the JSON is invalid or does not follow the rules above
 */
bool styleSheetJsonToXml(const std::string &json, std::string &xml);

/**
 * Converts a STYLESHEET in XML (bare elements or wrapped in <Configuration>) to JSON, without the Configuration wrapper.
 * Element text and attribute values become strings. Converting the JSON back gives the same elements, attributes and text.
 * @param indent Indentation of the JSON, -1 for a single line
 * @return false when the XML is invalid
 */
bool styleSheetXmlToJson(const std::string &xml, std::string &json, int indent = -1);

#endif

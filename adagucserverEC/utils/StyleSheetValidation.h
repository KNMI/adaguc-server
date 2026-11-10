#ifndef STYLESHEET_VALIDATION_H
#define STYLESHEET_VALIDATION_H

#include <string>
#include "CXMLSerializerInterface.h"

/**
 * Checks a STYLESHEET in JSON against the allowlist of what a client may post, and adds its Style and Legend elements to
 * configuration, as parseConfig does for the same elements in XML. The JSON uses the element and attribute names of the
 * configuration: a key is an element, "@name" an attribute, "#text" the text of an element that also has attributes, and an
 * array a repeated element. Values are strings, numbers or booleans.
 * {"Legend": {"@name": "l", "@type": "colorRange", "palette": [{"@index": 0, "@color": "#2B83BA"}, {"@index": 240, "@color": "#7A3B10"}]},
 *  "Style": {"@name": "s", "Legend": {"@fixedclasses": "true", "#text": "l"}, "Min": -5, "Max": 75}}
 * Only known Style/Legend elements and attributes are allowed, each with a typed and bounded value. Refused are, amongst others:
 * file paths (except png files, which applyStyleSheet resolves), fonts, DataPostProc with file or layer access, duplicate keys
 * and null. printf formats are accepted, the renderers print them with
 * CT::formatFloat, which falls back to %g for unsafe formats. Logs the reason with CDBError.
 * @return true when the stylesheet is allowed and added
 */
bool buildPostedStyleSheet(const std::string &json, CXMLObjectInterface *configuration);

/**
 * Lints the Style and Legend elements of a configuration file (adagucserver --lint) with the same rules, for values only:
 * configurations are trusted, so files, fonts, all DataPostProc algorithms and unknown attributes (reported by parseConfig) are fine.
 * Also reports printf formats that CT::formatFloat would replace with %g. Logs each issue with CDBLint.
 * @return the number of issues
 */
int lintConfiguredStyles(const std::string &xml, const std::string &datasetName);

#endif

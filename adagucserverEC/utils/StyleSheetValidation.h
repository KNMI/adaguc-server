#ifndef STYLESHEET_VALIDATION_H
#define STYLESHEET_VALIDATION_H

#include <string>

/**
 * Checks a STYLESHEET, wrapped in <Configuration>, against the allowlist of what a client may post:
 * only known Style/Legend elements and attributes, each with a typed and bounded value.
 * Refused are, amongst others: file paths (except png files, which applyStyleSheet resolves), fonts,
 * DataPostProc other than pointsfromgrid/beaufort/clipminmax, and DOCTYPEs. printf formats are accepted,
 * the renderers print them with CT::formatFloat, which falls back to %g for unsafe formats.
 * Logs the reason with CDBError.
 * @return true when the stylesheet may be applied
 */
bool isAllowedStyleSheet(const std::string &xml);

/**
 * Lints the Style and Legend elements of a configuration file (adagucserver --lint) with the same rules, for values only:
 * configurations are trusted, so files, fonts, all DataPostProc algorithms and unknown attributes (reported by parseConfig) are fine.
 * Also reports printf formats that CT::formatFloat would replace with %g. Logs each issue with CDBLint.
 * @return the number of issues
 */
int lintConfiguredStyles(const std::string &xml, const std::string &datasetName);

#endif

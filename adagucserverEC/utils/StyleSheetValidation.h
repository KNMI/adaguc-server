#ifndef STYLESHEET_VALIDATION_H
#define STYLESHEET_VALIDATION_H

#include <string>

/**
 * Checks a STYLESHEET, wrapped in <Configuration>, against the allowlist of what a client may post:
 * only known Style/Legend elements and attributes, each with a typed and bounded value.
 * Refused are, amongst others: file paths (except png files, which applyStyleSheet resolves), fonts,
 * unsafe printf formats, IncludeStyle, DataPostProc other than pointsfromgrid/beaufort/clipminmax, and DOCTYPEs.
 * Logs the reason with CDBError.
 * @return true when the stylesheet may be applied
 */
bool isAllowedStyleSheet(const std::string &xml);

#endif

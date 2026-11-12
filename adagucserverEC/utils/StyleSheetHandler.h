#ifndef STYLESHEET_HANDLER_H
#define STYLESHEET_HANDLER_H

#include <string>
#include <string_view>

/**
 * Decodes a STYLESHEET value that is base64 encoded XML, optionally compressed: raw deflate (as CompressionStream("deflate-raw")
 * in a browser), zlib or gzip. Base64 may be standard or URL safe, with or without padding.
 * Decompressing stops as soon as the XML is larger than 64 KB, so a small value can not expand into a huge one.
 * Logs the reason with CDBError.
 * @param value The STYLESHEET value, not starting with '<'. May refer to xml.
 * @param xml Receives the decoded XML
 * @return true when the value decodes to XML
 */
/**
 * @return true when ADAGUC_STYLESHEET_ENABLE is TRUE: STYLESHEET and SLD_BODY are allowed
 */
bool isStyleSheetEnabled();

bool decodeStyleSheet(std::string_view value, std::string &xml);

#endif

#ifndef STYLESHEET_HANDLER_H
#define STYLESHEET_HANDLER_H

#include <optional>
#include <string>
#include <string_view>

/**
 * Returns the XML of a STYLESHEET value.
 * XML, starting with '<' after whitespace and an optional UTF-8 byte order mark, is returned trimmed, without byte order mark.
 * Any other value is read as base64 encoded XML, optionally compressed: raw deflate (as CompressionStream("deflate-raw")
 * in a browser), zlib or gzip. Base64 may be standard or URL safe, with or without padding. A space in it is read as
 * '+', which a client did not url encode, also at the start or end of the value.
 * Decompressing stops as soon as the XML is larger than MAX_STYLESHEET_BYTES, so a small value can not expand into a huge one.
 * Logs the reason with CDBError when the value can not be decoded.
 * @param value The STYLESHEET value
 * @return The XML (empty for an empty value), or std::nullopt when the value does not decode to XML
 */
std::optional<std::string> styleSheetToXml(std::string_view value);

#endif

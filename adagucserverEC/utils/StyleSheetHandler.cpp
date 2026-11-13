#include <zlib.h>
#include <string_view>
#include "CDebugger.h"
#include "CTString.h"
#include "StyleSheetHandler.h"

// The largest stylesheet after decompressing. The same as the largest request, MAX_FORK_REQUEST_BYTES in
// python/lib/adaguc/fork_settings.py, so a compressed STYLESHEET can not be larger than an uncompressed one could be.
static const size_t MAX_STYLESHEET_BYTES = 65536;

static const std::string_view UTF8_BYTE_ORDER_MARK = "\xEF\xBB\xBF";

enum class Inflated { ok, invalid, tooLarge };

// Inflates deflate data with windowBits 15 + 32 (zlib or gzip header) or -15 (raw, no header).
// Stops as soon as the output is larger than MAX_STYLESHEET_BYTES, so a small value can not expand into a huge one.
static Inflated inflateStyleSheet(std::string_view compressed, int windowBits, std::string &output) {
  output.clear();
  z_stream stream = {};
  if (inflateInit2(&stream, windowBits) != Z_OK) return Inflated::invalid;
  stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.data()));
  stream.avail_in = compressed.size();
  char buffer[16384];
  int status = Z_OK;
  while (status == Z_OK && output.size() <= MAX_STYLESHEET_BYTES) {
    stream.next_out = reinterpret_cast<Bytef *>(buffer);
    stream.avail_out = sizeof(buffer);
    status = inflate(&stream, Z_NO_FLUSH);
    if (status == Z_OK || status == Z_STREAM_END) output.append(buffer, sizeof(buffer) - stream.avail_out);
  }
  inflateEnd(&stream);
  if (output.size() > MAX_STYLESHEET_BYTES) return Inflated::tooLarge;
  // Incomplete data or data after the compressed stream is refused
  return status == Z_STREAM_END && stream.avail_in == 0 ? Inflated::ok : Inflated::invalid;
}

// Removes surrounding whitespace and a UTF-8 byte order mark, which editors on Windows put before the XML
static std::string trimXml(std::string_view xml) {
  std::string trimmed = CT::trim(xml);
  if (trimmed.starts_with(UTF8_BYTE_ORDER_MARK)) return CT::trim(std::string_view(trimmed).substr(UTF8_BYTE_ORDER_MARK.size()));
  return trimmed;
}

std::optional<std::string> styleSheetToXml(std::string_view value) {
  std::string xml = trimXml(value);
  if (xml.empty() || xml.starts_with("<")) return xml;

  // Base64 has no spaces: a space is a '+' that was not url encoded. The value is not trimmed first, so a '+' at its
  // start or end is kept.
  std::string decoded;
  if (!CT::decodeBase64(CT::replace(value, " ", "+"), decoded)) {
    CDBError("STYLESHEET: invalid base64");
    return std::nullopt;
  }
  // Decompress first: compressed data can also start with '<', for example raw deflate starting with a non-final dynamic
  // block (byte 0x3C). Only when it is no compressed data, it can be uncompressed XML.
  std::string inflated;
  Inflated result = inflateStyleSheet(decoded, 15 + 32, inflated);
  if (result == Inflated::invalid) result = inflateStyleSheet(decoded, -15, inflated);
  if (result == Inflated::tooLarge) {
    CDBError("STYLESHEET: decompressed size exceeds %zu bytes", MAX_STYLESHEET_BYTES);
    return std::nullopt;
  }
  xml = trimXml(result == Inflated::ok ? inflated : decoded);
  if (!xml.starts_with("<")) {
    CDBError(result == Inflated::ok ? "STYLESHEET: decoded value is not XML" : "STYLESHEET: invalid compressed data");
    return std::nullopt;
  }
  return xml;
}

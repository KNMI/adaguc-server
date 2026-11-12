#include <zlib.h>
#include <string_view>
#include "CDebugger.h"
#include "CTString.h"
#include "StyleSheetHandler.h"

// The size limit of a request, so a compressed STYLESHEET can not be larger than an uncompressed one
static const size_t MAX_STYLESHEET_BYTES = 65536;

bool isStyleSheetEnabled() {
  const char *styleSheetEnable = getenv("ADAGUC_STYLESHEET_ENABLE");
  return styleSheetEnable != nullptr && std::string_view(styleSheetEnable) == "TRUE";
}

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

bool decodeStyleSheet(std::string_view value, std::string &xml) {
  // value may refer to xml, it is only read before xml is written.
  // A '+' that was not url encoded arrives as a space, base64 has no spaces.
  std::string decoded;
  if (!CT::decodeBase64(CT::replace(value, " ", "+"), decoded)) {
    CDBError("STYLESHEET: invalid base64");
    return false;
  }
  // Decompress first: compressed data can also start with '<', for example raw deflate starting with a non-final dynamic
  // block (byte 0x3C). Only when it is no compressed data, it can be uncompressed XML.
  Inflated result = inflateStyleSheet(decoded, 15 + 32, xml);
  if (result == Inflated::invalid) result = inflateStyleSheet(decoded, -15, xml);
  if (result == Inflated::tooLarge) {
    CDBError("STYLESHEET: decompressed size exceeds %zu bytes", MAX_STYLESHEET_BYTES);
    return false;
  }
  xml = CT::trim(result == Inflated::ok ? xml : decoded);
  if (!xml.starts_with("<")) {
    CDBError(result == Inflated::ok ? "STYLESHEET: decoded value is not XML" : "STYLESHEET: invalid compressed data");
    return false;
  }
  return true;
}

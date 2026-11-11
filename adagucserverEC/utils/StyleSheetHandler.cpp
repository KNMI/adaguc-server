#include <zlib.h>
#include <string_view>
#include "CDebugger.h"
#include "CTString.h"
#include "StyleSheetHandler.h"

// The size limit of a request, so a compressed STYLESHEET can not be larger than an uncompressed one
static const size_t MAX_STYLESHEET_BYTES = 65536;

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

// A '+' that was not url encoded arrives as a space, base64 has no spaces.
bool decodeStyleSheet(std::string value, std::string &xml) {
  std::string decoded;
  if (!CT::decodeBase64(CT::replace(value, " ", "+"), decoded)) {
    CDBError("STYLESHEET: invalid base64");
    return false;
  }
  xml = CT::trim(decoded);
  // Not compressed: compressed data never starts with '<'
  if (xml.starts_with("<")) return true;
  Inflated result = inflateStyleSheet(decoded, 15 + 32, xml);
  if (result == Inflated::invalid) result = inflateStyleSheet(decoded, -15, xml);
  if (result == Inflated::tooLarge) {
    CDBError("STYLESHEET: decompressed size exceeds %zu bytes", MAX_STYLESHEET_BYTES);
    return false;
  }
  if (result == Inflated::invalid) {
    CDBError("STYLESHEET: invalid compressed data");
    return false;
  }
  xml = CT::trim(xml);
  if (!xml.starts_with("<")) {
    CDBError("STYLESHEET: decoded value is not XML");
    return false;
  }
  return true;
}

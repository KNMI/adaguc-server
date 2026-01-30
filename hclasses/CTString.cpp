#include "CTString.h"

#include "CDebugger.h"
#include <regex>
#define CT_STRING_PRINT_BUFFER_SIZE 64

const char *strrstr(const char *x, const char *y) {
  const char *prev = nullptr;
  const char *next;
  if (*y == '\0') return strchr(x, '\0');
  while ((next = strstr(x, y)) != nullptr) {
    prev = next;
    x = next + 1;
  }
  return prev;
}

/**
 * Converts 0-15 to 0-F
 */
char tohex(char in) {
  in += 48;
  if (in > 57) in += 7;
  return in;
}

char fromhex(char in) {
  /* From lowercase to uppercase */
  if (in > 96) in -= 32;
  /* From number character to numeric value */
  in -= 48;
  /* When numeric value is more than 16 (eg ABCDEF) substract 7 to get numeric value 10,11,12,etc... */
  if (in > 16) in -= 7;
  return in;
}
namespace CT {

  string::string() = default;
  string &string::operator=(string const &f) = default;
  string::string(string const &f) = default;

  string::string(const char *value) {
    if (value != nullptr) stdstring = value;
  }

  string::string(CT::string *_string) { stdstring = *_string; }

  string::string(const char *_value, size_t _length) { copy(_value, _length); }

  string &string::operator=(const char *const &f) {
    this->stdstring = f == nullptr ? "" : f;
    return *this;
  }

  string &string::operator+=(std::string const &f) {
    this->stdstring += f;
    return *this;
  }

  string string::operator+(std::string const &f) {
    CT::string n(*this);
    n += f;
    return n;
  }

  char string::charAt(size_t n) {
    if (n > length()) return 0;
    return (stdstring.c_str())[n];
  }

  int string::indexOf(const char *search) {
    auto _length = strlen(search);
    if (_length == 0) return -1;
    if (length() == 0) return -1;
    auto value = stdstring.c_str();
    auto pi = strstr(value, search);
    if (pi == nullptr) return -1;
    auto c = pi - value;
    if (c < 0) c = -1;
    return c;
  }

  int string::lastIndexOf(const char *search) {
    auto _length = strlen(search);
    if (_length == 0) return -1;
    if (length() == 0) return -1;
    auto value = stdstring.c_str();
    auto pi = strrstr(value, search);
    if (pi == nullptr) return -1;
    auto c = pi - value;
    if (c < 0) c = -1;
    return c;
  }

  void string::copy(const char *_value, size_t _length) {
    if (_value == nullptr) {
      this->stdstring = "";
      return;
    }
    this->stdstring.assign(_value, _length);
  }

  void string::toLowerCaseSelf() {
    std::transform(stdstring.begin(), stdstring.end(), stdstring.begin(), [](unsigned char c) { return std::tolower(c); });
  }

  void string::toUpperCaseSelf() {
    std::transform(stdstring.begin(), stdstring.end(), stdstring.begin(), [](unsigned char c) { return std::toupper(c); });
  }

  void string::encodeURLSelf() {
    char *pszEncode = new char[stdstring.length() * 6 + 1];
    int p = 0;
    unsigned char szChar;
    const char *value = stdstring.c_str();
    for (unsigned int j = 0; j < stdstring.length(); j++) {
      szChar = value[j];
      if (szChar < 48 || (szChar > 59 && szChar < 63)) {
        pszEncode[p++] = '%';
        pszEncode[p++] = tohex(szChar / 16);
        pszEncode[p++] = tohex(szChar % 16);
      } else {
        pszEncode[p++] = szChar;
      }
    }
    copy(pszEncode, p);
    delete[] pszEncode;
  }

  void string::decodeURLSelf() {
    char *pszDecode = new char[stdstring.length() * 6 + 1];
    int p = 0;
    unsigned char szChar, d1, d2;
    replaceSelf("+", " ");
    const char *value = stdstring.c_str();
    for (unsigned int j = 0; j < stdstring.length(); j++) {
      szChar = value[j];
      if (szChar == '%') {
        d1 = fromhex(value[j + 1]);
        d2 = fromhex(value[j + 2]);
        pszDecode[p++] = d1 * 16 + d2;
        j = j + 2;
      } else {
        pszDecode[p++] = szChar;
      }
    }
    copy(pszDecode, p);

    delete[] pszDecode;
  }

  void string::print(const char *a, ...) {
    std::vector<char> buf(CT_STRING_PRINT_BUFFER_SIZE + 1);
    va_list ap;
    va_start(ap, a);
    int numWritten = vsnprintf(&buf[0], buf.size(), a, ap);
    va_end(ap);
    if (numWritten > CT_STRING_PRINT_BUFFER_SIZE) {
      buf.resize(numWritten + 1);
      va_list ap;
      va_start(ap, a);
      vsnprintf(&buf[0], buf.size(), a, ap);
      va_end(ap);
    }
    this->stdstring = std::string(buf.begin(), buf.end()).c_str();
  }

  void string::printconcat(const char *a, ...) {
    std::vector<char> buf(CT_STRING_PRINT_BUFFER_SIZE + 1);
    va_list ap;
    va_start(ap, a);
    int numWritten = vsnprintf(&buf[0], buf.size(), a, ap);
    va_end(ap);
    if (numWritten > CT_STRING_PRINT_BUFFER_SIZE) {
      buf.resize(numWritten + 1);
      va_list ap;
      va_start(ap, a);
      vsnprintf(&buf[0], buf.size(), a, ap);
      va_end(ap);
    }
    this->stdstring += std::string(buf.begin(), buf.end()).c_str();
  }

  const char *string::c_str() const { return stdstring.c_str(); }

  CT::string string::encodeXML() {
    CT::string str = this->c_str();
    str.encodeXMLSelf();
    return str;
  }

  void string::encodeXMLSelf() {
    replaceSelf("&amp;", "&");
    replaceSelf("&", "&amp;");

    replaceSelf("&lt;", "<");
    replaceSelf("<", "&lt;");

    replaceSelf("&gt;", "<");
    replaceSelf("<", "&gt;");
  }

  void string::trimSelf(bool trimWhiteSpace) {
    auto plength = this->stdstring.length();
    int s = -1, e = plength;
    const char *value = this->stdstring.c_str();
    for (size_t j = 0; j < plength; j++) {
      if (trimWhiteSpace ? value[j] != ' ' && value[j] != '\n' && value[j] != '\r' : value[j] != ' ') {
        s = j;
        break;
      }
    }
    for (size_t j = plength - 1; j > 0; j--) {
      if (trimWhiteSpace ? value[j] != ' ' && value[j] != '\n' && value[j] != '\r' : value[j] != ' ') {
        e = j;
        break;
      }
    }
    substringSelf(s, e + 1);
  }

  bool string::testRegEx(const char *pattern) {
    int status;
    regex_t re;
    /* TODO: Maarten Plieger 2021-12-31, cache compiled regcomp */
    if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
      return false;
    }
    status = regexec(&re, stdstring.c_str(), (size_t)0, nullptr, 0);
    regfree(&re);
    if (status != 0) {
      return false;
    }
    return true;
  }

  void string::setChar(size_t location, const char character) {
    if (location < length()) {
      stdstring.at(location) = character;
      if (character == '\0') stdstring.resize(location);
    }
  }

  bool string::equals(const std::string &_string) const { return stdstring == _string; }

  bool string::equalsIgnoreCase(const char *_value, size_t _length) {
    if (_value == nullptr) return false;
    if (length() != _length) return false;
    CT::string selfLowerCase = stdstring.c_str();
    CT::string testValueLowerCase = _value;
    selfLowerCase.toLowerCaseSelf();
    testValueLowerCase.toLowerCaseSelf();
    if (strncmp(selfLowerCase.c_str(), testValueLowerCase.c_str(), testValueLowerCase.length()) == 0) return true;
    return false;
  }

  bool string::equalsIgnoreCase(const char *_value) {
    if (_value == nullptr) return false;
    return equalsIgnoreCase(_value, strlen(_value));
  }

  bool string::equalsIgnoreCase(CT::string *_string) {
    if (_string == nullptr) return false;
    return equalsIgnoreCase(_string->c_str(), _string->length());
  }

  bool string::equalsIgnoreCase(CT::string _string) { return equalsIgnoreCase(_string.c_str(), _string.length()); }

  void string::copy(const CT::string *_string) {
    if (_string == nullptr) {
      this->stdstring = "";
      return;
    }
    this->stdstring = _string->stdstring;
  };

  void string::copy(const CT::string _string) { this->stdstring = _string.stdstring; };

  void string::copy(const char *_value) {
    if (_value == nullptr) {
      this->stdstring = "";
      return;
    }
    this->stdstring = _value;
  };

  CT::string string::toLowerCase() {
    CT::string t;
    t.copy(c_str(), length());
    t.toLowerCaseSelf();
    return t;
  }

  CT::string string::toUpperCase() {
    CT::string t;
    t.copy(c_str(), length());
    t.toUpperCaseSelf();
    return t;
  }

  bool string::empty() { return stdstring.size() == 0; }

  void string::setSize(int size) { stdstring.resize(size); }

  void string::concat(const CT::string *_string) { this->stdstring += _string->stdstring; }

  void string::concat(const CT::string &_string) { this->stdstring += _string.stdstring; }

  void string::concat(const char *_value) {
    if (_value == nullptr) return;
    this->stdstring.append(_value);
  };

  int string::endsWith(const char *search) { return (lastIndexOf(search) == int(length() - strlen(search))); };

  int string::startsWith(const char *search) { return (indexOf(search) == 0); };

  int string::startsWith(const std::string search) { return (indexOf(search.c_str()) == 0); };

  string string::trim() {
    CT::string r = stdstring.c_str();
    r.trimSelf();
    return r;
  }

  void string::replaceSelf(CT::string from, CT::string to) {
    std::string &str = this->stdstring;
    if (from.empty()) {
      return;
    }
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
  }

  string string::replaceAll(CT::string from, CT::string to) {
    CT::string str;
    str.stdstring = this->stdstring;
    if (from.empty()) {
      return this;
    }
    size_t start_pos = 0;
    while ((start_pos = str.stdstring.find(from, start_pos)) != std::string::npos) {
      str.stdstring.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
  }

  CT::string string::replace(const char *old, const char *newstr) {
    std::string from = old;
    std::string to = newstr;
    return replaceAll(from, to);
  }

  CT::string string::substring(int start, int end) {
    // Negative start means empty string
    if (start < 0) {
      return "";
    }
    // When end is negative, return till the end of the string
    if (end < 0) {
      return this->stdstring.substr(start);
    }
    // If end is less than start, return empty string
    if (end <= start) {
      return "";
    }
    return this->stdstring.substr(start, end - start);
  }

  int string::substringSelf(int start, int end) {
    this->stdstring = substring(start, end);
    return 0;
  }

  float string::toFloat() { return static_cast<float>(atof(trim().c_str())); }

  // TODO: When strings like "longlat are passed the function currently silently returns 0. Would be better to throw an exception"
  double string::toDouble() { return atof(trim().c_str()); }

  int string::toInt() { return atoi(c_str()); }

  long string::toLong() { return atol(c_str()); }

  bool is_digit(const char value) { return std::isdigit(value); }

  /* These need to be initialized once, this is a costly function */
  std::regex isNumericRegex = std::regex("[+-]?([0-9]*[.])?[0-9]+");
  std::regex isFloatRegex = std::regex("[+-]?[0-9]*[.][0-9]+f?");
  std::regex isIntRegex = std::regex("[+-]?[0-9]+");

  bool string::isNumeric() {

    const size_t inputLength = this->length();
    const char *inputStr = this->c_str();
    /*check size */
    if (this->empty() || inputLength > CT_MAX_NUM_CHARACTERS_FOR_NUMERIC) {
      return false;
    }
    /* NaN is a "number"...in this context */
    if (this->equals("NaN")) {
      return true;
    }

    if (std::regex_match(inputStr, isNumericRegex)) {
      return true;
    }
    return false;
  }

  bool string::isInt() {
    const size_t inputLength = this->length();
    const char *inputStr = this->c_str();
    /*check size */
    if (this->empty() || inputLength > CT_MAX_NUM_CHARACTERS_FOR_INT) {
      return false;
    }

    if (std::regex_match(inputStr, isIntRegex)) {
      return true;
    }
    return false;
  }

  bool string::isFloat() {
    CT::string inputStr = this->trim().c_str();

    /*check size */
    if (inputStr.empty() || inputStr.length() > CT_MAX_NUM_CHARACTERS_FOR_FLOAT) {
      return false;
    }
    /* NaN is a "float"...in this context */
    if (inputStr.equals("NaN")) {
      return true;
    }

    if (std::regex_match(inputStr.c_str(), isFloatRegex)) {
      return true;
    }
    return false;
  }

  string string::getHex(unsigned int number) {
    int hex = number % 256;
    unsigned char a = hex / 16;
    unsigned char b = hex % 16;
    CT::string result = "00";
    result.setChar(0, a < 10 ? a + 48 : a + 55);
    result.setChar(1, b < 10 ? b + 48 : b + 55);
    return result;
  }

  string string::toHex24() {
    string result;
    unsigned int value = this->toInt();
    result.print("%s%s%s", getHex(value % 256).c_str(), getHex((value >> 8) % 256).c_str(), getHex((value >> 16) % 256).c_str());
    return result;
  }

  void string::concatlength(const char *_value, size_t len) {
    std::string newString = _value;
    newString.resize(len);
    this->stdstring.append(newString);
  }

  std::vector<CT::string> string::split(const char *_value) {
    std::vector<CT::string> stringList;
    const char *fo = strstr(this->stdstring.c_str(), _value);
    const char *prevFo = this->stdstring.c_str();
    size_t keyLength = strlen(_value);
    while (fo != nullptr) {
      stringList.push_back(CT::string(prevFo, (fo - prevFo)));
      prevFo = fo + keyLength;
      fo = strstr(fo + keyLength, _value);
    }
    size_t prevFoLength = strlen(prevFo);
    if (prevFoLength > 0) {
      stringList.push_back(CT::string(prevFo, prevFoLength));
    }
    return stringList;
  }

  CT::string join(const std::vector<string> &items, CT::string separator) {
    CT::string newString;
    for (auto &item : items) {
      newString += item.stdstring;
      newString += separator;
    }
    return newString;
  }

  std::string basename(std::string input) { return input.substr(input.find_last_of("/\\") + 1); }
} /* namespace CT */

bool equalsIgnoreCase(const std::string str1, const std::string str2) {
  if (str1.length() != str2.length()) return false;
  for (size_t i = 0; i < str1.length(); ++i) {
    if (tolower(str1[i]) != tolower(str2[i])) return false;
  }
  return true;
}
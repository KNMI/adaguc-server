#include "CTString.h"

#include "CDebugger.h"
#include <regex>
#define CT_STRING_PRINT_BUFFER_SIZE 64

namespace CT {

  std::vector<CT::string> string::split(const char *_value) const {
    std::vector<CT::string> stringList;
    const char *fo = strstr(stdstring.c_str(), _value);
    const char *prevFo = stdstring.c_str();
    while (fo != nullptr) {
      stringList.push_back(CT::string(prevFo, (fo - prevFo)));
      prevFo = fo + 1;
      fo = strstr(prevFo, _value);
    }
    stringList.push_back(CT::string(prevFo));
    return stringList;
  }

  string::string() {}

  string::string(const char *_value) {
    if (_value != NULL) copy(_value, strlen(_value));
  }

  string::string(string const &f) { this->copy(f.stdstring.c_str(), f.stdstring.length()); }

  string::string(CT::string *_string) { copy(_string); }

  string::string(const char *_value, size_t _length) { copy(_value, _length); }

  string &string::operator=(string const &f) {
    if (this == &f) return *this;
    this->stdstring = f.stdstring;
    return *this;
  }

  string &string::operator=(const char *const &f) {
    if (f == nullptr) {
      this->stdstring = "";
    } else {
      this->stdstring = f;
    }
    return *this;
  }

  string &string::operator+=(std::string const &f) {
    this->stdstring += f;
    return *this;
  }

  string &string::operator+=(string const &f) {
    if (this == &f) return *this;
    this->stdstring += f;
    return *this;
  }

  string &string::operator+=(const char *const &f) {
    this->concat(f);
    return *this;
  }

  string string::operator+(string const &f) {
    CT::string n(*this);
    n.concat(f);
    return n;
  }

  string string::operator+(const char *const &f) {
    CT::string n(*this);
    n.concat(f);
    return n;
  }

  const char *string::strrstr(const char *x, const char *y) {
    const char *prev = NULL;
    const char *next;
    if (*y == '\0') return strchr(x, '\0');
    while ((next = strstr(x, y)) != NULL) {
      prev = next;
      x = next + 1;
    }
    return prev;
  }

  char string::charAt(size_t n) {
    if (n > length()) return 0;
    return (stdstring.c_str())[n];
  }

  int string::indexOf(const char *search, size_t _length) {
    if (_length == 0) return -1;
    if (length() == 0) return -1;

    const char *value = stdstring.c_str();
    const char *pi = strstr(value, search);
    if (pi == NULL) return -1;
    int c = pi - value;
    if (c < 0) c = -1;
    return c;
  }

  int string::lastIndexOf(const char *search, size_t _length) {
    if (_length == 0) return -1;
    if (length() == 0) return -1;

    const char *value = stdstring.c_str();
    const char *pi = strrstr(value, search);
    if (pi == NULL) return -1;
    int c = pi - value;
    if (c < 0) c = -1;
    return c;
  }

  void string::copy(const char *_value, size_t _length) {
    if (_value == NULL) {
      this->stdstring = "";
      return;
    }
    this->stdstring = _value;
    this->stdstring.resize(_length);
  }

  char string::_tohex(char in) {
    in += 48;
    if (in > 57) in += 7;
    return in;
  }
  char string::_fromhex(char in) {
    /* From lowercase to uppercase */
    if (in > 96) in -= 32;
    /* From number character to numeric value */
    in -= 48;
    /* When numeric value is more than 16 (eg ABCDEF) substract 7 to get numeric value 10,11,12,etc... */
    if (in > 16) in -= 7;
    return in;
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
        pszEncode[p++] = _tohex(szChar / 16);
        pszEncode[p++] = _tohex(szChar % 16);
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
        d1 = _fromhex(value[j + 1]);
        d2 = _fromhex(value[j + 2]);
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

  CT::string string::encodeXML(CT::string stringToEncode) {
    stringToEncode.encodeXMLSelf();
    return stringToEncode;
  }

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
    status = regexec(&re, stdstring.c_str(), (size_t)0, NULL, 0);
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

  bool string::equals(const char *_value) const {
    if (_value == NULL) return false;
    return (stdstring == _value);
  }

  bool string::equals(CT::string *_string) const {
    if (_string == NULL) return false;
    return (stdstring == _string->stdstring);
  }

  bool string::equals(CT::string &_string) const { return (stdstring == _string.stdstring); }

  bool string::equals(std::string const &_string) const { return (stdstring == _string); }

  bool string::equals(const char *_value, size_t _length) const {
    if (_value == NULL) return false;
    if (this->stdstring.length() != _length) return false;
    if (strncmp(stdstring.c_str(), _value, _length) == 0) return true;
    return false;
  }

  bool string::equalsIgnoreCase(const char *_value, size_t _length) {
    if (_value == NULL) return false;
    if (length() != _length) return false;
    CT::string selfLowerCase = stdstring.c_str();
    CT::string testValueLowerCase = _value;
    selfLowerCase.toLowerCaseSelf();
    testValueLowerCase.toLowerCaseSelf();
    if (strncmp(selfLowerCase.c_str(), testValueLowerCase.c_str(), testValueLowerCase.length()) == 0) return true;
    return false;
  }

  bool string::equalsIgnoreCase(const char *_value) {
    if (_value == NULL) return false;
    return equalsIgnoreCase(_value, strlen(_value));
  }

  bool string::equalsIgnoreCase(CT::string *_string) {
    if (_string == NULL) return false;
    return equalsIgnoreCase(_string->c_str(), _string->length());
  }

  bool string::equalsIgnoreCase(CT::string _string) { return equalsIgnoreCase(_string.c_str(), _string.length()); }

  void string::copy(const CT::string *_string) {
    if (_string == NULL) {
      this->stdstring = "";
      return;
    }
    this->stdstring = _string->stdstring;
  };

  void string::copy(const CT::string _string) { this->stdstring = _string.stdstring; };

  void string::copy(const char *_value) {
    if (_value == NULL) {
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
    if (_value == NULL) return;
    this->stdstring.append(_value);
  };

  int string::indexOf(const char *search) { return indexOf(search, strlen(search)); };

  int string::lastIndexOf(const char *search) { return lastIndexOf(search, strlen(search)); };

  int string::endsWith(const char *search) { return (lastIndexOf(search) == int(length() - strlen(search))); };

  int string::startsWith(const char *search) { return (indexOf(search) == 0); };

  int string::startsWith(std::string search) { return (indexOf(search.c_str()) == 0); };

  string string::trim() {
    CT::string r = stdstring.c_str();
    r.trimSelf();
    return r;
  }

  //  int CT::string::replaceSelf(const char *substr, size_t substrl, const char *newString, size_t newStringl) {
  //   if (this->empty()) return 0;
  //   CT::string thisString = this;

  //   std::vector<int> occurences;
  //   const char *thisStringValue = thisString.standardString.c_str();
  //   const char *value = this->standardString.c_str();
  //   const char *tempVal = value;
  //   const char *search = substr;
  //   int c = 0;
  //   size_t oc = 0;
  //   do {
  //     tempVal = value + oc;
  //     const char *pi = strstr(tempVal, search);
  //     if (pi != NULL) {
  //       c = pi - tempVal;
  //     } else {
  //       c = -1;
  //     }
  //     if (c >= 0) {
  //       oc += c;
  //       occurences.push_back(oc);
  //       oc += substrl;
  //     }

  //   } while (c >= 0 && oc < thisString.length());
  //   size_t newSize = length() + occurences.size() * (newStringl - substrl);

  //   char newvalue[newSize + 1];
  //   size_t pt = 0, ps = 0, j = 0;
  //   do {
  //     if (j < occurences.size()) {
  //       while (ps == (unsigned)occurences[j] && j < occurences.size()) {
  //         for (size_t i = 0; i < newStringl; i++) {
  //           newvalue[pt++] = newString[i];
  //         }
  //         ps += substrl;
  //         j++;
  //         if (j >= occurences.size()) break;
  //       }
  //     }
  //     newvalue[pt++] = thisStringValue[ps++];
  //   } while (pt < newSize);
  //   newvalue[newSize] = '\0';
  //   this->standardString.assign(newvalue);
  //   return 0;
  // }

  int string::replaceSelf(const char *substr, size_t, const char *newString, size_t) {
    std::string from = substr;
    std::string to = newString;
    std::string &str = this->stdstring;
    if (from.empty()) {
      return 0;
    }
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return 0;
  }

  void string::replaceSelf(std::string &from, std::string &to) {
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

  string string::replaceAll(std::string &from, std::string &to) {
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

  int string::replaceSelf(CT::string *substr, CT::string *newString) { return replaceSelf(substr->c_str(), substr->length(), newString->c_str(), newString->length()); }

  int string::replaceSelf(const char *substr, CT::string *newString) { return replaceSelf(substr, strlen(substr), newString->c_str(), newString->length()); }

  int string::replaceSelf(CT::string *substr, const char *newString) { return replaceSelf(substr->c_str(), substr->length(), newString, strlen(newString)); }

  int string::replaceSelf(const char *substr, const char *newString) { return replaceSelf(substr, strlen(substr), newString, strlen(newString)); }

  CT::string string::replace(const char *old, const char *newstr) {
    std::string from = old;
    std::string to = newstr;
    return replaceAll(from, to);
  }

  int string::substringSelf(size_t start, size_t end) {
    if (start >= stdstring.length() || end - start <= 0) {
      stdstring = "";
      return 0;
    }
    if (end > stdstring.length()) end = stdstring.length();
    stdstring = stdstring.c_str() + start;
    stdstring.resize(end - start);
    return 0;
  }

  CT::string string::substring(size_t start, size_t end) {
    CT::string test = this->stdstring.c_str();
    test.substringSelf(start, end);
    return test;
  }

  float string::toFloat() {
    float fValue = (float)atof(trim().c_str());
    return fValue;
  }

  double string::toDouble() {
    double fValue = (double)atof(c_str());
    return fValue;
  }

  int string::toInt() { return atoi(c_str()); }

  long string::toLong() { return atol(c_str()); }

  CT::string string::basename() {
    const char *last = rindex(this->c_str(), '/');
    CT::string fileBaseName;
    if ((last != NULL) && (*last)) {
      fileBaseName.copy(last + 1);
    } else {
      fileBaseName.copy(this);
    }
    return fileBaseName;
  }

  bool is_digit(const char value) { return std::isdigit(value); }

  bool includesFunction(const char *inputStr, size_t inputLength, const char testChar) {
    for (size_t intputCounter = 0; intputCounter < inputLength; intputCounter++) {
      if (testChar == inputStr[intputCounter]) return true;
    }
    return false;
  }

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

  string string::toHex8() { return getHex(this->toInt()); }

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
    while (fo != NULL) {
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

} /* namespace CT */
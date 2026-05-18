/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#ifndef CTSTRING_H
#define CTSTRING_H

#include "CTypes.h"
#include "CTStringRef.h"
#include "printfCheckMacro.h"
#include <string>

#define CT_MAX_NUM_CHARACTERS_FOR_FLOAT 18
#define CT_MAX_NUM_CHARACTERS_FOR_INT 12
#define CT_MAX_NUM_CHARACTERS_FOR_NUMERIC 39
namespace CT {

  class string {
  public:
    size_t count;

  private:
    char stackValue[CTSTRINGSTACKLENGTH + 1];
    int allocated;
    size_t privatelength; // Length of string
    size_t bufferlength;  // Length of buffer
    void _Free();
    void _Allocate(int _length);
    const char *strrstr(const char *x, const char *y);
    char _tohex(char in);
    char _fromhex(char in);

    bool useStack;
    char *heapValue;
    inline void init() {
      useStack = CTYPES_USESTACK;
      heapValue = NULL;
      stackValue[0] = 0;
      count = 0;
      allocated = 0;
      privatelength = 0;
      bufferlength = CTSTRINGSTACKLENGTH;
    }
    inline char *getValuePointer() { return useStack ? stackValue : heapValue; }

  public:
    /**
     * Default constructor
     */
    string();

    /**
     *Copy constructor
     */
    string(string const &);

    /**
     * Copy constructor which initialize the string with a character array
     * @param _value The character array to copy
     * @param _length the length of the character array
     */
    string(const char *_value, size_t _length);

    /**
     * Copy constructor which initialize the string with a character array
     * @param _value The character array to copy
     */
    string(const char *_value);

    /**
     * Copy constructor which initialize the string with the contents of a string pointer
     * @param _string Pointer to the string to copy
     */
    string(CT::string *_string);

    /**
     * assign operator
     * @param f The input string
     */
    string &operator=(string const &f);

    /**
     * assign operator
     * @param f The input character array
     */
    string &operator=(const char *const &f);

    /**
     * addition assignment operator
     * @param f The input string
     */
    string &operator+=(string const &f);

    /**
     * addition assignment operator
     * @param f The input character array
     */
    string &operator+=(const char *const &f);

    /**
     * addition operator
     * @param f The input string
     */
    string operator+(string const &f);

    /**
     * addition operator
     * @param f The input character array
     */
    string operator+(const char *const &f);

    /**
     * const char* conversion operator
     * Now it is not necessary to call c_str when a const char* is expected.
     */
    operator const char *() const;

    /**
     * std::string conversion operator
     * Marked explicit to avoid ambiguity with operator const char *() during the
     * CT::string -> std::string migration. Use std::string(ctstring) or
     * static_cast<std::string>(ctstring) when an explicit conversion is wanted;
     * otherwise std::string sees CT::string through operator const char *().
     */
    explicit operator std::string() const { return std::string(this->c_str(), this->length()); }

    /**
     * std::string copy constructor — initialize a CT::string from a std::string.
     * Allows passing std::string anywhere a CT::string is expected during migration.
     */
    string(const std::string &s) {
      init();
      copy(s.c_str(), s.length());
    }

    /**
     * Compare operator
     * @param f The input string
     */
    bool operator<(const string &str) const { return strcmp(this->c_str(), str.c_str()) < 0; }
    bool operator>(const string &str) const { return strcmp(this->c_str(), str.c_str()) > 0; }
    bool operator==(const string &str) const { return this->equals(str); }
    bool operator!=(const string &str) const { return !this->equals(str); }

    /**
     * Destructor
     */
    virtual ~string() { _Free(); }

    /**
     * returns length of the string
     * @return length
     */
    inline size_t length() const { return privatelength; }

    /**
     * returns the internal bufferlength of the string
     * @return internal bufferlength
     */
    inline size_t getbufferlength() { return bufferlength; }

    /**
     * Copy a character array into the string
     * @param _value The character array to copy
     * @param _length the length of the character array
     */
    void copy(const char *_value, size_t _length);

    /**
     * Copy a string pointer into the array
     * @param _string Pointer to the string to copy
     */
    void copy(const CT::string *_string);

    /**
     * Copy a string pointer into the array
     * @param _string Pointer to the string to copy
     */
    void copy(const CT::string _string);

    /**
     * Copy a character array into the string
     * @param _value The character array to copy
     */
    void copy(const char *_value);

    /**
     * Appends a pointer to a string object to this string object
     * @param string* The string pointer to append
     */
    void concat(const CT::string *_string);

    /**
     * Appends a string object to this string object
     * @param string The string to append
     */
    void concat(const CT::string _string);

    /**
     * Appends an array of characters with specified length to this string object
     * @param value The character array to append
     * @param len The length of the character array
     */
    void concatlength(const char *_value, size_t len);

    /**
     * Appends an array of characters terminated with a '\0' character.
     * @param value The 0-terminated character array to append
     */
    void concat(const char *_value);

    /**
     * Returns the char value at the specified index.
     * @param index The index of the character to get.
     */
    char charAt(size_t index);

    /**
     * Sets a character in the string object at specified location
     * @param location The location to set
     * @param character The character to set
     */
    void setChar(size_t location, const char character);

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param value The character array to compare
     * @param length The length of the character array to compare
     */
    bool equals(const char *value, size_t length) const;

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param value  The 0-terminated character array to compare
     */
    bool equals(const char *value) const;

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param string*  Pointer to the string object to compare
     */
    bool equals(CT::string *string) const;

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param string Copy of the string object to compare
     */
    bool equals(CT::string string) const;

    bool equals(std::string const &string) const;

    bool equalsIgnoreCase(const char *_value, size_t _length);

    bool equalsIgnoreCase(const char *_value);

    bool equalsIgnoreCase(CT::string *_string);

    bool equalsIgnoreCase(CT::string string);

    /**
     * Tests for a posix regular expression against the string object, returns true if matches.
     * @param pattern The 0-terminated character array containing the regular expression
     */
    bool testRegEx(const char *pattern);

    /**
     * Returns the index within this string of the first occurrence of the specified character.
     * If a character with value ch occurs in the character sequence represented by this String object, then the index of the first such occurrence is returned
     * @param search The character array to look for
     * @param length The length of the character array
     * @return -1 if not found, otherwise the index of the character sequence in this string object
     */
    int indexOf(const char *search, size_t length);

    /**
     * Returns the index within this string of the first occurrence of the specified character.
     * If a character with value ch occurs in the character sequence represented by this String object, then the index of the first such occurrence is returned
     * @param search The 0-terminated character array to look for
     * @return -1 if not found, otherwise the index of the character sequence in this string object
     */
    int indexOf(const char *search);

    /**
     * Returns the index within this string of the last occurrence of the specified character
     * @param search The character array to look for
     * @param length The length of the character array
     * @return -1 if not found, otherwise the last index of the character sequence in this string object
     */
    int lastIndexOf(const char *search, size_t _length);

    /**
     * Returns the index within this string of the last occurrence of the specified character
     * @param search The character array to look for
     * @param search The 0-terminated character array to look for
     * @return -1 if not found, otherwise the last index of the character sequence in this string object
     */
    int lastIndexOf(const char *search);

    /**
     * The endsWith() method determines whether a string ends with the characters of another string, returning true or false as appropriate.
     */
    int endsWith(const char *search);

    /**
     * The startsWith() method determines whether a string begins with the characters of another string, returning true or false as appropriate.
     */
    int startsWith(const char *search);

    /**
     * String to unicode
     */
    void toUnicodeSelf();

    /**
     * String to uppercase
     */
    void toUpperCaseSelf();

    /**
     * String to lowercase
     */
    void toLowerCaseSelf();

    /**
     * Return lowercase string
     */
    CT::string toLowerCase();

    /**
     * Return uppercase string
     */
    CT::string toUpperCase();

    /**
     * Decodes URL to string
     */
    void decodeURLSelf();

    /**
     * Encodes string to URL
     */
    void encodeURLSelf();

    /**
     * Encodes string using XML encoding
     */
    void encodeXMLSelf();

    static CT::string encodeXML(CT::string stringToEncode);
    CT::string encodeXML();

    /**
     * Removes spaces in this string
     */
    void trimSelf(bool trimWhiteSpace = false);

    /**
     * Returns a new string with removed spaces
     */
    string trim();

    /**
     * Function which returns a std::vector on the stack with a list of strings allocated on the stack
     * This function links its data to string data, it does not allocate new data or copy the data
     * Resources are freed automatically
     * @param _value The token to split the string on
     */
    StackList<CT::stringref> splitToStackReferences(const char *_value);

    /**
     * Function which returns a std::vector on the stack with a list of strings allocated on the stack
     * Data is automatically freed
     * @param _value The token to split the string on
     */
    StackList<CT::string> splitToStack(const char *_value);

    /**
     * Print like printf to this string
     * @param a The string to print
     */
    void print(const char *a, ...) PRINTF_FORMAT_CHECK(2, 3);

    /**
     * Like printf, but concatenates the string
     * @param a The string to print
     */
    void printconcat(const char *a, ...) PRINTF_FORMAT_CHECK(2, 3);

    /**
     * Get a character array with the string data
     * @return the character array
     */
    const char *c_str() const;

    /** Replace all strings with another string
     * @param substr the character array to replace
     * @param substrl the length of the character array to replace
     * @param newString the new character array to replace with
     * @param newStringl The length of the character array to replace with
     * @return Zero on success
     */
    int replaceSelf(const char *substr, size_t substrl, const char *newString, size_t newStringl);

    /** Replace all strings with another string
     * @param substr the string to replace
     * @param newString the new stringto replace with
     * @return Zero on success
     */
    int replaceSelf(CT::string *substr, CT::string *newString);

    /** Replace all strings with another string
     * @param substr the character array to replace
     * @param newString the new string to replace with
     * @return Zero on success
     */
    int replaceSelf(const char *substr, CT::string *newString);

    /** Replace all strings with another string
     * @param substr the string to replace
     * @param newString the new character array to replace with
     * @return Zero on success
     */
    int replaceSelf(CT::string *substr, const char *newString);

    /** Replace all strings with another string
     * @param substr the character array to replace
     * @param newString the new character array to replace with
     * @return Zero on success
     */
    int replaceSelf(const char *substr, const char *newString);

    /** Replace all strings with another string and returns the new string
     * @param substr the character array to replace
     * @param newString the new character array to replace with
     * @return the subsetted string
     */
    CT::string replace(const char *old, const char *newstr);

    /**
     * Subset the string from start till end
     * @param string Te input string to subset
     * @param start Where to subset from
     * @param end Where to subset to (-1 means till the end of the string)
     * @return Zero on success
     */
    int substringSelf(CT::string *string, size_t start, size_t end);

    /**
     * Subset the string from start till end
     * @param start Where to subset from
     * @param end Where to subset to (-1 means till the end of the string)
     * @return Zero on success
     */
    int substringSelf(size_t start, size_t end);

    /**
     * Returns a subsetted string from start till end
     * @param start Where to subset from
     * @param end Where to subset to (-1 means till the end of the string)
     * @return string with the subsetted string
     */
    CT::string substring(size_t start, size_t end);

    /**
     * Adjusts the size of the string
     */
    void setSize(int size);

    /**
     * Converts the string to a float number
     */
    float toFloat();

    /**
     * Converts the string to a double number
     */
    double toDouble();

    /**
     * Converts the string to an integer number
     */
    int toInt();

    /**
     * Converts the string to a long number
     */
    long toLong();

    /**
     * Test whether string is empty or not
     */
    bool empty();

    /**
     * Returns posix basename of path
     */
    CT::string basename();

    /**
     * Checks if this string represents a numeric value
     */
    bool isNumeric();

    /**
     * Checks if this string represents a float value
     */
    bool isFloat();

    /**
     * Checks if this string represents an int value
     */
    bool isInt();

    /**
     * Converts to hex8
     */
    CT::string toHex8();

    /**
     * Converts to hex24
     */
    CT::string toHex24();

    /**
     * Converts to hex8
     */
    static CT::string getHex(unsigned int number);
  };

  /* ---------------------------------------------------------------------- *
   *  Free helper functions in namespace CT::, all operating on std::string.
   *  These are introduced as part of the migration from CT::string to
   *  std::string. New code should use these helpers; existing CT::string
   *  methods continue to work unchanged.
   * ---------------------------------------------------------------------- */

  /**
   * Null-safe conversion from a C string to std::string.
   * Returns an empty std::string if @p p is nullptr.
   * Use this to replace patterns where CT::string was implicitly handling
   * a NULL pointer (CT::string allowed (const char*)NULL; std::string aborts).
   */
  inline std::string fromCStr(const char *p) { return p ? std::string(p) : std::string{}; }

  /**
   * Joins vector of strings into a new string.
   */
  std::string join(const std::vector<std::string> &items, const std::string &separator = ",");

  /**
   * Returns posix basename of path.
   */
  std::string basename(std::string input);

  /**
   * Case-insensitive equality.
   */
  bool equalsIgnoreCase(const std::string &str1, const std::string &str2);

  /**
   * printf into a freshly allocated std::string.
   */
  std::string printf(const char *a, ...) PRINTF_FORMAT_CHECK(1, 2);

  /**
   * printf appended to an existing std::string.
   */
  void printfconcat(std::string &appendString, const char *a, ...) PRINTF_FORMAT_CHECK(2, 3);

  /**
   * Replace all occurrences of @p from with @p to in @p input. Returns a new string.
   */
  std::string replace(const std::string &input, const std::string &from, const std::string &to);

  /**
   * Replace all occurrences of @p from with @p to in-place in @p input.
   */
  void replaceSelf(std::string &input, const std::string &from, const std::string &to);

  /**
   * Lowercase / uppercase copies.
   */
  std::string toLowerCase(const std::string &input);
  std::string toUpperCase(const std::string &input);

  /**
   * Trim leading/trailing whitespace (space, tab, newline, carriage return).
   */
  std::string trim(const std::string &input);

  /**
   * Split @p input on @p value. Preserves empty fields between consecutive delimiters
   * and at the start. Trailing empty field after final delimiter is dropped (matches
   * historical CT::string::splitToStack behaviour).
   */
  std::vector<std::string> split(const std::string &input, const std::string &value);

  /**
   * Generate a random string of the specified length using digits + ASCII letters.
   */
  std::string randomString(int length);

  /**
   * Index of the first occurrence of @p pattern in @p input, or -1 if not found.
   * Returns 0 if @p pattern is empty (matches historical semantics).
   */
  int indexOf(const std::string &input, const std::string &pattern);

  /**
   * Index of the last occurrence of @p pattern in @p input, or -1 if not found.
   * Returns 0 if @p pattern is empty.
   */
  int lastIndexOf(const std::string &input, const std::string &pattern);

  /**
   * True if @p input ends with @p pattern. True if @p pattern is empty.
   */
  bool endsWith(const std::string &input, const std::string &pattern);

  /**
   * True if @p input starts with @p pattern. True if @p pattern is empty.
   */
  bool startsWith(const std::string &input, const std::string &pattern);

  /**
   * Replace XML-special characters with their entities.
   */
  std::string encodeXml(const std::string &input);

  /**
   * Two-character hex of @p number (modulo 256).
   */
  std::string getHex(unsigned int number);

  /**
   * Six-character hex (3 bytes) representation of an int value.
   */
  std::string getHex24(int value);

  /**
   * Numeric / int / float predicates. Match historical CT::string semantics:
   *  - "NaN" is considered numeric/float.
   *  - leading/trailing whitespace tolerated for floats.
   */
  bool isNumeric(const std::string &input);
  bool isInt(const std::string &input);
  bool isFloat(const std::string &input);

  /**
   * Substring with sentinel: end < 0 means "to the end of the string".
   * If end <= start, returns empty string. If start < 0, returns empty string.
   */
  std::string substring(const std::string &input, int start, int end);

  /**
   * POSIX regex match against @p pattern.
   */
  bool testRegEx(const std::string &input, const char *pattern);

  /**
   * Numeric parsing helpers matching CT::string semantics:
   *  - Tolerate leading/trailing whitespace.
   *  - Return 0 silently on parse failure (no exception).
   */
  float toFloat(const std::string &input);
  double toDouble(const std::string &input);
  int toInt(const std::string &input);
  long toLong(const std::string &input);
}; /* namespace CT */

#endif

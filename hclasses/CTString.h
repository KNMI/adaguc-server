/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2026-09-10
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
#include <string>
#include <vector>
#include "printfCheckMacro.h"

#define CT_MAX_NUM_CHARACTERS_FOR_FLOAT 18
#define CT_MAX_NUM_CHARACTERS_FOR_NUMERIC 39
namespace CT {

  /** Joins vector of strings into a new string
   * @param items Items to join
   * @param separator optional separator, defaults to ","
   * @returns new string containing all items.
   */
  std::string join(const std::vector<std::string> &items, const std::string &separator = ",");

  /**
   * Returns posix basename of path
   * @param input The input path
   * @returns The basename of the path
   */
  std::string basename(const std::string &input);

  /**
   * The equalsIgnoreCase() method compares two strings, ignoring lower case and upper case differences.
   * This method returns true if the strings are equal, and false if not.
   * @param str1 - compare str1 against str2
   * @param str2 -
   * @return true if equal
   */
  bool equalsIgnoreCase(const std::string &str1, const std::string &str2);

  /**
   * Print like printf but returns a stdstring;
   * @param a The string to print
   * @returns std::string
   */

  std::string printf(const char *a, ...) PRINTF_FORMAT_CHECK(1, 2);

  /**
   * Like printf, but concatenates the string and returns a stdstring
   * @param a The string to print
   * @returns std::string
   */
  void printfconcat(std::string &appendString, const char *a, ...) PRINTF_FORMAT_CHECK(2, 3);

  /**
   * Replace all occurrences of a substring with another string and returns the new string
   * @param input The input string
   * @param from The string to replace
   * @param to The string to replace with
   * @returns new string with replaced values
   */

  std::string replace(const std::string &input, const std::string &from, const std::string &to);
  void replaceSelf(std::string &input, const std::string &from, const std::string &to);

  /**
   * Converts a string to lowercase
   * @param input The input string
   * @returns the input string converted to lowercase
   */
  std::string toLowerCase(const std::string &input);

  /**
   * Converts a string to uppercase
   * @param input The input string
   * @returns the input string converted to uppercase
   */
  std::string toUpperCase(const std::string &input);

  /**
   * Removes spaces in the string and returns the new string
   * @param input The input string
   * @returns the input string with removed spaces
   */
  std::string trim(const std::string &input);

  /**
   * Converts a string to double after trimming whitespace.
   */
  double toDouble(const std::string &input);

  /**
   * Function which returns a std::vector on the stack with a list of strings allocated on the stack
   * Data is automatically freed
   * @param stdstring The string to split
   * @param value The token to split the string on
   * @returns vector of split strings
   */
  std::vector<std::string> split(const std::string &stdstring, const std::string &value);

  /**
   * Generates a random string of the specified length consisting of digits, uppercase and lowercase letters
   * @param length The length of the random string to generate
   * @returns a random string of the specified length
   */
  std::string randomString(const int length);

  /**
   * Checks where the pattern is in the input string.
   *
   * @param input input string to check
   * @param pattern pattern tofind
   * @return Index of pattern in input. -1 if not found. 0 if pattern is an empty string
   */
  int indexOf(const std::string &input, const std::string &pattern);

  int lastIndexOf(const std::string &input, const std::string &pattern);

  /**
   * Checks if a string ends with another given string. If the argument is an empty string, then the method returns true.
   *
   * @param input input string to check
   * @param pattern pattern tofind
   * @return True if input ends with pattern. True if pattern is empty
   */
  bool endsWith(const std::string &input, const std::string &pattern);

  /**
   * Checks if a string starts with another given string. If the argument is an empty string, then the method returns true.
   *
   * @param input input string to check
   * @param pattern pattern tofind
   * @return True if input starts with pattern. True if pattern is empty
   */
  bool startsWith(const std::string &input, const std::string &pattern);

  /**
   * Replaces characters so it can be used as valid xml
   */
  std::string encodeXml(const std::string &input);

  /**
   * URL-encodes a string (percent-encoding).
   */
  std::string encodeURL(const std::string &input);

  /**
   * URL-decodes a string (percent-encoding), also replacing '+' with a space.
   */
  std::string decodeURL(const std::string &input);

  /**
   * Converts to hex string from int.
   */
  std::string getHex(unsigned int number);

  /**
   * Converts to hex24 string from int.
   */
  std::string getHex24(int value);

  /**
   * Checks if this string represents a numeric value
   */
  bool isNumeric(const std::string &input);

  /**
   * Checks if this string represents a float value
   */
  bool isFloat(const std::string &input);

  /**
   * Checks if this string represents an int value
   */
  bool isInt(const std::string &input);

  /**
   * Returns a subsetted string from start till end
   * @param start Where to subset from
   * @param end Where to subset to (-1 means till the end of the string). If end is less than start, an empty string is returned.
   * @return string with the subsetted string
   */
  std::string substring(const std::string &input, int start, int end);

  /**
   * Tests for a posix regular expression against the string object, returns true if matches.
   * @param pattern The 0-terminated character array containing the regular expression
   */
  bool testRegEx(const std::string &input, const char *pattern);

  /**
   * @param input Const char* to std::string. When null, it will become a string with length zero.
   * @param output std::string
   */
  std::string fromCharPointer(const char *input);
}; /* namespace CT */
#endif

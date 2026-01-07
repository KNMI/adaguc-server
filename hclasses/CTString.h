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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include <typeinfo>
#include <exception>
#include <stdlib.h>
#include <regex.h>
#include <vector>

#define CT_MAX_NUM_CHARACTERS_FOR_FLOAT 18
#define CT_MAX_NUM_CHARACTERS_FOR_INT 12
#define CT_MAX_NUM_CHARACTERS_FOR_NUMERIC 39
namespace CT {

  class string {

  private:
    const char *strrstr(const char *x, const char *y);
    char _tohex(char in);
    char _fromhex(char in);
    /**
     * std::string containing the string
     */
    std::string stdstring;

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
    string &operator+=(std::string const &f);
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

    // Conversion from and to std::string
    string(std::string s) { this->stdstring = std::move(s); }  // Implicit conversion allowed
    operator std::string() const { return {this->stdstring}; } // Implicit conversion allowed

    /**
     * Compare operator
     * @param f The input string
     */
    bool operator<(const string &str) const { return strcmp(this->c_str(), str.c_str()) < 0; }
    bool operator>(const string &str) const { return strcmp(this->c_str(), str.c_str()) > 0; }
    bool operator==(const string &str) const { return this->equals(str); }
    bool operator!=(const string &str) const { return !this->equals(str); }

    /**
     * returns length of the string
     * @return length
     */
    size_t length() { return this->stdstring.size(); }

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
    void concat(const CT::string &_string);

    /**
     * Appends an array of characters terminated with a '\0' character.
     * @param value The 0-terminated character array to append
     */
    void concat(const char *_value);
    /**
     * Appends an array of characters with specified length to this string object
     * @param value The character array to append
     * @param len The length of the character array
     */
    void concatlength(const char *_value, size_t len);

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
     * @param value  The 0-terminated character array to compare
     */
    bool equals(const char *value) const;

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param value The character array to compare
     * @param length The length of the character array to compare
     */
    bool equals(const char *value, size_t length) const;

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param string*  Pointer to the string object to compare
     */
    bool equals(CT::string *string) const;

    /**
     * Compares this string to the specified object. The result is true if the given argument is not null and representing the same sequence of characters as this object.
     * @param string Copy of the string object to compare
     */
    bool equals(CT::string &string) const;

    bool equals(const std::string &string) const;

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
    int startsWith(const std::string search);

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
     * Data is automatically freed
     * @param _value The token to split the string on
     */
    std::vector<CT::string> split(const char *_value);

    /**
     * Print like printf to this string
     * @param a The string to print
     */
    void print(const char *a, ...);

    /**
     * Like printf, but concatenates the string
     * @param a The string to print
     */
    void printconcat(const char *a, ...);

    /**
     * Get a character array with the string data
     * @return the character array
     */
    const char *c_str() const;

    /** Replace all strings with another string and returns the new string
     * @param substr the character array to replace
     * @param newString the new character array to replace with
     * @return the subsetted string
     */
    CT::string replace(const char *old, const char *newstr);

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

    /** Replace all strings with another string
     * @param substr the string to replace
     * @param newString the new stringto replace with
     */
    void replaceSelf(CT::string substr, CT::string newString);

    /** Replace all strings with another string
     * @param substr the string to replace
     * @param newString the new stringto replace with
     * @returns new string
     */
    CT::string replaceAll(CT::string substr, CT::string newString);

    friend CT::string join(const std::vector<string> &items, CT::string separator);
  };

  // Example  on how new implementation can help with moving towards fully using std::string instead of CT::String
  /** Joins vector of strings into a new string
   * @param items Items to join
   * @param separator optional separator, defaults to ","
   * @returns new string containing all items.
   */
  string join(const std::vector<string> &items, string separator = ",");
}; /* namespace CT */

#endif

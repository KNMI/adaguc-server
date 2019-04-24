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

#ifndef CTSTRINGREF_H
#define CTSTRINGREF_H

#include "CTypes.h"
#include "CTString.h"
#define CTSTRINGREFSTACKLENGTH 15
namespace CT{
  /**
  * Read only class of string
  */  
  class stringref:public basetype{
  private:
    void init();
    
    const char *constdata;
    char *voldata;
    char stackValue[CTSTRINGREFSTACKLENGTH + 1];
    size_t _length;
   public:
    ~stringref();
    stringref();
    stringref(const char * data,size_t length);

    /**
    *Copy constructor
    */
    stringref(stringref const &f);
    
    /**
    * assign operator
    * @param f The input string
    */
    stringref& operator= (stringref const& f);
    
   
    /**
     * Returns a new string with removed spaces
     */
    stringref trim();
    
    /**
     * returns length of the string
     * @return length
     */
    size_t length();
    
    /**
     * Get a character array with the string data
     * @return the character array
     */
    const char * c_str();
    
    
    /**
     * Assigns data
     * @param data The data to assign
     * @param length the number of characters to assign
     */
    void assign(const char *data,size_t length);
        
    /**
     * Function which returns a std::vector on the stack with a list of strings allocated on the stack
     * This function links its data to string data, it does not allocate new data or copy the data
     * Resources are freed automatically
     * @param _value The token to split the string on
     */
    StackList<CT::stringref> splitToStackReferences(const char * _value);
    
   /**
    * Returns the index within this string of the first occurrence of the specified character. 
    * If a character with value ch occurs in the character sequence represented by this String object, then the index of the first such occurrence is returned
    * @param search The 0-terminated character array to look for
    * @return -1 if not found, otherwise the index of the character sequence in this string object
    */
    int indexOf(const char* search);
  };
};
#endif

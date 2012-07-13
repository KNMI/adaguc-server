/* 
 * Copyright (C) 2012, Royal Netherlands Meteorological Institute (KNMI)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any 
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Project    : ADAGUC
 *
 * initial programmer :  M.Plieger
 * initial date       :  20120610
 */

#ifndef CTIME_H
#define CTIME_H
#include <udunits.h>

#include "CTypes.h"

#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#define CTIME_CONVERSION_ERROR 1

class CTime{
  private:
  CT::string currentUnit;
  void safestrcpy(char *s1,const char*s2,size_t size_s1);
  DEF_ERRORFUNCTION();
  public:
  static utUnit  dataunits;
  static bool isInitialized;
  /**
   * Class which holds date parameters like year, month, day, hour, minute, second and offset
   */
  class Date {
  public:
    int     year, month, day, hour, minute;
    float   second;
    double  offset;
  };
  
  /**
   * Constructor
   */
  CTime();
  
  /**
   * Destructor
   */
  ~CTime();
  
  /**
   * Static function which converts an exception into a readable message
   * @param int The value of catched exception 
   * @return CT::string with the readable message
   */
  static CT::string getErrorMessage(int CTimeParserException);
  
 
  
  /**
   * resets CTime
   */
  void reset();
  
  /**
   * Initializes CTime
   * @param units CF time units
   * @return 0 on success 1 on failure.
   */
  int init(const char *units);
  
  /**
   * Turns double value into a date object
   * Throws integer CTIME_CONVERSION_ERROR when failes
   * @param offset The value to convert to date
   * @return the date object
   */
  Date getDate(double offset);
  
  
  /**
   * Turns date object into double value
   * Throws integer CTIME_CONVERSION_ERROR when failes
   * @param Date the date object
   * @return The value to convert to date
   */
  double dateToOffset( Date date);
  
  /**
   * Converts YYYYmmddThhmmss string to Date
   * Throws integer CTIME_CONVERSION_ERROR when fails.
   * @param szTime the time string to convert
   * @return The Date
   */
  Date stringToDate(const char*szTime);
  
  /**
   * Converts date object to string
   * @param date
   * @param string Format YYYYmmddThhmmss
   */
  CT::string dateToString(Date date);
  
  /**
   * Converts date object to string
   * @param date
   * @param string Format YYYY-mm-ddThh:mm:ss
   */
  CT::string dateToISOString(Date date);
};
#endif


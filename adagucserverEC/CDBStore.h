/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-05-06
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

#ifndef CDBSTORE_H
#define CDBSTORE_H
#include <string>
#include <vector>

#define CDB_UNKNOWN_ERROR 0
#define CDB_UNKNOWN_COLUMNNAME 1
#define CDB_INDEX_OUT_OF_BOUNDS 2
#define CDB_CONNECTION_ERROR 3
#define CDB_NODATA 5
#define CDB_QUERYFAILED 6

struct CDBStore {

  static std::string getErrorMessage(int e);

  struct ColumnModel {
    std::vector<std::string> columnNames;
    size_t getIndex(const std::string &name) const;
  };

  /**
   * Record is a set of values for each row from the database. The vector of values has the length of the columnmodel.
   */
  struct Record {
    std::vector<std::string> values;
    const ColumnModel *_columnModel; // Reference to columnmodel in store, this is the same for all records.
    Record(const ColumnModel &columnModel);
    /**
     * @param name Index of the column
     * @return The value corresponding with the requested column index in this record, throws exception if not found.
     */
    const std::string &get(size_t index) const;
    /**
     * @param name Name of the column
     * @return The value corresponding with the requested column name in this record, throws exception if not found.
     */
    const std::string &get(const std::string &name) const;
  };

  struct Store {
    std::vector<Record> records;
    ColumnModel columnModel;
  };
};
#endif

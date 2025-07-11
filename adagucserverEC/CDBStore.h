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
#include <vector>
#include "../hclasses/CTString.h"

#define CDB_UNKNOWN_ERROR 0
#define CDB_UNKNOWN_COLUMNNAME 1
#define CDB_INDEX_OUT_OF_BOUNDS 2
#define CDB_CONNECTION_ERROR 3
#define CDB_TABLE_CREATION_ERROR 4
#define CDB_NODATA 5
#define CDB_QUERYFAILED 6

class CDBStore {
public:
  static const char *getErrorMessage(int e) {
    if (e == CDB_UNKNOWN_ERROR) return "CDB_UNKNOWN_ERROR";
    if (e == CDB_UNKNOWN_COLUMNNAME) return "CDB_UNKNOWN_COLUMNNAME";
    if (e == CDB_INDEX_OUT_OF_BOUNDS) return "CDB_INDEX_OUT_OF_BOUNDS";
    return getErrorMessage(0);
  }
  struct ColumnModel {
    std::vector<CT::string> columnNames;
    size_t getIndex(const char *name) {
      for (size_t j = 0; j < columnNames.size(); j++) {
        if (columnNames[j].equals(name)) return j;
      }
      throw(CDB_UNKNOWN_COLUMNNAME);
    }
    const char *getName(size_t index) {
      if (index >= columnNames.size()) throw(CDB_INDEX_OUT_OF_BOUNDS);
      return columnNames[index].c_str();
    }
    void push(const char *name) { columnNames.emplace_back(name); }
    size_t getSize() { return columnNames.size(); }
  };

  struct Record {
    std::vector<CT::string> values;
    ColumnModel *columnModel;
    void setColumnModel(ColumnModel *columnModel) {
      this->columnModel = columnModel;
      values.reserve(columnModel->getSize());
    }
    CT::string *get(int index) { return get((size_t)index); }
    CT::string *get(size_t index) {
      if (index >= values.size()) throw(CDB_INDEX_OUT_OF_BOUNDS);
      return &(values[index]);
    }
    CT::string *get(const char *name) { return get(columnModel->getIndex(name)); }
    void push(const char *value) { values.emplace_back(value); }
  };

  class Store {
  public:
    std::vector<Record> records;
    ColumnModel *columnModel;

    Store(ColumnModel *columnModel) { this->columnModel = columnModel; }
    ~Store() {
      records.clear();
      delete columnModel;
    }
    Record *getRecord(size_t rowNumber) {
      if (rowNumber >= records.size()) throw(CDB_INDEX_OUT_OF_BOUNDS);
      return &records[rowNumber];
    }
    size_t getSize() { return records.size(); }
    size_t size() { return records.size(); }
    void push(Record &record) { records.push_back(record); }
    std::vector<Record> getRecords() { return records; }
  };
};
#endif

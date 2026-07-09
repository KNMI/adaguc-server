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

#include "CDBStore.h"
#include <algorithm>
#include <cstddef>

std::string CDBStore::getErrorMessage(int e) {
  if (e == CDB_UNKNOWN_ERROR) return "CDB_UNKNOWN_ERROR";
  if (e == CDB_UNKNOWN_COLUMNNAME) return "CDB_UNKNOWN_COLUMNNAME";
  if (e == CDB_INDEX_OUT_OF_BOUNDS) return "CDB_INDEX_OUT_OF_BOUNDS";
  return getErrorMessage(0);
}

size_t CDBStore::ColumnModel::getIndex(const std::string &name) const {
  auto it = std::find_if(columnNames.begin(), columnNames.end(), [&name](const auto &a) { return a == name; });
  if (it != columnNames.end()) {
    return std::distance(columnNames.begin(), it);
  }
  throw(CDB_UNKNOWN_COLUMNNAME);
}

CDBStore::Record::Record(const ColumnModel &columnModel) {
  this->_columnModel = &columnModel;
  values.reserve(columnModel.columnNames.size());
}

const std::string &CDBStore::Record::get(size_t index) const { return values.at(index); }

const std::string &CDBStore::Record::get(const std::string &name) const { return values.at(_columnModel->getIndex(name)); }

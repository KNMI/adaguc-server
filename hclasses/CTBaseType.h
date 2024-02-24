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

#ifndef CTBASETYPE_H
#define CTBASETYPE_H

namespace CT {

  /**
   * Vector of pointers, which frees pointers upon deletion
   */
  template <class T2> class PointerList : public std::vector<T2> {
  public:
    ~PointerList() { free(); }
    void free() {
      for (size_t j = 0; j < this->size(); j++) {
        delete (*this)[j];
        (*this)[j] = NULL;
      }
    }
    T2 get(int j) { return (*this)[j]; }
    void add(T2 t) { this->push_back(t); }
  };

  /**
   * Vector of objects on the stack
   */
  template <class T3> class StackList : public std::vector<T3> {
  public:
    ~StackList() {}
    void add(T3 t) { this->push_back(t); }
    StackList() {}
    /*Copy constructor*/
    StackList(StackList<T3> const &f) : StackList<T3>() {
      for (size_t j = 0; j < f.size(); j++) {
        add(f[j]);
      }
    }
    StackList(StackList &&) = default;
    StackList &operator=(const StackList &) = default;
    StackList &operator=(StackList &&) = default;
  };
}; /* namespace CT */

#endif

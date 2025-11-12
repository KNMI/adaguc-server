/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2021-09-17
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

#ifndef CKEYVALUEPAIR_H
#define CKEYVALUEPAIR_H
#include <iostream>
#include <vector>
#include <stdio.h>
#include "CTypes.h"
struct CKeyValuePair {
  CT::string key;
  CT::string value;
};
typedef CT::StackList<CKeyValuePair> CKeyValuePairs;

struct CKeyValueDescriptionPair {
  CT::string key;
  CT::string description;
  CT::string value;
};

typedef std::vector<CKeyValueDescriptionPair> CKeyValueDescriptionPairs;

#endif
/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
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

#ifndef CCDFDIMENSION_H
#define CCDFDIMENSION_H

namespace CDF{
  class Dimension{
    
    public:
      Dimension(){
        isIterative=false;
        length=0;
      }
      CT::string name;
      size_t length;
      bool isIterative;
      int id;
      size_t getSize(){
        return length;
      }
      void setSize(size_t _length){
        length=_length;
      }
      void setName(const char *value){
        name.copy(value);
      }
      //Returns a new copy of this dimension
      Dimension *clone(){
        Dimension *newDim = new Dimension ();
        newDim->name=name.c_str();
        newDim->length=length;
        newDim->isIterative=isIterative;
        newDim->id=id;
        return newDim;
      }
  };
}

#endif


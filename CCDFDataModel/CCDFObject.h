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

#ifndef CCDFOBJECT_H
#define CCDFOBJECT_H

#include "CCDFVariable.h"
#include "CDebugger.h"

class CDFObject : public CDF::Variable {
public:
  CDFType ncmlTypeToCDFType(const char *type);

private:
  void putNCMLAttributes(void *a_node);
  void *reader;

public:
  ~CDFObject();
  CDFObject();

  /**
   * Returns the variable for the specified name or null if not found
   * @param name THe name of the variable, or NC_GLOBAL to get the root (this)
   * @returns the matching variable or null
   */
  CDF::Variable *getVar(std::string name);
  CDF::Variable *getVariableNE(const char *name); // Same as getVar
  CDF::Variable *getVariableNE(std::string name); // Same as getVar

  /**
   * Returns the dimension for the specified name or null if not found
   * @param name THe name of the dimension
   * @returns the matching dimension or null
   */
  CDF::Dimension *getDim(std::string name);
  CDF::Dimension *getDimensionNE(std::string name);
  CDF::Dimension *getDimensionNE(const char *name);

  std::vector<CDF::Dimension *> dimensions;
  std::vector<CDF::Variable *> variables;
  CT::string name;
  int getVariableIndexThrows(const char *name);
  CDF::Variable *getVariableThrows(std::string name);
  int getVariableIndexNE(const char *name);
  int getDimensionIndexThrows(const char *name);
  int getDimensionIndexNE(const char *name);

  /**
   * Returns the variable for given name. Throws error code  when something goes wrong
   * @param name The name of the dimension to look for
   * @return The variable pointer
   */
  CDF::Variable *getVariableThrows(const char *name);

  CDF::Variable *addVariable(CDF::Variable *var);
  int removeVariable(const char *name);
  int removeDimension(const char *name);
  CDF::Dimension *addDimension(CDF::Dimension *dim);
  CDF::Dimension *getDimensionThrows(const char *name);
  CDF::Dimension *getDimensionIgnoreCaseThrows(const char *name);

  int applyNCMLFile(const char *ncmlFileName);
  int aggregateDim(CDFObject *sourceCDFObject, const char *dimName);
  CT::string currentFile;
  int open(const char *fileName);
  int close();
  void clear();
  int attachCDFReader(void *reader);
  void *getCDFReader() { return reader; }
};

#endif

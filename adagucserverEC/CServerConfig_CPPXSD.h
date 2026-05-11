/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#ifndef CServerConfig_H
#define CServerConfig_H
#include "CXMLSerializerInterface.h"
#include "CDirReader.h"
#include "CColor.h"
#include <cfloat>

// Struct for keeping settings for a datapostprocessor. Can be assigned to new structs by assigment operator. Also used in CDataPostProcessor.cpp
struct XMLE_DataPostProcAttributes {
  int postProcIndexInLayer = 0;
  CT::string a, b, c, units, algorithm, mode, name, select, standard_name, long_name, variable, directionname, speedname, from_units, offset, stride;
};

class CServerConfig : public CXMLObjectInterface {
public:
  class XMLE_palette : public CXMLObjectInterface {
  public:
    XMLE_palette() {
      attr.alpha = 255;
      attr.index = -1;
    }
    class Cattr {
    public:
      int min, max, index, red, green, blue, alpha;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("min" == attrCfg.name) {
        attr.min = parseInt(attrCfg);
        return true;
      } else if ("max" == attrCfg.name) {
        attr.max = parseInt(attrCfg);
        return true;
      } else if ("red" == attrCfg.name) {
        attr.red = parseInt(attrCfg);
        return true;
      } else if ("blue" == attrCfg.name) {
        attr.blue = parseInt(attrCfg);
        return true;
      } else if ("green" == attrCfg.name) {
        attr.green = parseInt(attrCfg);
        return true;
      } else if ("alpha" == attrCfg.name) {
        attr.alpha = parseInt(attrCfg);
        return true;
      } else if ("index" == attrCfg.name) {
        attr.index = parseInt(attrCfg);
        return true;
      } else if ("color" == attrCfg.name) { // Hex color like: #A41D23
        if (attrCfg.value.length() > 1 && attrCfg.value.at(0) == '#') {
          CColor color(attrCfg.value);
          attr.red = color.r;
          attr.green = color.g;
          attr.blue = color.b;
          attr.alpha = color.a;
          return true;
        }
      }
      return false;
    }
  };

  class XMLE_WMSFormat : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, format, quality;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      } else if ("quality" == attrCfg.name) {
        attr.quality = attrCfg.value;
        return true;
      } else if ("format" == attrCfg.name) {
        attr.format = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_WMSExceptions : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string defaultValue, force;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("defaultvalue" == attrCfg.name) {
        attr.defaultValue = attrCfg.value;
        return true;
      } else if ("force" == attrCfg.name) {
        attr.force = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Logging : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string debug;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("debug" == attrCfg.name) {
        attr.debug = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Thinning : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string radius;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("radius" == attrCfg.name) {
        attr.radius = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_FilterPoints : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string use;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("use" == attrCfg.name) {
        attr.use = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Symbol : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, coordinates;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      } else if ("coordinates" == attrCfg.name) {
        attr.coordinates = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Vector : public CXMLObjectInterface {
  public:
    XMLE_Vector() {
      attr.scale = 1.0;
      attr.outlinewidth = 4.5;
      attr.linewidth = 1.0;
      attr.fontSize = 12;
      attr.discradius = 20;
      attr.min = -DBL_MAX;
      attr.max = DBL_MAX;
    }
    class Cattr {
    public:
      CT::string linecolor, plotstationid, vectorstyle, textformat, plotvalue, outlinecolor, textcolor, fillcolor, fontfile;
      float scale;
      double min, max, outlinewidth, fontSize, linewidth, discradius;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("linecolor" == attrCfg.name) {
        attr.linecolor = attrCfg.value;
        return true;
      } else if ("linewidth" == attrCfg.name) {
        attr.linewidth = parseDouble(attrCfg);
        return true;
      } else if ("scale" == attrCfg.name) {
        attr.scale = parseDouble(attrCfg);
        return true;
      } else if ("discradius" == attrCfg.name) {
        attr.discradius = parseDouble(attrCfg);
        return true;
      } else if ("vectorstyle" == attrCfg.name) {
        attr.vectorstyle = attrCfg.value;
        return true;
      } else if ("plotstationid" == attrCfg.name) {
        attr.plotstationid = attrCfg.value;
        return true;
      } else if ("textformat" == attrCfg.name) {
        attr.textformat = attrCfg.value;
        return true;
      } else if ("plotvalue" == attrCfg.name) {
        attr.plotvalue = attrCfg.value;
        return true;
      } else if ("fontfile" == attrCfg.name) {
        attr.fontfile = attrCfg.value;
        return true;
      } else if ("min" == attrCfg.name) {
        attr.min = parseDouble(attrCfg);
        return true;
      } else if ("max" == attrCfg.name) {
        attr.max = parseDouble(attrCfg);
        return true;
      } else if ("fontsize" == attrCfg.name) {
        attr.fontSize = parseDouble(attrCfg);
        return true;
      } else if ("outlinewidth" == attrCfg.name) {
        attr.outlinewidth = parseDouble(attrCfg);
        return true;
      } else if ("outlinecolor" == attrCfg.name) {
        attr.outlinecolor = attrCfg.value;
        return true;
      } else if ("textcolor" == attrCfg.name) {
        attr.textcolor = attrCfg.value;
        return true;
      } else if ("fillcolor" == attrCfg.name) {
        attr.fillcolor = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Point : public CXMLObjectInterface {
  public:
    XMLE_Point() {
      attr.min = -DBL_MAX;
      attr.max = DBL_MAX;

      attr.maxpointspercell = -1;
      attr.maxpointcellsize = -1;
    }
    class Cattr {
    public:
      CT::string fillcolor, linecolor, textcolor, textoutlinecolor, fontfile, fontsize, discradius, textradius, dot, textformat, plotstationid, pointstyle, symbol;
      double min, max;
      int maxpointspercell, maxpointcellsize;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("fillcolor" == attrCfg.name) {
        attr.fillcolor = attrCfg.value;
        return true;
      } else if ("linecolor" == attrCfg.name) {
        attr.linecolor = attrCfg.value;
        return true;
      } else if ("textcolor" == attrCfg.name) {
        attr.textcolor = attrCfg.value;
        return true;
      } else if ("textoutlinecolor" == attrCfg.name) {
        attr.textoutlinecolor = attrCfg.value;
        return true;
      } else if ("fontfile" == attrCfg.name) {
        attr.fontfile = attrCfg.value;
        return true;
      } else if ("fontsize" == attrCfg.name) {
        attr.fontsize = attrCfg.value;
        return true;
      } else if ("discradius" == attrCfg.name) {
        attr.discradius = attrCfg.value;
        return true;
      } else if ("textradius" == attrCfg.name) {
        attr.textradius = attrCfg.value;
        return true;
      } else if ("dot" == attrCfg.name) {
        attr.dot = attrCfg.value;
        return true;
      } else if ("textformat" == attrCfg.name) {
        attr.textformat = attrCfg.value;
        return true;
      } else if ("plotstationid" == attrCfg.name) {
        attr.plotstationid = attrCfg.value;
        return true;
      } else if ("pointstyle" == attrCfg.name) {
        attr.pointstyle = attrCfg.value;
        return true;
      } else if ("min" == attrCfg.name) {
        attr.min = parseDouble(attrCfg);
        return true;
      } else if ("max" == attrCfg.name) {
        attr.max = parseDouble(attrCfg);
        return true;
      } else if ("symbol" == attrCfg.name) {
        attr.symbol = attrCfg.value;
        return true;
      } else if ("maxpointspercell" == attrCfg.name) {
        attr.maxpointspercell = parseInt(attrCfg);
        return true;
      } else if ("maxpointcellsize" == attrCfg.name) {
        attr.maxpointcellsize = parseInt(attrCfg);
        return true;
      }
      return false;
    }
  };

  class XMLE_Legend : public CXMLObjectInterface {
  public:
    std::vector<XMLE_palette *> palette;
    ~XMLE_Legend() { XMLE_DELOBJ(palette); }
    class Cattr {
    public:
      CT::string name, type, tickround, tickinterval, fixedclasses, file, textformatting;
    } attr;
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("palette" == elName) {
        XMLE_ADDOBJ(palette);
      }
      return nullptr;
    }
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name.copy(value);
        return true;
      } else if ("type" == attrCfg.name) {
        attr.type.copy(value);
        return true;
      } else if ("file" == attrCfg.name) {
        attr.file.copy(value);
        return true;
      } else if ("tickround" == attrCfg.name) {
        attr.tickround.copy(value);
        return true;
      } else if ("tickinterval" == attrCfg.name) {
        attr.tickinterval.copy(value);
        return true;
      } else if ("fixedclasses" == attrCfg.name || "fixed" == attrCfg.name) {
        attr.fixedclasses.copy(value);
        return true;
      } else if ("textformatting" == attrCfg.name) {
        attr.textformatting.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_Scale : public CXMLObjectInterface {};
  class XMLE_Offset : public CXMLObjectInterface {};
  class XMLE_Min : public CXMLObjectInterface {};
  class XMLE_Max : public CXMLObjectInterface {};

  /*Deprecated*/
  class XMLE_ContourIntervalL : public CXMLObjectInterface {};
  /*Deprecated*/
  class XMLE_ContourIntervalH : public CXMLObjectInterface {};

  class XMLE_ContourLine : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string width, dashing, linecolor, textcolor, textstrokecolor, classes, interval, textformatting, textsize, textstrokewidth;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("width" == attrCfg.name) {
        attr.width = attrCfg.value;
        return true;
      } else if ("dashing" == attrCfg.name) {
        attr.dashing = attrCfg.value;
        return true;
      } else if ("linecolor" == attrCfg.name) {
        attr.linecolor = attrCfg.value;
        return true;
      } else if ("textsize" == attrCfg.name) {
        attr.textsize = attrCfg.value;
        return true;
      } else if ("textcolor" == attrCfg.name) {
        attr.textcolor = attrCfg.value;
        return true;
      } else if ("textstrokecolor" == attrCfg.name) {
        attr.textstrokecolor = attrCfg.value;
        return true;
      } else if ("classes" == attrCfg.name) {
        attr.classes = attrCfg.value;
        return true;
      } else if ("interval" == attrCfg.name) {
        attr.interval = attrCfg.value;
        return true;
      } else if ("textformatting" == attrCfg.name) {
        attr.textformatting = attrCfg.value;
        return true;
      } else if ("textstrokewidth" == attrCfg.name) {
        attr.textstrokewidth = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_ShadeInterval : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string min, max, label, fillcolor, bgcolor;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("min" == attrCfg.name) {
        attr.min = attrCfg.value;
        return true;
      } else if ("max" == attrCfg.name) {
        attr.max = attrCfg.value;
        return true;
      } else if ("label" == attrCfg.name) {
        attr.label = attrCfg.value;
        return true;
      } else if ("fillcolor" == attrCfg.name) {
        attr.fillcolor = attrCfg.value;
        return true;
      } else if ("bgcolor" == attrCfg.name) {
        attr.bgcolor = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_SymbolInterval : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string min, max, binary_and;
      CT::string file;
      CT::string offsetX, offsetY;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("min" == attrCfg.name) {
        attr.min = attrCfg.value;
        return true;
      } else if ("max" == attrCfg.name) {
        attr.max = attrCfg.value;
        return true;
      } else if ("file" == attrCfg.name) {
        attr.file = attrCfg.value;
        return true;
      } else if ("binary_and" == attrCfg.name) {
        attr.binary_and = attrCfg.value;
        return true;
      } else if ("offset_x" == attrCfg.name) {
        attr.offsetX = attrCfg.value;
        return true;
      } else if ("offset_y" == attrCfg.name) {
        attr.offsetY = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_SmoothingFilter : public CXMLObjectInterface {};
  class XMLE_StandardNames : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string units, standard_name, variable_name;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("units" == attrCfg.name) {
        attr.units = attrCfg.value;
        return true;
      } else if ("standard_name" == attrCfg.name) {
        attr.standard_name = attrCfg.value;
        return true;
      } else if ("variable_name" == attrCfg.name) {
        attr.variable_name = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_LegendGraphic : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string value;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("value" == attrCfg.name) {
        attr.value = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Log : public CXMLObjectInterface {};
  class XMLE_ValueRange : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string min, max;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("min" == attrCfg.name) {
        attr.min = attrCfg.value;
        return true;
      } else if ("max" == attrCfg.name) {
        attr.max = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_ContourFont : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string location, size;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("size" == attrCfg.name) {
        attr.size = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_TitleFont : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string location, size;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("size" == attrCfg.name) {
        attr.size = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_SubTitleFont : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string location, size;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("size" == attrCfg.name) {
        attr.size = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_DimensionFont : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string location, size;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("size" == attrCfg.name) {
        attr.size = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_GridFont : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string location, size;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("size" == attrCfg.name) {
        attr.size = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_LegendFont : public XMLE_GridFont {
  public:
    class Cattr {
    public:
      CT::string location, size;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("size" == attrCfg.name) {
        attr.size = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Dir : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string basedir, prefix;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("prefix" == attrCfg.name) {
        attr.prefix = attrCfg.value;
        return true;
      } else if ("basedir" == attrCfg.name) {
        attr.basedir = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_ImageText : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string attribute;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("attribute" == attrCfg.name) {
        attr.attribute = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_AutoResource : public CXMLObjectInterface {
  public:
    std::vector<XMLE_Dir *> Dir;
    std::vector<XMLE_ImageText *> ImageText;
    ~XMLE_AutoResource() {
      XMLE_DELOBJ(Dir);
      XMLE_DELOBJ(ImageText);
    }
    class Cattr {
    public:
      CT::string enableautoopendap, enablelocalfile;
    } attr;

    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Dir" == elName) {
        XMLE_ADDOBJ(Dir);
      } else if ("ImageText" == elName) {
        XMLE_ADDOBJ(ImageText);
      }
      return nullptr;
    }

    bool addAttribute(const attribute &attrCfg) {
      if ("enableautoopendap" == attrCfg.name) {
        attr.enableautoopendap = attrCfg.value;
        return true;
      } else if ("enablelocalfile" == attrCfg.name) {
        attr.enablelocalfile = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Dataset : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string enabled, location;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("enabled" == attrCfg.name) {
        attr.enabled = attrCfg.value;
        return true;
      } else if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Include : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string location;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("location" == attrCfg.name) {
        attr.location = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_IncludeStyle : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_NameMapping : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, title, abstract;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      } else if ("title" == attrCfg.name) {
        attr.title = attrCfg.value;
        return true;
      } else if ("abstract" == attrCfg.name) {
        attr.abstract = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_FeatureInterval : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string match, matchid, bgcolor, label, fillcolor, linecolor, linewidth, bordercolor, borderwidth;
      CT::string labelfontfile, labelfontsize, labelcolor, labelpropertyname, labelpropertyformat, labelangle;
      CT::string labelpadding;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("match" == attrCfg.name) {
        attr.match = attrCfg.value;
        return true;
      } else if ("label" == attrCfg.name) {
        attr.label = attrCfg.value;
        return true;
      } else if ("matchid" == attrCfg.name) {
        attr.matchid = attrCfg.value;
        return true;
      } else if ("fillcolor" == attrCfg.name) {
        attr.fillcolor = attrCfg.value;
        return true;
      } else if ("bgcolor" == attrCfg.name) {
        attr.bgcolor = attrCfg.value;
        return true;
      } else if ("borderwidth" == attrCfg.name) {
        attr.borderwidth = attrCfg.value;
        return true;
      } else if ("bordercolor" == attrCfg.name) {
        attr.bordercolor = attrCfg.value;
        return true;
      } else if ("labelfontsize" == attrCfg.name) {
        attr.labelfontsize = attrCfg.value;
        return true;
      } else if ("labelfontfile" == attrCfg.name) {
        attr.labelfontfile = attrCfg.value;
        return true;
      } else if ("labelcolor" == attrCfg.name) {
        attr.labelcolor = attrCfg.value;
        return true;
      } else if ("labelpropertyname" == attrCfg.name) {
        attr.labelpropertyname = attrCfg.value;
        return true;
      } else if ("labelpropertyformat" == attrCfg.name) {
        attr.labelpropertyformat = attrCfg.value;
        return true;
      } else if ("labelangle" == attrCfg.name) {
        attr.labelangle = attrCfg.value;
        return true;
      } else if ("labelpadding" == attrCfg.name) {
        attr.labelpadding = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Stippling : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string distancex, distancey, discradius, mode, color;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("distancex" == attrCfg.name) {
        attr.distancex = attrCfg.value;
        return true;
      } else if ("distancey" == attrCfg.name) {
        attr.distancey = attrCfg.value;
        return true;
      } else if ("discradius" == attrCfg.name) {
        attr.discradius = attrCfg.value;
        return true;
      } else if ("mode" == attrCfg.name) {
        attr.mode = attrCfg.value;
        return true;
      } else if ("color" == attrCfg.name) {
        attr.color = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_RenderMethod : public CXMLObjectInterface {};

  class XMLE_RenderSettings : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string settings, striding, scalewidth, scalecontours, renderhint, randomizefeatures, featuresoverlap, rendertextforvectors, cliplegend, interpolationmethod, drawgridboxoutline, drawgrid;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("settings" == attrCfg.name) {
        attr.settings.copy(value);
        return true;
      } else if ("striding" == attrCfg.name) {
        attr.striding.copy(value);
        return true;
      } else if ("renderhint" == attrCfg.name) {
        attr.renderhint.copy(value);
        return true;
      } else if ("scalewidth" == attrCfg.name) {
        attr.scalewidth.copy(value);
        return true;
      } else if ("scalecontours" == attrCfg.name) {
        attr.scalecontours.copy(value);
        return true;
      } else if ("randomizefeatures" == attrCfg.name) {
        attr.randomizefeatures.copy(value);
        return true;
      } else if ("featuresoverlap" == attrCfg.name) {
        attr.featuresoverlap.copy(value);
        return true;
      } else if ("rendertextforvectors" == attrCfg.name) {
        attr.rendertextforvectors.copy(value);
        return true;
      } else if ("cliplegend" == attrCfg.name) {
        attr.cliplegend.copy(value);
        return true;
      } else if ("interpolationmethod" == attrCfg.name) {
        attr.interpolationmethod.copy(value);
        return true;
      } else if ("drawgridboxoutline" == attrCfg.name) {
        attr.drawgridboxoutline.copy(value);
        return true;
      } else if ("drawgrid" == attrCfg.name) {
        attr.drawgrid.copy(value);
        return true;
      }
      return false;
    }
  };

  class XMLE_DataPostProc : public CXMLObjectInterface {
  public:
    XMLE_DataPostProcAttributes attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("a" == attrCfg.name) {
        attr.a = attrCfg.value;
        return true;
      } else if ("b" == attrCfg.name) {
        attr.b = attrCfg.value;
        return true;
      } else if ("c" == attrCfg.name) {
        attr.c = attrCfg.value;
        return true;
      } else if ("mode" == attrCfg.name) {
        attr.mode = attrCfg.value;
        return true;
      } else if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      } else if ("units" == attrCfg.name) {
        attr.units = attrCfg.value;
        return true;
      } else if ("select" == attrCfg.name) {
        attr.select = attrCfg.value;
        return true;
      } else if ("standard_name" == attrCfg.name) {
        attr.standard_name = attrCfg.value;
        return true;
      } else if ("variable" == attrCfg.name) {
        attr.variable = attrCfg.value;
        return true;
      } else if ("long_name" == attrCfg.name) {
        attr.long_name = attrCfg.value;
        return true;
      } else if ("algorithm" == attrCfg.name) {
        attr.algorithm = attrCfg.value;
        return true;
      } else if ("directionname" == attrCfg.name) {
        attr.directionname = attrCfg.value;
        return true;
      } else if ("speedname" == attrCfg.name) {
        attr.speedname = attrCfg.value;
        return true;
      } else if ("from_units" == attrCfg.name) {
        attr.from_units = attrCfg.value;
        return true;
      } else if ("offset" == attrCfg.name) {
        attr.offset = attrCfg.value;
        return true;
      } else if ("stride" == attrCfg.name) {
        attr.stride = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Style : public CXMLObjectInterface {
  public:
    std::vector<XMLE_Thinning *> Thinning;
    std::vector<XMLE_Point *> Point;
    std::vector<XMLE_Vector *> Vector;
    std::vector<XMLE_FilterPoints *> FilterPoints;
    std::vector<XMLE_Legend *> Legend;
    std::vector<XMLE_Scale *> Scale;
    std::vector<XMLE_Offset *> Offset;
    std::vector<XMLE_Min *> Min;
    std::vector<XMLE_Max *> Max;
    std::vector<XMLE_ContourIntervalL *> ContourIntervalL;
    std::vector<XMLE_ContourIntervalH *> ContourIntervalH;
    std::vector<XMLE_Log *> Log;
    std::vector<XMLE_ValueRange *> ValueRange;
    std::vector<XMLE_RenderMethod *> RenderMethod;
    std::vector<XMLE_ShadeInterval *> ShadeInterval;
    std::vector<XMLE_SymbolInterval *> SymbolInterval;
    std::vector<XMLE_ContourLine *> ContourLine;
    std::vector<XMLE_NameMapping *> NameMapping;
    std::vector<XMLE_SmoothingFilter *> SmoothingFilter;
    std::vector<XMLE_StandardNames *> StandardNames;
    std::vector<XMLE_LegendGraphic *> LegendGraphic;
    std::vector<XMLE_FeatureInterval *> FeatureInterval;
    std::vector<XMLE_Stippling *> Stippling;
    std::vector<XMLE_RenderSettings *> RenderSettings;
    std::vector<XMLE_DataPostProc *> DataPostProc;
    std::vector<XMLE_IncludeStyle *> IncludeStyle;

    ~XMLE_Style() {
      XMLE_DELOBJ(Thinning);
      XMLE_DELOBJ(Point);
      XMLE_DELOBJ(Vector);
      XMLE_DELOBJ(FilterPoints);
      XMLE_DELOBJ(Legend);
      XMLE_DELOBJ(Scale);
      XMLE_DELOBJ(Offset);
      XMLE_DELOBJ(Min);
      XMLE_DELOBJ(Max);
      XMLE_DELOBJ(Log);
      XMLE_DELOBJ(ValueRange);
      XMLE_DELOBJ(ContourIntervalL);
      XMLE_DELOBJ(ContourIntervalH);
      XMLE_DELOBJ(RenderMethod);
      XMLE_DELOBJ(ShadeInterval);
      XMLE_DELOBJ(SymbolInterval);
      XMLE_DELOBJ(ContourLine);
      XMLE_DELOBJ(NameMapping);
      XMLE_DELOBJ(SmoothingFilter);
      XMLE_DELOBJ(StandardNames);
      XMLE_DELOBJ(LegendGraphic);
      XMLE_DELOBJ(FeatureInterval);
      XMLE_DELOBJ(Stippling);
      XMLE_DELOBJ(RenderSettings);
      XMLE_DELOBJ(DataPostProc);
      XMLE_DELOBJ(IncludeStyle);
    }
    class Cattr {
    public:
      CT::string name, title, abstract;
    } attr;
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Thinning" == elName) {
        XMLE_ADDOBJ(Thinning);
      } else if ("Point" == elName) {
        XMLE_ADDOBJ(Point);
      } else if ("Vector" == elName) {
        XMLE_ADDOBJ(Vector);
      } else if ("FilterPoints" == elName) {
        XMLE_ADDOBJ(FilterPoints);
      } else if ("Legend" == elName) {
        XMLE_ADDOBJ(Legend);
      } else if ("Scale" == elName) {
        XMLE_ADDOBJ(Scale);
      } else if ("Offset" == elName) {
        XMLE_ADDOBJ(Offset);
      } else if ("Min" == elName) {
        XMLE_ADDOBJ(Min);
      } else if ("Max" == elName) {
        XMLE_ADDOBJ(Max);
      } else if ("Log" == elName) {
        XMLE_ADDOBJ(Log);
      } else if ("ValueRange" == elName) {
        XMLE_ADDOBJ(ValueRange);
      } else if ("ContourIntervalL" == elName) {
        XMLE_ADDOBJ(ContourIntervalL);
      } else if ("ContourIntervalH" == elName) {
        XMLE_ADDOBJ(ContourIntervalH);
      } else if ("RenderMethod" == elName) {
        XMLE_ADDOBJ(RenderMethod);
      } else if ("ShadeInterval" == elName) {
        XMLE_ADDOBJ(ShadeInterval);
      } else if ("SymbolInterval" == elName) {
        XMLE_ADDOBJ(SymbolInterval);
      } else if ("ContourLine" == elName) {
        XMLE_ADDOBJ(ContourLine);
      } else if ("NameMapping" == elName) {
        XMLE_ADDOBJ(NameMapping);
      } else if ("SmoothingFilter" == elName) {
        XMLE_ADDOBJ(SmoothingFilter);
      } else if ("StandardNames" == elName) {
        XMLE_ADDOBJ(StandardNames);
      } else if ("LegendGraphic" == elName) {
        XMLE_ADDOBJ(LegendGraphic);
      } else if ("FeatureInterval" == elName) {
        XMLE_ADDOBJ(FeatureInterval);
      } else if ("Stippling" == elName) {
        XMLE_ADDOBJ(Stippling);
      } else if ("RenderSettings" == elName) {
        XMLE_ADDOBJ(RenderSettings);
      } else if ("DataPostProc" == elName) {
        XMLE_ADDOBJ(DataPostProc);

      } else if ("IncludeStyle" == elName) {
        XMLE_ADDOBJ(IncludeStyle);
      }
      return nullptr;
    }

    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name.copy(value);
        return true;
      } else if ("title" == attrCfg.name) {
        attr.title.copy(value);
        return true;
      }
      if ("abstract" == attrCfg.name) {
        attr.abstract.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_Styles : public CXMLObjectInterface {};
  class XMLE_Title : public CXMLObjectInterface {};

  class XMLE_MetadataURL : public CXMLObjectInterface {};

  class XMLE_AuthorityURL : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, onlineresource;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name.copy(value);
        return true;
      } else if ("onlineresource" == attrCfg.name) {
        attr.onlineresource.copy(value);
        return true;
      }
      return false;
    }
  };

  class XMLE_Identifier : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string authority, id;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("id" == attrCfg.name) {
        attr.id.copy(value);
        return true;
      } else if ("authority" == attrCfg.name) {
        attr.authority.copy(value);
        return true;
      }
      return false;
    }
  };

  class XMLE_Name : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string force;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("force" == attrCfg.name) {
        attr.force.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_Abstract : public CXMLObjectInterface {};

  class XMLE_DataBaseTable : public CXMLObjectInterface {};
  class XMLE_Variable : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string orgname, long_name, standard_name, units;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("orgname" == attrCfg.name) {
        attr.orgname.copy(value);
        return true;
      } else if ("long_name" == attrCfg.name) {
        attr.long_name.copy(value);
        return true;
      } else if ("standard_name" == attrCfg.name) {
        attr.standard_name.copy(value);
        return true;
      } else if ("units" == attrCfg.name) {
        attr.units.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_DataReader : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string useendtime;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("useendtime" == attrCfg.name) {
        attr.useendtime.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_FilePath : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string filter, gfi_openall, ncml, maxquerylimit, retentionperiod, retentiontype;
    } attr;
    void handleValue() { this->value = CDirReader::makeCleanPath(this->value.c_str()); }

    bool addAttribute(const attribute &attrCfg) {
      if ("filter" == attrCfg.name) {
        attr.filter.copy(value);
        return true;
      } else if ("gfi_openall" == attrCfg.name) {
        attr.gfi_openall.copy(value);
        return true;
      } else if ("maxquerylimit" == attrCfg.name) {
        attr.maxquerylimit.copy(value);
        return true;
      } else if ("ncml" == attrCfg.name) {
        attr.ncml.copy(value);
        return true;
      } else if ("retentionperiod" == attrCfg.name) {
        attr.retentionperiod.copy(value);
        return true;
      } else if ("retentiontype" == attrCfg.name) {
        attr.retentiontype.copy(value);
        return true;
      }
      return false;
    }
  };

  class XMLE_TileSettings : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string tilewidthpx = "1024", tileheightpx = "1024", minlevel, maxlevel, tilemode, debug, maxtilesinimage, threads, autotile, optimizeextent, tilepath;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("tilemode" == attrCfg.name) {
        attr.tilemode.copy(value);
        return true;
      } else if ("debug" == attrCfg.name) {
        attr.debug.copy(value);
        return true;
      } else if ("tilewidthpx" == attrCfg.name) {
        attr.tilewidthpx.copy(value);
        return true;
      } else if ("tileheightpx" == attrCfg.name) {
        attr.tileheightpx.copy(value);
        return true;
      } else if ("minlevel" == attrCfg.name) {
        attr.minlevel.copy(value);
        return true;
      } else if ("maxlevel" == attrCfg.name) {
        attr.maxlevel.copy(value);
        return true;
      } else if ("maxtilesinimage" == attrCfg.name) {
        attr.maxtilesinimage.copy(value);
        return true;
      } else if ("threads" == attrCfg.name) {
        attr.threads.copy(value);
        return true;
      } else if ("autotile" == attrCfg.name) {
        attr.autotile.copy(value);
        return true;
      } else if ("optimizeextent" == attrCfg.name) {
        attr.optimizeextent.copy(value);
        return true;
      } else if ("tilepath" == attrCfg.name) {
        attr.tilepath.copy(value);
        return true;
      }
      return false;
    }
  };

  class XMLE_Group : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string value;
      CT::string collection;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("value" == attrCfg.name) {
        attr.value.copy(value);
        return true;
      }
      if ("collection" == attrCfg.name) {
        attr.collection.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_LatLonBox : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      float minx, miny, maxx, maxy;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("minx" == attrCfg.name) {
        attr.minx = parseDouble(attrCfg);
        return true;
      } else if ("miny" == attrCfg.name) {
        attr.miny = parseDouble(attrCfg);
        return true;
      } else if ("maxx" == attrCfg.name) {
        attr.maxx = parseDouble(attrCfg);
        return true;
      } else if ("maxy" == attrCfg.name) {
        attr.maxy = parseDouble(attrCfg);
        return true;
      }
      return false;
    }
  };

  class XMLE_Dimension : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, interval, defaultV, units, quantizeperiod, quantizemethod, fixvalue;
      bool hidden = false;
      CT::string type = "dimtype_none";
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      } else if ("units" == attrCfg.name) {
        attr.units = attrCfg.value;
        return true;
      } else if ("default" == attrCfg.name) {
        attr.defaultV = attrCfg.value;
        return true;
      } else if ("interval" == attrCfg.name) {
        attr.interval = attrCfg.value;
        return true;
      } else if ("quantizeperiod" == attrCfg.name) {
        attr.quantizeperiod = attrCfg.value;
        return true;
      } else if ("fixvalue" == attrCfg.name) {
        attr.fixvalue = attrCfg.value;
        return true;
      } else if ("hidden" == attrCfg.name) {
        attr.hidden = parseBool(attrCfg);
        return true;
      } else if ("quantizemethod" == attrCfg.name) {
        attr.quantizemethod = attrCfg.value;
        return true;
      } else if ("type" == attrCfg.name) {
        if (attrCfg.value == "vertical") {
          attr.type = "dimtype_vertical";
        } else if (attrCfg.value == "custom") {
          attr.type = "dimtype_custom";
        } else if (attrCfg.value == "time") {
          attr.type = "dimtype_time";
        } else if (attrCfg.value == "reference_time") {
          attr.type = "dimtype_reference_time";
        }
        return true;
      }
      return false;
    }
  };

  class XMLE_Path : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string value;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("value" == attrCfg.name) {
        attr.value = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_TempDir : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string value;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("value" == attrCfg.name) {
        attr.value = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Environment : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, defaultVal;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name.copy(value);
        return true;
      } else if ("default" == attrCfg.name) {
        attr.defaultVal.copy(value);
        return true;
      }
      return false;
    }
  };
  class XMLE_Settings : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string enablemetadatacache, enablecleanupsystem, cleanupsystemlimit, cache_age_cacheableresources, cache_age_volatileresources;
      CT::string enable_edr = "true";
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("enablemetadatacache" == attrCfg.name) {
        attr.enablemetadatacache = attrCfg.value;
        return true;
      } else if ("enablecleanupsystem" == attrCfg.name) {
        attr.enablecleanupsystem = attrCfg.value;
        return true;
      } else if ("cleanupsystemlimit" == attrCfg.name) {
        attr.cleanupsystemlimit = attrCfg.value;
        return true;
      } else if ("cache_age_cacheableresources" == attrCfg.name) {
        attr.cache_age_cacheableresources = attrCfg.value;
        return true;
      } else if ("cache_age_volatileresources" == attrCfg.name) {
        attr.cache_age_volatileresources = attrCfg.value;
        return true;
      } else if ("enable_edr" == attrCfg.name) {
        attr.enable_edr = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_OnlineResource : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string value;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("value" == attrCfg.name) {
        attr.value = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_DataBase : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string parameters, maxquerylimit;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("parameters" == attrCfg.name) {
        attr.parameters = attrCfg.value;
        return true;
      } else if ("maxquerylimit" == attrCfg.name) {
        attr.maxquerylimit = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_Projection : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string id, proj4;
    } attr;
    std::vector<XMLE_LatLonBox *> LatLonBox;
    ~XMLE_Projection() { XMLE_DELOBJ(LatLonBox); }
    bool addAttribute(const attribute &attrCfg) {
      if ("id" == attrCfg.name) {
        attr.id = attrCfg.value;
        return true;
      }
      if ("proj4" == attrCfg.name) {
        attr.proj4 = attrCfg.value;
        return true;
      }
      return false;
    }
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("LatLonBox" == elName) {
        XMLE_ADDOBJ(LatLonBox);
      }
      return nullptr;
    }
  };
  class XMLE_Cache : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string enabled, optimizeextent;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("enabled" == attrCfg.name) {
        attr.enabled = attrCfg.value;
        return true;
      } else if ("optimizeextent" == attrCfg.name) {
        attr.optimizeextent = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_WCSFormat : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, driver, mimetype, options;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      }
      if ("driver" == attrCfg.name) {
        attr.driver = attrCfg.value;
        return true;
      }
      if ("mimetype" == attrCfg.name) {
        attr.mimetype = attrCfg.value;
        return true;
      }
      if ("options" == attrCfg.name) {
        attr.options = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_RootLayer : public CXMLObjectInterface {
  public:
    std::vector<XMLE_Name *> Name;
    std::vector<XMLE_Title *> Title;
    std::vector<XMLE_Abstract *> Abstract;
    ~XMLE_RootLayer() {
      XMLE_DELOBJ(Name);
      XMLE_DELOBJ(Title);
      XMLE_DELOBJ(Abstract);
    }
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Name" == elName) {
        XMLE_SETOBJ(Name);
      } else if ("Title" == elName) {
        XMLE_SETOBJ(Title);
      } else if ("Abstract" == elName) {
        XMLE_SETOBJ(Abstract);
      }
      return nullptr;
    }
  };

  class XMLE_ViewServiceCSW : public CXMLObjectInterface {};
  class XMLE_DatasetCSW : public CXMLObjectInterface {};

  class XMLE_Inspire : public CXMLObjectInterface {
  public:
    std::vector<XMLE_ViewServiceCSW *> ViewServiceCSW;
    std::vector<XMLE_DatasetCSW *> DatasetCSW;
    std::vector<XMLE_AuthorityURL *> AuthorityURL;
    std::vector<XMLE_Identifier *> Identifier;
    ~XMLE_Inspire() {
      XMLE_DELOBJ(ViewServiceCSW);
      XMLE_DELOBJ(DatasetCSW);
      XMLE_DELOBJ(AuthorityURL);
      XMLE_DELOBJ(Identifier);
    }

    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("ViewServiceCSW" == elName) {
        XMLE_ADDOBJ(ViewServiceCSW);
      } else if ("DatasetCSW" == elName) {
        XMLE_ADDOBJ(DatasetCSW);
      } else if ("AuthorityURL" == elName) {
        XMLE_ADDOBJ(AuthorityURL);
      } else if ("Identifier" == elName) {
        XMLE_ADDOBJ(Identifier);
      }
      return nullptr;
    }
  };

  class XMLE_WMS : public CXMLObjectInterface {
  public:
    std::vector<XMLE_Title *> Title;
    std::vector<XMLE_Abstract *> Abstract;
    std::vector<XMLE_RootLayer *> RootLayer;
    std::vector<XMLE_TitleFont *> TitleFont;
    std::vector<XMLE_ContourFont *> ContourFont;
    std::vector<XMLE_LegendFont *> LegendFont;
    std::vector<XMLE_SubTitleFont *> SubTitleFont;
    std::vector<XMLE_DimensionFont *> DimensionFont;
    std::vector<XMLE_GridFont *> GridFont;
    std::vector<XMLE_WMSFormat *> WMSFormat;
    std::vector<XMLE_WMSExceptions *> WMSExceptions;
    std::vector<XMLE_Inspire *> Inspire;

    ~XMLE_WMS() {
      XMLE_DELOBJ(Title);
      XMLE_DELOBJ(Abstract);
      XMLE_DELOBJ(RootLayer);
      XMLE_DELOBJ(TitleFont);
      XMLE_DELOBJ(ContourFont);
      XMLE_DELOBJ(LegendFont);
      XMLE_DELOBJ(SubTitleFont);
      XMLE_DELOBJ(DimensionFont);
      XMLE_DELOBJ(GridFont);
      XMLE_DELOBJ(WMSFormat);
      XMLE_DELOBJ(WMSExceptions);
      // XMLE_DELOBJ(Keywords);

      XMLE_DELOBJ(Inspire);
    }
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Title" == elName) {
        XMLE_SETOBJ(Title);
      } else if ("GridFont" == elName) {
        XMLE_SETOBJ(GridFont);
      } else if ("Abstract" == elName) {
        XMLE_SETOBJ(Abstract);
      } else if ("RootLayer" == elName) {
        XMLE_SETOBJ(RootLayer);
      } else if ("WMSFormat" == elName) {
        XMLE_ADDOBJ(WMSFormat);
      } else if ("WMSExceptions" == elName) {
        XMLE_ADDOBJ(WMSExceptions);
      } else if ("TitleFont" == elName) {
        XMLE_ADDOBJ(TitleFont);
      } else if ("ContourFont" == elName) {
        XMLE_ADDOBJ(ContourFont);
      } else if ("LegendFont" == elName) {
        XMLE_SETOBJ(LegendFont);
      } else if ("SubTitleFont" == elName) {
        XMLE_ADDOBJ(SubTitleFont);
      } else if ("DimensionFont" == elName) {
        XMLE_ADDOBJ(DimensionFont);
      } else if ("Inspire" == elName) {
        XMLE_ADDOBJ(Inspire);
      }
      return nullptr;
    }
  };

  class XMLE_OpenDAP : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string enabled, path;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("enabled" == attrCfg.name) {
        attr.enabled = attrCfg.value;
        return true;
      } else if ("path" == attrCfg.name) {
        attr.path = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_WCS : public CXMLObjectInterface {
  public:
    std::vector<XMLE_Name *> Name;
    std::vector<XMLE_Title *> Title;
    std::vector<XMLE_Abstract *> Abstract;
    std::vector<XMLE_WCSFormat *> WCSFormat;
    ~XMLE_WCS() {
      XMLE_DELOBJ(Name);
      XMLE_DELOBJ(Title);
      XMLE_DELOBJ(Abstract);
      XMLE_DELOBJ(WCSFormat);
    }
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Name" == elName) {
        XMLE_ADDOBJ(Name);
      } else if ("Title" == elName) {
        XMLE_ADDOBJ(Title);
      } else if ("Abstract" == elName) {
        XMLE_ADDOBJ(Abstract);
      } else if ("WCSFormat" == elName) {
        XMLE_ADDOBJ(WCSFormat);
      }
      return nullptr;
    }
  };

  class XMLE_CacheDocs : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string enabled, cachefile;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("enabled" == attrCfg.name) {
        attr.enabled = attrCfg.value;
        return true;
      } else if ("cachefile" == attrCfg.name) {
        attr.cachefile = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_WMSLayer : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string service, layer, bgcolor, style;
      bool transparent;

      Cattr() {
        transparent = true;
        bgcolor.copy("");
        style.copy("");
      }
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("layer" == attrCfg.name) {
        attr.layer = attrCfg.value;
        return true;
      } else if ("service" == attrCfg.name) {
        attr.service = attrCfg.value;
        return true;
      } else if ("transparent" == attrCfg.name) {
        attr.transparent = parseBool(attrCfg);
        return true;
      } else if ("bgcolor" == attrCfg.name) {
        attr.bgcolor = attrCfg.value;
        return true;
      } else if ("style" == attrCfg.name) {
        attr.style = attrCfg.value;
        return true;
      }
      return false;
    }
  };
  class XMLE_Position : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string top, left, right, bottom;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("top" == attrCfg.name) {
        attr.top = attrCfg.value;
        return true;
      } else if ("left" == attrCfg.name) {
        attr.left = attrCfg.value;
        return true;
      } else if ("right" == attrCfg.name) {
        attr.right = attrCfg.value;
        return true;
      } else if ("bottom" == attrCfg.name) {
        attr.bottom = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Grid : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string name, precision, resolution;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("name" == attrCfg.name) {
        attr.name = attrCfg.value;
        return true;
      } else if ("precision" == attrCfg.name) {
        attr.precision = attrCfg.value;
        return true;
      } else if ("resolution" == attrCfg.name) {
        attr.resolution = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_AdditionalLayer : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string replace, style;
    } attr;
    bool addAttribute(const attribute &attrCfg) {
      if ("replace" == attrCfg.name) {
        attr.replace = attrCfg.value;
        return true;
      } else if ("style" == attrCfg.name) {
        attr.style = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Layer : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string type, hidden;
      CT::string enable_edr = "";
    } attr;

    std::vector<XMLE_Name *> Name;
    std::vector<XMLE_Group *> Group;
    std::vector<XMLE_Title *> Title;
    std::vector<XMLE_Abstract *> Abstract;

    std::vector<XMLE_DataBaseTable *> DataBaseTable;
    std::vector<XMLE_Variable *> Variable;
    std::vector<XMLE_FilePath *> FilePath;
    std::vector<XMLE_TileSettings *> TileSettings;
    std::vector<XMLE_DataReader *> DataReader;
    std::vector<XMLE_Dimension *> Dimension;
    std::vector<XMLE_Legend *> Legend;
    std::vector<XMLE_Scale *> Scale;
    std::vector<XMLE_Offset *> Offset;
    std::vector<XMLE_Min *> Min;
    std::vector<XMLE_Max *> Max;
    std::vector<XMLE_Log *> Log;
    std::vector<XMLE_ShadeInterval *> ShadeInterval;
    std::vector<XMLE_ContourLine *> ContourLine;
    std::vector<XMLE_ContourIntervalL *> ContourIntervalL;
    std::vector<XMLE_ContourIntervalH *> ContourIntervalH;
    std::vector<XMLE_SmoothingFilter *> SmoothingFilter;
    std::vector<XMLE_ValueRange *> ValueRange;
    std::vector<XMLE_ImageText *> ImageText;
    std::vector<XMLE_LatLonBox *> LatLonBox;
    std::vector<XMLE_Projection *> Projection;
    std::vector<XMLE_Styles *> Styles;
    std::vector<XMLE_RenderMethod *> RenderMethod;
    std::vector<XMLE_MetadataURL *> MetadataURL;
    std::vector<XMLE_Cache *> Cache;
    std::vector<XMLE_WMSLayer *> WMSLayer;
    std::vector<XMLE_DataPostProc *> DataPostProc;
    std::vector<XMLE_Position *> Position;
    std::vector<XMLE_WMSFormat *> WMSFormat;
    std::vector<XMLE_Grid *> Grid;
    std::vector<XMLE_AdditionalLayer *> AdditionalLayer;
    std::vector<XMLE_FeatureInterval *> FeatureInterval;

    ~XMLE_Layer() {
      XMLE_DELOBJ(Name);
      XMLE_DELOBJ(Group);
      XMLE_DELOBJ(Title);
      XMLE_DELOBJ(Abstract);
      XMLE_DELOBJ(DataBaseTable);
      XMLE_DELOBJ(Variable);
      XMLE_DELOBJ(FilePath);
      XMLE_DELOBJ(TileSettings)
      XMLE_DELOBJ(DataReader);
      XMLE_DELOBJ(Dimension);
      XMLE_DELOBJ(Legend);
      XMLE_DELOBJ(Scale);
      XMLE_DELOBJ(Offset);
      XMLE_DELOBJ(Min);
      XMLE_DELOBJ(Max);
      XMLE_DELOBJ(Log);
      XMLE_DELOBJ(ShadeInterval);
      XMLE_DELOBJ(ContourLine);
      XMLE_DELOBJ(ContourIntervalL);
      XMLE_DELOBJ(ContourIntervalH);
      XMLE_DELOBJ(SmoothingFilter);
      XMLE_DELOBJ(ValueRange);
      XMLE_DELOBJ(ImageText);
      XMLE_DELOBJ(LatLonBox);
      XMLE_DELOBJ(Projection);
      XMLE_DELOBJ(Styles);
      XMLE_DELOBJ(RenderMethod);
      XMLE_DELOBJ(MetadataURL);
      XMLE_DELOBJ(Cache);
      XMLE_DELOBJ(WMSLayer);
      XMLE_DELOBJ(DataPostProc);
      XMLE_DELOBJ(Position);
      XMLE_DELOBJ(WMSFormat);
      XMLE_DELOBJ(Grid);
      XMLE_DELOBJ(AdditionalLayer);
      XMLE_DELOBJ(FeatureInterval);
    }
    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Name" == elName) {
        XMLE_ADDOBJ(Name);
      } else if ("Group" == elName) {
        XMLE_ADDOBJ(Group);
      } else if ("Title" == elName) {
        XMLE_ADDOBJ(Title);
      } else if ("Abstract" == elName) {
        XMLE_ADDOBJ(Abstract);
      } else if ("DataBaseTable" == elName) {
        XMLE_ADDOBJ(DataBaseTable);
      } else if ("Variable" == elName) {
        XMLE_ADDOBJ(Variable);
      } else if ("FilePath" == elName) {
        XMLE_ADDOBJ(FilePath);
      } else if ("TileSettings" == elName) {
        XMLE_ADDOBJ(TileSettings);
      } else if ("DataReader" == elName) {
        XMLE_ADDOBJ(DataReader);
      } else if ("Dimension" == elName) {
        XMLE_ADDOBJ(Dimension);
      } else if ("Legend" == elName) {
        XMLE_ADDOBJ(Legend);
      } else if ("Scale" == elName) {
        XMLE_ADDOBJ(Scale);
      } else if ("Offset" == elName) {
        XMLE_ADDOBJ(Offset);
      } else if ("Min" == elName) {
        XMLE_ADDOBJ(Min);
      } else if ("Max" == elName) {
        XMLE_ADDOBJ(Max);
      } else if ("Log" == elName) {
        XMLE_ADDOBJ(Log);
      } else if ("ShadeInterval" == elName) {
        XMLE_ADDOBJ(ShadeInterval);
      } else if ("ContourLine" == elName) {
        XMLE_ADDOBJ(ContourLine);
      } else if ("ContourIntervalL" == elName) {
        XMLE_ADDOBJ(ContourIntervalL);
      } else if ("ContourIntervalH" == elName) {
        XMLE_ADDOBJ(ContourIntervalH);
      } else if ("ValueRange" == elName) {
        XMLE_ADDOBJ(ValueRange);
      } else if ("ImageText" == elName) {
        XMLE_ADDOBJ(ImageText);
      } else if ("LatLonBox" == elName) {
        XMLE_ADDOBJ(LatLonBox);
      } else if ("Projection" == elName) {
        XMLE_ADDOBJ(Projection);
      } else if ("Styles" == elName) {
        XMLE_ADDOBJ(Styles);
      } else if ("RenderMethod" == elName) {
        XMLE_ADDOBJ(RenderMethod);
      } else if ("MetadataURL" == elName) {
        XMLE_ADDOBJ(MetadataURL);
      } else if ("Cache" == elName) {
        XMLE_ADDOBJ(Cache);
      } else if ("WMSLayer" == elName) {
        XMLE_ADDOBJ(WMSLayer);
      } else if ("DataPostProc" == elName) {
        XMLE_ADDOBJ(DataPostProc);
      } else if ("SmoothingFilter" == elName) {
        XMLE_ADDOBJ(SmoothingFilter);
      } else if ("Position" == elName) {
        XMLE_ADDOBJ(Position);
      } else if ("WMSFormat" == elName) {
        XMLE_ADDOBJ(WMSFormat);
      } else if ("Grid" == elName) {
        XMLE_ADDOBJ(Grid);
      } else if ("AdditionalLayer" == elName) {
        XMLE_ADDOBJ(AdditionalLayer);
      } else if ("FeatureInterval" == elName) {
        XMLE_ADDOBJ(FeatureInterval);
      }
      return nullptr;
    }
    bool addAttribute(const attribute &attrCfg) {
      if ("type" == attrCfg.name) {
        attr.type = attrCfg.value;
        return true;
      } else if ("hidden" == attrCfg.name) {
        attr.hidden = attrCfg.value;
        return true;
      } else if ("enable_edr" == attrCfg.name) {
        attr.enable_edr = attrCfg.value;
        return true;
      }
      return false;
    }
  };

  class XMLE_Configuration : public CXMLObjectInterface {
  public:
    std::vector<XMLE_Legend *> Legend;
    std::vector<XMLE_WMS *> WMS;
    std::vector<XMLE_WCS *> WCS;
    std::vector<XMLE_OpenDAP *> OpenDAP;
    std::vector<XMLE_Path *> Path;
    std::vector<XMLE_TempDir *> TempDir;
    std::vector<XMLE_OnlineResource *> OnlineResource;
    std::vector<XMLE_DataBase *> DataBase;
    std::vector<XMLE_Projection *> Projection;
    std::vector<XMLE_Layer *> Layer;
    std::vector<XMLE_Style *> Style;
    std::vector<XMLE_AutoResource *> AutoResource;
    std::vector<XMLE_Dataset *> Dataset;
    std::vector<XMLE_Include *> Include;
    std::vector<XMLE_Logging *> Logging;
    std::vector<XMLE_Symbol *> Symbol;
    std::vector<XMLE_Settings *> Settings;
    std::vector<XMLE_Environment *> Environment;

    ~XMLE_Configuration() {
      XMLE_DELOBJ(Legend);
      XMLE_DELOBJ(WMS);
      XMLE_DELOBJ(WCS);
      XMLE_DELOBJ(OpenDAP);
      XMLE_DELOBJ(Path);
      XMLE_DELOBJ(TempDir);
      XMLE_DELOBJ(OnlineResource);
      XMLE_DELOBJ(DataBase);
      XMLE_DELOBJ(Projection);
      XMLE_DELOBJ(Layer);
      XMLE_DELOBJ(Style);
      XMLE_DELOBJ(AutoResource);
      XMLE_DELOBJ(Dataset);
      XMLE_DELOBJ(Include);
      XMLE_DELOBJ(Logging);
      XMLE_DELOBJ(Symbol);
      XMLE_DELOBJ(Settings);
      XMLE_DELOBJ(Environment);
    }

    CXMLObjectInterface *addElement(const std::string &elName) {
      if ("Legend" == elName) {
        XMLE_ADDOBJ(Legend);
      } else if ("WMS" == elName) {
        XMLE_SETOBJ(WMS);
      } else if ("WCS" == elName) {
        XMLE_SETOBJ(WCS);
      } else if ("Path" == elName) {
        XMLE_ADDOBJ(Path);
      } else if ("OpenDAP" == elName) {
        XMLE_ADDOBJ(OpenDAP);
      } else if ("TempDir" == elName) {
        XMLE_ADDOBJ(TempDir);
      } else if ("OnlineResource" == elName) {
        XMLE_ADDOBJ(OnlineResource);
      } else if ("DataBase" == elName) {
        XMLE_ADDOBJ(DataBase);
      } else if ("Projection" == elName) {
        XMLE_ADDOBJ(Projection);
      } else if ("Layer" == elName) {
        XMLE_ADDOBJ(Layer);
      } else if ("Style" == elName) {
        XMLE_ADDOBJ(Style);
      } else if ("AutoResource" == elName) {
        XMLE_ADDOBJ(AutoResource);
      } else if ("Dataset" == elName) {
        XMLE_ADDOBJ(Dataset);
      } else if ("Include" == elName) {
        XMLE_ADDOBJ(Include);
      } else if ("Logging" == elName) {
        XMLE_ADDOBJ(Logging);
      } else if ("Symbol" == elName) {
        XMLE_ADDOBJ(Symbol);
      } else if ("Settings" == elName) {
        XMLE_ADDOBJ(Settings);
      } else if ("Environment" == elName) {
        XMLE_ADDOBJ(Environment);
      }
      return nullptr;
    }
  };

  std::vector<XMLE_Configuration *> Configuration;

  CXMLObjectInterface *addElement(const std::string &elName) {
    if ("Configuration" == elName) {
      XMLE_SETOBJ(Configuration);
    }
    return nullptr;
  }

  ~CServerConfig() { XMLE_DELOBJ(Configuration); }
};
#endif

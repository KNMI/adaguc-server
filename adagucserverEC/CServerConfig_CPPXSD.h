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
    bool addAttribute(const char *name, const char *value) {
      if (equals("min", name)) {
        attr.min = parseInt(value);
        return true;
      } else if (equals("max", name)) {
        attr.max = parseInt(value);
        return true;
      } else if (equals("red", name)) {
        attr.red = parseInt(value);
        return true;
      } else if (equals("blue", name)) {
        attr.blue = parseInt(value);
        return true;
      } else if (equals("green", name)) {
        attr.green = parseInt(value);
        return true;
      } else if (equals("alpha", name)) {
        attr.alpha = parseInt(value);
        return true;
      } else if (equals("index", name)) {
        attr.index = parseInt(value);
        return true;
      } else if (equals("color", name)) { // Hex color like: #A41D23
        if (value[0] == '#')
          if (strlen(value) == 7 || strlen(value) == 9) {

            attr.red = CSERVER_HEXDIGIT_TO_DEC(value[1]) * 16 + CSERVER_HEXDIGIT_TO_DEC(value[2]);
            attr.green = CSERVER_HEXDIGIT_TO_DEC(value[3]) * 16 + CSERVER_HEXDIGIT_TO_DEC(value[4]);
            attr.blue = CSERVER_HEXDIGIT_TO_DEC(value[5]) * 16 + CSERVER_HEXDIGIT_TO_DEC(value[6]);
            if (strlen(value) == 9) {
              attr.alpha = CSERVER_HEXDIGIT_TO_DEC(value[7]) * 16 + CSERVER_HEXDIGIT_TO_DEC(value[8]);
            }
          }
        return true;
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      } else if (equals("quality", attrname)) {
        attr.quality.copy(attrvalue);
        return true;
      } else if (equals("format", attrname)) {
        attr.format.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("defaultvalue", attrname)) {
        attr.defaultValue.copy(attrvalue);
        return true;
      } else if (equals("force", attrname)) {
        attr.force.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("debug", attrname)) {
        attr.debug.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("radius", attrname)) {
        attr.radius.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("use", attrname)) {
        attr.use.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      } else if (equals("coordinates", attrname)) {
        attr.coordinates.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("linecolor", attrname)) {
        attr.linecolor.copy(attrvalue);
        return true;
      } else if (equals("linewidth", attrname)) {
        attr.linewidth = parseDouble(attrvalue);
        return true;
      } else if (equals("scale", attrname)) {
        attr.scale = parseFloat(attrvalue);
        return true;
      } else if (equals("discradius", attrname)) {
        attr.discradius = parseDouble(attrvalue);
        return true;
      } else if (equals("vectorstyle", attrname)) {
        attr.vectorstyle.copy(attrvalue);
        return true;
      } else if (equals("plotstationid", attrname)) {
        attr.plotstationid.copy(attrvalue);
        return true;
      } else if (equals("textformat", attrname)) {
        attr.textformat.copy(attrvalue);
        return true;
      } else if (equals("plotvalue", attrname)) {
        attr.plotvalue.copy(attrvalue);
        return true;
      } else if (equals("fontfile", attrname)) {
        attr.fontfile.copy(attrvalue);
        return true;
      } else if (equals("min", attrname)) {
        attr.min = parseDouble(attrvalue);
        return true;
      } else if (equals("max", attrname)) {
        attr.max = parseDouble(attrvalue);
        return true;
      } else if (equals("fontsize", attrname)) {
        attr.fontSize = parseDouble(attrvalue);
        return true;
      } else if (equals("outlinewidth", attrname)) {
        attr.outlinewidth = parseDouble(attrvalue);
        return true;
      } else if (equals("outlinecolor", attrname)) {
        attr.outlinecolor.copy(attrvalue);
        return true;
      } else if (equals("textcolor", attrname)) {
        attr.textcolor.copy(attrvalue);
        return true;
      } else if (equals("fillcolor", attrname)) {
        attr.fillcolor.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("fillcolor", attrname)) {
        attr.fillcolor.copy(attrvalue);
        return true;
      } else if (equals("linecolor", attrname)) {
        attr.linecolor.copy(attrvalue);
        return true;
      } else if (equals("textcolor", attrname)) {
        attr.textcolor.copy(attrvalue);
        return true;
      } else if (equals("textoutlinecolor", attrname)) {
        attr.textoutlinecolor.copy(attrvalue);
        return true;
      } else if (equals("fontfile", attrname)) {
        attr.fontfile.copy(attrvalue);
        return true;
      } else if (equals("fontsize", attrname)) {
        attr.fontsize.copy(attrvalue);
        return true;
      } else if (equals("discradius", attrname)) {
        attr.discradius.copy(attrvalue);
        return true;
      } else if (equals("textradius", attrname)) {
        attr.textradius.copy(attrvalue);
        return true;
      } else if (equals("dot", attrname)) {
        attr.dot.copy(attrvalue);
        return true;
      } else if (equals("textformat", attrname)) {
        attr.textformat.copy(attrvalue);
        return true;
      } else if (equals("plotstationid", attrname)) {
        attr.plotstationid.copy(attrvalue);
        return true;
      } else if (equals("pointstyle", attrname)) {
        attr.pointstyle.copy(attrvalue);
        return true;
      } else if (equals("min", attrname)) {
        attr.min = parseDouble(attrvalue);
        return true;
      } else if (equals("max", attrname)) {
        attr.max = parseDouble(attrvalue);
        return true;
      } else if (equals("symbol", attrname)) {
        attr.symbol.copy(attrvalue);
        return true;
      } else if (equals("maxpointspercell", attrname)) {
        attr.maxpointspercell = parseInt(attrvalue);
        return true;
      } else if (equals("maxpointcellsize", attrname)) {
        attr.maxpointcellsize = parseInt(attrvalue);
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
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("palette", name)) {
        XMLE_ADDOBJ(palette);
      }
      return nullptr;
    }
    bool addAttribute(const char *name, const char *value) {
      if (equals("name", name)) {
        attr.name.copy(value);
        return true;
      } else if (equals("type", name)) {
        attr.type.copy(value);
        return true;
      } else if (equals("file", name)) {
        attr.file.copy(value);
        return true;
      } else if (equals("tickround", name)) {
        attr.tickround.copy(value);
        return true;
      } else if (equals("tickinterval", name)) {
        attr.tickinterval.copy(value);
        return true;
      } else if (equals("fixedclasses", name) || equals("fixed", name)) {
        attr.fixedclasses.copy(value);
        return true;
      } else if (equals("textformatting", name)) {
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("width", attrname)) {
        attr.width.copy(attrvalue);
        return true;
      } else if (equals("dashing", attrname)) {
        attr.dashing.copy(attrvalue);
        return true;
      } else if (equals("linecolor", attrname)) {
        attr.linecolor.copy(attrvalue);
        return true;
      } else if (equals("textsize", attrname)) {
        attr.textsize.copy(attrvalue);
        return true;
      } else if (equals("textcolor", attrname)) {
        attr.textcolor.copy(attrvalue);
        return true;
      } else if (equals("textstrokecolor", attrname)) {
        attr.textstrokecolor.copy(attrvalue);
        return true;
      } else if (equals("classes", attrname)) {
        attr.classes.copy(attrvalue);
        return true;
      } else if (equals("interval", attrname)) {
        attr.interval.copy(attrvalue);
        return true;
      } else if (equals("textformatting", attrname)) {
        attr.textformatting.copy(attrvalue);
        return true;
      } else if (equals("textstrokewidth", attrname)) {
        attr.textstrokewidth.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("min", attrname)) {
        attr.min.copy(attrvalue);
        return true;
      } else if (equals("max", attrname)) {
        attr.max.copy(attrvalue);
        return true;
      } else if (equals("label", attrname)) {
        attr.label.copy(attrvalue);
        return true;
      } else if (equals("fillcolor", attrname)) {
        attr.fillcolor.copy(attrvalue);
        return true;
      } else if (equals("bgcolor", attrname)) {
        attr.bgcolor.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("min", attrname)) {
        attr.min.copy(attrvalue);
        return true;
      } else if (equals("max", attrname)) {
        attr.max.copy(attrvalue);
        return true;
      } else if (equals("file", attrname)) {
        attr.file.copy(attrvalue);
        return true;
      } else if (equals("binary_and", attrname)) {
        attr.binary_and.copy(attrvalue);
        return true;
      } else if (equals("offset_x", attrname)) {
        attr.offsetX.copy(attrvalue);
        return true;
      } else if (equals("offset_y", attrname)) {
        attr.offsetY.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("units", attrname)) {
        attr.units.copy(attrvalue);
        return true;
      } else if (equals("standard_name", attrname)) {
        attr.standard_name.copy(attrvalue);
        return true;
      } else if (equals("variable_name", attrname)) {
        attr.variable_name.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("value", attrname)) {
        attr.value.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("min", attrname)) {
        attr.min.copy(attrvalue);
        return true;
      } else if (equals("max", attrname)) {
        attr.max.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("size", attrname)) {
        attr.size.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("size", attrname)) {
        attr.size.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("size", attrname)) {
        attr.size.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("size", attrname)) {
        attr.size.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("size", attrname)) {
        attr.size.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("size", attrname)) {
        attr.size.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("prefix", attrname)) {
        attr.prefix.copy(attrvalue);
        return true;
      } else if (equals("basedir", attrname)) {
        attr.basedir.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("attribute", attrname)) {
        attr.attribute.copy(attrvalue);
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

    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Dir", name)) {
        XMLE_ADDOBJ(Dir);
      } else if (equals("ImageText", name)) {
        XMLE_ADDOBJ(ImageText);
      }
      return nullptr;
    }

    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("enableautoopendap", attrname)) {
        attr.enableautoopendap.copy(attrvalue);
        return true;
      } else if (equals("enablelocalfile", attrname)) {
        attr.enablelocalfile.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("enabled", attrname)) {
        attr.enabled.copy(attrvalue);
        return true;
      } else if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("location", attrname)) {
        attr.location.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      } else if (equals("title", attrname)) {
        attr.title.copy(attrvalue);
        return true;
      } else if (equals("abstract", attrname)) {
        attr.abstract.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("match", attrname)) {
        attr.match.copy(attrvalue);
        return true;
      } else if (equals("label", attrname)) {
        attr.label.copy(attrvalue);
        return true;
      } else if (equals("matchid", attrname)) {
        attr.matchid.copy(attrvalue);
        return true;
      } else if (equals("fillcolor", attrname)) {
        attr.fillcolor.copy(attrvalue);
        return true;
      } else if (equals("bgcolor", attrname)) {
        attr.bgcolor.copy(attrvalue);
        return true;
      } else if (equals("borderwidth", attrname)) {
        attr.borderwidth.copy(attrvalue);
        return true;
      } else if (equals("bordercolor", attrname)) {
        attr.bordercolor.copy(attrvalue);
        return true;
      } else if (equals("labelfontsize", attrname)) {
        attr.labelfontsize.copy(attrvalue);
        return true;
      } else if (equals("labelfontfile", attrname)) {
        attr.labelfontfile.copy(attrvalue);
        return true;
      } else if (equals("labelcolor", attrname)) {
        attr.labelcolor.copy(attrvalue);
        return true;
      } else if (equals("labelpropertyname", attrname)) {
        attr.labelpropertyname.copy(attrvalue);
        return true;
      } else if (equals("labelpropertyformat", attrname)) {
        attr.labelpropertyformat.copy(attrvalue);
        return true;
      } else if (equals("labelangle", attrname)) {
        attr.labelangle.copy(attrvalue);
        return true;
      } else if (equals("labelpadding", attrname)) {
        attr.labelpadding.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("distancex", attrname)) {
        attr.distancex.copy(attrvalue);
        return true;
      } else if (equals("distancey", attrname)) {
        attr.distancey.copy(attrvalue);
        return true;
      } else if (equals("discradius", attrname)) {
        attr.discradius.copy(attrvalue);
        return true;
      } else if (equals("mode", attrname)) {
        attr.mode.copy(attrvalue);
        return true;
      } else if (equals("color", attrname)) {
        attr.color.copy(attrvalue);
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("settings", name)) {
        attr.settings.copy(value);
        return true;
      } else if (equals("striding", name)) {
        attr.striding.copy(value);
        return true;
      } else if (equals("renderhint", name)) {
        attr.renderhint.copy(value);
        return true;
      } else if (equals("scalewidth", name)) {
        attr.scalewidth.copy(value);
        return true;
      } else if (equals("scalecontours", name)) {
        attr.scalecontours.copy(value);
        return true;
      } else if (equals("randomizefeatures", name)) {
        attr.randomizefeatures.copy(value);
        return true;
      } else if (equals("featuresoverlap", name)) {
        attr.featuresoverlap.copy(value);
        return true;
      } else if (equals("rendertextforvectors", name)) {
        attr.rendertextforvectors.copy(value);
        return true;
      } else if (equals("cliplegend", name)) {
        attr.cliplegend.copy(value);
        return true;
      } else if (equals("interpolationmethod", name)) {
        attr.interpolationmethod.copy(value);
        return true;
      } else if (equals("drawgridboxoutline", name)) {
        attr.drawgridboxoutline.copy(value);
        return true;
      } else if (equals("drawgrid", name)) {
        attr.drawgrid.copy(value);
        return true;
      }
      return false;
    }
  };

  class XMLE_DataPostProc : public CXMLObjectInterface {
  public:
    class Cattr {
    public:
      CT::string a, b, c, units, algorithm, mode, name, select, standard_name, long_name, variable, directionname, speedname, from_units, offset, stride;
    } attr;
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("a", attrname)) {
        attr.a.copy(attrvalue);
        return true;
      } else if (equals("b", attrname)) {
        attr.b.copy(attrvalue);
        return true;
      } else if (equals("c", attrname)) {
        attr.c.copy(attrvalue);
        return true;
      } else if (equals("mode", attrname)) {
        attr.mode.copy(attrvalue);
        return true;
      } else if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      } else if (equals("units", attrname)) {
        attr.units.copy(attrvalue);
        return true;
      } else if (equals("select", attrname)) {
        attr.select.copy(attrvalue);
        return true;
      } else if (equals("standard_name", attrname)) {
        attr.standard_name.copy(attrvalue);
        return true;
      } else if (equals("variable", attrname)) {
        attr.variable.copy(attrvalue);
        return true;
      } else if (equals("long_name", attrname)) {
        attr.long_name.copy(attrvalue);
        return true;
      } else if (equals("algorithm", attrname)) {
        attr.algorithm.copy(attrvalue);
        return true;
      } else if (equals("directionname", attrname)) {
        attr.directionname.copy(attrvalue);
        return true;
      } else if (equals("speedname", attrname)) {
        attr.speedname.copy(attrvalue);
        return true;
      } else if (equals("from_units", attrname)) {
        attr.from_units.copy(attrvalue);
      } else if (equals("offset", attrname)) {
        attr.offset.copy(attrvalue);
        return true;
      } else if (equals("stride", attrname)) {
        attr.stride.copy(attrvalue);
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
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Thinning", name)) {
        XMLE_ADDOBJ(Thinning);
      } else if (equals("Point", name)) {
        XMLE_ADDOBJ(Point);
      } else if (equals("Vector", name)) {
        XMLE_ADDOBJ(Vector);
      } else if (equals("FilterPoints", name)) {
        XMLE_ADDOBJ(FilterPoints);
      } else if (equals("Legend", name)) {
        XMLE_ADDOBJ(Legend);
      } else if (equals("Scale", name)) {
        XMLE_ADDOBJ(Scale);
      } else if (equals("Offset", name)) {
        XMLE_ADDOBJ(Offset);
      } else if (equals("Min", name)) {
        XMLE_ADDOBJ(Min);
      } else if (equals("Max", name)) {
        XMLE_ADDOBJ(Max);
      } else if (equals("Log", name)) {
        XMLE_ADDOBJ(Log);
      } else if (equals("ValueRange", name)) {
        XMLE_ADDOBJ(ValueRange);
      } else if (equals("ContourIntervalL", name)) {
        XMLE_ADDOBJ(ContourIntervalL);
      } else if (equals("ContourIntervalH", name)) {
        XMLE_ADDOBJ(ContourIntervalH);
      } else if (equals("RenderMethod", name)) {
        XMLE_ADDOBJ(RenderMethod);
      } else if (equals("ShadeInterval", name)) {
        XMLE_ADDOBJ(ShadeInterval);
      } else if (equals("SymbolInterval", name)) {
        XMLE_ADDOBJ(SymbolInterval);
      } else if (equals("ContourLine", name)) {
        XMLE_ADDOBJ(ContourLine);
      } else if (equals("NameMapping", name)) {
        XMLE_ADDOBJ(NameMapping);
      } else if (equals("SmoothingFilter", name)) {
        XMLE_ADDOBJ(SmoothingFilter);
      } else if (equals("StandardNames", name)) {
        XMLE_ADDOBJ(StandardNames);
      } else if (equals("LegendGraphic", name)) {
        XMLE_ADDOBJ(LegendGraphic);
      } else if (equals("FeatureInterval", name)) {
        XMLE_ADDOBJ(FeatureInterval);
      } else if (equals("Stippling", name)) {
        XMLE_ADDOBJ(Stippling);
      } else if (equals("RenderSettings", name)) {
        XMLE_ADDOBJ(RenderSettings);
      } else if (equals("DataPostProc", name)) {
        XMLE_ADDOBJ(DataPostProc);

      } else if (equals("IncludeStyle", name)) {
        XMLE_ADDOBJ(IncludeStyle);
      }
      return nullptr;
    }

    bool addAttribute(const char *name, const char *value) {
      if (equals("name", name)) {
        attr.name.copy(value);
        return true;
      } else if (equals("title", name)) {
        attr.title.copy(value);
        return true;
      }
      if (equals("abstract", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("name", name)) {
        attr.name.copy(value);
        return true;
      } else if (equals("onlineresource", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("id", name)) {
        attr.id.copy(value);
        return true;
      } else if (equals("authority", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("force", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("orgname", name)) {
        attr.orgname.copy(value);
        return true;
      } else if (equals("long_name", name)) {
        attr.long_name.copy(value);
        return true;
      } else if (equals("standard_name", name)) {
        attr.standard_name.copy(value);
        return true;
      } else if (equals("units", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("useendtime", name)) {
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

    bool addAttribute(const char *name, const char *value) {
      if (equals("filter", name)) {
        attr.filter.copy(value);
        return true;
      } else if (equals("gfi_openall", name)) {
        attr.gfi_openall.copy(value);
        return true;
      } else if (equals("maxquerylimit", name)) {
        attr.maxquerylimit.copy(value);
        return true;
      } else if (equals("ncml", name)) {
        attr.ncml.copy(value);
        return true;
      } else if (equals("retentionperiod", name)) {
        attr.retentionperiod.copy(value);
        return true;
      } else if (equals("retentiontype", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("tilemode", name)) {
        attr.tilemode.copy(value);
        return true;
      } else if (equals("debug", name)) {
        attr.debug.copy(value);
        return true;
      } else if (equals("tilewidthpx", name)) {
        attr.tilewidthpx.copy(value);
        return true;
      } else if (equals("tileheightpx", name)) {
        attr.tileheightpx.copy(value);
        return true;
      } else if (equals("minlevel", name)) {
        attr.minlevel.copy(value);
        return true;
      } else if (equals("maxlevel", name)) {
        attr.maxlevel.copy(value);
        return true;
      } else if (equals("maxtilesinimage", name)) {
        attr.maxtilesinimage.copy(value);
        return true;
      } else if (equals("threads", name)) {
        attr.threads.copy(value);
        return true;
      } else if (equals("autotile", name)) {
        attr.autotile.copy(value);
        return true;
      } else if (equals("optimizeextent", name)) {
        attr.optimizeextent.copy(value);
        return true;
      } else if (equals("tilepath", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("value", name)) {
        attr.value.copy(value);
        return true;
      }
      if (equals("collection", name)) {
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("minx", name)) {
        attr.minx = parseFloat(value);
        return true;
      } else if (equals("miny", name)) {
        attr.miny = parseFloat(value);
        return true;
      } else if (equals("maxx", name)) {
        attr.maxx = parseFloat(value);
        return true;
      } else if (equals("maxy", name)) {
        attr.maxy = parseFloat(value);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      } else if (equals("units", attrname)) {
        attr.units.copy(attrvalue);
        return true;
      } else if (equals("default", attrname)) {
        attr.defaultV.copy(attrvalue);
        return true;
      } else if (equals("interval", attrname)) {
        attr.interval.copy(attrvalue);
        return true;
      } else if (equals("quantizeperiod", attrname)) {
        attr.quantizeperiod.copy(attrvalue);
        return true;
      } else if (equals("fixvalue", attrname)) {
        attr.fixvalue.copy(attrvalue);
        return true;
      } else if (equals("hidden", attrname)) {
        if (equals("false", attrvalue)) {
          attr.hidden = false;
        }
        if (equals("true", attrvalue)) {
          attr.hidden = true;
        }
        return true;
      } else if (equals("quantizemethod", attrname)) {
        attr.quantizemethod.copy(attrvalue);
        return true;
      } else if (equals("type", attrname)) {
        if (equals(attrvalue, "vertical")) {
          attr.type = "dimtype_vertical";
        } else if (equals(attrvalue, "custom")) {
          attr.type = "dimtype_custom";
        } else if (equals(attrvalue, "time")) {
          attr.type = "dimtype_time";
        } else if (equals(attrvalue, "reference_time")) {
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("value", attrname)) {
        attr.value.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("value", attrname)) {
        attr.value.copy(attrvalue);
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
    bool addAttribute(const char *name, const char *value) {
      if (equals("name", name)) {
        attr.name.copy(value);
        return true;
      } else if (equals("default", name)) {
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("enablemetadatacache", attrname)) {
        attr.enablemetadatacache.copy(attrvalue);
        return true;
      } else if (equals("enablecleanupsystem", attrname)) {
        attr.enablecleanupsystem.copy(attrvalue);
        return true;
      } else if (equals("cleanupsystemlimit", attrname)) {
        attr.cleanupsystemlimit.copy(attrvalue);
        return true;
      } else if (equals("cache_age_cacheableresources", attrname)) {
        attr.cache_age_cacheableresources.copy(attrvalue);
        return true;
      } else if (equals("cache_age_volatileresources", attrname)) {
        attr.cache_age_volatileresources.copy(attrvalue);
        return true;
      } else if (equals("enable_edr", attrname)) {
        attr.enable_edr.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("value", attrname)) {
        attr.value.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("parameters", attrname)) {
        attr.parameters.copy(attrvalue);
        return true;
      } else if (equals("maxquerylimit", attrname)) {
        attr.maxquerylimit.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("id", attrname)) {
        attr.id.copy(attrvalue);
        return true;
      }
      if (equals("proj4", attrname)) {
        attr.proj4.copy(attrvalue);
        return true;
      }
      return false;
    }
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("LatLonBox", name)) {
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("enabled", attrname)) {
        attr.enabled.copy(attrvalue);
        return true;
      } else if (equals("optimizeextent", attrname)) {
        attr.optimizeextent.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      }
      if (equals("driver", attrname)) {
        attr.driver.copy(attrvalue);
        return true;
      }
      if (equals("mimetype", attrname)) {
        attr.mimetype.copy(attrvalue);
        return true;
      }
      if (equals("options", attrname)) {
        attr.options.copy(attrvalue);
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
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Name", name)) {
        XMLE_SETOBJ(Name);
      } else if (equals("Title", name)) {
        XMLE_SETOBJ(Title);
      } else if (equals("Abstract", name)) {
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

    CXMLObjectInterface *addElement(const char *name) {
      if (equals("ViewServiceCSW", name)) {
        XMLE_ADDOBJ(ViewServiceCSW);
      } else if (equals("DatasetCSW", name)) {
        XMLE_ADDOBJ(DatasetCSW);
      } else if (equals("AuthorityURL", name)) {
        XMLE_ADDOBJ(AuthorityURL);
      } else if (equals("Identifier", name)) {
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
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Title", name)) {
        XMLE_SETOBJ(Title);
      } else if (equals("GridFont", name)) {
        XMLE_SETOBJ(GridFont);
      } else if (equals("Abstract", name)) {
        XMLE_SETOBJ(Abstract);
      } else if (equals("RootLayer", name)) {
        XMLE_SETOBJ(RootLayer);
      } else if (equals("WMSFormat", name)) {
        XMLE_ADDOBJ(WMSFormat);
      } else if (equals("WMSExceptions", name)) {
        XMLE_ADDOBJ(WMSExceptions);
      } else if (equals("TitleFont", name)) {
        XMLE_ADDOBJ(TitleFont);
      } else if (equals("ContourFont", name)) {
        XMLE_ADDOBJ(ContourFont);
      } else if (equals("LegendFont", name)) {
        XMLE_SETOBJ(LegendFont);
      } else if (equals("SubTitleFont", name)) {
        XMLE_ADDOBJ(SubTitleFont);
      } else if (equals("DimensionFont", name)) {
        XMLE_ADDOBJ(DimensionFont);
      } else if (equals("Inspire", name)) {
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("enabled", attrname)) {
        attr.enabled.copy(attrvalue);
        return true;
      } else if (equals("path", attrname)) {
        attr.path.copy(attrvalue);
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
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Name", name)) {
        XMLE_ADDOBJ(Name);
      } else if (equals("Title", name)) {
        XMLE_ADDOBJ(Title);
      } else if (equals("Abstract", name)) {
        XMLE_ADDOBJ(Abstract);
      } else if (equals("WCSFormat", name)) {
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("enabled", attrname)) {
        attr.enabled.copy(attrvalue);
        return true;
      } else if (equals("cachefile", attrname)) {
        attr.cachefile.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("layer", attrname)) {
        attr.layer.copy(attrvalue);
        return true;
      } else if (equals("service", attrname)) {
        attr.service.copy(attrvalue);
        return true;
      } else if (equals("transparent", attrname)) {
        attr.transparent = parseBool(attrvalue);
        return true;
      } else if (equals("bgcolor", attrname)) {
        attr.bgcolor.copy(attrvalue);
        return true;
      } else if (equals("style", attrname)) {
        attr.style.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("top", attrname)) {
        attr.top.copy(attrvalue);
        return true;
      } else if (equals("left", attrname)) {
        attr.left.copy(attrvalue);
        return true;
      } else if (equals("right", attrname)) {
        attr.right.copy(attrvalue);
        return true;
      } else if (equals("bottom", attrname)) {
        attr.bottom.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("name", attrname)) {
        attr.name.copy(attrvalue);
        return true;
      } else if (equals("precision", attrname)) {
        attr.precision.copy(attrvalue);
        return true;
      } else if (equals("resolution", attrname)) {
        attr.resolution.copy(attrvalue);
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
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("replace", attrname)) {
        attr.replace.copy(attrvalue);
        return true;
      } else if (equals("style", attrname)) {
        attr.style.copy(attrvalue);
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
    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Name", name)) {
        XMLE_ADDOBJ(Name);
      } else if (equals("Group", name)) {
        XMLE_ADDOBJ(Group);
      } else if (equals("Title", name)) {
        XMLE_ADDOBJ(Title);
      } else if (equals("Abstract", name)) {
        XMLE_ADDOBJ(Abstract);
      } else if (equals("DataBaseTable", name)) {
        XMLE_ADDOBJ(DataBaseTable);
      } else if (equals("Variable", name)) {
        XMLE_ADDOBJ(Variable);
      } else if (equals("FilePath", name)) {
        XMLE_ADDOBJ(FilePath);
      } else if (equals("TileSettings", name)) {
        XMLE_ADDOBJ(TileSettings);
      } else if (equals("DataReader", name)) {
        XMLE_ADDOBJ(DataReader);
      } else if (equals("Dimension", name)) {
        XMLE_ADDOBJ(Dimension);
      } else if (equals("Legend", name)) {
        XMLE_ADDOBJ(Legend);
      } else if (equals("Scale", name)) {
        XMLE_ADDOBJ(Scale);
      } else if (equals("Offset", name)) {
        XMLE_ADDOBJ(Offset);
      } else if (equals("Min", name)) {
        XMLE_ADDOBJ(Min);
      } else if (equals("Max", name)) {
        XMLE_ADDOBJ(Max);
      } else if (equals("Log", name)) {
        XMLE_ADDOBJ(Log);
      } else if (equals("ShadeInterval", name)) {
        XMLE_ADDOBJ(ShadeInterval);
      } else if (equals("ContourLine", name)) {
        XMLE_ADDOBJ(ContourLine);
      } else if (equals("ContourIntervalL", name)) {
        XMLE_ADDOBJ(ContourIntervalL);
      } else if (equals("ContourIntervalH", name)) {
        XMLE_ADDOBJ(ContourIntervalH);
      } else if (equals("ValueRange", name)) {
        XMLE_ADDOBJ(ValueRange);
      } else if (equals("ImageText", name)) {
        XMLE_ADDOBJ(ImageText);
      } else if (equals("LatLonBox", name)) {
        XMLE_ADDOBJ(LatLonBox);
      } else if (equals("Projection", name)) {
        XMLE_ADDOBJ(Projection);
      } else if (equals("Styles", name)) {
        XMLE_ADDOBJ(Styles);
      } else if (equals("RenderMethod", name)) {
        XMLE_ADDOBJ(RenderMethod);
      } else if (equals("MetadataURL", name)) {
        XMLE_ADDOBJ(MetadataURL);
      } else if (equals("Cache", name)) {
        XMLE_ADDOBJ(Cache);
      } else if (equals("WMSLayer", name)) {
        XMLE_ADDOBJ(WMSLayer);
      } else if (equals("DataPostProc", name)) {
        XMLE_ADDOBJ(DataPostProc);
      } else if (equals("SmoothingFilter", name)) {
        XMLE_ADDOBJ(SmoothingFilter);
      } else if (equals("Position", name)) {
        XMLE_ADDOBJ(Position);
      } else if (equals("WMSFormat", name)) {
        XMLE_ADDOBJ(WMSFormat);
      } else if (equals("Grid", name)) {
        XMLE_ADDOBJ(Grid);
      } else if (equals("AdditionalLayer", name)) {
        XMLE_ADDOBJ(AdditionalLayer);
      } else if (equals("FeatureInterval", name)) {
        XMLE_ADDOBJ(FeatureInterval);
      }
      return nullptr;
    }
    bool addAttribute(const char *attrname, const char *attrvalue) {
      if (equals("type", attrname)) {
        attr.type.copy(attrvalue);
        return true;
      } else if (equals("hidden", attrname)) {
        attr.hidden.copy(attrvalue);
        return true;
      } else if (equals("enable_edr", attrname)) {
        attr.enable_edr.copy(attrvalue);
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

    CXMLObjectInterface *addElement(const char *name) {
      if (equals("Legend", name)) {
        XMLE_ADDOBJ(Legend);
      } else if (equals("WMS", name)) {
        XMLE_SETOBJ(WMS);
      } else if (equals("WCS", name)) {
        XMLE_SETOBJ(WCS);
      } else if (equals("Path", name)) {
        XMLE_ADDOBJ(Path);
      } else if (equals("OpenDAP", name)) {
        XMLE_ADDOBJ(OpenDAP);
      } else if (equals("TempDir", name)) {
        XMLE_ADDOBJ(TempDir);
      } else if (equals("OnlineResource", name)) {
        XMLE_ADDOBJ(OnlineResource);
      } else if (equals("DataBase", name)) {
        XMLE_ADDOBJ(DataBase);
      } else if (equals("Projection", name)) {
        XMLE_ADDOBJ(Projection);
      } else if (equals("Layer", name)) {
        XMLE_ADDOBJ(Layer);
      } else if (equals("Style", name)) {
        XMLE_ADDOBJ(Style);
      } else if (equals("AutoResource", name)) {
        XMLE_ADDOBJ(AutoResource);
      } else if (equals("Dataset", name)) {
        XMLE_ADDOBJ(Dataset);
      } else if (equals("Include", name)) {
        XMLE_ADDOBJ(Include);
      } else if (equals("Logging", name)) {
        XMLE_ADDOBJ(Logging);
      } else if (equals("Symbol", name)) {
        XMLE_ADDOBJ(Symbol);
      } else if (equals("Settings", name)) {
        XMLE_ADDOBJ(Settings);
      } else if (equals("Environment", name)) {
        XMLE_ADDOBJ(Environment);
      }
      return nullptr;
    }
  };

  std::vector<XMLE_Configuration *> Configuration;

  CXMLObjectInterface *addElement(const char *name) {
    if (equals("Configuration", name)) {
      XMLE_SETOBJ(Configuration);
    }
    return nullptr;
  }

  ~CServerConfig() { XMLE_DELOBJ(Configuration); }
};
#endif

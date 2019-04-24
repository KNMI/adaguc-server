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


// f 102 >15
// F 70 > 15
// 0 48 > 0
#define CSERVER_HEXDIGIT_TO_DEC(DIGIT) (DIGIT>96?DIGIT-87:DIGIT>64?DIGIT-55:DIGIT-48) //Converts "9" to 9, "A" to 10 and "a" to 10

class CServerConfig:public CXMLSerializerInterface{
  public:
    class XMLE_palette: public CXMLObjectInterface{
      public:
        XMLE_palette(){
          attr.alpha=255;
          attr.index = -1;
        }
        class Cattr{
          public:
            int min,max,index,red,green,blue,alpha;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(     equals("min",3,name)){attr.min=parseInt(value);return;}
          else if(equals("max",3,name)){attr.max=parseInt(value);return;}
          else if(equals("red",3,name)){attr.red=parseInt(value);return;}
          else if(equals("blue",4,name)){attr.blue=parseInt(value);return;}
          else if(equals("green",5,name)){attr.green=parseInt(value);return;}
          else if(equals("alpha",5,name)){attr.alpha=parseInt(value);return;}
          else if(equals("index",5,name)){attr.index=parseInt(value);return;}
          else if(equals("color",5,name)){//Hex color like: #A41D23
            if(value[0]=='#')if(strlen(value)==7||strlen(value)==9){
              
              attr.red  = CSERVER_HEXDIGIT_TO_DEC(value[1])*16+CSERVER_HEXDIGIT_TO_DEC(value[2]);
              attr.green= CSERVER_HEXDIGIT_TO_DEC(value[3])*16+CSERVER_HEXDIGIT_TO_DEC(value[4]);
              attr.blue = CSERVER_HEXDIGIT_TO_DEC(value[5])*16+CSERVER_HEXDIGIT_TO_DEC(value[6]);
              if(strlen(value)==9){
                attr.alpha = CSERVER_HEXDIGIT_TO_DEC(value[7])*16+CSERVER_HEXDIGIT_TO_DEC(value[8]);
              }
            }
            return;
          }
        }
    };
    
    class XMLE_WMSFormat: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string name,format;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("name",4,attrname)){attr.name.copy(attrvalue);return;}
        else if(equals("format",6,attrname)){attr.format.copy(attrvalue);return;}
      }
    };
    
    class XMLE_WMSExceptions: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string defaultValue, force;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("defaultvalue",12,attrname)){attr.defaultValue.copy(attrvalue);return;}
        else if(equals("force",5,attrname)){attr.force.copy(attrvalue);return;}
      }
    };
    
    class XMLE_Thinning: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string radius;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("radius",6 ,attrname)){attr.radius.copy(attrvalue);return;}
      }
    };
    
    class XMLE_FilterPoints: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string skip, use;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("skip",4,attrname)){attr.skip.copy(attrvalue);return;}
        else if(equals("use",3,attrname)){attr.use.copy(attrvalue);return;}
      }
    };
    
    class XMLE_Vector: public CXMLObjectInterface{
    public:
      XMLE_Vector() {
        attr.scale=1.0;
      }
      class Cattr{
      public:
        CT::string linecolor,linewidth,plotstationid,vectorstyle,textformat,plotvalue;
        float scale;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("linecolor",9,attrname)){attr.linecolor.copy(attrvalue);return;}
        else if(equals("linewidth",9,attrname)){attr.linewidth.copy(attrvalue);return;}
        else if(equals("scale",5,attrname)){attr.scale=parseFloat(attrvalue);return;}
        else if(equals("vectorstyle",11,attrname)){attr.vectorstyle.copy(attrvalue);return;}
        else if(equals("plotstationid",13,attrname)){attr.plotstationid.copy(attrvalue);return;}
        else if(equals("textformat",10,attrname)){attr.textformat.copy(attrvalue);return;}
        else if(equals("plotvalue",9,attrname)){attr.plotvalue.copy(attrvalue);return;}
      }
    };
    
    class XMLE_Point: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string fillcolor,linecolor,textcolor,fontfile,fontsize,discradius,textradius,dot,anglestart,anglestep,textformat,plotstationid,pointstyle;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("fillcolor",9,attrname)){attr.fillcolor.copy(attrvalue);return;}
        else if(equals("linecolor",9,attrname)){attr.linecolor.copy(attrvalue);return;}
        else if(equals("textcolor",9,attrname)){attr.textcolor.copy(attrvalue);return;}
        else if(equals("fontfile",8,attrname)){attr.fontfile.copy(attrvalue);return;}
        else if(equals("fontsize",8,attrname)){attr.fontsize.copy(attrvalue);return;}
        else if(equals("discradius",10,attrname)){attr.discradius.copy(attrvalue);return;}
        else if(equals("textradius",10,attrname)){attr.textradius.copy(attrvalue);return;}
        else if(equals("dot",3,attrname)){attr.dot.copy(attrvalue);return;}
        else if(equals("anglestart",10,attrname)){attr.anglestart.copy(attrvalue);return;}
        else if(equals("anglestep",9,attrname)){attr.anglestep.copy(attrvalue);return;}
        else if(equals("textformat",10,attrname)){attr.textformat.copy(attrvalue);return;}
        else if(equals("plotstationid",13,attrname)){attr.plotstationid.copy(attrvalue);return;}
        else if(equals("pointstyle",10,attrname)){attr.pointstyle.copy(attrvalue);return;}
      }
    };
    
    class XMLE_Legend: public CXMLObjectInterface{
      public:
        std::vector <XMLE_palette*> palette;
        ~XMLE_Legend(){
          XMLE_DELOBJ(palette);
        }
        class Cattr{
          public:
            CT::string name,type,tickround,tickinterval,fixedclasses,file;
        }attr;
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
            pt2Class=NULL;
            if(equals("palette",7,name)){XMLE_ADDOBJ(palette);}
           
          }
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
        void addAttribute(const char *name,const char *value){
          if(equals("name",4,name)){attr.name.copy(value);return;}
          else if(equals("type",4,name)){attr.type.copy(value);return;}
          else if(equals("file",4,name)){attr.file.copy(value);return;}
          else if(equals("tickround",9,name)){attr.tickround.copy(value);return;}
          else if(equals("tickinterval",12,name)){attr.tickinterval.copy(value);return;}
          else if(equals("fixedclasses",12,name)){attr.fixedclasses.copy(value);return;}
          
          
          
        }
    };
    class XMLE_Scale: public CXMLObjectInterface{};
    class XMLE_Offset: public CXMLObjectInterface{};
    class XMLE_Min: public CXMLObjectInterface{};
    class XMLE_Max: public CXMLObjectInterface{};
    
    /*Deprecated*/
    class XMLE_ContourIntervalL: public CXMLObjectInterface{};
    /*Deprecated*/
    class XMLE_ContourIntervalH: public CXMLObjectInterface{};
    
    class XMLE_ContourLine: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string width,linecolor,textcolor,classes,interval,textformatting;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("width",5,attrname)){attr.width.copy(attrvalue);return;}
        else if(equals("linecolor",9,attrname)){attr.linecolor.copy(attrvalue);return;}
        else if(equals("textcolor",9,attrname)){attr.textcolor.copy(attrvalue);return;}
        else if(equals("classes",7,attrname)){attr.classes.copy(attrvalue);return;}
        else if(equals("interval",8,attrname)){attr.interval.copy(attrvalue);return;}
        else if(equals("textformatting",14,attrname)){attr.textformatting.copy(attrvalue);return;}
      }
    };
    
    
    class XMLE_ShadeInterval: public CXMLObjectInterface{
      public:
      class Cattr{
      public:
        CT::string min,max,label,fillcolor,bgcolor;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("min",3,attrname)){attr.min.copy(attrvalue);return;}
        else if(equals("max",3,attrname)){attr.max.copy(attrvalue);return;}
        else if(equals("label",5,attrname)){attr.label.copy(attrvalue);return;}
        else if(equals("fillcolor",9,attrname)){attr.fillcolor.copy(attrvalue);return;}
        else if(equals("bgcolor",7,attrname)){attr.bgcolor.copy(attrvalue); return;}
      }
    };
    
    class XMLE_SymbolInterval: public CXMLObjectInterface{
      public:
      class Cattr{
      public:
        CT::string min,max,binary_and;
        CT::string file;
        CT::string offsetX, offsetY;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("min",3,attrname)){attr.min.copy(attrvalue);return;}
        else if(equals("max",3,attrname)){attr.max.copy(attrvalue);return;}
        else if(equals("file",4,attrname)){attr.file.copy(attrvalue);return;}
        else if(equals("binary_and",10,attrname)){attr.binary_and.copy(attrvalue);return;}
        else if(equals("offset_x",8,attrname)){attr.offsetX.copy(attrvalue);return;}
        else if(equals("offset_y",8,attrname)){attr.offsetY.copy(attrvalue);return;}
      }
     };
    
    class XMLE_SmoothingFilter: public CXMLObjectInterface{};
    class XMLE_StandardNames: public CXMLObjectInterface{
      public:
        class Cattr{
        public:
          CT::string units,standard_name,variable_name;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("units",5,attrname)){attr.units.copy(attrvalue);return;}
          else if(equals("standard_name",13,attrname)){attr.standard_name.copy(attrvalue);return;}
          else if(equals("variable_name",13,attrname)){attr.variable_name.copy(attrvalue);return;}
        }
    };
    
    class XMLE_LegendGraphic: public CXMLObjectInterface{
      public:
        class Cattr{
        public:
          CT::string value;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("value",5,attrname)){attr.value.copy(attrvalue);return;}
          
        }
    };
    
    class XMLE_Log: public CXMLObjectInterface{};
    class XMLE_ValueRange: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string min,max;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("min",3,attrname)){attr.min.copy(attrvalue);return;}
          else if(equals("max",3,attrname)){attr.max.copy(attrvalue);return;}
        }
    };
    
    class XMLE_ContourFont: public CXMLObjectInterface{
      public:
      class Cattr{
        public:
        CT::string location,size;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("size",4,attrname)){attr.size.copy(attrvalue);return;}
        else if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    class XMLE_TitleFont: public CXMLObjectInterface{
      public:
      class Cattr{
      public:
        CT::string location,size;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("size",4,attrname)){attr.size.copy(attrvalue);return;}
        else if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    class XMLE_SubTitleFont: public CXMLObjectInterface{
      public:
      class Cattr{
      public:
        CT::string location,size;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("size",4,attrname)){attr.size.copy(attrvalue);return;}
        else if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    
    class XMLE_DimensionFont: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string location,size;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("size",4,attrname)){attr.size.copy(attrvalue);return;}
        else if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    class XMLE_GridFont: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string location,size;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("size",4,attrname)){attr.size.copy(attrvalue);return;}
        else if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    
    
    class XMLE_Dir: public CXMLObjectInterface{
    public:
      class Cattr{
        public:
        CT::string basedir, prefix;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("prefix",6,attrname)){attr.prefix.copy(attrvalue);return;}
        else if(equals("basedir",7,attrname)){attr.basedir.copy(attrvalue);return;}        
      }
    };
    
    class XMLE_ImageText: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string attribute;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("attribute",9,attrname)){attr.attribute.copy(attrvalue);return;}
      }
    };
    
    class XMLE_AutoResource: public CXMLObjectInterface{
    public:
      std::vector <XMLE_Dir*> Dir;
      std::vector <XMLE_ImageText*> ImageText;
      ~XMLE_AutoResource(){
        XMLE_DELOBJ(Dir);
        XMLE_DELOBJ(ImageText);
      }
      class Cattr{
      public:
        CT::string enableautoopendap, enablelocalfile,enablecache;
      }attr;
      
      void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
        CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
        base->currentNode=(CXMLObjectInterface*)this;
        if(rc==0)if(value!=NULL)this->value.copy(value);
        if(rc==1){
          pt2Class=NULL;
          if(equals("Dir",3,name)){XMLE_ADDOBJ(Dir);} 
          else if(equals("ImageText",9,name)){XMLE_ADDOBJ(ImageText);}
        }
        if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
      }
      
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("enableautoopendap",17,attrname)){attr.enableautoopendap.copy(attrvalue);return;}
        else if(equals("enablecache",11,attrname)){attr.enablecache.copy(attrvalue);return;}        
        else if(equals("enablelocalfile",15,attrname)){attr.enablelocalfile.copy(attrvalue);return;}        
      }
    };
    
    class XMLE_Dataset: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string enabled,location;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("enabled",7,attrname)){attr.enabled.copy(attrvalue);return;}
        else if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    
    class XMLE_Include: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string location;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("location",8,attrname)){attr.location.copy(attrvalue);return;}
      }
    };
    
    class XMLE_NameMapping: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string name,title,abstract;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("name",4,attrname)){attr.name.copy(attrvalue);return;}
        else if(equals("title",5,attrname)){attr.title.copy(attrvalue);return;}
        else if(equals("abstract",8,attrname)){attr.abstract.copy(attrvalue);return;}
      }
    };
    
    class XMLE_FeatureInterval: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string match,matchid,bgcolor,label,fillcolor,linecolor, linewidth, bordercolor, borderwidth;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("match",5,attrname)){attr.match.copy(attrvalue); return;}
        else if(equals("label",5,attrname)){attr.label.copy(attrvalue); return;}
        else if(equals("matchid", 7,attrname)){attr.matchid.copy(attrvalue); return;}
        else if(equals("fillcolor",9,attrname)){attr.fillcolor.copy(attrvalue); return;}
        else if(equals("bgcolor",7,attrname)){attr.bgcolor.copy(attrvalue); return;}
        else if(equals("linecolor",9,attrname)){attr.linecolor.copy(attrvalue); return;}
        else if(equals("linewidth",9,attrname)){attr.linewidth.copy(attrvalue); return;}
        else if(equals("borderwidth",11,attrname)){attr.borderwidth.copy(attrvalue); return;}
        else if(equals("bordercolor",11,attrname)){attr.bordercolor.copy(attrvalue); return;}
      }
    };
    
    
    class XMLE_Stippling: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string distancex,distancey,discradius,mode,color;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("distancex",9,attrname)){attr.distancex.copy(attrvalue); return;}
        else if(equals("distancey", 9,attrname)){attr.distancey.copy(attrvalue); return;}
        else if(equals("discradius",10,attrname)){attr.discradius.copy(attrvalue); return;}
        else if(equals("mode",4,attrname)){attr.mode.copy(attrvalue); return;}
        else if(equals("color",5,attrname)){attr.color.copy(attrvalue); return;}        
      }
    };
    
    class XMLE_RenderMethod: public CXMLObjectInterface{};
    
        
    class XMLE_RenderSettings: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string settings,striding,renderer;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(equals("settings",8,name)){attr.settings.copy(value);return;}
          else if(equals("renderer",8,name)){attr.renderer.copy(value);return;}
          else if(equals("striding",8,name)){attr.striding.copy(value);return;}
        }
    };
    
    class XMLE_Style: public CXMLObjectInterface{
      public:
        std::vector <XMLE_Thinning*> Thinning;
        std::vector <XMLE_Point*> Point;
        std::vector <XMLE_Vector*> Vector;
        std::vector <XMLE_FilterPoints*> FilterPoints;
        std::vector <XMLE_Legend*> Legend;
        std::vector <XMLE_Scale*> Scale;
        std::vector <XMLE_Offset*> Offset;
        std::vector <XMLE_Min*> Min;
        std::vector <XMLE_Max*> Max;
        std::vector <XMLE_ContourIntervalL*> ContourIntervalL;
        std::vector <XMLE_ContourIntervalH*> ContourIntervalH;
        std::vector <XMLE_Log*> Log;
        std::vector <XMLE_ValueRange*> ValueRange;
        std::vector <XMLE_RenderMethod*> RenderMethod;
        std::vector <XMLE_ShadeInterval*> ShadeInterval;
        std::vector <XMLE_SymbolInterval*> SymbolInterval;
        std::vector <XMLE_ContourLine*> ContourLine;
        std::vector <XMLE_NameMapping*> NameMapping;
        std::vector <XMLE_SmoothingFilter*> SmoothingFilter;
        std::vector <XMLE_StandardNames*> StandardNames;
        std::vector <XMLE_LegendGraphic*> LegendGraphic;
        std::vector <XMLE_FeatureInterval*> FeatureInterval;
        std::vector <XMLE_Stippling*> Stippling;
        std::vector <XMLE_RenderSettings*> RenderSettings;
        
        
        
        
        ~XMLE_Style(){
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
          
        }
        class Cattr{
          public:
            CT::string name;
        }attr;
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          pt2Class=NULL;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
           
            if(equals("Thinning",8,name)){XMLE_ADDOBJ(Thinning);}
            else if(equals("Point",5,name)){XMLE_ADDOBJ(Point);}
            else if(equals("Vector",6,name)){XMLE_ADDOBJ(Vector);}
            else if(equals("FilterPoints",12,name)){XMLE_ADDOBJ(FilterPoints);}
            else if(equals("Legend",6,name)){XMLE_ADDOBJ(Legend);}
            else if(equals("Scale",5,name)){XMLE_ADDOBJ(Scale);}
            else if(equals("Offset",6,name)){XMLE_ADDOBJ(Offset);}
            else if(equals("Min",3,name)){XMLE_ADDOBJ(Min);}
            else if(equals("Max",3,name)){XMLE_ADDOBJ(Max);}
            else if(equals("Log",3,name)){XMLE_ADDOBJ(Log);}
            else if(equals("ValueRange",10,name)){XMLE_ADDOBJ(ValueRange);}
            else if(equals("ContourIntervalL",16,name)){XMLE_ADDOBJ(ContourIntervalL);}
            else if(equals("ContourIntervalH",16,name)){XMLE_ADDOBJ(ContourIntervalH);}
            else if(equals("RenderMethod",12,name)){XMLE_ADDOBJ(RenderMethod);}
            else if(equals("ShadeInterval",13,name)){XMLE_ADDOBJ(ShadeInterval);}
            else if(equals("SymbolInterval",14,name)){XMLE_ADDOBJ(SymbolInterval);}
            else if(equals("ContourLine",11,name)){XMLE_ADDOBJ(ContourLine);}
            else if(equals("NameMapping",11,name)){XMLE_ADDOBJ(NameMapping);}
            else if(equals("SmoothingFilter",15,name)){XMLE_ADDOBJ(SmoothingFilter);}
            else if(equals("StandardNames",13,name)){XMLE_ADDOBJ(StandardNames);}
            else if(equals("LegendGraphic",13,name)){XMLE_ADDOBJ(LegendGraphic);}
            else if(equals("FeatureInterval",15,name)){XMLE_ADDOBJ(FeatureInterval);}
            else if(equals("Stippling",9,name)){XMLE_ADDOBJ(Stippling);}           
            else if(equals("RenderSettings",14,name)){XMLE_ADDOBJ(RenderSettings);}
          }
          if(pt2Class!=NULL){pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);pt2Class=NULL;}
        }
        
        void addAttribute(const char *name,const char *value){
          if(equals("name",4,name)){attr.name.copy(value);return;}
        }
    };
    class XMLE_Styles: public CXMLObjectInterface{};
    class XMLE_Title: public CXMLObjectInterface{};
    

    //class XMLE_Keywords: public CXMLObjectInterface{};
    class XMLE_MetadataURL: public CXMLObjectInterface{};
    
    class XMLE_AuthorityURL: public CXMLObjectInterface{
      public:
        class Cattr{
        public:
          CT::string name,onlineresource;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(equals("name",4,name)){attr.name.copy(value);return;}
          else if(equals("onlineresource",14,name)){attr.onlineresource.copy(value);return;}
        }
    };        
    
    class XMLE_Identifier: public CXMLObjectInterface{
      public:
        class Cattr{
        public:
          CT::string authority,id;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(equals("id",2,name)){attr.id.copy(value);return;}
          else if(equals("authority",9,name)){attr.authority.copy(value);return;}
          
        }
    };        
  
    class XMLE_Name: public CXMLObjectInterface{
      public:
        class Cattr{
        public:
          CT::string force;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(equals("force",5,name)){attr.force.copy(value);return;}
        }
    };
    class XMLE_Abstract: public CXMLObjectInterface{};
    class XMLE_DataBaseTable: public CXMLObjectInterface{};
    class XMLE_Variable: public CXMLObjectInterface{};
    class XMLE_DataReader: public CXMLObjectInterface{
      public:
        class Cattr{
        public:
          CT::string useendtime;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(equals("useendtime", 10,name)){attr.useendtime.copy(value);return;}
        }
    };
    class XMLE_FilePath: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string filter,gfi_openall,ncml, maxquerylimit;
        }attr;
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(CDirReader::makeCleanPath(value));
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
        
        void addAttribute(const char *name,const char *value){
          if(equals("filter",6,name)){attr.filter.copy(value);return;}
          else if(equals("gfi_openall",11,name)){attr.gfi_openall.copy(value);return;}
          else if(equals("maxquerylimit",13,name)){attr.maxquerylimit.copy(value);return;}
          else if(equals("ncml",4,name)){attr.ncml.copy(value);return;}
        }
    };

    class XMLE_TileSettings: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string tilewidthpx,tileheightpx,
            tilecellsizex,tilecellsizey,
            left,right,bottom,top,
            numtilesx,numtilesy,
            tileprojection,minlevel,maxlevel,tilepath,
            tilemode,
            threads,
            debug,
            prefix,
            readonly,
            optimizeextent,
            maxtilesinimage;
        }attr;
//           <TileSettings  tilewidth="600" 
//                    tileheight="600" 
//                    tilebboxwidth="15000" 
//                    tilebboxheight="15000" 
//                    tileprojection="+proj=laea +lat_0=52 +lon_0=10 +x_0=4321000 +y_0=3210000 +ellps=GRS80 +units=m +no_defs"
//                    maxlevel="7"
//                    tilepath="/nobackup/users/plieger/data/clipc/tiles/"/>
//     
    
        void addAttribute(const char *name,const char *value){
          if(equals("tilemode",8,name)){attr.tilemode.copy(value);return;}
          else if(equals("debug",5,name)){attr.debug.copy(value);return;}
          else if(equals("prefix",6,name)){attr.prefix.copy(value);return;}
          else if(equals("readonly",8,name)){attr.readonly.copy(value);return;}
          else if(equals("threads",7,name)){attr.threads.copy(value);return;}
          else if(equals("tilewidthpx",11,name)){attr.tilewidthpx.copy(value);return;}
          else if(equals("tileheightpx",12,name)){attr.tileheightpx.copy(value);return;}
          else if(equals("tilecellsizex",13,name)){attr.tilecellsizex.copy(value);return;}
          else if(equals("tilecellsizey",13,name)){attr.tilecellsizey.copy(value);return;}
          else if(equals("left",4,name)){attr.left.copy(value);return;}
          else if(equals("bottom",6,name)){attr.bottom.copy(value);return;}
          else if(equals("right",5,name)){attr.right.copy(value);return;}
          else if(equals("top",3,name)){attr.top.copy(value);return;}
          else if(equals("numtilesx",9,name)){attr.numtilesx.copy(value);return;}
          else if(equals("numtilesy",9,name)){attr.numtilesy.copy(value);return;}
          else if(equals("tileprojection",14,name)){attr.tileprojection.copy(value);return;}
          else if(equals("minlevel",8,name)){attr.minlevel.copy(value);return;}
          else if(equals("maxlevel",8,name)){attr.maxlevel.copy(value);return;}
          else if(equals("tilepath",8,name)){attr.tilepath.copy(value);return;}
          else if(equals("optimizeextent",14,name)){attr.optimizeextent.copy(value);return;}
          else if(equals("maxtilesinimage",15,name)){attr.maxtilesinimage.copy(value);return;}
          
        }
    };
    
    class XMLE_Group: public CXMLObjectInterface{
      public:
      class Cattr{
        public:
          CT::string value;
      }attr;
      void addAttribute(const char *name,const char *value){
        if(equals("value",5,name)){attr.value.copy(value);return;}
      }
    };
    class XMLE_LatLonBox: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            float minx,miny,maxx,maxy;
        }attr;
        void addAttribute(const char *name,const char *value){
          if(equals("minx",4,name)){attr.minx=parseFloat(value);return;}
          else if(equals("miny",4,name)){attr.miny=parseFloat(value);return;}
          else if(equals("maxx",4,name)){attr.maxx=parseFloat(value);return;}
          else if(equals("maxy",4,name)){attr.maxy=parseFloat(value);return;}
        }
    };
  
 
    class XMLE_Dimension: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string name,interval,defaultV,units,quantizeperiod,quantizemethod;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("name",4,attrname)){attr.name.copy(attrvalue);return;}
          if(equals("units",5,attrname)){attr.units.copy(attrvalue);return;}
          if(equals("default",7,attrname)){attr.defaultV.copy(attrvalue);return;}
          if(equals("interval",8,attrname)){attr.interval.copy(attrvalue);return;}
          if(equals("quantizeperiod",14,attrname)){attr.quantizeperiod.copy(attrvalue);return;}
          if(equals("quantizemethod",14,attrname)){attr.quantizemethod.copy(attrvalue);return;}
        }
    };
    
    class XMLE_Path: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string value;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("value",5,attrname)){attr.value.copy(attrvalue);return;}
        }
    };
    class XMLE_TempDir: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string value;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("value",5,attrname)){attr.value.copy(attrvalue);return;}
        }
    };
    class XMLE_OnlineResource: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string value;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("value",5,attrname)){attr.value.copy(attrvalue);return;}
        }
    };
    class XMLE_DataBase: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string parameters,dbtype,maxquerylimit;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("parameters",10,attrname)){attr.parameters.copy(attrvalue);return;}
          else if(equals("dbtype",6,attrname)){attr.dbtype.copy(attrvalue);return;}
          else if(equals("maxquerylimit",13,attrname)){attr.maxquerylimit.copy(attrvalue);return;}
        }
    };
    class XMLE_Projection: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string id,proj4;
        }attr;
        std::vector <XMLE_LatLonBox*> LatLonBox;
        ~XMLE_Projection(){
          XMLE_DELOBJ(LatLonBox);
        }
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("id",2,attrname)){attr.id.copy(attrvalue);return;}
          if(equals("proj4",5,attrname)){attr.proj4.copy(attrvalue);return;}
        }
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
            pt2Class=NULL;
            if(equals("LatLonBox",9,name)){XMLE_ADDOBJ(LatLonBox);}
          }
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
    };
    class XMLE_Cache: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string enabled,optimizeextent;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("enabled",7,attrname)){attr.enabled.copy(attrvalue);return;}
          else if(equals("optimizeextent",14,attrname)){attr.optimizeextent.copy(attrvalue);return;}
        }
    };
    class XMLE_WCSFormat: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string name,driver,mimetype,options;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("name",4,attrname)){attr.name.copy(attrvalue);return;}
          if(equals("driver",6,attrname)){attr.driver.copy(attrvalue);return;}
          if(equals("mimetype",8,attrname)){attr.mimetype.copy(attrvalue);return;}
          if(equals("options",7,attrname)){attr.options.copy(attrvalue);return;}
        }
    };
    
    class XMLE_RootLayer: public CXMLObjectInterface{
      public:
        std::vector <XMLE_Name*> Name;
        std::vector <XMLE_Title*> Title;
        std::vector <XMLE_Abstract*> Abstract;
        ~XMLE_RootLayer(){
          XMLE_DELOBJ(Name);
          XMLE_DELOBJ(Title);
          XMLE_DELOBJ(Abstract);
        }
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
            pt2Class=NULL;
            if(equals("Name",4,name)){XMLE_ADDOBJ(Name);}
            else if(equals("Title",5,name)){XMLE_ADDOBJ(Title);}
            else if(equals("Abstract",8,name)){XMLE_ADDOBJ(Abstract);}
          }
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
    };

    
    class XMLE_ViewServiceCSW: public CXMLObjectInterface{};
    class XMLE_DatasetCSW: public CXMLObjectInterface{};
    
    
    class XMLE_Inspire: public CXMLObjectInterface{
      public:
      std::vector <XMLE_ViewServiceCSW*> ViewServiceCSW;
      std::vector <XMLE_DatasetCSW*> DatasetCSW;
      std::vector <XMLE_AuthorityURL*> AuthorityURL;
      std::vector <XMLE_Identifier*> Identifier;
      ~XMLE_Inspire(){
          XMLE_DELOBJ(ViewServiceCSW);
          XMLE_DELOBJ(DatasetCSW);
          XMLE_DELOBJ(AuthorityURL);
          XMLE_DELOBJ(Identifier);
      }
      
      void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
        CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
        base->currentNode=(CXMLObjectInterface*)this;
        if(rc==0)if(value!=NULL)this->value.copy(value);
        if(rc==1){
          pt2Class=NULL;
          if(equals("ViewServiceCSW",14,name)){XMLE_ADDOBJ(ViewServiceCSW);}
          else if(equals("DatasetCSW",10,name)){XMLE_ADDOBJ(DatasetCSW);}
          else if(equals("AuthorityURL",12,name)){XMLE_ADDOBJ(AuthorityURL);}
          else if(equals("Identifier",10,name)){XMLE_ADDOBJ(Identifier);}
        }
        if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
      }  
    };
    
    
    class XMLE_WMS: public CXMLObjectInterface{
      public:
        std::vector <XMLE_Title*> Title;
        std::vector <XMLE_Abstract*> Abstract;
        std::vector <XMLE_RootLayer*> RootLayer;
        std::vector <XMLE_TitleFont*> TitleFont;
        std::vector <XMLE_ContourFont*> ContourFont;
        std::vector <XMLE_SubTitleFont*> SubTitleFont;
        std::vector <XMLE_DimensionFont*> DimensionFont;
        std::vector <XMLE_GridFont*> GridFont;
        std::vector <XMLE_WMSFormat*> WMSFormat;
        std::vector <XMLE_WMSExceptions*> WMSExceptions;
        std::vector <XMLE_Inspire*> Inspire;

         
        
        
        
        ~XMLE_WMS(){
          XMLE_DELOBJ(Title);
          XMLE_DELOBJ(Abstract);
          XMLE_DELOBJ(RootLayer);
          XMLE_DELOBJ(TitleFont);
          XMLE_DELOBJ(ContourFont);
          XMLE_DELOBJ(SubTitleFont);
          XMLE_DELOBJ(DimensionFont);
          XMLE_DELOBJ(GridFont);
          XMLE_DELOBJ(WMSFormat);
          XMLE_DELOBJ(WMSExceptions);
          //XMLE_DELOBJ(Keywords);

          XMLE_DELOBJ(Inspire);
          
          
        }
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
            pt2Class=NULL;
            if(equals("Title",5,name)){XMLE_ADDOBJ(Title);}
            else if(equals("GridFont",8,name)){XMLE_ADDOBJ(GridFont);}
            else if(equals("Abstract",8,name)){XMLE_ADDOBJ(Abstract);}
            else if(equals("RootLayer",9,name)){XMLE_ADDOBJ(RootLayer);}
            else if(equals("WMSFormat",9,name)){XMLE_ADDOBJ(WMSFormat);}
            else if(equals("WMSExceptions",13,name)){XMLE_ADDOBJ(WMSExceptions);}
            else if(equals("TitleFont",9,name)){XMLE_ADDOBJ(TitleFont);}
            else if(equals("ContourFont",11,name)){XMLE_ADDOBJ(ContourFont);}
            else if(equals("SubTitleFont",12,name)){XMLE_ADDOBJ(SubTitleFont);}
            else if(equals("DimensionFont",13,name)){XMLE_ADDOBJ(DimensionFont);}
            else if(equals("Inspire",7,name)){XMLE_SETOBJ(Inspire);}
            //else if(equals("Keywords",8,name)){XMLE_ADDOBJ(Keywords);}
            //else if(equals("MetadataURL",11,name)){XMLE_ADDOBJ(MetadataURL);}
          }
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
    };
    
    class XMLE_OpenDAP: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string enabled,path;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("enabled",7,attrname)){attr.enabled.copy(attrvalue);return;}
        else if(equals("path",4,attrname)){attr.path.copy(attrvalue);return;}
      }
    };
  
    class XMLE_WCS: public CXMLObjectInterface{
      public:
        std::vector <XMLE_Name*> Name;
        std::vector <XMLE_Title*> Title;
        std::vector <XMLE_Abstract*> Abstract;
        std::vector <XMLE_WCSFormat*> WCSFormat;
        ~XMLE_WCS(){
          XMLE_DELOBJ(Name);
          XMLE_DELOBJ(Title);
          XMLE_DELOBJ(Abstract);
          XMLE_DELOBJ(WCSFormat);
        }
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
            pt2Class=NULL;
            if(equals("Name",4,name)){XMLE_ADDOBJ(Name);}
            else if(equals("Title",5,name)){XMLE_ADDOBJ(Title);}
            else if(equals("Abstract",8,name)){XMLE_ADDOBJ(Abstract);}
            else if(equals("WCSFormat",9,name)){XMLE_ADDOBJ(WCSFormat);}
          }
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
    };
  
    
    class XMLE_CacheDocs: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string enabled,cachefile;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("enabled",7,attrname)){attr.enabled.copy(attrvalue);return;}
          else if(equals("cachefile",9,attrname)){attr.cachefile.copy(attrvalue);return;}
        }
    };
  
    
    class XMLE_WMSLayer: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string service,layer,bgcolor,style;
            bool transparent;

            Cattr() {
              transparent=true;
              bgcolor.copy("");
              style.copy("");
            }
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("layer",5,attrname)){attr.layer.copy(attrvalue);return;}
          else if(equals("service",7,attrname)){attr.service.copy(attrvalue);return;}
          else if(equals("transparent",11,attrname)){attr.transparent=parseBool(attrvalue);return;}
          else if(equals("bgcolor",7,attrname)){attr.bgcolor.copy(attrvalue);return;}
          else if(equals("style",5,attrname)){attr.style.copy(attrvalue);return;}
        }
    };
    
    class XMLE_DataPostProc: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string a,b,c,units,algorithm,mode,name;
        }attr;
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("a",1,attrname)){attr.a.copy(attrvalue);return;}
          else if(equals("b",1,attrname)){attr.b.copy(attrvalue);return;}
          else if(equals("c",1,attrname)){attr.c.copy(attrvalue);return;}
          else if(equals("mode",4,attrname)){attr.mode.copy(attrvalue);return;}
          else if(equals("name",4,attrname)){attr.name.copy(attrvalue);return;}
          else if(equals("units",5,attrname)){attr.units.copy(attrvalue);return;}
          else if(equals("algorithm",9,attrname)){attr.algorithm.copy(attrvalue);return;}
        }
    };
    
    class XMLE_Position: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string top,left,right,bottom;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("top",3,attrname)){attr.top.copy(attrvalue);return;}
        else if(equals("left",4,attrname)){attr.left.copy(attrvalue);return;}
        else if(equals("right",5,attrname)){attr.right.copy(attrvalue);return;}
        else if(equals("bottom",6,attrname)){attr.bottom.copy(attrvalue);return;}
      }
    };
    
  
    
    class XMLE_Grid: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string name,precision,resolution;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if(equals("name",4,attrname)){attr.name.copy(attrvalue);return;}
        else if(equals("precision",9,attrname)){attr.precision.copy(attrvalue);return;}
        else if(equals("resolution",10,attrname)){attr.resolution.copy(attrvalue);return;}
      }
    };
    
    class XMLE_AdditionalLayer: public CXMLObjectInterface{
    public:
      class Cattr{
      public:
        CT::string replace,style;
      }attr;
      void addAttribute(const char *attrname,const char *attrvalue){
        if (equals("replace",7,attrname)){attr.replace.copy(attrvalue);return;}
        else if (equals("style",5,attrname)){attr.style.copy(attrvalue);return;}
      }
    };
    
    class XMLE_Layer: public CXMLObjectInterface{
      public:
        class Cattr{
          public:
            CT::string type,hidden;
        }attr;
        
        
        std::vector <XMLE_Name*> Name;
        std::vector <XMLE_Group*> Group;
        std::vector <XMLE_Title*> Title;
        std::vector <XMLE_Abstract*> Abstract;
        
        std::vector <XMLE_DataBaseTable*> DataBaseTable;
        std::vector <XMLE_Variable*> Variable;
        std::vector <XMLE_FilePath*> FilePath;
        std::vector <XMLE_TileSettings*> TileSettings;
        std::vector <XMLE_DataReader*> DataReader;
        std::vector <XMLE_Dimension*> Dimension;
        std::vector <XMLE_Legend*> Legend;
        std::vector <XMLE_Scale*> Scale;
        std::vector <XMLE_Offset*> Offset;
        std::vector <XMLE_Min*> Min;
        std::vector <XMLE_Max*> Max;
        std::vector <XMLE_Log*> Log;
        std::vector <XMLE_ShadeInterval*> ShadeInterval;
        std::vector <XMLE_ContourLine*> ContourLine;
        std::vector <XMLE_ContourIntervalL*> ContourIntervalL;
        std::vector <XMLE_ContourIntervalH*> ContourIntervalH;
        std::vector <XMLE_SmoothingFilter*> SmoothingFilter;
        std::vector <XMLE_ValueRange*> ValueRange;
        std::vector <XMLE_ImageText*> ImageText;
        std::vector <XMLE_LatLonBox*> LatLonBox;
        std::vector <XMLE_Projection*> Projection;
        std::vector <XMLE_Styles*> Styles;
        std::vector <XMLE_RenderMethod*> RenderMethod;
        std::vector <XMLE_MetadataURL*> MetadataURL;
        std::vector <XMLE_Cache*> Cache;
        std::vector <XMLE_WMSLayer*> WMSLayer;
        std::vector <XMLE_DataPostProc*> DataPostProc;
        std::vector <XMLE_Position*> Position;
        std::vector <XMLE_WMSFormat*> WMSFormat;
        std::vector <XMLE_Grid*> Grid;
        std::vector <XMLE_AdditionalLayer*> AdditionalLayer;
        std::vector <XMLE_FeatureInterval*> FeatureInterval;
        
        
        ~XMLE_Layer(){
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
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          pt2Class=NULL;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
          
            if(equals("Name",4,name)){XMLE_ADDOBJ(Name);}
            else if(equals("Group",5,name)){XMLE_ADDOBJ(Group);}
            else if(equals("Title",5,name)){XMLE_ADDOBJ(Title);}
            else if(equals("Abstract",8,name)){XMLE_ADDOBJ(Abstract);}
            else if(equals("DataBaseTable",13,name)){XMLE_ADDOBJ(DataBaseTable);}
            else if(equals("Variable",8,name)){XMLE_ADDOBJ(Variable);}
            else if(equals("FilePath",8,name)){XMLE_ADDOBJ(FilePath);}
            else if(equals("TileSettings",12,name)){XMLE_ADDOBJ(TileSettings);}
            else if(equals("DataReader",10,name)){XMLE_ADDOBJ(DataReader);}
            else if(equals("Dimension",9,name)){XMLE_ADDOBJ(Dimension);}
            else if(equals("Legend",6,name)){XMLE_ADDOBJ(Legend);}
            else if(equals("Scale",5,name)){XMLE_ADDOBJ(Scale);}
            else if(equals("Offset",6,name)){XMLE_ADDOBJ(Offset);}
            else if(equals("Min",3,name)){XMLE_ADDOBJ(Min);}
            else if(equals("Max",3,name)){XMLE_ADDOBJ(Max);}            
            else if(equals("Log",3,name)){XMLE_ADDOBJ(Log);}
            else if(equals("ShadeInterval",13,name)){XMLE_ADDOBJ(ShadeInterval);}
            else if(equals("ContourLine",11,name)){XMLE_ADDOBJ(ContourLine);}
            else if(equals("ContourIntervalL",16,name)){XMLE_ADDOBJ(ContourIntervalL);}
            else if(equals("ContourIntervalH",16,name)){XMLE_ADDOBJ(ContourIntervalH);}
            else if(equals("ValueRange",10,name)){XMLE_ADDOBJ(ValueRange);}
            else if(equals("ImageText",9,name)){XMLE_ADDOBJ(ImageText);}
            else if(equals("LatLonBox",9,name)){XMLE_ADDOBJ(LatLonBox);}
            else if(equals("Projection",10,name)){XMLE_ADDOBJ(Projection);}
            else if(equals("Styles",6,name)){XMLE_ADDOBJ(Styles);}
            else if(equals("RenderMethod",12,name)){XMLE_ADDOBJ(RenderMethod);}
            else if(equals("MetadataURL",11,name)){XMLE_ADDOBJ(MetadataURL);} 
            else if(equals("Cache",5,name)){XMLE_ADDOBJ(Cache);}
            else if(equals("WMSLayer",8,name)){XMLE_ADDOBJ(WMSLayer);}
            else if(equals("DataPostProc",12,name)){XMLE_ADDOBJ(DataPostProc);}
            else if(equals("SmoothingFilter",15,name)){XMLE_ADDOBJ(SmoothingFilter);}
            else if(equals("Position",8,name)){XMLE_ADDOBJ(Position);}
            else if(equals("WMSFormat",9,name)){XMLE_ADDOBJ(WMSFormat);}
            else if(equals("Grid",4,name)){XMLE_ADDOBJ(Grid);}
            else if(equals("AdditionalLayer",15,name)){XMLE_ADDOBJ(AdditionalLayer);}
            else if(equals("FeatureInterval",15,name)){XMLE_ADDOBJ(FeatureInterval);}
            
            
          }
          if(pt2Class!=NULL){pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);  pt2Class=NULL;}
        }
        void addAttribute(const char *attrname,const char *attrvalue){
          if(equals("type",4,attrname)){attr.type.copy(attrvalue);return;}
          else if(equals("hidden",6,attrname)){attr.hidden.copy(attrvalue);return;}
        }
    };

    class XMLE_Configuration: public CXMLObjectInterface{
      public:
        std::vector <XMLE_Legend*> Legend;
        std::vector <XMLE_WMS*> WMS;
        std::vector <XMLE_WCS*> WCS;
        std::vector <XMLE_OpenDAP*> OpenDAP;
        std::vector <XMLE_Path*> Path;
        std::vector <XMLE_TempDir*> TempDir;
        std::vector <XMLE_OnlineResource*> OnlineResource;
        std::vector <XMLE_DataBase*> DataBase;
        std::vector <XMLE_Projection*> Projection;
        std::vector <XMLE_Layer*> Layer;
        std::vector <XMLE_Style*> Style;
        std::vector <XMLE_CacheDocs*> CacheDocs;
        std::vector <XMLE_AutoResource*> AutoResource;
        std::vector <XMLE_Dataset*> Dataset;
        std::vector <XMLE_Include*> Include;

        ~XMLE_Configuration(){
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
          XMLE_DELOBJ(CacheDocs);
          XMLE_DELOBJ(AutoResource);
          XMLE_DELOBJ(Dataset);
          XMLE_DELOBJ(Include);
        }
        void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
          CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
          base->currentNode=(CXMLObjectInterface*)this;
          if(rc==0)if(value!=NULL)this->value.copy(value);
          if(rc==1){
            pt2Class=NULL;
            if(equals("Legend",6,name)){XMLE_ADDOBJ(Legend);}
            else if(equals("WMS",3,name)){XMLE_SETOBJ(WMS);}
            else if(equals("WCS",3,name)){XMLE_SETOBJ(WCS);}
            else if(equals("Path",4,name)){XMLE_ADDOBJ(Path);}
            else if(equals("OpenDAP",7,name)){XMLE_ADDOBJ(OpenDAP);}
            else if(equals("TempDir",7,name)){XMLE_ADDOBJ(TempDir);}
            else if(equals("OnlineResource",14,name)){XMLE_ADDOBJ(OnlineResource);}
            else if(equals("DataBase",8,name)){XMLE_ADDOBJ(DataBase);}
            else if(equals("Projection",10,name)){XMLE_ADDOBJ(Projection);}
            else if(equals("Layer",5,name)){XMLE_ADDOBJ(Layer);}
            else if(equals("Style",5,name)){XMLE_ADDOBJ(Style);}
            else if(equals("CacheDocs",9,name)){XMLE_ADDOBJ(CacheDocs);}
            else if(equals("AutoResource",12,name)){XMLE_ADDOBJ(AutoResource);}
            else if(equals("Dataset",7,name)){XMLE_ADDOBJ(Dataset);}
            else if(equals("Include",7,name)){XMLE_ADDOBJ(Include);}
          }
          if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
        }
    };
    void addElementEntry(int rc,const char *name,const char *value){
      CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
      base->currentNode=(CXMLObjectInterface*)this;
      if(rc==0){
        pt2Class=NULL;
        if(equals("Configuration",13,name)){
          XMLE_SETOBJ(Configuration);
        }
      }
      if(pt2Class!=NULL)pt2Class->addElement(baseClass,rc-pt2Class->level,name,value);
    }
    void addAttributeEntry(const char *name,const char *value){
      if(currentNode!=NULL&&pt2Class!=NULL){
        currentNode->addAttribute(name,value);
      }
    }
    std::vector <XMLE_Configuration*> Configuration;
    ~CServerConfig(){
      XMLE_DELOBJ(Configuration);
    }
};
#endif

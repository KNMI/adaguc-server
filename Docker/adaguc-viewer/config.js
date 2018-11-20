var baseLayerConfiguration = [
  {
    service:"http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?",
    name:"naturalearth2",
    title:"World base layer Natural Earth ",
    enabled: true,
    type: 'twms',
    format:"image/gif"
  },{
    title:"arcGisCanvas",
    name:"arcGisCanvas",
    type: 'twms',
    enabled:false
  }, {
    title:"OSM",
    name:"OSM",
    type: 'twms',
    enabled:false
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?",
    name:"streetmap",
    title:"Open streetmap",
    enabled: false,
    type: 'wms',
    format:"image/png"
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/MODIS_Netherlands.cgi?",
    name:"modis_250m_netherlands_8bit",
    title:"Static Modis image of the Netherlands",
    enabled: false,
    format:"image/png"
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"ne_10m_admin_0_countries_simplified",
    format:"image/png",
    title:"World country borders",
    enabled: true,
    keepOnTop:true
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"grid1x1",
    format:"image/png",
    title:"Grid 1x1 degrees",
    enabled: false,
    keepOnTop:true
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"grid2x2",
    format:"image/png",
    title:"Grid 2x2 degrees",
    enabled: false,
    keepOnTop:true
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"grid5x5",
    format:"image/png",
    title:"Grid 5x5 degrees",
    enabled: false,
    keepOnTop:true
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"grid10x10",
    format:"image/png",
    title:"Grid 10x10 degrees",
    enabled: false,
    keepOnTop:true
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"nl_raster_latlon",
    format:"image/png",
    title:"KNMI baselayer of the Netherlands",
    enabled: false,
    keepOnTop:false
  },{
    service:"http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?",
    name:"world_country_names",
    format:"image/png",
    title:"Country names",
    enabled: false,
    keepOnTop:true
  }
];

var defaultProjection = {srs:'EPSG:4326',bbox:'-180,-90,180,90'};

var hashLocationNotfiyAddLayer = false;

var getFeatureInfoApplications = [
  {name:'Time series mode',iconCls:'button_getfeatureinfo',location:'apps/gfiapp_d3c3.html'}
  //,{name:'Glameps application',iconCls:'button_getfeatureinfo',location:'../gfiapps/GLAMEPS_gfiapp.html'}
];

var dataChooserConfiguration = [
  {
    title:'KNMI realtime precipitation radar',
    thumbnail:'img/knmi_radar_icon.png',
    service:'http://geoservices.knmi.nl/cgi-bin/RADNL_OPER_R___25PCPRR_L3.cgi?',
    layer:'RADNL_OPER_R___25PCPRR_L3_COLOR',
    srs:'EPSG:3857',
    bbox:'220000,6500000,1000000,7200000',
    baselayerservice:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',
    baselayername:'streetmap',
    opacity:0.8
  },{
    title:'MSGCPP last 7 days',
    thumbnail:'img/750px-MSG-CPP_Screenshot-Google_Earth-cwp.png',
    service:'http://msgcpp-ogc-realtime.knmi.nl/msgrt.cgi?',
    layer:'atmosphere_optical_thickness_due_to_cloud'
  },{
    title:'MSGCPP archive',
    thumbnail:'img/750px-MSG-CPP_Screenshot-Google_Earth-cwp.png',
    service:'http://msgcpp-ogc-archive.knmi.nl/msgar.cgi?',
    layer:'atmosphere_optical_thickness_due_to_cloud'
  },/*{
    title:'OMI daily SO2',
    thumbnail:'http://adaguc.knmi.nl/contents/webservices/OMI_TDCSO2_L3.jpg',
    service:'http://geoservices.knmi.nl/cgi-bin/OMI___OPER_R___TDCSO2__L3.cgi?',
    layer:'OMI_L3_TDCSO2',
    srs:'EPSG:4326',
    bbox:'-180,-90,180,90'
  },*/{
    title:'OMI yearly NO2',
    thumbnail:'img/OMI_NO2_TYTRCNO_screenshot.png',
    service:'http://geoservices.knmi.nl/cgi-bin/OMI___OPER_R___TYTRCNO_L3.cgi?',
    layer:'omi_yearly_tropospheric_no2',
    srs:'EPSG:4326',
    bbox:'-180,-90,180,90'
  },/*{
    title:'Geoserver @ suite.opengeo.org',
    thumbnail:'http://suite.opengeo.org/geoserver/wms?service=WMS&version=1.3.0&request=GetMap&layers=usa:states&srs=EPSG:4326&bbox=24.956,-124.731,49.372,-66.97&format=image/png&width=780&height=330',
    service:'http://suite.opengeo.org/geoserver/wms?',
    layer:'usa:states'
  },*/{
    title:'Inspire->KNMI: Actuele 10min data',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/inspireviewservice.cgi?DATASET=urn:xkdc:ds:nl.knmi::Actuele10mindataKNMIstations/1/&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,ta,overlay&WIDTH=200&HEIGHT=150&CRS=EPSG%3A3857&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&STYLES=temperature%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&&time=current&showdims=true',
    service:'http://geoservices.knmi.nl/cgi-bin/inspireviewservice.cgi?DATASET=urn:xkdc:ds:nl.knmi::Actuele10mindataKNMIstations/1/&SERVICE=WMS&',
    layer:'ta'    
  },
  {
    title:'inspire:aardbevingen_nederland_2',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/inspire/aardbevingen_nederland_2.cgi?SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,magnitude,overlay&WIDTH=200&HEIGHT=150&CRS=EPSG%3A3857&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&STYLES=temperature%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&&time=current&showdims=true',
    service:'http://geoservices.knmi.nl/cgi-bin/inspire/aardbevingen_nederland_2.cgi?SERVICE=WMS&',
    layer:'magnitude'    
  }
  ,
  {
    title:'inspire:aardbevingen_catalogus_1',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/inspire/aardbevingen_catalogus_1.cgi?SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,magnitude&WIDTH=300&HEIGHT=200&CRS=EPSG%3A3857&BBOX=585870.0766630658,6555091.875607174,880402.9486353853,6744633.534542043&STYLES=magnitude_continous%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&',
    service:'http://geoservices.knmi.nl/cgi-bin/inspire/aardbevingen_catalogus_1.cgi?',
    layer:'magnitude'    
  }
  
  ,{
    title:'Inspire->KNMI: Waarneem stations',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/inspireviewservice.cgi?DATASET=urn:xkdc:ds:nl.knmi::waarneemstations/2/&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,obs_temp,overlay&WIDTH=200&HEIGHT=150&CRS=EPSG%3A3857&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&STYLES=temperature%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&&time=current&showdims=true',
    service:'http://geoservices.knmi.nl/cgi-bin/inspireviewservice.cgi?DATASET=urn:xkdc:ds:nl.knmi::waarneemstations/2/&SERVICE=WMS&',
    layer:'obs_temp'    
  },
  {
    title:'Inspire->KNMI: Etmaalgegevens',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/inspireviewservice.cgi?DATASET=urn:xkdc:ds:nl.knmi::etmaalgegevensKNMIstations/1/&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,TG,overlay&WIDTH=200&HEIGHT=150&CRS=EPSG%3A3857&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&STYLES=temperature%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&time=current&showdims=true',
    service:'http://geoservices.knmi.nl/cgi-bin/inspireviewservice.cgi?DATASET=urn:xkdc:ds:nl.knmi::etmaalgegevensKNMIstations/1/&SERVICE=WMS&',
    layer:'TG'    
  },{
    title:'Harmonie forecasts',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/HARM_N25.cgi?SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,precipitation_flux,overlay&WIDTH=200&HEIGHT=150&CRS=EPSG%3A3857&BBOX=143873.45679012343,6500000,1076126.5432098766,7200000&STYLES=precip_cwk%2Fbilinearcontour&FORMAT=image/png&TRANSPARENT=TRUE&showdims=true',
    service:'http://geoservices.knmi.nl/cgi-bin/HARM_N25.cgi?',
    layer:'precipitation_flux'    

    
  },{
    title:'MSG Seviri Ash products',
    thumbnail:'http://geoservices.knmi.nl/cgi-bin/SEVIR_OPER_R___VOLE____L2.cgi?SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,MSG3/ash_loading,overlay&WIDTH=400&HEIGHT=300&CRS=EPSG%3A4326&BBOX=-74.36030201,-123.8533563909424,74.88885300999999,123.8533563909424&STYLES=ash_loading%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&timecurrent&title=MSG%20Seviri%20Ash',
    service:'http://geoservices.knmi.nl/cgi-bin/SEVIR_OPER_R___VOLE____L2.cgi',
    layer:'MSG3/ash_loading'    

    
  },{
        title:'Accidents in traffic',
        thumbnail:'img/accidents_weather.jpg',
        service:'http://geoservices.knmi.nl/cgi-bin/weathertraffic.cgi',
        layer:'Loctypon'
  },
  {
        title:'Himawari Demo',
        thumbnail:'http://geoservices.knmi.nl/cgi-bin/himawari.cgi?SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tiled&WIDTH=400&HEIGHT=300&CRS=EPSG%3A4326&BBOX=-31.97397264884439,80.83215795209837,13.546051112849678,156.38109791260632&STYLES=auto%2Frgba&FORMAT=image/png&TRANSPARENT=TRUE&&time=2016-11-14T07%3A50%3A00Z',
        service:'http://geoservices.knmi.nl/cgi-bin/himawari.cgi',
        layer:'tiled'

  }
  
  
  
  
];

var mapTypeConfiguration = [ 
  { 
    title: 'World WGS84', 
    bbox: [-180,-90,180,90], 
    srs: 'EPSG:4326',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },/*{ 
    title: 'Mollweide (7399)', 
    bbox: [-18157572.744146045,-11212941.682924412,18085661.018022258,11419683.192411266],
    srs: 'EPSG:7399',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },*/
  { 
    title: 'Robinson', 
    bbox: [-17036744.451383516,-10711364.114367772,16912038.081015453,10488456.659686875],
    srs: 'EPSG:54030',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },{ 
    title: 'World Mercator', 
    bbox: [-19000000,-19000000,19000000,19000000],
    srs: 'EPSG:3857',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },{ 
    title: 'Openstreetmap', 
    bbox: [-19000000,-19000000,19000000,19000000],   
    srs: 'EPSG:3857', 
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'streetmap',type: 'wms'}
  },{ 
    title: 'Northern Hemisphere', 
    bbox: [-5661541.927991125,-3634073.745615984,5795287.923063262,2679445.334384017],
    srs: 'EPSG:3411',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },{ 
    title: 'Southern Hemisphere', 
    bbox: [-4589984.273212382,-2752857.546211313,5425154.657417289,2986705.2537886878],
    srs: 'EPSG:3412',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },{ 
    title: 'Europe North Pole', 
    bbox: [-13000000,-13000000,13000000,13000000],
    srs: 'EPSG:3575',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },
  
  
  
 
  
  { 
    title: 'Europe stereographic', 
    bbox: [-2776118.977564746,-6499490.259201691,9187990.785775745,971675.53185069], 
    srs: 'EPSG:32661',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  }, { 
    title: 'North America', 
    bbox: [-2015360.8716608454,-697107.5349683464,9961718.159421016,6782157.107682772],
    srs: 'EPSG:50001',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },
  
  { 
    title: 'Openstreetmap NL', 
    bbox: [220000,6500000,1000000,7200000],   
    srs: 'EPSG:3857', 
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'streetmap',type: 'wms'}
  }/*,{ 
    title: 'The Netherlands (28992)', 
    bbox: [-350000,125000,700000,900000 ],
    srs: 'EPSG:28992',
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'naturalearth2',type: 'wms'}
  },*//*{ 
    title: 'PDOK BRT NL', 
    bbox: [-350000,125000,700000,900000],   
    srs: 'EPSG:28992', 
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'streetmap',type: 'wms'}
  },*//*{ 
    title: 'Openstreetmap Schiphol', 
    bbox: [515813.2988839851,6850485.5537392385,544160.8241851525,6869318.449956324],   
    srs: 'EPSG:3857', 
    baselayer:{service:'http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?',name:'streetmap',type: 'wms'}
  }*/
  
  /*,{ 
    title: 'Schiphol Satellite + Aerial Mercator', 
    bbox: [522402.16196646384,6852917.910540686,537831.8792436487,6863959.075064662],
    srs: 'EPSG:3857',
    baselayer:{service:'http://birdexp03.knmi.nl/cgi-bin/plieger/wmst.cgi?',name:'satellite'}
  },{ 
    title: 'Topographic Mercator in NL', 
    bbox: [220000,6500000,1000000,7200000],
    srs: 'EPSG:3857',
    baselayer:{service:'http://birdexp03.knmi.nl/cgi-bin/plieger/wmst.cgi?',name:'topo'}
  },{ 
    title: 'Canvas Mercator in NL', 
    bbox: [220000,6500000,1000000,7200000],
    srs: 'EPSG:3857',
    baselayer:{service:'http://birdexp03.knmi.nl/cgi-bin/plieger/wmst.cgi?',name:'canvas'}
  }*/
];

var defaultLanguage = { language: 'en' }; // <-- Language for the ADAGUC viewer.
var defaultUsernameSearch = "adaguc"; // <-- Username for the GeoNames API. 1
var geoNamesURL = "http://api.geonames.org/search?q={searchTerm}&username={username}&maxRows=1"; // <-- URL for the GeoNames API. Requires 'defaultUsernameSearch'    
      
//FOR JSP:
/*
var scaleBarURL        = "http://webgis.nmdc.eu/viewer2.0/webmapjs/php/makeScaleBar.php?";
var xml2jsonrequestURL = "/impactportal/AdagucViewer?SERVICE=XML2JSON&";
var requestProxy = "/impactportal/AdagucViewer?SERVICE=PROXY&";
*/

//For PHP:

var requestProxy       = "webmapjs_php/MakeRequest.php?";
var xml2jsonrequestURL = "webmapjs_php/xml2jsonrequest.php?"

// xml2jsonrequestURL = 'http://localhost:8080/adaguc-services/xml2json?'
getFeatureInfoApplications.push({name:'AutoWMS',iconCls:'button_getfeatureinfo'});
getFeatureInfoApplications.open = 'AutoWMS';


var WMJSTileRendererTileSettings = {
  arcGisCanvas: {
    'EPSG:3857': {
      home: 'http://services.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Light_Gray_Base/MapServer/tile/',
      minLevel: 1,
      maxLevel: 16,
      tileServerType: 'arcgisonline',
      copyRight: 'Basemap copyright: 2013 Esri, DeLorme, NAVTEQ'
    },
    'EPSG:28992': {
      home: 'http://services.arcgisonline.nl/arcgis/rest/services/Basiskaarten/Canvas/MapServer/tile/',
      minLevel: 1,
      maxLevel: 12,
      origX:-285401.92,
      origY:903401.92,
      resolution:3440.64,
      tileServerType: 'arcgisonline',
      copyRight: 'Basiskaart bronnen: Esri Nederland, Esri, Kadaster, CBS en Rijkswaterstaat'
    }
  },
  arcGisTopo: {
    'EPSG:3857': {
      home: 'http://services.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/',
      minLevel: 1,
      maxLevel: 19,
      tileServerType: 'arcgisonline',
      copyRight: 'Basemap sources: Esri, DeLorme, NAVTEQ, TomTom, Intermap, increment P Corp., GEBCO, USGS, FAO, NPS, NRCAN, GeoBase, ' +
        'IGN, Kadaster NL, Ordnance Survey, Esri Japan, METI, Esri China (Hong Kong), and the GIS User Community'
    },
    'EPSG:28992': {
      home: 'http://services.arcgisonline.nl/arcgis/rest/services/Basiskaarten/Topo/MapServer/tile/',
      minLevel: 1,
      maxLevel: 12,
      origX:-285401.92,
      origY:903401.92,
      resolution:3440.64,
      tileServerType: 'arcgisonline',
      copyRight: 'Basiskaart bronnen: Esri Nederland, Esri, Kadaster, CBS, Min VROM, Rijkswaterstaat en gemeenten: Rotterdam, Breda, Tilburg'
    }
  },
  arcGisOceanBaseMap:  {
    home: 'http://services.arcgisonline.com/ArcGIS/rest/services/Ocean_Basemap/MapServer/tile/',
    minLevel: 1,
    maxLevel: 19,
    tileServerType: 'arcgisonline',
    copyRight: 'Basemap sources: Esri, GEBCO, NOAA, National Geographic, DeLorme, NAVTEQ, Geonames.org, and other contributors'
  },
  arcGisSat: {
    'EPSG:4326': {
      home: 'http://services.arcgisonline.com/ArcGIS/rest/services/ESRI_Imagery_World_2D/MapServer/tile/',
      minLevel: 1,
      maxLevel: 15,
      tileServerType: 'arcgisonline',
      origX:-180,
      origY:90,
      resolution:0.3515625,
      tileSize:512
    },
    'EPSG:3857': {
      home: 'http://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/',
      minLevel: 1,
      maxLevel: 18,
      tileServerType: 'arcgisonline'
    }
  },
  OSM: {
    'EPSG:3857': {
      home: '//b.tile.openstreetmap.org/',
      minLevel: 1,
      maxLevel: 16,
      tileServerType: 'osm'
    },
    'EPSG:28992': {
      home: 'http://services.arcgisonline.nl/ArcGIS/rest/services/Basiskaarten/PDOK_BRT/MapServer/tile/',
      minLevel: 1,
      maxLevel: 12,
      origX:-285401.92,
      origY:903401.92,
      resolution:3440.64,
      tileServerType: 'arcgisonline',
      copyRight: 'Basiskaart bronnen: PDOK, Kadaster, OpenStreetMap'
    }
  },
  NaturalEarth2: {
    'EPSG:3411': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG3411/',
      minLevel: 1,
      maxLevel: 6,
      origX:-12400000,
      origY:12400000,
      resolution:96875,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:3412': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG3412/',
      minLevel: 1,
      maxLevel: 6,
      origX:-12400000,
      origY:12400000,
      resolution:96875,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:3575': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG3575/',
      minLevel: 1,
      maxLevel: 6,
      origX:-13000000,
      origY:13000000,
      resolution:101562.5,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:3857': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG3857/',
      minLevel: 1,
      maxLevel: 7,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:4258': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG4326/',
      minLevel: 1,
      maxLevel: 6,
      origX:-180,
      origY:90,
      resolution:0.703125,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:4326': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG4326/',
      minLevel: 1,
      maxLevel: 6,
      origX:-180,
      origY:90,
      resolution:0.703125,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:28992': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG28992/',
      minLevel: 1,
      maxLevel: 5,
      origX:-2999000,
      origY:2995500,
      resolution:23437,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:32661': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG32661/',
      minLevel: 1,
      maxLevel: 7,
      origX:-5000000.0,
      origY:4000000.0,
      resolution:58593.75,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    },
    'EPSG:54030': {
      home: 'http://geoservices.knmi.nl/tiledbasemaps/NaturalEarth2/EPSG54030/',
      minLevel: 1,
      maxLevel: 7,
      origX:-17000000.0,
      origY:8625830.0,
      resolution:132812.5,
      tileServerType: 'wmst',
      copyRight: 'NPS - Natural Earth II'
    }
  }
};


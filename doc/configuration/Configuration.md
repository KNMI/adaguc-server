Configuration
=============

[Back to readme](../../Readme.md)

Here all configuration options of adaguc-server are listed. The configuration is done in XML. The hierarchical structure of the configuration is reflected in this document. XML elements can be configured with attributes and values. 

-   (name1,name2) - Indicates the possible attributes of an XML element
-   `<value>` - Indicates that a value can be assigned to the XML
    element.

Configuration

-   Path (value) - Mandatory, this is the basepath of the server where
    it searches for e.g it's templates.
-   TempDir (value) - Mandatory, directory where cache-files are stored.
-   [OnlineResource](OnlineResource.md) (value) - Mandatory, the external address of
    the server, usually ends with a ? token.
-   [DataBase](DataBase.md) (parameters) - Mandatory, parameters is the
    attribute where database settings can be configured.
-   CacheDocs (enabled) - Defaults to false. Set enabled to "true" to
    enable caching of GetCapabilities documents.
-   [AutoResource](AutoResource.md) (enableautoopendap,enablelocalfile,enablecache)
    -   [Dir](Dir.md) (basedir,prefix)
-   [Dataset](Dataset.md) (enabled,location) - Allows to load additional
    configuration files via the URL using the dataset keyword
-   [Include](Include.md) - (location) - Include additional configuration
    files to the service
-   [Logging](Logging.md) - (debug) - Configure the type of logging
-   [Settings](Settings.md) - (enablemetadatacache, enablecleanupsystem, cleanupsystemlimit, cache_age_cacheableresources, cache_age_volatileresources) - Configure global settings of the server / Dataset
-   [Environment](Environment.md) - (name, default) - For within dataset configuration, specify which values should be substituted
-   [EDR](EDRConfiguration/EDR.md) - Configuration options to enable EDR collections

<!-- -->

-   WMS
    -   Title - Mandatory, the title of the service
    -   Abstract - Mandatory, service description
    -   RootLayer
        -   Title - Mandatory, the title of the root layer
        -   Abstract - Mandatory, the abstract of the root layer
    -   TitleFont (location,size) - see [Font](Font.md) configuration
    -   SubTitleFont (location,size) - see [Font](Font.md) configuration
    -   DimensionFont (location,size) - see [Font](Font.md) configuration
    -   ContourFont (location,size) - see [Font](Font.md) configuration
    -   GridFont (location,size) - see [Font](Font.md) configuration
    -   [WMSFormat](WMSFormat.md) (name,format)
    -   Inspire - Optional, for inspire configuration, see
        [Configuration of an INSPIRE View Service](Configuration of an INSPIRE View Service.md)
        -   ViewServiceCSW - The global view service document (CSW
            service) describing the view services
        -   DatasetCSW - The dataset CSW service describing the dataset
        -   AuthorityURL - The name and URL of the Authority offering
            the data
        -   Identifier - The identifier of the Authority

<!-- -->

-   WCS
    -   Title - Mandatory, the title of the service
    -   Label - Mandatory, the description of the service
    -   [WCSFormat](WCSFormat.md) (name, driver, mimetype, options)

<!-- -->

-   [OpenDAP](OpenDAP.md) (enabled, path) - Settings for the ADAGUC OpenDAP
    server

<!-- -->

-   [Projection](Projection.md) (id,proj4) - Mandatory, defines the projections
    supported by the services.
    -   LatLonBox (minx, miny, maxx, maxy)
-   [Legend](Legend.md) (name, type, file) - Color palette definitions
    -   [palette](palette.md) (index,min,max,red,green,blue,alpha,color)

<!-- -->

-   [Style](Style.md) (name, title, abstract) - Style definition for the layers
    -   [Legend](Legend.md) (tickinterval, tickround, fixed) `<value>` - Specifies which legend to use
    -   Min `<value>` - The minimum value of the image to display, corresponds with the first colors in the legend.
    -   Max `<value>` - The minimum value of the image to display, corresponds with the last colors in the legend.
    -   Log `<value>` - Scale the colors using log transformation
    -   [ValueRange](ValueRange.md) (min,max) - Values between min and max are visible, outside this range is transparent
    -   [RenderMethod](RenderMethod.md) `<value>` - The way to render the image, e.g. contour, nearest or bilinear.
    -   [ContourLine](ContourLine.md) (width,linecolor,textcolor,textformatting,interval,classes) - Draw contourlines in the images, if the appropriate [RenderMethod](RenderMethod.md) is selected.
    -   [ShadeInterval](ShadeInterval.md) (min,max,label,bgcolor, fillcolor) - Draw the image using shading.
    -   [FeatureInterval](FeatureInterval.md) (match, matchid, label, bgcolor, fillcolor) - Draw GeoJSON features and select features based on regular expressions.
    -   [SymbolInterval](SymbolInterval.md) (min, max, binary_and, file, offset_x, offset_y) - Draw symbols according to point value
    -   SmoothingFilter `<value>` - Optional, defaults to 1. The filter to smooth the image when using contour lines and shading.
    -   [NameMapping](NameMapping.md) (name,title,abstract) - Optional, Provide detailed descriptions and human readable names for every style.
    -   [StandardNames](StandardNames.md) (standard_name, variable_name, units) - Optional, assigns this style to layers matching the standard_name and optionally the units.
    -   [Point](Point.md) (min,max,pointstyle,fillcolor,linecolor,textcolor,textformat,fontfile,fontsize,discradius,textradius,dot,anglestart,anglestop,plotstationid) Configuration of rendering point data.
    -   [Vector](Vector.md) (linecolor, linewidth,scale,vectorstyle,plotstationid,plotvalue,textformat, min, max, fontsize, outlinewidth, outlinecolor, textcolor) Configuration of rendering point data as vector.
    -   [FilterPoints](FilterPoints.md) (skip, use) Definition of set of points to
        skip or to use
    -   [Thinning](Thinning.md) (radius) Thinning of points to make sure that
        drawn points do not overlap
    -   [LegendGraphic](LegendGraphic.md) (value) Override legendgraphic request URL
        with custom image
    -   [Stippling](Stippling.md) (distancex, distancey, discradius)
        Configuration of stippling renderer.
    -   [RenderSettings](RenderSettings.md) (settings, striding, renderer, scalewidth, scalecontours, renderhint, rendertextforvectors) Configuration of
        renderers

<!-- -->

-   [Layer](Layer.md) (type,hidden)
    -   Name (force) `<value>` - Recommended, provide a name for the
        layer. Use the force="true" attribute to display the name as
        configured, otherwise the name is adjusted to become unique
        within groups.
    -   Title `<value>` - Optional, provide a title for the layer.
        If not configured it is derived from long_name, standard_name
        or variable names of the selected variable from the file.
    -   [Group](Group.md) (value) - Optional, layers are hierarchically
        nested based on the value attribute set in the Group element.
        The "/" token nests the layer further.
    -   Variable `<value>` - Mandatory, The variable from the file
        to use in the Layer. Multiple can be used for e.g. wind vector
        plotting.
    -   [FilePath](FilePath.md) (filter, maxquerylimit, ncml, retentionperiod, retentiontype) `<value>` -
        Mandatory, The directory to scan files with additional filter,
        or the file to use, or the OpenDAP url to use.
    -   [Dimension](Dimension.md)
        (name,interval,default,units,quantizeperiod,quantizemethod,hidden,fixvalue)
        `<value>` - Optional, Configure the dimensions (time,
        elevation, member) used. If none given the server tries to
        autodetect the dimensions.
    -   Styles `<value>` - Optional, A comma separated list of
        styles to use for this layer.
    -   [Projection](Projection.md) (proj4) - Override the projection defined in
        the file
    -   Cache (enabled) - <deprecated> Set enabled to "true" to
        enable caching of this layer. Useful when visualizing OpenDAP
        resources.
    -   [ImageText](ImageText.md) (attribute) `<value>` - Optional, print
        text in the bottom of the map.
    -   MetadataURL
    -   [TileSettings](TileSettings.md) - Configuration settings for tiling high
        resolution layers
    -   [DataPostProc](DataPostProc.md) (algorithm, a, b, c, units, name, mode)
    -   DataBaseTable `<value>` Override the auto assigned
        database table
    -   [AdditionalLayer](AdditionalLayer.md) (replace, style) `<value>` -
        Configure a new layer which is a composition of multiple other
        layers.
    -   [WMSFormat](WMSFormat.md) (name, quality) - Force the image format for this layer.
    -   \*All Style elements are accessible in Layer and can be
        overridden


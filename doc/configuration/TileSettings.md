TileSettings - Tiling functionality for high resolution datasets
================================================================

See https://github.com/KNMI/adaguc-datasets/blob/master/adaguc.tiled.xml
for example configuration

Mandatory settings:

-   tilewidthpx="720
-   tileheightpx="720"
-   tilecellsizex="0.0125"
-   tilecellsizey="0.0125"
-   tileprojection="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
-   minlevel="1"
-   maxlevel="4"
-   debug="false"
-   optimizeextent="true" <-- Increases performance of tilecreation
    with --createtiles command. It will detect which region from the
    base grid is needed for creating a tile, the result is that only a
    fraction of the big base netcdf file is read into memory.
    Optimizeextent will also detect if the region has any data in it, if
    not a tile will not be created.
-   maxtilesinimage="24" <!-- limit on how many tiles are allowed to
    appear in a single getmap request or are used during a
    getfeatureinfo request.
-   tilepath="/data/adaguc-autowms/tiles"

Optional settings:

-   left="50" right="180" bottom="~~90" top="90" <-~~ Note these are
    optional and when left out they will be calculated automatically.
-   tilemode="avg_rgba" or "rgba" <!-- only useful when using rgba
    netcdf's, dont use if you have standard byte/short/int/float/double
    data in your variable
-   threads="1" <!-- Optional, default is "1". Experimental setting
    to render a GetMap request with multiple threads

```

1.  To recreate all tiles, do:
    rm -rf /data/adaguc-autowm/tiles/\*
    ./adagucserver --updatedb --config /config/tilesconfig.xml
    --recreate
    ./adagucserver --createtiles --config /config/tilesconfig.xml
    ./adagucserver --updatedb --config /config/tilesconfig.xml
    ```

Some datasets in CLIPC are in such a high resolution that they cannot be
stored in a single file. For example the flooding indicator from
CLIPC/PIK is at 25 meter resolution along the coast in Europe. The data
is organized in 6000 different NetCDF files, each covering its own area
in 600x600 grid cells, which need to be “stitched” together. This can be
compared with composing a large panoramic picture from multiple pictures
taken along the horizon. The challenge here is to provide interactive
visualization at several cartographic projections and zoom levels.

To achieve this, ADAGUC has been extended to allow building image
pyramids at several zoom levels on coarser resolutions (see Figure ).
For CLIPC 7 different pyramid levels are used for the flooding
indicator, the top level consists of three tiles, the bottom level of
6000 tiles. When zooming to a specific distortion of the map, the level
with the best matching resolution is used to render the requested image.
That means that ADAGUC projects and combines several tiles into a single
image suitable for WMS. This type of tiled datasets in ADAGUC behave
like any other WMS, e.g. interactive zooming and panning at several
cartographic map projections remain possible.For CLIPC the flooding
indicator in the cartographic projection lambert equal area is remapped
on request to the Mercator projection used in the CLIPC portal.

![](Adaguc-server-TileSettings-Pyramid.png)

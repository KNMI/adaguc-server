Run ADAGUC Server from python
=============================

This run ADAGUC without a webserver, useful as a standalone
visualization tool.

**Get adaguc, compile it and run the python script**

1.  cd \~ \#This is important, it is also configured in
    data/python/runadagucserver.py
2.  git clone https://github.com/KNMI/adaguc-server
3.  cd adagucserver
4.  export ADAGUCCOMPONENTS="-DADAGUC\_USE\_SQLITE -DENABLE\_CURL" \# No
    GDAL or PostgreSQL: WMS Only
5.  bash compile.sh
6.  python data/python/runadagucserver.py \# Should show an image

**Now your data:**

1.  wget
    "http://opendap.knmi.nl/knmi/thredds/fileServer/ADAGUC/testsets/rgba\_truecolor\_images/mc-eur-954786\_150\_3\_cropped.nc"
    -O \~/adagucserver/data/datasets/mc-eur-954786\_150\_3\_cropped.nc
2.  adjust data/python/runadagucserver.py, line 7, variable 'url' with:
3.  url="source=mc-eur-954786\_150\_3\_cropped.nc&amp;SERVICE=WMS&amp;SERVICE=WMS&amp;VERSION=1.3.0&amp;REQUEST=GetMap&amp;LAYERS=worldmap&amp;WIDTH=1000&amp;HEIGHT=500&amp;CRS=EPSG%3A4326&amp;BBOX=-90,-180,90,180&amp;STYLES=testdata%2Fnearest&amp;FORMAT=image/png&amp;TRANSPARENT=FALSE&amp;"
4.  python data/python/runadagucserver.py \# Should show your data!!!


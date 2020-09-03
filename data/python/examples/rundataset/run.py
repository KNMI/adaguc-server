from adaguc.runAdaguc import runAdaguc

adagucInstance = runAdaguc()

import os
adagucServerHome = os.getenv('ADAGUC_PATH', os.getcwd() + "/../../../../")
adagucInstance.setAdagucPath(adagucServerHome)
adagucInstance.setConfiguration(adagucServerHome + "/data/python/adaguc/adaguc-server-config-python.xml")
adagucInstance.setDatasetDir(adagucServerHome + "/data/python/examples/rundataset/config")
adagucInstance.setDataDir(adagucServerHome + "/data/python/examples/rundataset/data")
adagucInstance.setTmpDir(adagucServerHome + "/data/python/examples/rundataset/")

# Scan the dataset:
logfile = adagucInstance.scanDataset("exampledataset")
print(logfile)

# Do the GetMap request
url = "dataset=exampledataset&&service=WMS&request=getmap&format=image/png32&layers=interpolatedObs,filledcountries,countryborders,provinces&width=461&CRS=EPSG%3A28992&BBOX=8687.5,286500,291312.5,663500&STYLES=monthlytemperaturenormals%2Fnearestcontour,filledcountries,countryborders&EXCEPTIONS=INIMAGE&showlegend=true&time=2019-07-01T00:00:00Z&title=Langjarig%20gemiddelde%201991-2020&subtitle=Gemiddelde%20maandtemperatuur%20juli&SHOWNORTHARROW=true&showscalebar=true"

#url = "dataset=exampledataset&&service=WMS&request=getmap&format=image/png32&layers=interpolatedObs&width=461&CRS=EPSG%3A28992&BBOX=8687.5,286500,291312.5,663500&STYLES=monthlytemperaturenormals%2Fnearestcontour,filledcountries,countryborders&EXCEPTIONS=INIMAGE&showlegend=true&time=2019-01-01T00:00:00Z&title=Langjarig%20gemiddelde%201991-2020&subtitle=Gemiddelde%20maandtemperatuur%20juli&SHOWNORTHARROW=true&showscalebar=true"

img, logfile = adagucInstance.runGetMapUrl(url)

print(logfile)

if img is not None:
  img.show()

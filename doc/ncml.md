The following NCML file, which can be stored in the datasets folder with the path /data/adaguc-datasets/myncml.ncml:
```
<?xml version="1.0" encoding="UTF-8"?>
<netcdf xmlns="http://www.unidata.ucar.edu/namespaces/netcdf/ncml-2.2">
  <attribute name="test" type="String" value="Example Data" />
</netcdf>
```
Can be used in the Layers FilePath element:

`<FilePath ncml="/data/adaguc-datasets/myncml.ncml" filter=".*\.nc$">/data/adaguc-autowms/testsets/`

The responding NetCDF metadata looks like:
(../adagucserver?dataset=RADNL_OPER_R___25PCPRR_L3&&service=wms&request=getmetadata&format=text/plain&layer=RADNL_OPER_R___25PCPRR_L3_KNMI)
        
```
        custom:radar1.radar_operational = 1 ;
                custom:radar2.radar_name = "DenHelder" ;
                custom:radar2.radar_location = 4.789970f ;
                custom:radar2.radar_operational = 1 ;

// global attributes:
                :title = "RADNL_OPER_R___25PCPRR_L3" ;
                :institution = "Royal Netherlands Meteorological Institute (KNMI)" ;
                :source = "Royal Netherlands Meteorological Institute (KNMI)" ;
                :history = "File created from KNMI RAD_NL25_PCP_CM files. A conversion from dbZ to mm/hour is applied with the formula R = 10^((PixelValue -109)/32)" ;
                :references = "http://adaguc.knmi.nl" ;
                :comment = "none" ;
                :Conventions = "CF-1.4" ;
                :test = "Example Data" ;
}

```

You can see that the extra global attribute test has been added.

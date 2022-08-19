Create a layer based on RGBA netcdf data
========================================

A special case in the NetCDF files are goegraphical grids which do not
contain values of a parameter, but contains the RGBA value of the colour
that should be presented at that grid point. This kind of information is
usually found in satellite products, where the values of different bands
are combined (composited) into one RGB or RGBA value. The value of the
parameter at a grid point is the value that should be plotted on the
visualisation. You could see these fields as color photos, where each
pixel is georeferenced.
The CF-conventions currently have no way to describe these parameters,
so these files are generated as fields of 32 bit unsigned integers, each
containing an RGBA value. The configuration of the layer tells the
service that the parameter values should be interpeted as pixel color
values. The service can then reproject the image from the source
projection, if needed, by doing a nearest neighbor "interpolation".

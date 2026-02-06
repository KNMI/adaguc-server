Dir (basedir,prefix)
====================

Back to [Configuration](./Configuration.md)

Directory to read netcdf files from to create WMS services from netcdf
files on the fly.

-   basedir - The server cannot read outside this path, but can only
    descend. The server is locked into this path. Realpath is used for
    checks, symbolic links are not followed.
-   prefix - The part that will be removed from the path for external
    presentation.

Multiple Dir elements are allowed.

For example:
```xml
<AutoResource enableautoopendap="false" enablelocalfile="false" >
  <Dir basedir="/data/sdpkdc/" prefix="/data/sdpkdc/"/>
  <Dir basedir="/data/omi/" prefix="/data/omi/"/>
</AutoResource>
```

-   See [AutoResource](AutoResource.md)


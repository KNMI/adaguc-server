RenderSettings (settings, striding, renderer)
=============================================

Back to [Configuration](./Configuration.md)

-   settings : auto / precise / fast

Controls behaviour of nearestneighbour rendering
(https://github.com/KNMI/adaguc-server/blob/master/adagucserverEC/CImgWarpNearestNeighbour.h).

- auto: Selects automatically between precise and fast based on grid size. Grids larger than 700x700 pixels will be done with fast (smaller grids with precise).
- precise: Gridded files up to 1000x1000 pixels can be done with the precise renderer. This uses the GenericDataWarper
- fast: Grid larger than 1000x1000 should be rendered with the option fast.  This uses the AreaMapper
- For files over 4000x4000 pixels tiling is recommended.

```xml
<RenderSettings settings="precise"/>
```

-   striding

Controls how many grid cells are skipped. E.g. if set to 2, every other
(even) grid cell is used for reading.

```xml
<RenderSettings striding="1"/>
```

-   renderer

At the moment GD can be forced

```xml
<RenderSettings renderer="gd" />
```

# Antialiasing differences between environments

`CDrawImage` renders through [Cairo](https://www.cairographics.org/) - circles, polygons, lines and text all go through Cairo's own path-filling and antialiasing (`cairo_arc`, `cairo_fill`, `cairo_stroke`, and Cairo's FreeType-backed text rendering). Cairo delegates the actual pixel-coverage computation for antialiasing to [pixman](http://pixman.org/). Neither library's output is part of any stability guarantee across versions: pixman's rasterizer has had coverage-computation changes between minor releases, and different Linux distributions (and even different point releases of the same distribution) ship different pixman/cairo versions.

In practice this means: **the same adaguc-server source code, compiled on two machines with different cairo/pixman versions, can produce a handful of pixels that differ by a few color units at shape edges** - even though the drawing calls, coordinates and colors are bit-for-bit identical. Flat, unambiguous fill regions are unaffected; only pixels where coverage is fractional (an antialiased edge, a clipped shape boundary, two semi-transparent shapes overlapping right at their edge) are sensitive to this.

## Why this matters now

Before the Cairo integration, `CDrawImage`'s shape-drawing primitives were first-party, dependency-free code - deterministic across any machine that compiles it, regardless of installed libraries. That's no longer true for the parts that now go through Cairo. This is an accepted tradeoff of using Cairo (correctness, maintainability, feature set), not a regression to chase down line-by-line.

## What it looks like in a test failure

A real, concrete example: a WMS GetMap test rendering several semi-transparent, partially canvas-clipped marker shapes failed in a Docker build but passed identically, byte-for-byte matching the golden image, on a local build of the exact same commit.

```
my local machine:   cairo 1.18.0, pixman 0.42.2
docker image:        cairo 1.18.4, pixman 0.44.0
```

The diff was 73 pixels out of ~1,000,000 (0.007%), concentrated at:

- the top tangent of each circle (where the rasterizer's fractional coverage is most sensitive to sub-pixel position),
- the one marker additionally clipped by the canvas edge (clipping combined with antialiasing compounds the sensitivity).

Every other pixel - including the interior of every filled shape, and every shape not touching an edge or another shape - matched exactly.

### How to tell this apart from a real rendering bug

Before assuming a color-difference test failure is this kind of environment noise, check:

1. **Does the diff reproduce on a clean local build of the same commit?** If a local (non-Docker) build matches the golden image exactly, and only a Docker (or otherwise differently-toolchained) build differs, that's a strong signal it's library-version noise, not a logic bug.
2. **Is the difference small and localized?** A handful of pixels, each off by single-digit-to-low-double-digit color units, clustered at shape edges/overlaps/clipping boundaries, fits this pattern. Large shifts, shifted positions, wrong colors in the interior of a shape, or differences affecting a large fraction of the image point to an actual bug instead.
3. **Does `git diff` against `master` show the relevant drawing code as unchanged?** If the exact primitive producing the differing pixels is untouched, the cause is external (library version), not something to fix in that code.

## Handling it in tests

`AdagucTestTools.compareImage()` takes `maxAllowedColorDifference` (max per-channel difference allowed in any single pixel) and `maxAllowedColorPercentage` (max fraction of pixels allowed to differ at all). For a test whose layer composes several semi-transparent and/or clipped shapes, a stricter default tolerance can be too tight for this kind of noise. See `test_WMSGetMap_Discs_example_windbarbs_from_pointdata_csv` in `tests/AdagucTests/TestPointsAndVectors.py` for an example of raising `maxAllowedColorDifference` with a comment explaining why.

Don't raise the tolerance reflexively to make a failure go away - confirm it against the checklist above first. A tolerance that's too loose can hide a genuine rendering regression.

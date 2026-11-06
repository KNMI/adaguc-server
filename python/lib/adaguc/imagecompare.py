"""Image comparison for the functional tests.

Text, lines and shape edges are drawn by cairo, pixman and FreeType. Different versions of these
libraries compute antialiasing coverage slightly differently, so the same code can produce edge
pixels with different colours on different machines. See doc/developing/antialiasing-differences.md.

Each comparison uses one of three tolerances, from strict to loose:

- EXACT: all pixels must be identical (after converting both images to RGBA). Size and image mode
  must match as well. This is the default.
- GEOMETRY: each pixel is compared with the same pixel in the other image. A pixel is flagged if it
  differs by more than MARGIN. Detects shifts of one pixel, but fails on most images with text or
  antialiased lines when the libraries differ.
- ANTIALIASING: a pixel is only flagged if its colour, in either image, lies outside the range of
  colours in the 3x3 neighbourhood of the same pixel in the other image, plus MARGIN. At an edge the
  neighbourhood holds the colours of both sides, so a different coverage value is accepted.
  Limitation: an edge that moves by one pixel or less looks the same as an antialiasing difference,
  so it is not detected. Shifts of two pixels and more are.

For GEOMETRY and ANTIALIASING two more rules apply:

- Flat areas: if the 3x3 neighbourhood is (nearly) flat in either image, a pixel is flagged when it
  differs by more than FLAT_MARGIN. Antialiasing only changes pixels near edges, so this catches
  small colour changes (a slightly different palette, a seam that is blended twice).
- Cluster filter: a flagged pixel only counts if at least MIN_FLAGGED_NEIGHBOURS of its 8 neighbours
  are flagged as well, and the images match if no more than MAX_FLAGGED_PIXELS pixels remain.
  Antialiasing differences are scattered, real rendering changes are not.

Animated images (more than one frame) are not supported: adagucserver does not produce them, and
comparing only the first frame would silently ignore the rest.
"""

import os
from dataclasses import dataclass, field

import numpy as np
from PIL import Image

EXACT = "exact"
GEOMETRY = "geometry"
ANTIALIASING = "antialiasing"
TOLERANCES = (EXACT, GEOMETRY, ANTIALIASING)

MARGIN = 16
# In a 3x3 neighbourhood with a colour range of at most FLAT_RANGE (per channel), there is no edge,
# so the pixel must match within FLAT_MARGIN.
FLAT_RANGE = 4
FLAT_MARGIN = 2
MIN_FLAGGED_NEIGHBOURS = 2
MAX_FLAGGED_PIXELS = 8


@dataclass
class ImageComparison:
    """Result of comparing two images."""

    match: bool
    messages: list = field(default_factory=list)
    flagged: np.ndarray = None  # bool mask of the pixels that made the comparison fail
    differing: np.ndarray = None  # bool mask of all pixels that differ at all
    expected: np.ndarray = None
    actual: np.ndarray = None


def load_image(path):
    """Returns an image file as an int16 RGBA array (height x width x 4)."""
    with Image.open(path) as im:
        return np.asarray(im.convert("RGBA"), dtype=np.int16)


def _neighbourhood_range(img):
    """Per-pixel minimum and maximum of each channel over the 3x3 neighbourhood (edge-padded)."""
    h, w = img.shape[:2]
    pad = np.pad(img, ((1, 1), (1, 1), (0, 0)), mode="edge")
    windows = np.stack([pad[dy : dy + h, dx : dx + w] for dy in range(3) for dx in range(3)])
    return windows.min(axis=0), windows.max(axis=0)


def _outside(img, lo, hi, margin):
    return ((img < lo - margin) | (img > hi + margin)).any(axis=2)


def _clustered(mask):
    """Keeps only the flagged pixels that have at least MIN_FLAGGED_NEIGHBOURS flagged 8-neighbours."""
    h, w = mask.shape
    pad = np.pad(mask, 1).astype(np.int8)
    neighbours = sum(pad[1 + dy : 1 + dy + h, 1 + dx : 1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if (dy, dx) != (0, 0))
    return mask & (neighbours >= MIN_FLAGGED_NEIGHBOURS)


def compare_arrays(expected, actual, tolerance=EXACT):
    """Compares two RGBA int16 arrays of the same shape. Returns an ImageComparison."""
    if tolerance not in TOLERANCES:
        raise ValueError(f"Unknown tolerance {tolerance!r}, use one of {TOLERANCES}")
    result = ImageComparison(match=True, expected=expected, actual=actual)
    absdiff = np.abs(expected - actual).max(axis=2)
    result.differing = absdiff > 0
    n_differing = int(result.differing.sum())
    if n_differing == 0:
        result.flagged = result.differing
        return result
    summary = (
        f"{n_differing} of {absdiff.size} pixels differ ({n_differing * 100.0 / absdiff.size:.4f} %), "
        f"max channel difference {int(absdiff.max())}."
    )

    if tolerance == EXACT:
        result.flagged = result.differing
        result.messages.append(f"{summary} Tolerance {EXACT}: no differences allowed.")
        result.match = False
    else:
        elo, ehi = _neighbourhood_range(expected)
        alo, ahi = _neighbourhood_range(actual)
        if tolerance == GEOMETRY:
            candidates = absdiff > MARGIN
        else:
            candidates = _outside(actual, elo, ehi, MARGIN) | _outside(expected, alo, ahi, MARGIN)
        flat = ((ehi - elo).max(axis=2) <= FLAT_RANGE) | ((ahi - alo).max(axis=2) <= FLAT_RANGE)
        candidates |= flat & (absdiff > FLAT_MARGIN)
        result.flagged = _clustered(candidates)
        n_flagged = int(result.flagged.sum())
        result.messages.append(
            f"{summary} Tolerance {tolerance}, margin {MARGIN} ({FLAT_MARGIN} in flat areas): "
            f"{int(candidates.sum())} pixels outside the margin, {n_flagged} of them clustered (allowed: {MAX_FLAGGED_PIXELS})."
        )
        result.match = n_flagged <= MAX_FLAGGED_PIXELS
    if not result.match:
        ys, xs = np.nonzero(result.flagged)
        result.messages.append(f"Failing pixels in the area x={xs.min()}..{xs.max()}, y={ys.min()}..{ys.max()}.")
    return result


def write_diff_image(comparison, path):
    """Writes an image that shows where the images differ: the actual image faded to grey, pixels that differ
    but are accepted in yellow, and the pixels that made the comparison fail in red."""
    actual = comparison.actual
    alpha = actual[..., 3:4] / 255.0
    grey = (actual[..., :3] * [0.299, 0.587, 0.114]).sum(axis=2, keepdims=True) * alpha + 255 * (1 - alpha)
    out = np.repeat(192 + grey / 4, 3, axis=2)
    out[comparison.differing] = [255, 200, 0]
    out[comparison.flagged] = [255, 0, 0]
    Image.fromarray(out.astype(np.uint8), "RGB").save(path)


def diff_image_path(actual_path):
    return os.path.splitext(actual_path)[0] + ".diff.png"


def compare_image_files(expected_path, actual_path, tolerance=EXACT):
    """Compares two image files. On failure, writes a diff image next to the actual image (see diff_image_path).
    Returns an ImageComparison."""
    if tolerance not in TOLERANCES:
        raise ValueError(f"Unknown tolerance {tolerance!r}, use one of {TOLERANCES}")
    diff_path = diff_image_path(actual_path)
    if os.path.exists(diff_path):
        os.remove(diff_path)

    with Image.open(expected_path) as e, Image.open(actual_path) as a:
        for path, im in ((expected_path, e), (actual_path, a)):
            if getattr(im, "n_frames", 1) > 1:
                return ImageComparison(False, [f"Animated images are not supported: {path} has {im.n_frames} frames"])
        if e.size != a.size:
            return ImageComparison(False, [f"Size of expected and actual image do not match: {e.size} vs {a.size}"])
        if e.mode != a.mode:
            return ImageComparison(False, [f"Mode of expected and actual image do not match: {e.mode} vs {a.mode}"])

    result = compare_arrays(load_image(expected_path), load_image(actual_path), tolerance)
    if not result.match:
        write_diff_image(result, diff_path)
        result.messages.append(f"Diff image written to {diff_path} (red: failing pixels, yellow: accepted differences).")
    return result

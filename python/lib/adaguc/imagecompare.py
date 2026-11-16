"""Image comparison for the functional tests.

Text, lines and shape edges are drawn by cairo, pixman and FreeType. Different versions of these
libraries compute antialiasing coverage slightly differently, so the same code can produce edge
pixels with different colours on different machines. See doc/developing/antialiasing-differences.md.

All tolerances work the same way: rules flag pixels, and each rule allows a number of flagged
pixels. The images match if no rule flags more pixels than it allows.

- EXACT (the default): one rule, every pixel that differs is flagged, none allowed.
- GEOMETRY and ANTIALIASING: three rules.

  Rule       A pixel is flagged when                                              Allowed
  ---------  -------------------------------------------------------------------  ------------------
  flat area  its 3x3 neighbourhood is flat (colour range <= FLAT_RANGE) in either  0
             image, and it differs by more than FLAT_MARGIN
  strong     it differs by more than STRONG_MARGIN, and at least                   0
             MIN_FLAGGED_NEIGHBOURS of its 8 neighbours do too
  cluster    it is off by more than MARGIN, and at least MIN_FLAGGED_NEIGHBOURS    MAX_FLAGGED_PIXELS
             of its 8 neighbours are too

  The two tolerances only differ in what "off" means in the cluster rule. GEOMETRY compares a pixel
  with the same pixel in the other image. ANTIALIASING compares it with the colour range of the 3x3
  neighbourhood around the same pixel in the other image, in both directions: at an edge that range
  holds the colours of both sides, so a different antialiasing coverage is accepted.

Why each rule: antialiasing only changes pixels at edges (flat area), changes them by a limited
amount (strong), and the changed pixels are scattered (cluster). Real rendering changes break at
least one of these. Each comparison is symmetric: swapping expected and actual gives the same result.

Known limitations, outside flat areas: ANTIALIASING accepts a 1 pixel shift of an edge with a contrast
below STRONG_MARGIN. Both loose tolerances accept a change of a few pixels next to an edge or text by
less than STRONG_MARGIN (such as a 2x2 dot in a similar colour drawn on a line), a single pixel or a
pair of pixels that changes by any amount, and a raster whose colours are all off by up to MARGIN.
See doc/developing/antialiasing-differences.md, also for the detection rates of changes in text.

Before the pixels are compared, the size, the image mode and the encoding structure must match
(see encoding_structure). Animated images are not supported: adagucserver does not produce them, and
comparing only the first frame would silently ignore the rest. The compression is not compared;
compare_file_sizes checks it in a few tests per format.
"""

import filecmp
import os
import struct
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
# Between cairo/pixman/FreeType builds (Mac vs Linux), text pixels differed by at most 86 and other pixels by at most 142,
# but never two neighbouring pixels both by more than 80. A changed digit in a 12 px label does produce such pixels,
# also on a darker background where the text differs by only about 125 from it.
STRONG_MARGIN = 80
MIN_FLAGGED_NEIGHBOURS = 2
MAX_FLAGGED_PIXELS = 8
# Room for other zlib/libwebp versions; switching the compression off makes files several times larger.
MAX_FILE_SIZE_RATIO = 1.5
# A palette image that uses this many entries was quantized from more colours. adagucserver's octree quantizer averages
# the colours per entry, so a few pixels that differ can change the palette, and with it the colours all over the image.
QUANTIZED_PALETTE_ENTRIES = 250


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
    lo, hi = pad[1 : 1 + h, 1 : 1 + w].copy(), pad[1 : 1 + h, 1 : 1 + w].copy()
    for dy in range(3):
        for dx in range(3):
            window = pad[dy : dy + h, dx : dx + w]
            np.minimum(lo, window, out=lo)
            np.maximum(hi, window, out=hi)
    return lo, hi


def _outside(img, lo, hi, margin):
    return ((img < lo - margin) | (img > hi + margin)).any(axis=2)


def _clustered(mask):
    """Keeps only the flagged pixels that have at least MIN_FLAGGED_NEIGHBOURS flagged 8-neighbours."""
    h, w = mask.shape
    pad = np.pad(mask, 1).astype(np.int8)
    neighbours = sum(pad[1 + dy : 1 + dy + h, 1 + dx : 1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if (dy, dx) != (0, 0))
    return mask & (neighbours >= MIN_FLAGGED_NEIGHBOURS)


def _rules(expected, actual, absdiff, tolerance):
    """The rules of a tolerance, as a list of (name, mask of the pixels the rule flags, number of pixels allowed)."""
    if tolerance == EXACT:
        return [("differing", absdiff > 0, 0)]
    elo, ehi = _neighbourhood_range(expected)
    alo, ahi = _neighbourhood_range(actual)
    flat = ((ehi - elo).max(axis=2) <= FLAT_RANGE) | ((ahi - alo).max(axis=2) <= FLAT_RANGE)
    if tolerance == GEOMETRY:
        off = absdiff > MARGIN
    else:
        off = _outside(actual, elo, ehi, MARGIN) | _outside(expected, alo, ahi, MARGIN)
    return [
        ("flat area", flat & (absdiff > FLAT_MARGIN), 0),
        ("strong", _clustered(absdiff > STRONG_MARGIN), 0),
        ("cluster", _clustered(off), MAX_FLAGGED_PIXELS),
    ]


def _check_tolerance(tolerance):
    if tolerance not in TOLERANCES:
        raise ValueError(f"Unknown tolerance {tolerance!r}, use one of {TOLERANCES}")


def compare_arrays(expected, actual, tolerance=EXACT):
    """Compares two RGBA int16 arrays of the same shape. Returns an ImageComparison."""
    _check_tolerance(tolerance)
    result = ImageComparison(match=True, expected=expected, actual=actual)
    absdiff = np.abs(expected - actual).max(axis=2)
    result.differing = absdiff > 0
    result.flagged = np.zeros_like(result.differing)
    n_differing = int(result.differing.sum())
    if n_differing == 0:
        return result
    counts = []
    for name, mask, allowed in _rules(expected, actual, absdiff, tolerance):
        n = int(mask.sum())
        counts.append(f"{name} {n} (allowed {allowed})")
        if n > allowed:
            result.match = False
            result.flagged |= mask
    result.messages.append(
        f"{n_differing} of {absdiff.size} pixels differ ({n_differing * 100.0 / absdiff.size:.4f} %), "
        f"max channel difference {int(absdiff.max())}. Tolerance {tolerance}, flagged pixels per rule: {', '.join(counts)}."
    )
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


def encoding_structure(path):
    """Describes how a PNG or WebP file is encoded, independent of the encoder library version: for PNG the bit
    depth, colour type, interlacing and chunk types (consecutive IDAT chunks counted as one, because their number
    depends on the zlib output), for WebP the chunk types. Returns None for other formats."""
    with open(path, "rb") as f:
        data = f.read()
    chunks = []
    if data[:8] == b"\x89PNG\r\n\x1a\n":
        pos, header = 8, ""
        while pos + 8 <= len(data):
            length, chunk_type = struct.unpack(">I4s", data[pos : pos + 8])
            chunk_type = chunk_type.decode("latin-1")
            if chunk_type == "IHDR":
                header = f"bit depth {data[pos + 16]}, colour type {data[pos + 17]}, interlace {data[pos + 20]}"
            if not (chunk_type == "IDAT" and chunks and chunks[-1] == "IDAT"):
                chunks.append(chunk_type)
            pos += 12 + length
        return f"PNG ({header}), chunks {' '.join(chunks)}"
    if data[:4] == b"RIFF" and data[8:12] == b"WEBP":
        pos = 12
        while pos + 8 <= len(data):
            size = struct.unpack("<I", data[pos + 4 : pos + 8])[0]
            chunks.append(data[pos : pos + 4].decode("latin-1").strip())
            pos += 8 + size + (size & 1)
        return f"WebP, chunks {' '.join(chunks)}"
    return None


def compare_file_sizes(expected_path, actual_path, max_ratio=MAX_FILE_SIZE_RATIO):
    """Checks that the actual file is at most max_ratio times as large as the expected one, to detect a change in
    the compression. Returns (ok, message)."""
    expected_size, actual_size = os.path.getsize(expected_path), os.path.getsize(actual_path)
    ok = actual_size <= max_ratio * expected_size
    return ok, f"File size {actual_size} bytes, expected {expected_size} (allowed: up to {max_ratio} times as large)."


def compare_image_files(expected_path, actual_path, tolerance=EXACT):
    """Compares two image files. On failure, writes a diff image next to the actual image (see diff_image_path), and
    removes an old one otherwise. Returns an ImageComparison."""
    _check_tolerance(tolerance)  # also when the files are identical, so that a misspelled tolerance fails right away
    diff_path = diff_image_path(actual_path)
    if os.path.exists(diff_path):
        os.remove(diff_path)
    if filecmp.cmp(expected_path, actual_path, shallow=False):
        return ImageComparison(True)

    with Image.open(expected_path) as e, Image.open(actual_path) as a:
        for path, im in ((expected_path, e), (actual_path, a)):
            if getattr(im, "n_frames", 1) > 1:
                return ImageComparison(False, [f"Animated images are not supported: {path} has {im.n_frames} frames"])
        if e.size != a.size:
            return ImageComparison(False, [f"Size of expected and actual image do not match: {e.size} vs {a.size}"])
        if e.mode != a.mode:
            return ImageComparison(False, [f"Mode of expected and actual image do not match: {e.mode} vs {a.mode}"])
    expected_encoding, actual_encoding = encoding_structure(expected_path), encoding_structure(actual_path)
    if expected_encoding != actual_encoding:
        return ImageComparison(False, [f"Encoding of expected and actual image do not match: {expected_encoding} vs {actual_encoding}"])

    result = compare_arrays(load_image(expected_path), load_image(actual_path), tolerance)
    if not result.match:
        write_diff_image(result, diff_path)
        result.messages.append(f"Diff image written to {diff_path} (red: failing pixels, yellow: accepted differences).")
        if any(_is_quantized_palette(path) for path in (expected_path, actual_path)):
            result.messages.append(
                "Hint: this palette image was quantized from more than 256 colours. Small rendering differences can change the "
                "palette, and with it the colours all over the image. For a test of rendering, request FORMAT=image/png32."
            )
    return result


def _is_quantized_palette(path):
    with Image.open(path) as im:
        return im.mode == "P" and len(np.unique(np.asarray(im))) >= QUANTIZED_PALETTE_ENTRIES

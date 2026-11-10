"""Tests for the image comparison used by the functional tests (adaguc.imagecompare).

The images are synthetic: antialiased discs, drawn by supersampling. Computing the coverage with a
different number of samples per pixel mimics a different version of the rasterizer (pixman).
"""

import itertools
import os

import numpy as np
import pytest
from PIL import Image, ImageDraw, ImageFont, PngImagePlugin

from adaguc import imagecompare
from adaguc.AdagucTestTools import AdagucTestTools

BACKGROUND = (160, 240, 128)
DISCS = [
    # x, y, radius, colour. The third disc is clipped by the canvas edge, the first two overlap.
    (60, 60, 40, (255, 0, 0)),
    (95, 80, 30, (0, 0, 255)),
    (195, 120, 30, (255, 255, 0)),
    (120, 160, 25, (0, 128, 255)),
]


def render(samples=16, shift=0.0, discs=DISCS, size=(200, 200)):
    """Semi-transparent antialiased discs, coverage computed with samples x samples points per pixel."""
    w, h = size
    img = np.tile(np.array(BACKGROUND, float), (h, w, 1))
    offsets = (np.arange(samples) + 0.5) / samples
    ys = (np.arange(h)[:, None] + offsets[None, :]).reshape(-1)
    xs = (np.arange(w)[:, None] + offsets[None, :]).reshape(-1)
    for cx, cy, r, colour in discs:
        inside = (xs[None, :] - cx - shift) ** 2 + (ys[:, None] - cy) ** 2 <= r * r
        coverage = inside.reshape(h, samples, w, samples).mean(axis=(1, 3))[..., None]
        alpha = 0.6 * coverage
        img = img * (1 - alpha) + np.array(colour, float) * alpha
    return np.round(img).astype(np.uint8)


def save(path, pixels, mode="RGB"):
    im = Image.fromarray(pixels, "RGB")
    if mode == "P":
        im = im.quantize(colors=256)
    im.save(path)
    return str(path)


def compare(tmp_path, expected, actual, **kwargs):
    """Saves both pixel arrays as PNG files and compares them with compareImage, like the functional tests do."""
    expected_path = save(tmp_path / "expected.png", expected)
    actual_path = save(tmp_path / "actual.png", actual)
    return AdagucTestTools().compareImage(expected_path, actual_path, **kwargs)


# The expected image for most tests: the discs as defined above. The tests compare variants of it against it.
REFERENCE = render()


ALL_TOLERANCES = pytest.mark.parametrize("tolerance", ["exact", "geometry", "antialiasing"])
LOOSE_TOLERANCES = pytest.mark.parametrize("tolerance", ["geometry", "antialiasing"])


@ALL_TOLERANCES
def test_identical(tmp_path, tolerance):
    assert compare(tmp_path, REFERENCE, render(), tolerance=tolerance)
    assert not os.path.exists(imagecompare.diff_image_path(str(tmp_path / "actual.png")))


def test_exact_is_the_default(tmp_path):
    actual = render()
    actual[100, 100] = actual[100, 100] + 1
    assert not compare(tmp_path, REFERENCE, actual)
    assert compare(tmp_path, REFERENCE, actual, tolerance="geometry")


def test_unknown_tolerance(tmp_path):
    with pytest.raises(ValueError):
        compare(tmp_path, REFERENCE, render(shift=1), tolerance="loose")


def test_other_rasterizer(tmp_path):
    """Differences in antialiasing coverage only: accepted by the antialiasing tolerance only."""
    for samples in (3, 4, 5):
        assert compare(tmp_path, REFERENCE, render(samples), tolerance="antialiasing")
        assert not compare(tmp_path, REFERENCE, render(samples))


def two_halves():
    """Green left half, blue right half: a straight vertical edge between x=99 and x=100."""
    img = np.tile(np.array(BACKGROUND, np.uint8), (200, 200, 1))
    img[:, 100:] = (60, 90, 200)
    return img


def moderate_change(img, rows, cols):
    """Changes pixels by 70 (green side of the edge) or 55 (blue side): outside the colour range of the neighbourhood
    plus MARGIN, but within STRONG_MARGIN, so only the cluster filter applies."""
    changed = np.where(np.arange(img.shape[1])[None, :, None] < 100, (230, 240, 128), (60, 90, 255))
    img[rows, cols] = np.broadcast_to(changed, img.shape)[rows, cols]


@LOOSE_TOLERANCES
@pytest.mark.parametrize(
    "rows, cols, passes",
    [
        (slice(50, 60), 100, True),  # 1 px line of 10 px along the edge: the end pixels have 1 flagged neighbour, 8 count
        (slice(50, 61), 100, False),  # 11 px: 9 count, more than MAX_FLAGGED_PIXELS
        (slice(50, 52), slice(99, 101), True),  # 2x2 block on the edge: 4 count
        (slice(50, 53), slice(98, 101), False),  # 3x3 block on the edge: its outer column lies in a flat area
    ],
    ids=["line_10px", "line_11px", "block_2x2", "block_3x3"],
)
def test_limit_near_an_edge(tmp_path, tolerance, rows, cols, passes):
    """Near an edge, only the cluster filter applies to a moderate colour change. This pins where its limit is."""
    actual = two_halves()
    moderate_change(actual, rows, cols)
    assert compare(tmp_path, two_halves(), actual, tolerance=tolerance) == passes


@LOOSE_TOLERANCES
def test_strong_change_near_an_edge_fails(tmp_path, tolerance):
    """A 2x2 block on the edge that differs strongly fails, while the moderate one in test_limit_near_an_edge passes."""
    actual = two_halves()
    actual[50:52, 99:101] = (0, 0, 0)
    assert not compare(tmp_path, two_halves(), actual, tolerance=tolerance)


FONT = os.path.join(os.path.dirname(__file__), "..", "..", "data", "fonts", "FreeSans.ttf")
# Background without flat areas (every 3x3 neighbourhood has a colour range above FLAT_RANGE), like text on top of
# a map. Then only the strong-difference rule can catch a changed digit.
TEXTURED = np.random.default_rng(0).integers(100, 200, (40, 80, 3)).astype(np.uint8)
# A smooth, darker colour gradient, like shading on a map: no flat areas either, and the black text differs by only
# about 125 from it. Then a changed digit gives fewer and smaller strong differences.
_y, _x = np.mgrid[0:40, 0:80]
GRADIENT = np.clip(np.array([40, 90, 220]) + (_x + _y)[..., None] * np.array([1.5, 1.2, -1.5]), 0, 255).astype(np.uint8)


def render_label(text, background=None, samples=8, size=12, offset=(0, 0)):
    """A black label of size px, like a legend tick label or a station plot value. The text is rasterized once at 8
    times the resolution, and the coverage of each pixel is computed with samples x samples of those points. A
    different number of samples mimics a different rasterizer with exactly the same glyph shapes, whatever FreeType
    version Pillow uses."""
    scale = 8
    mask = Image.new("L", (80 * scale, 40 * scale))
    position = (20 * scale + offset[0], 8 * scale + offset[1])  # offset in 1/8 px
    ImageDraw.Draw(mask).text(position, text, font=ImageFont.truetype(FONT, size * scale), fill=255)
    step = scale // samples
    points = (np.asarray(mask, float) / 255).reshape(40, scale, 80, scale)[:, ::step, :, ::step]
    coverage = points.mean(axis=(1, 3))[..., None]
    img = np.tile(np.array(BACKGROUND, float), (40, 80, 1)) if background is None else background.astype(float)
    return np.round(img * (1 - coverage)).astype(np.uint8)


@LOOSE_TOLERANCES
@pytest.mark.parametrize("background", [None, TEXTURED, GRADIENT], ids=["plain", "textured", "gradient"])
@pytest.mark.parametrize("other", ["12.6", "12.8", "12.3", "12.0", "17.5"])
def test_label_with_other_digit_fails(tmp_path, tolerance, background, other):
    assert not compare(tmp_path, render_label("12.5", background), render_label(other, background), tolerance=tolerance)


_LABELS = {}


def detection_rate(pairs, background, size):
    """The fraction of label changes that the antialiasing tolerance detects, over 4 sub-pixel positions of the text.
    Whether a small change is detected also depends on where the glyphs fall on the pixel grid."""

    def label(text, offset):
        key = (text, id(background), size, offset)
        if key not in _LABELS:
            _LABELS[key] = np.dstack([render_label(text, background, size=size, offset=offset), np.full((40, 80), 255, np.uint8)]).astype(
                np.int16
            )
        return _LABELS[key]

    offsets = [(0, 0), (4, 0), (0, 4), (4, 4)]
    detected = [
        not imagecompare.compare_arrays(label(a, o), label(b, o), imagecompare.ANTIALIASING).match for a, b in pairs for o in offsets
    ]
    return sum(detected) / len(detected)


# adagucserver draws text at fontsize * 100 / 72 px: the common fontsize 8 is about 11 px, fontsize 10 about 14 px.
# The minimum rates are a little below the measured ones. Measured over 16 positions, the misses are pairs that differ by
# one small stroke, such as 6 <-> 8 and 8 <-> 9 at 11 px, f -> t, r -> t and "Temp" -> "Tamp".
DIGIT_CHANGES = [(f"x{a}x", f"x{b}x") for a, b in itertools.permutations("0123456789", 2)]
LETTER_CHANGES = [(f"x{a}x", f"x{b}x") for a, b in itertools.permutations("aceosnurmhbdpgtkfvwxyz", 2)][::5]
WORD_CHANGES = [("hPa", "kPa"), ("°C", "°F"), ("mm", "cm"), ("m/s", "km/s"), ("max", "min"), ("NNE", "NNW"), ("Temp", "Tamp")]


@pytest.mark.parametrize("background", [None, TEXTURED, GRADIENT], ids=["plain", "textured", "gradient"])
@pytest.mark.parametrize(
    "size, min_digits, min_letters_and_words",
    [(11, 0.9, 0.85), (14, 1.0, 0.98), (18, 1.0, 1.0)],
    ids=["11px", "14px", "18px"],
)
def test_text_changes_are_detected(background, size, min_digits, min_letters_and_words):
    assert detection_rate(DIGIT_CHANGES, background, size) >= min_digits
    assert detection_rate(LETTER_CHANGES + WORD_CHANGES, background, size) >= min_letters_and_words


@pytest.mark.parametrize("background", [None, TEXTURED, GRADIENT], ids=["plain", "textured", "gradient"])
def test_label_with_other_antialiasing_passes(tmp_path, background):
    """Coverage from 4x4 instead of 8x8 points per pixel: as large as the differences between real FreeType builds.
    (2x2 points is coarser than any real rasterizer: it gives neighbouring pixels that both differ by more than 80.)"""
    expected, actual = render_label("12.5", background), render_label("12.5", background, samples=4)
    assert not np.array_equal(expected, actual)
    assert compare(tmp_path, expected, actual, tolerance="antialiasing")


@ALL_TOLERANCES
def test_shift_of_two_pixels_fails_and_writes_diff_image(tmp_path, tolerance):
    assert not compare(tmp_path, REFERENCE, render(shift=2), tolerance=tolerance)
    diff_path = imagecompare.diff_image_path(str(tmp_path / "actual.png"))
    assert os.path.exists(diff_path)
    diff = np.asarray(Image.open(diff_path))
    assert ((diff == [255, 0, 0]).all(axis=2)).sum() > 100


def test_diff_image_removed_when_passing_again(tmp_path):
    assert not compare(tmp_path, REFERENCE, render(shift=2))
    assert compare(tmp_path, REFERENCE, render())
    assert not os.path.exists(imagecompare.diff_image_path(str(tmp_path / "actual.png")))


@LOOSE_TOLERANCES
def test_shift_of_one_pixel_of_high_contrast_edges_fails(tmp_path, tolerance):
    assert not compare(tmp_path, REFERENCE, render(shift=1), tolerance=tolerance)


def test_shift_of_one_pixel_of_medium_contrast_edges(tmp_path):
    """Known limitation: an edge whose sides differ by less than STRONG_MARGIN (here about 50) that moves by one pixel
    is accepted by antialiasing, geometry detects it."""
    discs = [(x, y, r, (160, 240, 40)) for x, y, r, _ in DISCS]
    expected = render(discs=discs)
    assert compare(tmp_path, expected, render(discs=discs, shift=1), tolerance="antialiasing")
    assert not compare(tmp_path, expected, render(discs=discs, shift=1), tolerance="geometry")


@LOOSE_TOLERANCES
def test_changed_colour_fails(tmp_path, tolerance):
    discs = list(DISCS)
    discs[3] = (120, 160, 25, (0, 160, 255))
    assert not compare(tmp_path, REFERENCE, render(discs=discs), tolerance=tolerance)


@LOOSE_TOLERANCES
def test_small_colour_change_in_flat_area_fails(tmp_path, tolerance):
    discs = list(DISCS)
    discs[3] = (120, 160, 25, (0, 128, 247))
    assert not compare(tmp_path, REFERENCE, render(discs=discs), tolerance=tolerance)


@LOOSE_TOLERANCES
def test_seam_blended_twice_fails(tmp_path, tolerance):
    """A 1 pixel wide line that is a few colour units off, like a seam between triangles that is blended twice."""
    actual = render()
    actual[:, 100] = np.clip(actual[:, 100].astype(int) + 5, 0, 255)
    assert not compare(tmp_path, REFERENCE, actual, tolerance=tolerance)


@LOOSE_TOLERANCES
def test_missing_small_shape_fails(tmp_path, tolerance):
    actual = render()
    actual[20:24, 150:154] = (0, 0, 0)
    assert not compare(tmp_path, REFERENCE, actual, tolerance=tolerance)


@LOOSE_TOLERANCES
def test_missing_thin_line_fails(tmp_path, tolerance):
    expected = render()
    expected[180, 20:50] = (0, 0, 0)
    assert not compare(tmp_path, expected, render(), tolerance=tolerance)


@LOOSE_TOLERANCES
def test_few_scattered_pixels_on_edges_pass(tmp_path, tolerance):
    """Scattered single pixels on the edges of the discs, where antialiasing differences occur."""
    actual = render()
    for y, x in [(60, 20), (120, 165), (135, 120)]:
        actual[y, x] = (0, 0, 0)
    assert compare(tmp_path, REFERENCE, actual, tolerance=tolerance)
    assert not compare(tmp_path, REFERENCE, actual)


@LOOSE_TOLERANCES
def test_single_pixel_in_flat_area_fails(tmp_path, tolerance):
    actual = render()
    actual[10, 10] = (0, 0, 0)
    assert not compare(tmp_path, REFERENCE, actual, tolerance=tolerance)


@LOOSE_TOLERANCES
def test_missing_marker_of_two_by_two_pixels_fails(tmp_path, tolerance):
    expected = render()
    expected[20:22, 150:152] = (0, 0, 0)
    assert not compare(tmp_path, expected, render(), tolerance=tolerance)


@LOOSE_TOLERANCES
def test_missing_dotted_line_fails(tmp_path, tolerance):
    """Single-pixel dots, 2 pixels apart: no dot has a flagged neighbour, so the cluster filter alone would accept it."""
    expected = render()
    expected[190, 20:70:2] = (0, 0, 0)
    assert not compare(tmp_path, expected, render(), tolerance=tolerance)


@ALL_TOLERANCES
def test_size_mismatch_fails(tmp_path, tolerance):
    assert not compare(tmp_path, REFERENCE, render(size=(200, 201)), tolerance=tolerance)


@ALL_TOLERANCES
def test_mode_mismatch_fails(tmp_path, tolerance):
    expected_path = save(tmp_path / "expected.png", render())
    actual_path = save(tmp_path / "actual.png", render(), mode="P")
    assert not AdagucTestTools().compareImage(expected_path, actual_path, tolerance=tolerance)


def test_palette_images_are_compared_by_colour(tmp_path):
    """Two palette images with the same colours but a different palette order are equal, also for exact."""
    pixels = render()
    expected = Image.fromarray(pixels, "RGB").quantize(colors=64)
    palette = np.array(expected.getpalette()[: 64 * 3]).reshape(64, 3)
    order = np.arange(64)[::-1]
    actual = Image.fromarray(np.argsort(order)[np.asarray(expected)].astype(np.uint8), "P")
    actual.putpalette(palette[order].reshape(-1).tolist())
    expected.save(tmp_path / "expected.png")
    actual.save(tmp_path / "actual.png")
    assert not np.array_equal(np.asarray(expected), np.asarray(actual))
    assert AdagucTestTools().compareImage(str(tmp_path / "expected.png"), str(tmp_path / "actual.png"))


def save_gif(path, frames):
    images = [Image.fromarray(f, "RGB").quantize(colors=128) for f in frames]
    images[0].save(path, save_all=True, append_images=images[1:], duration=100, loop=0)
    return str(path)


@ALL_TOLERANCES
def test_animation_is_not_supported(tmp_path, tolerance, capsys):
    """adagucserver produces no animations; comparing only the first frame would silently ignore the others."""
    expected = save_gif(tmp_path / "expected.gif", [render(), render(shift=5)])
    other = save_gif(tmp_path / "other.gif", [render(), render(shift=10)])
    single = save_gif(tmp_path / "single.gif", [render()])
    assert not AdagucTestTools().compareImage(expected, other, tolerance=tolerance)
    assert "Animated images are not supported" in capsys.readouterr().out
    assert not AdagucTestTools().compareImage(single, other, tolerance=tolerance)
    assert "Animated images are not supported" in capsys.readouterr().out


@ALL_TOLERANCES
def test_png_with_other_chunks_fails(tmp_path, tolerance, capsys):
    """The same pixels, but an extra metadata chunk: a change in how the encoder is called."""
    im = Image.fromarray(REFERENCE)
    im.save(tmp_path / "expected.png")
    info = PngImagePlugin.PngInfo()
    info.add_text("Software", "adagucserver")
    im.save(tmp_path / "actual.png", pnginfo=info)
    assert not AdagucTestTools().compareImage(str(tmp_path / "expected.png"), str(tmp_path / "actual.png"), tolerance=tolerance)
    assert "Encoding of expected and actual image do not match" in capsys.readouterr().out


def test_png_with_other_compression_passes(tmp_path):
    """Another zlib version or compression level gives other bytes, but the same encoding structure and pixels."""
    im = Image.fromarray(REFERENCE)
    im.save(tmp_path / "expected.png", compress_level=9)
    im.save(tmp_path / "actual.png", compress_level=1)
    assert (tmp_path / "expected.png").read_bytes() != (tmp_path / "actual.png").read_bytes()
    assert AdagucTestTools().compareImage(str(tmp_path / "expected.png"), str(tmp_path / "actual.png"))


def test_webp_lossy_instead_of_lossless_fails(tmp_path, capsys):
    im = Image.fromarray(REFERENCE).convert("RGBA")
    im.save(tmp_path / "expected.webp", lossless=True)
    im.save(tmp_path / "actual.webp", lossless=False, quality=100)
    assert not AdagucTestTools().compareImage(str(tmp_path / "expected.webp"), str(tmp_path / "actual.webp"), tolerance="antialiasing")
    assert "Encoding of expected and actual image do not match" in capsys.readouterr().out


def test_file_size_detects_compression_switched_off(tmp_path):
    im = Image.fromarray(REFERENCE)
    im.save(tmp_path / "expected.png", compress_level=9)
    im.save(tmp_path / "smaller_compression.png", compress_level=6)
    im.save(tmp_path / "no_compression.png", compress_level=0)
    expected = str(tmp_path / "expected.png")
    assert AdagucTestTools().compareFileSizes(expected, str(tmp_path / "smaller_compression.png"))
    assert AdagucTestTools().compareImage(expected, str(tmp_path / "no_compression.png"))
    assert not AdagucTestTools().compareFileSizes(expected, str(tmp_path / "no_compression.png"))

"""Tests for the image comparison used by the functional tests (adaguc.imagecompare).

The images are synthetic: antialiased discs, drawn by supersampling. Computing the coverage with a
different number of samples per pixel mimics a different version of the rasterizer (pixman).
"""

import os

import numpy as np
import pytest
from PIL import Image

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


def test_shift_of_one_pixel(tmp_path):
    """Known limitation: a shift of one pixel is accepted by the antialiasing tolerance, geometry detects it."""
    assert compare(tmp_path, REFERENCE, render(shift=1), tolerance="antialiasing")
    assert not compare(tmp_path, REFERENCE, render(shift=1), tolerance="geometry")


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
def test_few_scattered_pixels_pass(tmp_path, tolerance):
    actual = render()
    for y, x in [(10, 10), (50, 170), (190, 30)]:
        actual[y, x] = (0, 0, 0)
    assert compare(tmp_path, REFERENCE, actual, tolerance=tolerance)
    assert not compare(tmp_path, REFERENCE, actual)


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

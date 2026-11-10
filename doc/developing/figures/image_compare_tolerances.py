"""Generates doc/developing/image-compare-tolerances.png: what each image-comparison tolerance does with four
kinds of differences, computed with the real comparison in python/lib/adaguc/imagecompare.py.

Regenerate it after changing imagecompare.py. Needs numpy, Pillow and matplotlib (matplotlib is not in the
requirements files, install it with `pip install matplotlib`). From the repository root:

    python doc/developing/figures/image_compare_tolerances.py

The synthetic discs are a copy of the ones in tests/AdagucTests/TestImageCompare.py.
"""

import importlib.util
import sys
from pathlib import Path

import matplotlib
import numpy as np

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

REPO = Path(__file__).resolve().parents[3]
DEFAULT_OUTPUT = REPO / "doc" / "developing" / "image-compare-tolerances.png"


def load_imagecompare():
    """Loads imagecompare.py from this repository, not from an installed adaguc package (which may be another checkout)."""
    path = REPO / "python" / "lib" / "adaguc" / "imagecompare.py"
    spec = importlib.util.spec_from_file_location("imagecompare", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


BACKGROUND = (160, 240, 128)
DISCS = [
    # x, y, radius, colour. The first two overlap; the crop below shows their edges.
    (60, 60, 40, (255, 0, 0)),
    (95, 80, 30, (0, 0, 255)),
    (195, 120, 30, (255, 255, 0)),
    (120, 160, 25, (0, 128, 255)),
]


def render(samples=16, shift=0.0, discs=DISCS, size=(200, 200)):
    """Semi-transparent antialiased discs as an RGBA int16 array, coverage computed with samples x samples points per pixel.
    A different number of samples mimics a different rasterizer."""
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
    out = np.empty((h, w, 4), np.int16)
    out[..., :3] = np.round(img)
    out[..., 3] = 255
    return out


TOLERANCES = ["exact", "geometry", "antialiasing"]
CROP = (slice(52, 92), slice(78, 118))  # 40 x 40 px around the overlapping edges of the red and blue discs
INK, MUTED = "#1f1f1f", "#6b6b6b"
FAIL_RED, ACCEPT_YELLOW = np.array([230, 0, 0]), np.array([255, 200, 0])  # the same meaning as in the diff images


def crop(img):
    return img[CROP][..., :3].astype(np.uint8)


def mask_view(actual, result):
    """The actual image faded to grey, accepted differences in yellow, failing pixels in red."""
    grey = (actual[..., :3] * [0.299, 0.587, 0.114]).sum(axis=2, keepdims=True)
    out = np.repeat(200 + grey / 5, 3, axis=2)
    out[result.differing] = ACCEPT_YELLOW
    out[result.flagged] = FAIL_RED
    return np.clip(out, 0, 255)[CROP].astype(np.uint8)


def main(output):
    ic = load_imagecompare()
    expected = render()
    recoloured = list(DISCS)
    recoloured[1] = (95, 80, 30, (0, 0, 247))
    # Discs that differ by about 50 from the background: less than STRONG_MARGIN.
    faint = [(x, y, r, (160, 240, 40)) for x, y, r, _ in DISCS]
    cases = [
        ("Antialiasing noise\n(other rasterizer)", expected, render(samples=4)),
        ("Edge moved 1 px", expected, render(shift=1)),
        ("Medium-contrast\nedge moved 1 px", render(discs=faint), render(discs=faint, shift=1)),
        ("Colour 8 units off\n(blue disc)", expected, render(discs=recoloured)),
    ]

    fig, axes = plt.subplots(len(cases), 2 + len(TOLERANCES), figsize=(11, 10.4), gridspec_kw=dict(wspace=0.08, hspace=0.42))
    headings = ["Expected", "Actual"] + [f'tolerance=\n"{t}"' for t in TOLERANCES]
    for j, heading in enumerate(headings):
        axes[0, j].set_title(heading, fontsize=11, color=INK, pad=10, family="monospace" if j > 1 else None)
    for i, (label, expected, actual) in enumerate(cases):
        axes[i, 0].imshow(crop(expected), interpolation="nearest")
        axes[i, 1].imshow(crop(actual), interpolation="nearest")
        axes[i, 0].set_ylabel(label, fontsize=11, color=INK, rotation=0, ha="right", va="center", labelpad=12)
        for j, tolerance in enumerate(TOLERANCES):
            result = ic.compare_arrays(expected, actual, tolerance)
            ax = axes[i, 2 + j]
            ax.imshow(mask_view(actual, result), interpolation="nearest")
            verdict = "✓ pass" if result.match else "✗ fail"
            ax.set_xlabel(
                f"{verdict}\n{int(result.flagged.sum())} px failing",
                fontsize=10,
                color=INK,
                labelpad=4,
                fontweight="normal" if result.match else "bold",
            )
    for ax in axes.flat:
        ax.set_xticks([])
        ax.set_yticks([])
        for spine in ax.spines.values():
            spine.set_color("#c8c8c8")
    fig.text(
        0.5,
        0.02,
        "Zoomed crop (40 × 40 px) of a 200 × 200 test image: two semi-transparent discs overlapping.\n"
        "Red: pixels that make the comparison fail.  Yellow: pixels that differ but are accepted.  "
        "Pass/fail and pixel counts are for the whole image.",
        ha="center",
        fontsize=9,
        color=MUTED,
    )
    fig.savefig(output, dpi=110, bbox_inches="tight", facecolor="white")
    print(f"Written {output}")


if __name__ == "__main__":
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUTPUT)

"""Generates doc/developing/image-compare-strong-rule.png: why the "antialiasing" tolerance needs the strong rule
to detect a changed digit in a small label, computed with the real comparison in python/lib/adaguc/imagecompare.py.

Regenerate it after changing imagecompare.py. Needs numpy, Pillow and matplotlib (matplotlib is not in the
requirements files, install it with `pip install matplotlib`). From the repository root:

    python doc/developing/figures/image_compare_strong_rule.py

The label rendering is a copy of render_label in tests/AdagucTests/TestImageCompare.py.
"""

import importlib.util
import sys
from pathlib import Path

import matplotlib
import numpy as np
from PIL import Image, ImageDraw, ImageFont

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

REPO = Path(__file__).resolve().parents[3]
DEFAULT_OUTPUT = REPO / "doc" / "developing" / "image-compare-strong-rule.png"
FONT = REPO / "data" / "fonts" / "FreeSans.ttf"
W, H = 80, 40
CROP = (slice(10, 32), slice(17, 63))
INK, MUTED = "#1f1f1f", "#6b6b6b"
FLAG_RED, BELOW_ORANGE = np.array([230, 0, 0]), np.array([255, 170, 60])


def load_imagecompare():
    """Loads imagecompare.py from this repository, not from an installed adaguc package (which may be another checkout)."""
    path = REPO / "python" / "lib" / "adaguc" / "imagecompare.py"
    spec = importlib.util.spec_from_file_location("imagecompare", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def background():
    """A smooth diagonal colour gradient, like shading on a map: no 3x3 neighbourhood is flat, so the flat-area rule
    cannot help."""
    y, x = np.mgrid[0:H, 0:W]
    ramp = (x + y)[..., None] * np.array([1.5, 1.2, -1.5])
    return np.clip(np.array([40, 90, 220]) + ramp, 0, 255)


def render_label(text, samples=8):
    """A black 12 px label. The text is rasterized at 8 times the resolution, and the coverage of each pixel is
    computed with samples x samples of those points: another number of samples mimics another rasterizer."""
    scale = 8
    mask = Image.new("L", (W * scale, H * scale))
    ImageDraw.Draw(mask).text((20 * scale, 12 * scale), text, font=ImageFont.truetype(str(FONT), 12 * scale), fill=255)
    step = scale // samples
    points = (np.asarray(mask, float) / 255).reshape(H, scale, W, scale)[:, ::step, :, ::step]
    coverage = points.mean(axis=(1, 3))[..., None]
    rgb = np.round(background() * (1 - coverage))
    return np.dstack([rgb, np.full((H, W), 255)]).astype(np.int16)


def grey(img):
    return np.repeat(150 + (img[..., :3] * [0.299, 0.587, 0.114]).sum(axis=2, keepdims=True) / 2.5, 3, axis=2)


def rule_view(actual, flagged, below=None):
    """The actual image faded to grey, the pixels the rule flags in red, and (optionally) other candidates in orange."""
    out = grey(actual)
    if below is not None:
        out[below] = BELOW_ORANGE
    out[flagged] = FLAG_RED
    return np.clip(out, 0, 255)[CROP].astype(np.uint8)


def main(output):
    ic = load_imagecompare()
    expected = render_label("12.5")
    cases = [
        ("Other antialiasing\n(12.5, other rasterizer)", render_label("12.5", samples=4)),
        ("Other digit\n(12.5 → 12.3)", render_label("12.3")),
    ]

    fig, axes = plt.subplots(len(cases), 5, figsize=(13, 3.9), gridspec_kw=dict(wspace=0.1, hspace=0.45))
    headings = [
        "Expected",
        "Actual",
        "Difference per pixel\n(darker = larger)",
        "Rule cluster",
        f"Rule strong\n(more than {ic.STRONG_MARGIN})",
    ]
    for j, heading in enumerate(headings):
        axes[0, j].set_title(heading, fontsize=11, color=INK, pad=10)
    for i, (label, actual) in enumerate(cases):
        absdiff = np.abs(expected - actual).max(axis=2)
        rules = {name: (mask, allowed) for name, mask, allowed in ic._rules(expected, actual, absdiff, ic.ANTIALIASING)}
        axes[i, 0].imshow(expected[CROP][..., :3].astype(np.uint8), interpolation="nearest")
        axes[i, 1].imshow(actual[CROP][..., :3].astype(np.uint8), interpolation="nearest")
        axes[i, 2].imshow(255 - absdiff[CROP], cmap="gray", vmin=0, vmax=255, interpolation="nearest")
        axes[i, 2].set_xlabel(f"largest: {int(absdiff.max())}", fontsize=10, color=INK, labelpad=4)
        axes[i, 0].set_ylabel(label, fontsize=11, color=INK, rotation=0, ha="right", va="center", labelpad=12)
        strong_candidates = absdiff > ic.STRONG_MARGIN
        for j, name, below in ((3, "cluster", None), (4, "strong", strong_candidates)):
            mask, allowed = rules[name]
            n = int(mask.sum())
            axes[i, j].imshow(rule_view(actual, mask, below), interpolation="nearest")
            verdict = "✓" if n <= allowed else "✗ fails"
            axes[i, j].set_xlabel(
                f"{n} flagged, {allowed} allowed  {verdict}",
                fontsize=10,
                color=INK,
                labelpad=4,
                fontweight="normal" if n <= allowed else "bold",
            )
    for ax in axes.flat:
        ax.set_xticks([])
        ax.set_yticks([])
        for spine in ax.spines.values():
            spine.set_color("#c8c8c8")
    fig.text(
        0.5,
        0.0,
        'Zoomed crop of a 12 px label on a colour gradient (no flat areas, so the flat-area rule cannot help), tolerance "antialiasing".\n'
        f"Red: pixels the rule flags.  Orange: pixels that differ by more than {ic.STRONG_MARGIN} but have fewer than 2 such neighbours.",
        ha="center",
        fontsize=9,
        color=MUTED,
    )
    fig.savefig(output, dpi=110, bbox_inches="tight", facecolor="white")
    print(f"Written {output}")


if __name__ == "__main__":
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUTPUT)

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


def label_components(mask: np.ndarray) -> tuple[np.ndarray, list[int]]:
    height, width = mask.shape
    labels = np.zeros((height, width), dtype=np.int32)
    sizes = [0]
    label = 0

    for y in range(height):
        for x in np.flatnonzero(mask[y] & (labels[y] == 0)):
            if labels[y, x] != 0:
                continue
            label += 1
            labels[y, x] = label
            queue: deque[tuple[int, int]] = deque([(y, int(x))])
            size = 0
            while queue:
                cy, cx = queue.popleft()
                size += 1
                for ny in range(max(0, cy - 1), min(height, cy + 2)):
                    for nx in range(max(0, cx - 1), min(width, cx + 2)):
                        if mask[ny, nx] and labels[ny, nx] == 0:
                            labels[ny, nx] = label
                            queue.append((ny, nx))
            sizes.append(size)

    return labels, sizes


def remove_small_components(mask: np.ndarray, minimum_fraction: float) -> tuple[np.ndarray, list[int]]:
    labels, sizes = label_components(mask)
    largest = max(sizes[1:], default=0)
    if largest == 0:
        raise ValueError("No opaque icon pixels were found")
    minimum_size = max(32, int(largest * minimum_fraction))
    kept = [index for index, size in enumerate(sizes) if index > 0 and size >= minimum_size]
    return np.isin(labels, kept), [sizes[index] for index in kept]


def fill_small_holes(mask: np.ndarray, maximum_fraction: float) -> np.ndarray:
    inverse = ~mask
    labels, sizes = label_components(inverse)
    height, width = mask.shape
    exterior = set(labels[0, :]) | set(labels[-1, :]) | set(labels[:, 0]) | set(labels[:, -1])
    icon_area = int(mask.sum())
    maximum_size = max(16, int(icon_area * maximum_fraction))
    fill_labels = [
        index
        for index, size in enumerate(sizes)
        if index > 0 and index not in exterior and size <= maximum_size
    ]
    if fill_labels:
        mask = mask | np.isin(labels, fill_labels)
    return mask


def prepare_icon(source: Path, destination: Path, canvas_size: int, content_size: int) -> None:
    image = Image.open(source).convert("RGBA")
    alpha = np.asarray(image.getchannel("A"), dtype=np.uint8)
    mask_image = Image.fromarray(np.where(alpha >= 96, 255, 0).astype(np.uint8), mode="L")
    mask_image = mask_image.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.MinFilter(5))
    mask = np.asarray(mask_image, dtype=np.uint8) > 0

    mask, component_sizes = remove_small_components(mask, minimum_fraction=0.035)
    mask = fill_small_holes(mask, maximum_fraction=0.012)

    ys, xs = np.nonzero(mask)
    left, right = int(xs.min()), int(xs.max()) + 1
    top, bottom = int(ys.min()), int(ys.max()) + 1
    crop = Image.fromarray((mask[top:bottom, left:right] * 255).astype(np.uint8), mode="L")

    scale = min(content_size / crop.width, content_size / crop.height)
    target_width = max(1, round(crop.width * scale))
    target_height = max(1, round(crop.height * scale))
    resized = crop.resize((target_width, target_height), Image.Resampling.LANCZOS)

    alpha_canvas = Image.new("L", (canvas_size, canvas_size), 0)
    offset = ((canvas_size - target_width) // 2, (canvas_size - target_height) // 2)
    alpha_canvas.paste(resized, offset)
    output = Image.new("RGBA", (canvas_size, canvas_size), (255, 255, 255, 0))
    output.putalpha(alpha_canvas)

    destination.parent.mkdir(parents=True, exist_ok=True)
    output.save(destination, optimize=True)
    print(
        f"{destination}: source={image.size}, crop={crop.size}, output={output.size}, "
        f"kept_components={component_sizes}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Clean an ImageGen HUD glyph into a crisp white RGBA mask.")
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--canvas-size", type=int, default=1024)
    parser.add_argument("--content-size", type=int, default=700)
    args = parser.parse_args()
    prepare_icon(args.source, args.destination, args.canvas_size, args.content_size)


if __name__ == "__main__":
    main()

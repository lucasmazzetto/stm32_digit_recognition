#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ctypes
import re
import sys
from pathlib import Path
from typing import Tuple

import numpy as np


def ensure_contiguous(array: np.ndarray) -> np.ndarray:
    """
    @brief Ensures a NumPy array is C-contiguous.
    """
    return np.ascontiguousarray(array) if not array.flags["C_CONTIGUOUS"] else array


def load_c_lib(library_path: Path):
    """
    @brief Loads the compiled quantized C inference library.
    """
    try:
        c_lib = ctypes.CDLL(str(library_path.resolve()))
    except OSError as exc:
        raise RuntimeError(
            f"Unable to load C library: {library_path}\n"
            "Build it first (from external/quantized_digit_recognition):\n"
            "  mkdir -p lib && gcc -O3 -std=c11 -fPIC -shared -Iinclude "
            "src/convnet.c src/nn.c src/params.c -o lib/convnet.so"
        ) from exc

    c_int_p = ctypes.POINTER(ctypes.c_int)
    c_uint_p = ctypes.POINTER(ctypes.c_uint)
    c_lib.run_convnet.argtypes = (c_int_p, c_uint_p)
    c_lib.run_convnet.restype = None
    return c_lib


def load_expected_input_size(params_header: Path) -> Tuple[int, int, int]:
    """
    @brief Parses INPUT_HEIGHT, INPUT_WIDTH and INPUT_FLAT_SIZE from params.h.
    """
    if not params_header.exists():
        raise FileNotFoundError(f"Params header not found: {params_header}")

    define_re = re.compile(r"^#define\s+([A-Z0-9_]+)\s+([0-9]+)$")
    defines = {}
    for line in params_header.read_text(encoding="utf-8").splitlines():
        match = define_re.match(line.strip())
        if match:
            defines[match.group(1)] = int(match.group(2))

    required = ("INPUT_HEIGHT", "INPUT_WIDTH", "INPUT_FLAT_SIZE")
    missing = [key for key in required if key not in defines]
    if missing:
        raise KeyError(f"Missing required defines in params header: {missing}")

    return defines["INPUT_HEIGHT"], defines["INPUT_WIDTH"], defines["INPUT_FLAT_SIZE"]


def read_pgm(path: Path) -> np.ndarray:
    """
    @brief Reads a binary PGM image (P5) and returns a uint8 array (H, W).
    """
    data = path.read_bytes()
    idx = 0

    def skip_ws_and_comments(position: int) -> int:
        while position < len(data):
            byte = data[position]
            if byte in b" \t\r\n":
                position += 1
                continue
            if byte == ord("#"):
                while position < len(data) and data[position] not in b"\r\n":
                    position += 1
                continue
            break
        return position

    def read_token(position: int) -> Tuple[bytes, int]:
        position = skip_ws_and_comments(position)
        start = position
        while position < len(data) and data[position] not in b" \t\r\n#":
            position += 1
        if start == position:
            raise ValueError("Malformed PGM: missing token.")
        return data[start:position], position

    magic, idx = read_token(idx)
    if magic != b"P5":
        raise ValueError(f"Unsupported PGM format '{magic.decode(errors='replace')}', expected P5.")

    width_token, idx = read_token(idx)
    height_token, idx = read_token(idx)
    maxval_token, idx = read_token(idx)

    width = int(width_token)
    height = int(height_token)
    maxval = int(maxval_token)
    if maxval <= 0 or maxval > 255:
        raise ValueError(f"Unsupported maxval {maxval}; expected 1..255.")

    idx = skip_ws_and_comments(idx)

    expected = width * height
    payload = data[idx:]
    if len(payload) < expected:
        raise ValueError(
            f"Malformed PGM payload: got {len(payload)} bytes, expected at least {expected}."
        )

    image = np.frombuffer(payload[:expected], dtype=np.uint8).reshape(height, width)
    return image


def normalize_to_fp16(image_u8: np.ndarray) -> np.ndarray:
    """
    @brief Normalizes uint8 grayscale image to [-1, 1] and stores it as float16.
    """
    # Match training/eval normalization used by Normalize((0.5,), (0.5,))
    image_f32 = image_u8.astype(np.float32) / 255.0
    normalized_f32 = (image_f32 - 0.5) / 0.5
    return normalized_f32.astype(np.float16)


def fp16_to_q16_input(image_fp16: np.ndarray, frac_bits: int) -> np.ndarray:
    """
    @brief Converts normalized FP16 image to Q-format int32 expected by C inference.
    """
    scale = float(1 << frac_bits)
    # Convert back to float32 for predictable rounding behavior, then quantize.
    quantized = np.rint(image_fp16.astype(np.float32) * scale).astype(np.int32)
    return ensure_contiguous(quantized.flatten().astype(np.intc))


def run_convnet_inference(c_lib, input_q: np.ndarray) -> int:
    """
    @brief Executes one inference call through run_convnet().
    """
    prediction = ensure_contiguous(np.zeros(1, dtype=np.uintc))
    c_int_p = ctypes.POINTER(ctypes.c_int)
    c_uint_p = ctypes.POINTER(ctypes.c_uint)

    c_lib.run_convnet(input_q.ctypes.data_as(c_int_p), prediction.ctypes.data_as(c_uint_p))
    return int(prediction[0])


def main() -> int:
    """
    @brief Runs one-image quantized inference on capture.pgm.
    """
    repo_root = Path(__file__).resolve().parent.parent
    default_lib = repo_root / "external" / "quantized_digit_recognition" / "lib" / "convnet.so"
    default_params = repo_root / "external" / "quantized_digit_recognition" / "include" / "params.h"

    parser = argparse.ArgumentParser(
        description="Run quantized C inference for one PGM image (default: capture.pgm).",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--image",
        type=Path,
        default=Path("capture.pgm"),
        help="Path to grayscale PGM image (P5).",
    )
    parser.add_argument(
        "--lib-path",
        type=Path,
        default=default_lib,
        help="Path to compiled C shared library (convnet.so).",
    )
    parser.add_argument(
        "--params-header",
        type=Path,
        default=default_params,
        help="Path to params.h used to validate expected input dimensions.",
    )
    parser.add_argument(
        "--input-frac-bits",
        type=int,
        default=16,
        help="Fractional bits used to convert normalized image to fixed-point input.",
    )

    args = parser.parse_args()

    try:
        image_path = args.image.expanduser().resolve()
        if not image_path.exists():
            raise FileNotFoundError(f"Input image not found: {image_path}")

        expected_h, expected_w, expected_flat = load_expected_input_size(args.params_header)
        image_u8 = read_pgm(image_path)
        image_h, image_w = image_u8.shape

        if (image_h != expected_h) or (image_w != expected_w):
            raise ValueError(
                f"Input size mismatch: image is {image_w}x{image_h}, "
                f"but model expects {expected_w}x{expected_h}."
            )

        # Requested pipeline:
        # 1) normalize to model domain
        # 2) keep normalized image in FP16
        # 3) convert FP16 image to Q-format integer input for C quantized inference
        image_fp16 = normalize_to_fp16(image_u8)
        input_q = fp16_to_q16_input(image_fp16, args.input_frac_bits)

        if input_q.size != expected_flat:
            raise ValueError(
                f"Flat input mismatch: got {input_q.size} values, expected {expected_flat}."
            )

        c_lib = load_c_lib(args.lib_path)
        pred = run_convnet_inference(c_lib, input_q)

        print(f"Image: {image_path}")
        print(f"Input size: {image_w}x{image_h}")
        print(f"Predicted digit: {pred}")
        return 0
    except Exception as exc:  # noqa: BLE001
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

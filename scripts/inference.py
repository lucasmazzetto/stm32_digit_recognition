import argparse
import ctypes
import sys
from pathlib import Path

import numpy as np

from generate_lut import generate_u8_to_q16_lut


def load_c_lib(library_path: Path):
    """
    @brief Loads the compiled quantized C inference library.

    @param library_path Path to the compiled shared object.
    @return ctypes library handle with run_convnet signature configured.
    """
    try:
        c_lib = ctypes.CDLL(str(library_path.resolve()))
    except OSError as exc:
        raise RuntimeError(f'Unable to load C library: {library_path}\n'
                           'Build it first (from external/quantized_digit_recognition):\n'
                           '  mkdir -p lib && gcc -O3 -std=c11 -fPIC -shared -Iinclude '
                           'src/convnet.c src/nn.c src/params.c -o lib/convnet.so') from exc

    c_int_p = ctypes.POINTER(ctypes.c_int)
    c_uint_p = ctypes.POINTER(ctypes.c_uint)
    c_lib.run_convnet.argtypes = (c_int_p, c_uint_p)
    c_lib.run_convnet.restype = None
    return c_lib


def read_pgm(path: Path):
    """
    @brief Reads a binary PGM image (P5) and returns a uint8 array (H, W).

    @param path Path to the input PGM image.
    @return Grayscale image as a uint8 array with shape (H, W).
    """
    data = path.read_bytes()
    idx = 0

    def skip_ws_and_comments(position: int):
        while position < len(data):
            byte = data[position]
            if byte in b' \t\r\n':
                position += 1
                continue
            if byte == ord('#'):
                while position < len(data) and data[position] not in b'\r\n':
                    position += 1
                continue
            break
        return position

    def read_token(position: int):
        position = skip_ws_and_comments(position)
        start = position
        while position < len(data) and data[position] not in b' \t\r\n#':
            position += 1
        return data[start:position], position

    _, idx = read_token(idx)

    width_token, idx = read_token(idx)
    height_token, idx = read_token(idx)
    _, idx = read_token(idx)

    width = int(width_token)
    height = int(height_token)
    idx = skip_ws_and_comments(idx)

    expected = width * height
    payload = data[idx:idx + expected]
    image = np.frombuffer(payload, dtype=np.uint8).reshape(height, width)
    return image


def run_convnet_inference(c_lib, input_q: np.ndarray):
    """
    @brief Executes one inference call through run_convnet().

    @param c_lib Loaded C shared library handle.
    @param input_q Flattened quantized input array in model format.
    @return Predicted class index.
    """
    predictions = np.zeros(1, dtype=np.uintc)
    c_int_p = ctypes.POINTER(ctypes.c_int)
    c_uint_p = ctypes.POINTER(ctypes.c_uint)

    c_lib.run_convnet(input_q.ctypes.data_as(c_int_p),
                      predictions.ctypes.data_as(c_uint_p))
    
    return int(predictions[0])


def main(args: argparse.Namespace):
    """
    @brief Runs one-image quantized inference on capture.pgm.

    @param args Parsed CLI arguments.
    @return Process exit code (0 on success, non-zero on failure).
    """
    try:
        image_path = args.image.expanduser().resolve()
        image_u8 = read_pgm(image_path)
        image_h, image_w = image_u8.shape

        # Pipeline implemented via 256-entry LUT
        lut_q = generate_u8_to_q16_lut(args.input_frac_bits)
        input_q = np.ascontiguousarray(lut_q[image_u8].reshape(-1).astype(np.intc))

        c_lib = load_c_lib(args.lib_path)
        pred = run_convnet_inference(c_lib, input_q)

        print(f'Image: {image_path}')
        print(f'Input size: {image_w}x{image_h}')
        print(f'Predicted digit: {pred}')
        return 0
    except Exception as exc:
        print(f'Error: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    repo_root = Path(__file__).resolve().parent.parent
    default_lib = repo_root / 'external' / 'quantized_digit_recognition' / 'lib' / 'convnet.so'

    parser = argparse.ArgumentParser(description="Run quantized C inference for one "
                                                 "PGM image (default: capture.pgm).",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--lib_path", "--lib-path", type=Path, default=default_lib,
                        help="Path to compiled C shared library (convnet.so).")

    parser.add_argument("--image", type=Path, default=Path("capture.pgm"),
                        help="Path to grayscale PGM image (P5).")

    parser.add_argument("--input_frac_bits", "--input-frac-bits", type=int, default=16,
                        help="Fractional bits used to convert normalized image "
                             "to fixed-point input.")

    raise SystemExit(main(parser.parse_args()))

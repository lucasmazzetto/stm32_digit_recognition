from pathlib import Path

import numpy as np


def read_pgm(path: Path):
    """
    @brief Reads a binary PGM image (P5) and returns a uint8 array (H, W).

    @details This parser tolerates whitespace and comment lines in the header,
    validates basic PGM fields, and returns the image payload as a 2-D array.

    @param path Path to one `.pgm` image.
    @return Grayscale image as a uint8 array with shape (H, W).
    """
    data = path.read_bytes()

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
        if start == position:
            raise ValueError(f'Invalid PGM header in {path}')
        return data[start:position], position

    magic, idx = read_token(0)
    if magic != b'P5':
        raise ValueError(f'Unsupported PGM format in {path}: {magic!r}')

    width_token, idx = read_token(idx)
    height_token, idx = read_token(idx)
    maxval_token, idx = read_token(idx)

    width = int(width_token)
    height = int(height_token)
    maxval = int(maxval_token)

    if maxval <= 0 or maxval > 255:
        raise ValueError(f'Unsupported PGM maxval in {path}: {maxval}')

    if idx >= len(data):
        raise ValueError(f'Missing PGM payload in {path}')

    separator = data[idx]
    if separator not in b' \t\r\n':
        raise ValueError(f'Invalid PGM separator in {path}')

    idx += 1
    if separator == ord('\r') and idx < len(data) and data[idx] == ord('\n'):
        idx += 1

    expected = width * height
    payload = data[idx:idx + expected]
    if len(payload) != expected:
        raise ValueError(
            f'Invalid payload size in {path}: {len(payload)} (expected {expected})'
        )

    return np.frombuffer(payload, dtype=np.uint8).reshape(height, width)

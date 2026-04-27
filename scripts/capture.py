import argparse
import re
import sys
import time
from pathlib import Path

import serial


# FRAME_BEGIN <width> <height> <payload_size>
FRAME_BEGIN_RE = re.compile(r'^FRAME_BEGIN\s+(\d+)\s+(\d+)\s+(\d+)$')

# FRAME_END status=<hal_status>
FRAME_END_RE = re.compile(r'^FRAME_END\b')

# NN_PRED <digit>
NN_PRED_RE = re.compile(r'^NN_PRED\s+(\d+)$')


def print_protocol_line(text: str):
    """
    @brief Prints known protocol/log lines with clear type separation.

    @param text One decoded log line received from serial.
    """
    nn_match = NN_PRED_RE.search(text)

    if nn_match:
        print(f'NN prediction: {int(nn_match.group(1))}')
        return

    if text == 'READY' or text.startswith('FRAME_'):
        print(text)
        return

    # Keep non-protocol firmware logs visible for debugging capture failures
    print(f'FW: {text}')


def write_pgm(path: Path, width: int, height: int, payload: bytes):
    """
    @brief Writes an 8-bit grayscale image as binary PGM (P5).

    @param path Destination .pgm file path.
    @param width Image width in pixels.
    @param height Image height in pixels.
    @param payload Grayscale pixel bytes (width*height).
    """
    header = f'P5\n{width} {height}\n255\n'.encode('ascii')
    path.write_bytes(header + payload)


def wait_for_frame_header(ser: serial.Serial, timeout_s: float):
    """
    @brief Waits for and parses the FRAME_BEGIN header line.

    @param ser Open serial port object.
    @param timeout_s Timeout in seconds to wait for the frame header.
    @return Tuple containing width, height, and payload size in bytes.
    """
    # timeout_s <= 0 means "wait forever" in continuous mode
    deadline = None

    if timeout_s > 0.0:
        deadline = time.monotonic() + timeout_s

    print('Waiting for frame header...')

    while True:
        if deadline is not None and time.monotonic() >= deadline:
            break

        line = ser.readline()

        if not line:
            continue

        text = line.decode('utf-8', errors='replace').strip()

        if text:
            print_protocol_line(text)

        match = FRAME_BEGIN_RE.search(text)

        if match:
            width = int(match.group(1))
            height = int(match.group(2))
            size = int(match.group(3))
            print(f'Frame header detected: {width}x{height}, {size} bytes')
            return width, height, size

    raise TimeoutError(f'Timed out after {timeout_s:.1f}s waiting for frame header.')


def read_exact(ser: serial.Serial, size: int, timeout_s: float):
    """
    @brief Reads an exact number of payload bytes from the serial port.

    @param ser Open serial port object.
    @param size Number of bytes expected from the payload.
    @param timeout_s Timeout in seconds for receiving the payload.
    @return Raw payload bytes with exact requested size.
    """
    # timeout_s <= 0 means "wait forever" for the full payload
    deadline = None

    if timeout_s > 0.0:
        deadline = time.monotonic() + timeout_s

    data = bytearray()

    while len(data) < size:
        if deadline is not None and time.monotonic() >= deadline:
            break

        chunk = ser.read(min(4096, size - len(data)))

        if not chunk:
            continue

        data.extend(chunk)

    if len(data) != size:
        raise TimeoutError(f'Timed out after {timeout_s:.1f}s receiving payload '
                           f'({len(data)}/{size} bytes).')

    return bytes(data)


def drain_until_frame_end(ser: serial.Serial, timeout_s: float):
    """
    @brief Drains log lines until FRAME_END is seen or timeout elapses.

    @param ser Open serial port object.
    @param timeout_s Maximum duration in seconds to drain trailing logs.
    """
    deadline = time.monotonic() + timeout_s

    while time.monotonic() < deadline:
        line = ser.readline()

        if not line:
            continue

        text = line.decode('utf-8', errors='replace').strip()

        if not text:
            continue

        print_protocol_line(text)

        if FRAME_END_RE.search(text):
            return


def main(args: argparse.Namespace):
    """
    @brief Runs host-side continuous capture and overwrites one output frame.

    @param args Parsed CLI arguments.
    @return Process exit code (0 on success, non-zero on failure).
    """
    if args.frame_timeout is not None and args.frame_timeout < 0:
        print('Error: --frame-timeout must be >= 0.', file=sys.stderr)
        return 2

    if args.frame_end_timeout < 0:
        print('Error: --frame-end-timeout must be >= 0.', file=sys.stderr)
        return 2

    output_path = Path(args.output).expanduser().resolve()

    if output_path.suffix == '':
        output_path = output_path.with_suffix('.pgm')

    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Default behavior is to wait forever for next frame/payload
    frame_timeout = 0.0 if args.frame_timeout is None else args.frame_timeout

    try:
        with serial.Serial(args.port, args.baud, timeout=0.25) as ser:
            ser.reset_input_buffer()
            print('Listening for frames. Press B1 on the board to capture.')
            print('Capture mode: continuous (Ctrl+C to stop).')

            if frame_timeout <= 0.0:
                print('Header/payload timeout: infinite.')
            else:
                print(f'Header/payload timeout: {frame_timeout:.1f}s.')
            print(f'Output mode: overwrite {output_path} on each capture.')

            captured = 0

            while True:
                try:
                    width, height, size = wait_for_frame_header(ser, frame_timeout)
                    payload = read_exact(ser, size, frame_timeout)
                except TimeoutError as exc:
                    print(f'Warning: {exc} Continuing to wait for next frame.')
                    continue

                print(f'Resolution: {width}x{height}, bytes: {len(payload)}')

                expected_size = width * height

                if len(payload) != expected_size:

                    print(f'Warning: expected grayscale payload with {expected_size} '
                          f'bytes, got {len(payload)} bytes. Discarding frame.')
                                        
                    drain_until_frame_end(ser, args.frame_end_timeout)

                    continue

                write_pgm(output_path, width, height, payload)

                captured += 1

                print(f'Saved frame #{captured} to {output_path}')

                drain_until_frame_end(ser, args.frame_end_timeout)

    except KeyboardInterrupt:
        print('\nInterrupted.', file=sys.stderr)
        return 130
        
    except (OSError, serial.SerialException, TimeoutError, ValueError) as exc:
        print(f'Error: {exc}', file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Capture FRAME_BEGIN packets continuously '
                                                 'and overwrite one grayscale frame file.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                     add_help=False)

    parser.add_argument('--port', type=str, default='/dev/ttyACM0',
                        help='Serial device path.')

    parser.add_argument('--baud', type=int, default=115200,
                        help='Serial baud rate.')

    parser.add_argument('--output', type=str, default='capture.pgm',
                        help='Output grayscale PGM file path.')

    parser.add_argument('--frame_timeout', '--frame-timeout', type=float, default=None,
                        help='Seconds to wait for frame header/payload. '
                             'Use 0 for infinite wait.')

    parser.add_argument('--frame_end_timeout', '--frame-end-timeout', type=float, default=2.0,
                        help='Seconds to wait for optional FRAME_END log after payload.')

    raise SystemExit(main(parser.parse_args()))

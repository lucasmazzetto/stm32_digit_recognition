#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

import serial


# FRAME_BEGIN <width> <height> <payload_size>
FRAME_BEGIN_RE = re.compile(r"^FRAME_BEGIN\s+(\d+)\s+(\d+)\s+(\d+)$")
# FRAME_END status=<hal_status>
FRAME_END_RE = re.compile(r"^FRAME_END\b")
# NN_PRED <digit>
NN_PRED_RE = re.compile(r"^NN_PRED\s+(\d+)$")


def print_protocol_line(text: str):
    """
    @brief Prints known protocol/log lines with clear type separation.
    """
    nn_match = NN_PRED_RE.search(text)
    if nn_match:
        print(f"NN prediction: {int(nn_match.group(1))}")
        return

    if text == "READY" or text.startswith("FRAME_"):
        print(text)
        return

    # Keep non-protocol firmware logs visible for debugging capture failures
    print(f"FW: {text}")


def write_pgm(path: Path, width: int, height: int, payload: bytes):
    """
    @brief Writes an 8-bit grayscale image as binary PGM (P5).

    @param path Destination .pgm file path.
    @param width Image width in pixels.
    @param height Image height in pixels.
    @param payload Grayscale pixel bytes (width*height).
    """
    header = f"P5\n{width} {height}\n255\n".encode("ascii")
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
    print("Waiting for frame header...")

    while True:
        # Exit only when we use bounded timeout and deadline is reached
        if deadline is not None and time.monotonic() >= deadline:
            break

        # Read one line because FRAME_BEGIN/READY/FRAME_END are line-oriented logs
        line = ser.readline()
        if not line:
            continue

        text = line.decode("utf-8", errors="replace").strip()
        # Keep terminal output focused on protocol-level lines only
        if text:
            print_protocol_line(text)

        # Parse FRAME_BEGIN and return dimensions + payload size
        match = FRAME_BEGIN_RE.search(text)
        if match:
            width = int(match.group(1))
            height = int(match.group(2))
            size = int(match.group(3))
            print(f"Frame header detected: {width}x{height}, {size} bytes")
            return width, height, size

    raise TimeoutError(f"Timed out after {timeout_s:.1f}s waiting for frame header.")


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

    # Keep reading until we collect exactly the announced payload size
    while len(data) < size:
        # Exit only when we use bounded timeout and deadline is reached
        if deadline is not None and time.monotonic() >= deadline:
            break

        # Read at most remaining bytes (up to 4KB per syscall)
        chunk = ser.read(min(4096, size - len(data)))
        if not chunk:
            continue
        data.extend(chunk)

    if len(data) != size:
        raise TimeoutError(
            f"Timed out after {timeout_s:.1f}s receiving payload "
            f"({len(data)}/{size} bytes)."
        )

    return bytes(data)


def drain_until_frame_end(ser: serial.Serial, timeout_s: float):
    """
    @brief Drains log lines until FRAME_END is seen or timeout elapses.
    """
    deadline = time.monotonic() + timeout_s

    while time.monotonic() < deadline:
        # Drain line logs after payload so next loop starts from a clean boundary
        line = ser.readline()
        if not line:
            continue

        text = line.decode("utf-8", errors="replace").strip()
        if not text:
            continue

        print_protocol_line(text)
        # Stop draining once we see the explicit end marker for this frame cycle
        if FRAME_END_RE.search(text):
            return


def resolve_output_path(base_path: Path, output_pattern: str | None, index: int,
                        multi_capture: bool) -> Path:
    """
    @brief Builds one output filename for the captured frame.
    """
    if output_pattern is not None:
        # User-supplied printf-style template takes precedence over default naming
        if "%d" not in output_pattern:
            raise ValueError("output pattern must contain %d (example: captures/frame_%04d.pgm).")
        output_path = Path(output_pattern % index).expanduser().resolve()
    elif multi_capture:
        # Finite multi-capture mode auto-indexes output files
        output_path = base_path.with_name(f"{base_path.stem}_{index:04d}{base_path.suffix}")
    else:
        # Single/continuous default mode writes to one fixed output path
        output_path = base_path

    # Force .pgm extension to match the image writer format
    if output_path.suffix.lower() != ".pgm":
        output_path = output_path.with_suffix(".pgm")

    # Ensure destination folder exists before writing
    output_path.parent.mkdir(parents=True, exist_ok=True)
    return output_path


def main(args: argparse.Namespace):
    """
    @brief Runs host-side capture and writes one or more frames to disk.

    @param args Parsed CLI arguments.
    @return Process exit code (0 on success, non-zero on failure).
    """
    if args.count < 0:
        print("Error: --count must be >= 0.", file=sys.stderr)
        return 2
    if args.frame_timeout is not None and args.frame_timeout < 0:
        print("Error: --frame-timeout must be >= 0.", file=sys.stderr)
        return 2

    # Naming per frame is derived from this base path
    base_output_path = Path(args.output).expanduser().resolve()
    if base_output_path.suffix == "":
        base_output_path = base_output_path.with_suffix(".pgm")

    # Enable indexed filenames only for finite multi-capture sessions
    multi_capture = args.count > 1
    target_count = args.count

    # Timeout policy
    frame_timeout = args.frame_timeout
    if frame_timeout is None:
        if target_count == 0:
            frame_timeout = 0.0
        else:
            frame_timeout = 60.0

    try:
        with serial.Serial(args.port, args.baud, timeout=0.25) as ser:
            # Drop stale buffered bytes from previous runs/reset noise
            ser.reset_input_buffer()
            print("Listening for frames. Press B1 on the board to capture.")
            if target_count == 0:
                print("Capture mode: continuous (Ctrl+C to stop).")
                if frame_timeout <= 0.0:
                    print("Header/payload timeout: infinite.")
                if args.output_pattern is None:
                    print(f"Output mode: overwrite {base_output_path} on each capture.")
            else:
                print(f"Capture mode: {target_count} frame(s).")

            captured = 0
            # Main capture loop: bounded by --count unless count=0 (continuous)
            while (target_count == 0) or (captured < target_count):
                frame_index = captured + 1

                try:
                    # 1) Wait for protocol header 2) Read exact payload bytes
                    width, height, size = wait_for_frame_header(ser, frame_timeout)
                    payload = read_exact(ser, size, frame_timeout)
                except TimeoutError as exc:
                    # Continuous mode keeps running after missing/late frames
                    if target_count == 0:
                        print(f"Warning: {exc} Continuing to wait for next frame.")
                        continue
                    # Finite mode treats timeout as terminal error
                    raise

                print(f"Resolution: {width}x{height}, bytes: {len(payload)}")
                expected_size = width * height
                # Reject malformed payloads (size announced vs size expected by WxH)
                if len(payload) != expected_size:
                    message = (
                        f"expected grayscale payload with {expected_size} bytes, "
                        f"got {len(payload)} bytes."
                    )
                    if target_count == 0:
                        print(f"Warning: {message} Discarding frame.")
                        drain_until_frame_end(ser, args.frame_end_timeout)
                        continue
                    print(f"Error: {message}", file=sys.stderr)
                    return 2

                # Resolve destination filename and persist one PGM frame
                output_path = resolve_output_path(base_output_path,
                                                  args.output_pattern,
                                                  frame_index,
                                                  multi_capture)
                write_pgm(output_path, width, height, payload)
                print(f"Saved frame #{frame_index} to {output_path}")

                # Consume trailing frame logs before waiting for the next header
                drain_until_frame_end(ser, args.frame_end_timeout)
                captured += 1
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        return 130
    except (OSError, serial.SerialException, TimeoutError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Wait for FRAME_BEGIN packets and save one or more grayscale frames.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--port', type=str, default='/dev/ttyACM0',
                        help='Serial device path.')

    parser.add_argument('--baud', type=int, default=115200,
                        help='Serial baud rate.')

    parser.add_argument('--output', type=str, default='capture.pgm',
                        help='Output grayscale PGM file path.')

    parser.add_argument('--frame-timeout', type=float, default=None,
                        help='Seconds to wait for frame header/payload. Use 0 for infinite wait. '
                             'If omitted: continuous mode uses infinite wait; finite mode uses 60s.')

    parser.add_argument('--frame-end-timeout', type=float, default=2.0,
                        help='Seconds to wait for optional FRAME_END log after payload.')

    parser.add_argument('--count', type=int, default=0,
                        help='Number of frames to capture (0 means capture until interrupted).')

    parser.add_argument('--output-pattern', type=str, default=None,
                        help='Optional printf-style path pattern with %%d (e.g. captures/frame_%%04d.pgm).')

    raise SystemExit(main(parser.parse_args()))

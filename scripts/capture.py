#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

import serial


READY_MARKER = b"Press B1 to capture one frame"
FRAME_BEGIN_RE = re.compile(r"^FRAME_BEGIN\s+(\d+)\s+(\d+)\s+(\d+)$")


def wait_for_ready(ser: serial.Serial, timeout_s: float):
    """
    @brief Waits for the firmware ready marker on the serial stream.

    @param ser Open serial port object.
    @param timeout_s Timeout in seconds to wait for the ready marker.
    """
    deadline = time.monotonic() + timeout_s
    raw_window = bytearray()

    print("Waiting for firmware initialization...")

    while time.monotonic() < deadline:
        chunk = ser.read(256)
        if not chunk:
            continue

        raw_window.extend(chunk)
        if len(raw_window) > 4096:
            del raw_window[:-4096]

        if READY_MARKER in raw_window:
            print("Firmware ready. Press B1 to capture one frame.")
            return

    raise TimeoutError(
        f"Timed out after {timeout_s:.1f}s waiting for init marker: "
        f"{READY_MARKER.decode('ascii')}"
    )


def wait_for_frame_header(ser: serial.Serial, timeout_s: float):
    """
    @brief Waits for and parses the FRAME_BEGIN header line.

    @param ser Open serial port object.
    @param timeout_s Timeout in seconds to wait for the frame header.
    @return Tuple containing width, height, and payload size in bytes.
    """
    deadline = time.monotonic() + timeout_s
    print("Waiting for frame header...")

    while time.monotonic() < deadline:
        line = ser.readline()
        if not line:
            continue

        text = line.decode("utf-8", errors="replace").strip()
        if text:
            print(text)

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
    deadline = time.monotonic() + timeout_s
    data = bytearray()

    while len(data) < size and time.monotonic() < deadline:
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


def main():
    """
    @brief Runs the host-side capture workflow and writes one frame to disk.

    @return Process exit code (0 on success, non-zero on failure).
    """
    parser = argparse.ArgumentParser(
        description='Wait for OV2640 init logs and save the next frame.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--port', type=str, default='/dev/ttyACM0',
                        help='Serial device path.')

    parser.add_argument('--baud', type=int, default=115200,
                        help='Serial baud rate.')

    parser.add_argument('--output', type=str, default='capture.bin',
                        help='Output raw frame file path.')

    parser.add_argument('--init-timeout', type=float, default=60.0,
                        help='Seconds to wait for the firmware ready message.')

    parser.add_argument('--frame-timeout', type=float, default=60.0,
                        help='Seconds to wait for frame header and payload after init is ready.')
    args = parser.parse_args()
    output_path = Path(args.output).expanduser().resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        with serial.Serial(args.port, args.baud, timeout=0.25) as ser:
            ser.reset_input_buffer()
            wait_for_ready(ser, args.init_timeout)
            width, height, size = wait_for_frame_header(ser, args.frame_timeout)
            payload = read_exact(ser, size, args.frame_timeout)
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        return 130
    except (OSError, serial.SerialException, TimeoutError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    output_path.write_bytes(payload)
    print(f"Saved frame to {output_path}")
    print(f"Resolution: {width}x{height}, bytes: {len(payload)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

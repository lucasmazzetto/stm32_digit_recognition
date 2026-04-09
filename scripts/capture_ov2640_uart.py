#!/usr/bin/env python3
"""Wait for OV2640 firmware init, then save one JPEG frame from UART."""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

import serial


READY_MARKER = b"Press B1 to capture one JPEG frame"
JPEG_SOI = b"\xff\xd8"
JPEG_EOI = b"\xff\xd9"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Wait for OV2640 init logs on UART and save the next JPEG frame."
    )
    parser.add_argument("--port", default="/dev/ttyACM0", help="Serial device path.")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate.")
    parser.add_argument(
        "--output",
        default="capture.jpg",
        help="Output JPEG file path.",
    )
    parser.add_argument(
        "--init-timeout",
        type=float,
        default=60.0,
        help="Seconds to wait for the firmware ready message.",
    )
    parser.add_argument(
        "--capture-timeout",
        type=float,
        default=30.0,
        help="Seconds to wait for a complete JPEG after init is ready.",
    )
    return parser.parse_args()


def print_line_buffer(buffer: bytearray) -> None:
    while True:
        newline_index = buffer.find(b"\n")
        if newline_index < 0:
            return

        line = bytes(buffer[: newline_index + 1])
        del buffer[: newline_index + 1]
        text = line.decode("utf-8", errors="replace").rstrip("\r\n")
        if text:
            print(text)


def wait_for_ready(ser: serial.Serial, timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s
    raw_window = bytearray()
    line_buffer = bytearray()

    print("Waiting for firmware initialization...")

    while time.monotonic() < deadline:
        chunk = ser.read(256)
        if not chunk:
            continue

        raw_window.extend(chunk)
        if len(raw_window) > 4096:
            del raw_window[:-4096]

        line_buffer.extend(chunk)
        print_line_buffer(line_buffer)

        if READY_MARKER in raw_window:
            print("Firmware ready. Press B1 to capture one frame.")
            return

    raise TimeoutError(
        f"Timed out after {timeout_s:.1f}s waiting for init marker: "
        f"{READY_MARKER.decode('ascii')}"
    )


def capture_jpeg(ser: serial.Serial, timeout_s: float) -> bytes:
    deadline = time.monotonic() + timeout_s
    pending = bytearray()
    image = bytearray()
    started = False

    print("Waiting for JPEG stream...")

    while time.monotonic() < deadline:
        chunk = ser.read(512)
        if not chunk:
            continue

        pending.extend(chunk)

        if not started:
            soi_index = pending.find(JPEG_SOI)
            if soi_index < 0:
                if len(pending) > 1:
                    del pending[:-1]
                continue

            started = True
            image.extend(pending[soi_index:])
            pending.clear()
            print("JPEG start marker detected.")
        else:
            image.extend(pending)
            pending.clear()

        eoi_index = image.find(JPEG_EOI)
        if eoi_index >= 0:
            end_index = eoi_index + len(JPEG_EOI)
            print(f"JPEG end marker detected. Size: {end_index} bytes")
            return bytes(image[:end_index])

    raise TimeoutError(f"Timed out after {timeout_s:.1f}s waiting for JPEG frame.")


def main() -> int:
    args = parse_args()
    output_path = Path(args.output).expanduser().resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        with serial.Serial(args.port, args.baud, timeout=0.25) as ser:
            ser.reset_input_buffer()
            wait_for_ready(ser, args.init_timeout)
            jpeg = capture_jpeg(ser, args.capture_timeout)
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        return 130
    except (OSError, serial.SerialException, TimeoutError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    output_path.write_bytes(jpeg)
    print(f"Saved JPEG to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

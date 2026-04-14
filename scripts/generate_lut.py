import argparse
from pathlib import Path

import numpy as np


def generate_u8_to_q16_lut(frac_bits: int = 16):
    """
    @brief Builds a 256-entry LUT that matches host normalization -> FP16 -> Q-format.

    Each entry converts an 8-bit pixel into the model input domain by normalizing it to
    [-1, 1], applying the same FP16 rounding used in the host pipeline, and scaling the
    result into fixed-point Q-format with frac_bits fractional bits.

    @param frac_bits Fractional bits used by fixed-point representation.
    @return Contiguous int32 LUT with shape (256,).
    """
    if frac_bits < 0:
        raise ValueError(f'frac_bits must be >= 0, got {frac_bits}')

    values_u8 = np.arange(256, dtype=np.uint8)
    normalized_f32 = (values_u8.astype(np.float32) / 255.0 - 0.5) / 0.5
    fp16_values = normalized_f32.astype(np.float16)
    scale = float(1 << frac_bits)
    
    q_values = np.rint(fp16_values.astype(np.float32) * scale).astype(np.int32)

    return np.ascontiguousarray(q_values)


def write_header_file(path: Path, guard: str, lut_symbol: str, size_symbol: str):
    """
    @brief Writes generated lut.h.

    @param path Output header path.
    @param guard Preprocessor include guard symbol.
    @param lut_symbol Exported LUT symbol name.
    @param size_symbol Macro name storing LUT size.
    """
    with path.open('w', encoding='utf-8') as file:
        file.write(f'#ifndef {guard}\n#define {guard}\n\n')
        file.write('#include <stdint.h>\n\n')
        file.write(f'#define {size_symbol} 256\n\n')
        file.write(f'extern const int32_t {lut_symbol}[{size_symbol}];\n\n')
        file.write(f'#endif // {guard}\n')


def write_source_file(path: Path, header_name: str, lut_symbol: str,
                      size_symbol: str, values: np.ndarray):
    """
    @brief Writes generated lut.c.

    @param path Output source path.
    @param header_name Header file included by the generated source.
    @param lut_symbol Exported LUT symbol name.
    @param size_symbol Macro name storing LUT size.
    @param values LUT values used to build the C initializer.
    """
    if values.ndim != 1:
        raise ValueError(f'values must be 1-D, got shape {values.shape}')

    values_text = ', '.join(str(int(v)) for v in values)

    with path.open('w', encoding='utf-8') as file:
        file.write(f'#include "{header_name}"\n\n')
        file.write(f'const int32_t {lut_symbol}[{size_symbol}] = '
                   f'{{{values_text}}};\n')


def generate_c_lut_files(output_dir: Path, frac_bits: int = 16):
    """
    @brief Generates lut.h and lut.c under output_dir/include and output_dir/src.

    @param output_dir App folder that contains include/ and src/ directories.
    @param frac_bits Q-format fractional bits used to generate LUT.
    @return Tuple of generated header path and source path.
    """
    lut_values = generate_u8_to_q16_lut(frac_bits=frac_bits)

    include_dir = output_dir / 'include'
    source_dir = output_dir / 'src'
    include_dir.mkdir(parents=True, exist_ok=True)
    source_dir.mkdir(parents=True, exist_ok=True)

    header_path = include_dir / 'lut.h'
    source_path = source_dir / 'lut.c'

    guard = 'LUT'
    lut_symbol = 'u8_to_q16_lut'
    size_symbol = 'U8_TO_Q16_LUT_SIZE'

    write_header_file(header_path, guard, lut_symbol, size_symbol)
    write_source_file(source_path, 'lut.h', lut_symbol, size_symbol, lut_values)

    return header_path, source_path


def main(args: argparse.Namespace):
    """
    @brief Generates LUT C files from parsed CLI arguments.

    @param args Parsed CLI arguments.
    @return Process exit code (0 on success, non-zero on failure).
    """
    output_dir = args.output_dir.expanduser().resolve()

    try:
        header_path, source_path = generate_c_lut_files(
            output_dir=output_dir, frac_bits=args.frac_bits)
        
        print(f'Generated {header_path}')
        print(f'Generated {source_path}')
        return 0
    except Exception as exc:  # noqa: BLE001
        print(f'Error: {exc}')
        return 1


if __name__ == '__main__':
    repo_root = Path(__file__).resolve().parent.parent
    default_output_dir = repo_root / 'firmware' / 'app'

    parser = argparse.ArgumentParser(description="Generate LUT C files (lut.h and lut.c) "
                                                 "for firmware preprocessing.",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--output_dir", "--output-dir", type=Path, default=default_output_dir,
                        help="App directory where include/lut.h and src/lut.c will be generated.")

    parser.add_argument("--frac_bits", "--frac-bits", type=int, default=16,
                        help="Fractional bits used by Q-format LUT generation.")

    raise SystemExit(main(parser.parse_args()))

import argparse
import math
from pathlib import Path

import numpy as np
from utils import read_pgm


def load_digits(dataset_dir: Path):
    """
    @brief Loads all digit samples from folders `0` to `9`.

    @param dataset_dir Dataset root path.
    @return Matrix with shape [num_samples, 784] and dtype uint8.
    """
    samples = []
    for folder in tuple(str(index) for index in range(10)):
        for path in sorted((dataset_dir / folder).glob('*.pgm')):
            samples.append(read_pgm(path).reshape(-1))
    return np.asarray(samples, dtype=np.uint8)


def build_center_vector(x_digits: np.ndarray):
    """
    @brief Builds the dataset center vector using rounded per-pixel mean.

    @param x_digits Digit sample matrix [N, 784], uint8.
    @return Center vector [784], uint8.
    """
    sample_count = int(x_digits.shape[0])
    sums = np.sum(x_digits, axis=0, dtype=np.uint64)
    return ((sums + (sample_count // 2)) // sample_count).astype(np.uint8)


def cosine_distance(sample: np.ndarray, center_vector: np.ndarray, center_norm: int,
        frac_bits: int):
    """
    @brief Computes integer cosine distance between one sample and center.

    @details Similarity is represented in a fixed-point format with
    `frac_bits` fractional bits:
    similarity ~= (dot / (||x||*||c||)) * 2^frac_bits.
    Distance uses the same scale and is computed as
    `fixed_point_max - similarity`.

    @param sample One flattened image vector [784], uint8.
    @param center_vector Center vector [784], uint8.
    @param center_norm Precomputed integer norm of center_vector.
    @return Cosine distance in fixed-point integer scale.
    """
    dot = 0
    norm_sample_sq = 0
    fixed_point_scale = int(1 << frac_bits)
    fixed_point_min = -fixed_point_scale
    fixed_point_max = fixed_point_scale - 1

    for index in range(sample.shape[0]):
        x_value = int(sample[index])
        c_value = int(center_vector[index])
        # Integer dot product and squared norm accumulation.
        dot += x_value * c_value
        norm_sample_sq += x_value * x_value

    norm_sample = math.isqrt(norm_sample_sq)
    denominator = norm_sample * center_norm

    if denominator == 0:
        similarity = 0
    else:
        # Rounded integer division while preserving fixed-point scale (2^frac_bits).
        similarity = ((dot << frac_bits) + (denominator // 2)) // denominator

    if similarity < fixed_point_min:
        similarity = fixed_point_min
    if similarity > fixed_point_max:
        similarity = fixed_point_max

    return fixed_point_max - similarity


def compute_percentile_threshold(x_digits: np.ndarray, center_vector: np.ndarray, percentile: float,
        frac_bits: int):
    """
    @brief Computes the digit percentile distance threshold in fixed-point scale.

    @details Distances are sorted and nearest-rank percentile selection is used:
    index = ceil((p / 100) * N) - 1.

    @param x_digits Digit sample matrix [N, 784], uint8.
    @param center_vector Center vector [784], uint8.
    @param percentile Percentile value in range [0, 100].
    @return Percentile cosine distance threshold in fixed-point scale.
    """
    center_norm_sq = 0
    for value in center_vector:
        center_norm_sq += int(value) * int(value)
    center_norm = math.isqrt(center_norm_sq)

    distances = np.zeros(x_digits.shape[0], dtype=np.int32)
    for sample_index in range(x_digits.shape[0]):
        distances[sample_index] = cosine_distance(x_digits[sample_index], center_vector,
                center_norm, frac_bits)

    sorted_distances = np.sort(distances)
    sample_count = int(sorted_distances.shape[0])
    target_percentile = int(percentile * 100.0)
    # Percentiles are represented as hundredths of a percent (x100), so 100%
    # becomes 100 * 100 and keeps nearest-rank math fully integer.
    rank_index = (
        (target_percentile * sample_count) + (100 * 100) - 1
    ) // (100 * 100) - 1
    rank_index = max(0, min(rank_index, sample_count - 1))
    return int(sorted_distances[rank_index])


def write_header_file(path: Path, threshold: int, vector_size: int, frac_bits: int):
    """
    @brief Writes generated distance header file.

    @param path Output `distance.h` path.
    @param threshold Integer distance threshold in fixed-point scale.
    @param vector_size Center vector length (expected 784).
    """
    with path.open('w', encoding='utf-8') as file:
        file.write('#ifndef DISTANCE_H\n#define DISTANCE_H\n\n')
        file.write('#include <stdint.h>\n\n')
        file.write(f'#define DISTANCE_VECTOR_SIZE {vector_size}\n')
        file.write(f'#define DISTANCE_FRAC_BITS {frac_bits}\n')
        file.write(f'#define DISTANCE_THRESHOLD {threshold}\n\n')
        file.write('extern const uint8_t distance_center_vector[DISTANCE_VECTOR_SIZE];\n\n')
        file.write('#endif // DISTANCE_H\n')


def write_source_file(path: Path, center_vector: np.ndarray):
    """
    @brief Writes generated distance source file.

    @param path Output `distance.c` path.
    @param center_vector Center vector values to emit as C initializer.
    """
    values = [str(int(value)) for value in center_vector]
    line_width = 16
    lines = []
    for index in range(0, len(values), line_width):
        lines.append('    ' + ', '.join(values[index:index + line_width]))
    body = ',\n'.join(lines)

    with path.open('w', encoding='utf-8') as file:
        file.write('#include "distance.h"\n\n')
        file.write('const uint8_t distance_center_vector[DISTANCE_VECTOR_SIZE] = {\n')
        file.write(body)
        file.write('\n};\n')


def main(args: argparse.Namespace):
    """
    @brief Generates distance constants for firmware known/unknown filtering.

    @details Pipeline:
    1) load digit images (0..9),
    2) compute rounded mean center vector,
    3) compute percentile integer cosine-distance threshold in signed 16-bit fixed-point,
    4) write `distance.h` and `distance.c`.

    @param args Parsed CLI arguments.
    @return Process exit code (0 on success).
    """
    dataset_dir = args.dataset_dir.expanduser().resolve()
    output_dir = args.output_dir.expanduser().resolve()

    x_digits = load_digits(dataset_dir)
    center_vector = build_center_vector(x_digits)
    threshold = compute_percentile_threshold(
        x_digits, center_vector, args.percentile, args.frac_bits
    )

    include_dir = output_dir / 'include'
    src_dir = output_dir / 'src'
    include_dir.mkdir(parents=True, exist_ok=True)
    src_dir.mkdir(parents=True, exist_ok=True)

    header_path = include_dir / 'distance.h'
    source_path = src_dir / 'distance.c'

    write_header_file(header_path, threshold, center_vector.shape[0], args.frac_bits)
    write_source_file(source_path, center_vector)

    fixed_point_scale = int(1 << args.frac_bits)
    print(f'Dataset: {dataset_dir}')
    print(f'Loaded digit samples: {x_digits.shape[0]}')
    print(f'DISTANCE_THRESHOLD={threshold}')
    print(f'DISTANCE_THRESHOLD_DECIMAL={threshold / fixed_point_scale:.6f}')
    print(f'Generated {header_path}')
    print(f'Generated {source_path}')

    return 0


if __name__ == '__main__':
    repo_root = Path(__file__).resolve().parent.parent
    default_output_dir = repo_root / 'firmware' / 'app'

    parser = argparse.ArgumentParser(description='Generate distance.h and distance.c from digit '
                                                 'p90 cosine threshold.')

    parser.add_argument('--dataset-dir', type=Path, default=Path('/mnt/Data/Datasets/mnist'))
    parser.add_argument('--output-dir', type=Path, default=default_output_dir)
    parser.add_argument('--percentile', type=float, default=90.0)
    parser.add_argument('--frac-bits', type=int, default=16)

    raise SystemExit(main(parser.parse_args()))

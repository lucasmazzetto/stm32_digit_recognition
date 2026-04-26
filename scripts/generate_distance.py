import argparse
import math
from pathlib import Path

import numpy as np

from generate_lut import generate_u8_to_q16_lut
from inference import load_c_lib, run_convnet_inference
from utils import read_pgm


def build_center_vector(vectors: np.ndarray, output_dtype):
    """
    @brief Builds one center vector using rounded per-element mean.

    @param vectors Matrix [N, D].
    @param output_dtype Output dtype for returned center vector.
    @return Center vector [D].
    """
    sample_count = int(vectors.shape[0])
    sums = np.sum(vectors, axis=0, dtype=np.int64)
    return ((sums + (sample_count // 2)) // sample_count).astype(output_dtype)


def cosine_distance(sample: np.ndarray, center_vector: np.ndarray, 
                    center_norm: int, frac_bits: int):
    """
    @brief Computes cosine distance using integer-only fixed-point arithmetic.

    This function computes cosine distance entirely with integers (fixed-point Q format) to match
    firmware behavior. It accumulates dot product and norms in integer form, uses math.isqrt for 
    magnitude, performs rounded integer division to estimate similarity in Q-scale, clamps to 
    valid range, and converts similarity to distance with distance = max_q - similarity.

    @param sample Input vector to score against class center.
    @param center_vector Class center vector for same feature space.
    @param center_norm Precomputed integer norm of `center_vector`.
    @param frac_bits Number of fractional bits used by fixed-point scale.
    @return Integer cosine distance in fixed-point domain.
    """
    dot = 0
    norm_sample_sq = 0
    fixed_point_scale = int(1 << frac_bits)
    fixed_point_min = -fixed_point_scale
    fixed_point_max = fixed_point_scale - 1

    for index in range(sample.shape[0]):
        sample_value = int(sample[index])
        center_value = int(center_vector[index])

        # Integer dot-product accumulation
        dot += sample_value * center_value

        # Integer squared-norm accumulation for sample magnitude
        norm_sample_sq += sample_value * sample_value

    # Integer norm (floor sqrt) keeps computation fully in integer domain
    norm_sample = math.isqrt(norm_sample_sq)
    denominator = norm_sample * center_norm

    # This approximates cosine similarity in Q(frac_bits) without floats
    if denominator == 0:
        # Degenerate vector case: treat as neutral similarity
        similarity = 0
    elif dot >= 0:
        similarity = ((dot << frac_bits) + (denominator // 2)) // denominator
    else:
        similarity = -(((-dot << frac_bits) + (denominator // 2)) // denominator)

    # Saturate similarity to fixed-point representable range
    if similarity < fixed_point_min:
        similarity = fixed_point_min
    if similarity > fixed_point_max:
        similarity = fixed_point_max

    # Convert similarity to distance in the same fixed-point scale
    return fixed_point_max - similarity


def compute_percentile_threshold(vectors: np.ndarray, center_vector: np.ndarray, 
                                 percentile: float, frac_bits: int):
    """
    @brief Computes percentile distance threshold in fixed-point scale.
    """
    center_norm_sq = 0
    for value in center_vector:
        center_norm_sq += int(value) * int(value)
    center_norm = math.isqrt(center_norm_sq)

    distances = np.zeros(vectors.shape[0], dtype=np.int32)
    for sample_index in range(vectors.shape[0]):
        distances[sample_index] = cosine_distance(vectors[sample_index], center_vector, center_norm,
                frac_bits)

    sorted_distances = np.sort(distances)
    sample_count = int(sorted_distances.shape[0])
    target_percentile = int(percentile * 100.0)

    # Nearest-rank percentile in integer math
    rank_index = ((target_percentile * sample_count) + (100 * 100) - 1) // (100 * 100) - 1
    rank_index = max(0, min(rank_index, sample_count - 1))
    
    return int(sorted_distances[rank_index])


def collect_vectors(dataset_dir: Path, c_lib, lut_q: np.ndarray, class_count: int):
    """
    @brief Collects class-wise image/logits vectors from dataset samples.

    For each class folder, this function runs inference on every image
    and keeps vectors only when predicted class matches the folder class.
    It also tracks total/kept counts per class for reporting.

    @param dataset_dir Root directory with class subfolders (`0`..`class_count-1`).
    @param c_lib Loaded C inference library handle.
    @param lut_q Precomputed uint8-to-fixed-point LUT used for input conversion.
    @param class_count Number of class folders to scan.
    @return Tuple
    """
    image_vectors_by_class = {}
    logits_vectors_by_class = {}
    total_samples = 0
    kept_samples = 0
    class_stats = []

    for class_id in range(class_count):
        # Temporary buffers for vectors that belong to this class
        class_image_vectors = []
        class_logits_vectors = []

        # Track how many samples were seen/kept for this class
        class_total = 0
        class_kept = 0

        for image_path in sorted((dataset_dir / str(class_id)).glob('*.pgm')):

            # Load class sample and flatten image for center/threshold generation
            image_u8 = read_pgm(image_path)
            image_vector = image_u8.reshape(-1).astype(np.uint8)

            # Convert image to model input domain and run quantized inference
            input_q = np.ascontiguousarray(lut_q[image_u8].reshape(-1).astype(np.intc))
            _, logits, predictions = run_convnet_inference(c_lib, input_q)

            predicted_class = int(predictions[0])

            # Keep only samples that the current network already recognizes as their class
            if predicted_class == class_id:
                # Store image/logits vectors used later to compute class centers
                class_image_vectors.append(image_vector.copy())
                class_logits_vectors.append(np.asarray(logits, dtype=np.int32).copy())
                class_kept += 1
                kept_samples += 1

            # Count every visited sample, independent of keep/discard decision
            class_total += 1
            total_samples += 1

        # Convert collected lists to compact numpy arrays for downstream math
        image_vectors_by_class[class_id] = np.asarray(class_image_vectors, dtype=np.uint8)
        logits_vectors_by_class[class_id] = np.asarray(class_logits_vectors, dtype=np.int32)

        # Persist per-class bookkeeping for generation summary output
        class_stats.append((class_id, class_kept, class_total))

    return image_vectors_by_class, logits_vectors_by_class, total_samples, kept_samples, class_stats


def write_header_file(path: Path, frac_bits: int, class_count: int, 
                      image_vector_size: int, logits_vector_size: int):
    """
    @brief Writes generated distance header file.
    """
    with path.open('w', encoding='utf-8') as file:
        file.write('#ifndef DISTANCE_H\n#define DISTANCE_H\n\n')
        file.write('#include <stdint.h>\n\n')
        file.write(f'#define DISTANCE_CLASS_COUNT {class_count}\n')
        file.write(f'#define DISTANCE_IMAGE_VECTOR_SIZE {image_vector_size}\n')
        file.write(f'#define DISTANCE_LOGITS_VECTOR_SIZE {logits_vector_size}\n')
        file.write(f'#define DISTANCE_FRAC_BITS {frac_bits}\n\n')

        file.write('extern const uint8_t distance_image_center_vectors[DISTANCE_CLASS_COUNT]'
                '[DISTANCE_IMAGE_VECTOR_SIZE];\n')
        file.write('extern const int32_t distance_logits_center_vectors[DISTANCE_CLASS_COUNT]'
                '[DISTANCE_LOGITS_VECTOR_SIZE];\n')
        file.write('extern const int32_t distance_image_thresholds[DISTANCE_CLASS_COUNT];\n')
        file.write('extern const int32_t distance_logits_thresholds[DISTANCE_CLASS_COUNT];\n\n')

        file.write('#endif // DISTANCE_H\n')


def write_source_file(path: Path, image_centers: np.ndarray, logits_centers: np.ndarray,
                      image_thresholds: np.ndarray, logits_thresholds: np.ndarray):
    """
    @brief Writes generated distance source file.
    """
    with path.open('w', encoding='utf-8') as file:
        file.write('#include "distance.h"\n\n')

        image_centers_text = ', '.join(
            '{' + ', '.join(str(int(value)) for value in row) + '}'
            for row in image_centers
        )
        logits_centers_text = ', '.join(
            '{' + ', '.join(str(int(value)) for value in row) + '}'
            for row in logits_centers
        )
        image_threshold_text = ', '.join(str(int(value)) for value in image_thresholds)
        logits_threshold_text = ', '.join(str(int(value)) for value in logits_thresholds)

        file.write('const uint8_t distance_image_center_vectors[DISTANCE_CLASS_COUNT]'
                   f'[DISTANCE_IMAGE_VECTOR_SIZE] = {{{image_centers_text}}};\n\n')
        file.write('const int32_t distance_logits_center_vectors[DISTANCE_CLASS_COUNT]'
                   f'[DISTANCE_LOGITS_VECTOR_SIZE] = {{{logits_centers_text}}};\n\n')
        file.write('const int32_t distance_image_thresholds[DISTANCE_CLASS_COUNT] = '
                   f'{{{image_threshold_text}}};\n\n')
        file.write('const int32_t distance_logits_thresholds[DISTANCE_CLASS_COUNT] = '
                   f'{{{logits_threshold_text}}};\n')


def main(args: argparse.Namespace):
    """
    @brief Generates firmware distance constants from a labeled image dataset.

    This entry point builds the data used by the firmware known/unknown filter.
    It loads class-labeled images, runs quantized inference, and keeps only the
    samples that the model classifies correctly for their folder label. Those
    kept samples are treated as reliable class representatives and are used to
    estimate one center vector per class in two feature spaces: raw image
    pixels and output logits.

    After center estimation, the function computes class-wise cosine-distance
    thresholds in fixed-point integer domain using a configurable percentile.
    The generated thresholds describe how far a valid sample is allowed to be
    from its class center before being treated as an outlier. Finally, the
    function writes `distance.h` and `distance.c` so the embedded application
    can apply the same class-aware filtering logic at runtime using integer-only
    operations.

    @param args Parsed CLI arguments for dataset path, output path, model library,
    fixed-point format, class/vector dimensions, and percentile selection.
    @return Process exit code (0 on success).
    """
    dataset_dir = args.dataset_dir.expanduser().resolve()
    output_dir = args.output_dir.expanduser().resolve()

    class_count = args.class_count
    image_vector_size = args.image_vector_size
    logits_vector_size = args.logits_vector_size

    # Reuse existing inference module so host-side probing matches model execution.
    c_lib = load_c_lib(args.lib_path)
    
    # Same LUT used by inference path, so sampled vectors match runtime preprocessing.
    lut_q = generate_u8_to_q16_lut(args.frac_bits)

    # Gather class-grouped vectors and collection stats from dataset traversal.
    (image_vectors_by_class, logits_vectors_by_class, total_samples, kept_samples,
            class_stats) = collect_vectors(dataset_dir, c_lib, lut_q, class_count)

    # One center + one threshold per class, for both image and logits domains.
    image_centers = np.zeros((class_count, image_vector_size), dtype=np.uint8)
    logits_centers = np.zeros((class_count, logits_vector_size), dtype=np.int32)
    image_thresholds = np.zeros(class_count, dtype=np.int32)
    logits_thresholds = np.zeros(class_count, dtype=np.int32)

    for class_id in range(class_count):
        # Vectors retained for this class after prediction-vs-label filtering.
        class_image_vectors = image_vectors_by_class[class_id]
        class_logits_vectors = logits_vectors_by_class[class_id]

        # Build representative class centers in both feature spaces.
        image_centers[class_id] = build_center_vector(class_image_vectors, np.uint8)
        logits_centers[class_id] = build_center_vector(class_logits_vectors, np.int32)

        # Thresholds are learned from kept samples of the same class.
        image_thresholds[class_id] = compute_percentile_threshold(class_image_vectors,
                image_centers[class_id], args.percentile, args.frac_bits)
        logits_thresholds[class_id] = compute_percentile_threshold(class_logits_vectors,
                logits_centers[class_id], args.percentile, args.frac_bits)

    # Ensure output folders exist before writing generated firmware artifacts.
    include_dir = output_dir / 'include'
    src_dir = output_dir / 'src'
    include_dir.mkdir(parents=True, exist_ok=True)
    src_dir.mkdir(parents=True, exist_ok=True)

    # Target files consumed by firmware build.
    header_path = include_dir / 'distance.h'
    source_path = src_dir / 'distance.c'

    # Emit macros/declarations and corresponding constant definitions.
    write_header_file(header_path, args.frac_bits, class_count, image_vector_size,
            logits_vector_size)
    write_source_file(source_path, image_centers, logits_centers, image_thresholds,
            logits_thresholds)

    # Print concise generation summary for quick dataset/threshold inspection.
    print(f'Dataset: {dataset_dir}')
    print(f'Loaded digit samples: {total_samples}')
    print(f'Kept samples (pred == class): {kept_samples}')

    for class_id, class_kept, class_total in class_stats:
        print(f'Class {class_id}: kept={class_kept} total={class_total}')

    print(f'Percentile: {args.percentile}')
    print(f'Generated {header_path}')
    print(f'Generated {source_path}')

    return 0


if __name__ == '__main__':
    repo_root = Path(__file__).resolve().parent.parent
    default_output_dir = repo_root / 'firmware' / 'app'
    default_lib = repo_root / 'external' / 'quantized_digit_recognition' / 'lib' / 'convnet.so'

    parser = argparse.ArgumentParser(description='Generate distance.h and distance.c from image '
                                                  'and logits class-wise cosine thresholds.')

    parser.add_argument('--dataset-dir', type=Path,
                        default=Path('/mnt/Data/Datasets/mnist/augmented'))
    
    parser.add_argument('--output-dir', type=Path, default=default_output_dir)
    parser.add_argument('--lib-path', '--lib_path', type=Path, default=default_lib)
    parser.add_argument('--percentile', type=float, default=99.0)
    parser.add_argument('--frac-bits', type=int, default=16)
    parser.add_argument('--class-count', type=int, default=10)
    parser.add_argument('--image-vector-size', type=int, default=(28 * 28))
    parser.add_argument('--logits-vector-size', type=int, default=10)

    raise SystemExit(main(parser.parse_args()))

#ifndef DISTANCE_H
#define DISTANCE_H

#include <stdint.h>

#define DISTANCE_CLASS_COUNT 10
#define DISTANCE_IMAGE_VECTOR_SIZE 784
#define DISTANCE_LOGITS_VECTOR_SIZE 10
#define DISTANCE_FRAC_BITS 16

extern const uint8_t distance_image_center_vectors[DISTANCE_CLASS_COUNT][DISTANCE_IMAGE_VECTOR_SIZE];
extern const int32_t distance_logits_center_vectors[DISTANCE_CLASS_COUNT][DISTANCE_LOGITS_VECTOR_SIZE];
extern const int32_t distance_image_thresholds[DISTANCE_CLASS_COUNT];
extern const int32_t distance_logits_thresholds[DISTANCE_CLASS_COUNT];

#endif // DISTANCE_H

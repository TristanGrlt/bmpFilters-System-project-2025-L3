#ifndef BMP_H
#define BMP_H

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#define BMP_SIGNATURE 0x4D42

typedef struct __attribute__((packed)) {
  uint16_t signature;
  uint32_t file_size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t pixel_array_offset;
} bmp_file_header_t;

typedef struct {
  uint32_t header_size;
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bit_count;
  uint32_t compression;
  uint32_t image_size;
  int32_t x_pixels_per_meter;
  int32_t y_pixels_per_meter;
  uint32_t colors_used;
  uint32_t colors_important;
} bmp_dib_header_t;

// typedef struct {
//   uint8_t b;
//   uint8_t g;
//   uint8_t r;
// } bmp_pixel_t;

typedef struct {
  bmp_file_header_t *file_h;
  bmp_dib_header_t *dib_h;
  void *pixels;
} bmp_mapped_image_t;

#endif

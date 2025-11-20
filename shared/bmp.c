#include "bmp.h"
#include <stdint.h>
#include <stdio.h>

// arg est un pointeur vers thread_filter_args_t
void *identity_filter(void *arg) {
  thread_filter_args_t *args = (thread_filter_args_t *)arg;
  bmp_mapped_image_t *img = args->img;
  (void)img;
  return nullptr;
}

void *blackAndWhite_filter(void *arg) {
  thread_filter_args_t *args = (thread_filter_args_t *)arg;
  bmp_mapped_image_t *img = args->img;
  int32_t width = img->dib_h->width;

  // REAL SIZE
  int32_t row_size = ((width * 3 + 3) / 4) * 4;

  // FOR EACH LINE
  for (int32_t y = args->start_line; y < args->end_line; y++) {
    uint8_t *row = (uint8_t *)img->pixels + y * row_size;

    // FOR EACH PIXEL
    for (int32_t x = 0; x < width; x++) {
      uint8_t blue = row[x * 3];
      uint8_t green = row[x * 3 + 1];
      uint8_t red = row[x * 3 + 2];
      uint8_t gray = (uint8_t)(0.299 * red + 0.587 * green + 0.114 * blue);
      row[x * 3] = gray;
      row[x * 3 + 1] = gray;
      row[x * 3 + 2] = gray;
    }
  }
  return nullptr;
}
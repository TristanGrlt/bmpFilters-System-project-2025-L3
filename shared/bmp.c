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

// MATRICE CONVOLUTION
typedef struct {
  float *matrix;
  int32_t size; // 3 5 7
} convolution_matrix_t;

// apply_convolution : applique la matrice de convolution conv au pixel (x, y)
// de l'image pointé par img en utilisant l'image de référence pointé par
// ref_img pour les valeurs des pixels voisins.
static void apply_convolution(bmp_mapped_image_t *img,
                              bmp_mapped_image_t *ref_img, int32_t x, int32_t y,
                              int32_t width, int32_t height, int32_t row_size,
                              convolution_matrix_t *conv) {
  int32_t half_size = conv->size / 2;
  float sum_r = 0, sum_g = 0, sum_b = 0;
  float weight_sum = 0;

  uint8_t *row_out = (uint8_t *)img->pixels + y * row_size;

  for (int32_t ky = -half_size; ky <= half_size; ky++) {
    for (int32_t kx = -half_size; kx <= half_size; kx++) {
      int32_t px = x + kx;
      int32_t py = y + ky;

      // BORDER
      if (px < 0) {
        px = 0;
      }
      if (px >= width) {
        px = width - 1;
      }
      if (py < 0) {
        py = 0;
      }
      if (py >= height) {
        py = height - 1;
      }

      uint8_t *ref_row = (uint8_t *)ref_img->pixels + py * row_size;
      uint8_t blue = ref_row[px * 3];
      uint8_t green = ref_row[px * 3 + 1];
      uint8_t red = ref_row[px * 3 + 2];

      float weight =
          conv->matrix[(ky + half_size) * conv->size + (kx + half_size)];
      sum_b += blue * weight;
      sum_g += green * weight;
      sum_r += red * weight;
      weight_sum += weight;
    }
  }

  // NORMALIZE
  if (weight_sum > 0) {
    sum_b /= weight_sum;
    sum_g /= weight_sum;
    sum_r /= weight_sum;
  }

  // CLAMPING
  row_out[x * 3] = (uint8_t)(sum_b < 0 ? 0 : (sum_b > 255 ? 255 : sum_b));
  row_out[x * 3 + 1] = (uint8_t)(sum_g < 0 ? 0 : (sum_g > 255 ? 255 : sum_g));
  row_out[x * 3 + 2] = (uint8_t)(sum_r < 0 ? 0 : (sum_r > 255 ? 255 : sum_r));
}

// generic_convolution_filter : applique une matrice de convolution générique
// conv à l'image pointé par img en utilisant l'image de référence pointé par
// ref_img pour les valeurs des pixels voisins. Sur les lignes entre
// start_line et end_line.
void *generic_convolution_filter(void *arg, convolution_matrix_t *conv) {
  thread_filter_args_t *args = (thread_filter_args_t *)arg;
  bmp_mapped_image_t *img = args->img;
  bmp_mapped_image_t *ref_img = args->ref_img;

  int32_t width = img->dib_h->width;
  int32_t height = img->dib_h->height;

  // REAL SIZE
  int32_t row_size = ((width * 3 + 3) / 4) * 4;

  // FOR EACH LINE
  for (int32_t y = args->start_line; y < args->end_line; y++) {

    // FOR EACH PIXEL
    for (int32_t x = 0; x < width; x++) {
      apply_convolution(img, ref_img, x, y, width, height, row_size, conv);
    }
  }

  return nullptr;
}

void *blurbox_filter(void *arg) {
  float matrix_data[9] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  convolution_matrix_t conv = {.matrix = matrix_data, .size = 3};
  return generic_convolution_filter(arg, &conv);
}
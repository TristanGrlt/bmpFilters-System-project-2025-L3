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

typedef struct {
  bmp_file_header_t *file_h;
  bmp_dib_header_t *dib_h;
  void *pixels;
} bmp_mapped_image_t;

typedef struct {
  bmp_mapped_image_t *img;
  int32_t start_line; // (inclusif)
  int32_t end_line;   // (exclusif)
  bmp_mapped_image_t *ref_img;
} thread_filter_args_t;

// Toutes les fonction suivantes prennent un parametre de type void *arg pour la
// compataibilité avec des appels par des threads mais qui attend en réalité un
// pointeur vers une structure de type thread_filter_args_t afin d'appliqué le
// filtre spécifié dans leur nom sur l'image en mémoire pointé par img entre les
// lignes start_Line inclusif et end_line exclusif. Cette fonction modifie
// l'image pointé par img. Elle utilise l'image pointé par ref_img comme
// référence pour les valeurs des pixels voisins.
void *blurbox_filter(void *arg);
void *gaussian_blur_filter(void *arg);
void *gaussian_blur5x5_filter(void *arg);
void *sharpen_filter(void *arg);
void *sharpen_intense_filter(void *arg);
void *edge_detect_filter(void *arg);
void *sobel_horizontal_filter(void *arg);
void *sobel_vertical_filter(void *arg);
void *laplacian_filter(void *arg);
void *emboss_filter(void *arg);
void *emboss_intense_filter(void *arg);
void *motion_blur_filter(void *arg);
void *motion_blur_horizontal_filter(void *arg);
void *motion_blur_vertical_filter(void *arg);
void *oil_painting_filter(void *arg);
void *crosshatch_filter(void *arg);

// Toutes les fonction qui prennent un parametre de type void *arg pour la
// compataibilité avec des appels par des threads mais qui attend en réalité un
// pointeur vers une structure de type thread_filter_args_t afin d'appliqué le
// filtre spécifié dans leur nom sur l'image en mémoire pointé par img entre les
// lignes start_Line inclusif et end_line exclusif. Cette fonction modifie
// l'image pointé par img.
void *identity_filter(void *arg);
void *blackAndWhite_filter(void *arg);
void *red_filter(void *arg);
void *green_filter(void *arg);
void *blue_filter(void *arg);
void *cyan_filter(void *arg);
void *magenta_filter(void *arg);
void *yellow_filter(void *arg);
void *sepia_filter(void *arg);
void *invert_filter(void *arg);

#endif
#include "bmp.h"
#include <stdio.h>

// arg est un pointeur vers thread_filter_args_t
void *identity_filter(void *arg) {
  thread_filter_args_t *args = (thread_filter_args_t *)arg;
  bmp_mapped_image_t *img = args->img;
  (void)img;
  return nullptr;
}
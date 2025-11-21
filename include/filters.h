#ifndef FILTERS_H
#define FILTERS_H

#include "bmp.h"

// filter, short_flag, long_flag, description, function

#define OPT_TO_REQUEST_SIMPLE_FILTERS                                          \
  OPT_TO_REQUEST_SIMPLE_FILTER(identity, "id", "identity",                     \
                               "Apply no filter to the image",                 \
                               identity_filter)                                \
  OPT_TO_REQUEST_SIMPLE_FILTER(blackAndWhite, "bw", "blackAndWhite",           \
                               "Apply a black and white filter to the image",  \
                               blackAndWhite_filter)                           \
  OPT_TO_REQUEST_SIMPLE_FILTER(toto, "t", "toto",                              \
                               "Apply a toto filter to the image",             \
                               blackAndWhite_filter)

#endif
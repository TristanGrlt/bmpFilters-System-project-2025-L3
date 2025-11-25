#ifndef FILTERS_H
#define FILTERS_H

#include "bmp.h"

// filter, short_flag, long_flag, description, function

// Liste des filtres implémentés par le serveur et disponibles à la demande du
// client. Ces filtres sont définis à l'aide de listes de macros, permettant de
// centraliser la déclaration des filtres. La définition et l'indéfinition
// successives de ces macros permettent à différents endroits du code
// d'exploiter ces informations afin d'automatiser, via le préprocesseur, la
// génération de données redondantes pour chaque filtre. Ces macros sont
// utilisées à la fois par le client et le serveur, et facilitent également la
// création d'un gestionnaire d'arguments en ligne de commande via le module
// opt_to_request.h.

#define OPT_TO_REQUEST_SIMPLE_FILTERS                                          \
  OPT_TO_REQUEST_SIMPLE_FILTER(identity, "id", "identity",                     \
                               "Apply no filter to the image",                 \
                               identity_filter)                                \
  OPT_TO_REQUEST_SIMPLE_FILTER(blackAndWhite, "bw", "blackAndWhite",           \
                               "Apply a black and white filter to the image",  \
                               blackAndWhite_filter)                           \
  OPT_TO_REQUEST_SIMPLE_FILTER(red, "r", "red", "Keep only red channel",       \
                               red_filter)                                     \
  OPT_TO_REQUEST_SIMPLE_FILTER(green, "g", "green", "Keep only green channel", \
                               green_filter)                                   \
  OPT_TO_REQUEST_SIMPLE_FILTER(blue, "b", "blue", "Keep only blue channel",    \
                               blue_filter)                                    \
  OPT_TO_REQUEST_SIMPLE_FILTER(cyan, "c", "cyan", "Keep cyan (blue + green)",  \
                               cyan_filter)                                    \
  OPT_TO_REQUEST_SIMPLE_FILTER(magenta, "m", "magenta",                        \
                               "Keep magenta (red + blue)", magenta_filter)    \
  OPT_TO_REQUEST_SIMPLE_FILTER(yellow, "y", "yellow",                          \
                               "Keep yellow (red + green)", yellow_filter)     \
  OPT_TO_REQUEST_SIMPLE_FILTER(sepia, "sep", "sepia",                          \
                               "Apply sepia tone effect", sepia_filter)        \
  OPT_TO_REQUEST_SIMPLE_FILTER(invert, "inv", "invert",                        \
                               "Invert all colors (negative)", invert_filter)

#define OPT_TO_REQUEST_COMPLEX_FILTERS                                         \
  OPT_TO_REQUEST_COMPLEX_FILTER(                                               \
      blur, "bl", "blur", "Apply a box blur filter (3x3)", blurbox_filter)     \
  OPT_TO_REQUEST_COMPLEX_FILTER(gaussian_blur, "gb", "gaussian-blur",          \
                                "Apply a gaussian blur filter (3x3)",          \
                                gaussian_blur_filter)                          \
  OPT_TO_REQUEST_COMPLEX_FILTER(gaussian_blur5x5, "gb5", "gaussian-blur-5x5",  \
                                "Apply a strong gaussian blur (5x5)",          \
                                gaussian_blur5x5_filter)                       \
  OPT_TO_REQUEST_COMPLEX_FILTER(sharpen, "sh", "sharpen",                      \
                                "Apply a sharpen filter", sharpen_filter)      \
  OPT_TO_REQUEST_COMPLEX_FILTER(sharpen_intense, "shi", "sharpen-intense",     \
                                "Apply an intense sharpen filter",             \
                                sharpen_intense_filter)                        \
  OPT_TO_REQUEST_COMPLEX_FILTER(edge_detect, "ed", "edge-detect",              \
                                "Apply edge detection", edge_detect_filter)    \
  OPT_TO_REQUEST_COMPLEX_FILTER(sobel_h, "soh", "sobel-horizontal",            \
                                "Apply Sobel horizontal edge detection",       \
                                sobel_horizontal_filter)                       \
  OPT_TO_REQUEST_COMPLEX_FILTER(sobel_v, "sov", "sobel-vertical",              \
                                "Apply Sobel vertical edge detection",         \
                                sobel_vertical_filter)                         \
  OPT_TO_REQUEST_COMPLEX_FILTER(laplacian, "lap", "laplacian",                 \
                                "Apply Laplacian edge detection",              \
                                laplacian_filter)                              \
  OPT_TO_REQUEST_COMPLEX_FILTER(emboss, "em", "emboss",                        \
                                "Apply an emboss effect", emboss_filter)       \
  OPT_TO_REQUEST_COMPLEX_FILTER(emboss_intense, "emi", "emboss-intense",       \
                                "Apply an intense emboss effect",              \
                                emboss_intense_filter)                         \
  OPT_TO_REQUEST_COMPLEX_FILTER(motion_blur, "mb", "motion-blur",              \
                                "Apply diagonal motion blur",                  \
                                motion_blur_filter)                            \
  OPT_TO_REQUEST_COMPLEX_FILTER(                                               \
      motion_blur_h, "mbh", "motion-blur-horizontal",                          \
      "Apply horizontal motion blur", motion_blur_horizontal_filter)           \
  OPT_TO_REQUEST_COMPLEX_FILTER(motion_blur_v, "mbv", "motion-blur-vertical",  \
                                "Apply vertical motion blur",                  \
                                motion_blur_vertical_filter)                   \
  OPT_TO_REQUEST_COMPLEX_FILTER(oil_painting, "oil", "oil-painting",           \
                                "Apply oil painting effect",                   \
                                oil_painting_filter)                           \
  OPT_TO_REQUEST_COMPLEX_FILTER(crosshatch, "ch", "crosshatch",                \
                                "Apply crosshatch drawing effect",             \
                                crosshatch_filter)

#endif

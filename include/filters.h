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
                               blackAndWhite_filter)

#define OPT_TO_REQUEST_COMPLEX_FILTERS                                         \
  OPT_TO_REQUEST_COMPLEX_FILTER(                                               \
      blur, "b", "blur", "Apply a blur filter to the image", blurbox_filter)

#endif

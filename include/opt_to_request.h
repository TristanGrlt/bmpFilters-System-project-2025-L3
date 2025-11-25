#ifndef OPT_TO_REQUEST_H
#define OPT_TO_REQUEST_H

// Ce module centralise la déclaration des filtres d'image disponibles à la
// demande du client. Les filtres sont définis à l'aide de listes de macros,
// permettant d'automatiser la génération de code redondant via le
// préprocesseur. Cette approche facilite la maintenance et l'extension des
// filtres, tout en assurant la cohérence entre le client et le serveur.

// Chaque filtre est déclaré à l'aide de la macro OPT_TO_REQUEST_SIMPLE_FILTER,
// qui prend en paramètres :
// - filter : identifiant du filtre (nom interne utilisé dans le code)
// - short_flag : option courte pour la ligne de commande (ex : "bw")
// - long_flag : option longue pour la ligne de commande (ex : "blackAndWhite")
// - description : description textuelle du filtre (dans l'aide)
// - function : nom de la fonction réalisant le traitement du filtre

// L'utilisation successive de la définition et de l'indéfinition de la macro
// OPT_TO_REQUEST_SIMPLE_FILTER permet d'exploiter ces informations à différents
// endroits du code, notamment pour :
//   - Générer automatiquement les structures de données associées à chaque
//   filtre
//   - Créer dynamiquement le gestionnaire d'arguments en ligne de commande
//   - Assurer la synchronisation entre les options disponibles côté client et
//   serveur

// - OPT_TO_REQUEST_SIMPLE_FILTERS : liste l'ensemble des filtres disponibles,
// chacun étant déclaré via la macro OPT_TO_REQUEST_SIMPLE_FILTER.
// - OPT_TO_REQUEST_SIMPLE_FILTER : macro paramétrée pour la déclaration d'un
// filtre individuel.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "filters.h"
#include <linux/limits.h>

#define REQUEST_FIFO_SIZE 10
#define MAX_PATH_LENGTH 4096

#define REQUEST_FIFO_PATH "/filter_request_fifo"
#define REQUEST_EMPTY_PATH "/mutex_empty"
#define REQUEST_FULL_PATH "/mutex_full"
#define REQUEST_WRITE_PATH "/mutex_write"

#ifndef OPT_TO_REQUEST_SHORT_PREFIX
#define OPT_TO_REQUEST_SHORT_PREFIX "-"
#endif

#ifndef OPT_TO_REQUEST_LONG_PREFIX
#define OPT_TO_REQUEST_LONG_PREFIX "--"
#endif

#ifndef OPT_TO_REQUEST_SHORT_HELP
#define OPT_TO_REQUEST_SHORT_HELP "h"
#endif

#ifndef OPT_TO_REQUEST_LONG_HELP
#define OPT_TO_REQUEST_LONG_HELP "help"
#endif

#define OPT_TO_REQUEST_SIMPLE_FILTER(filter, ...) filter,
#define OPT_TO_REQUEST_COMPLEX_FILTER(filter, ...) filter,

typedef enum {
#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
  OPT_TO_REQUEST_SIMPLE_FILTERS
#endif
#undef OPT_TO_REQUEST_SIMPLE_FILTER
#ifdef OPT_TO_REQUEST_COMPLEX_FILTERS
      OPT_TO_REQUEST_COMPLEX_FILTERS
#endif
#undef OPT_TO_REQUEST_COMPLEX_FILTER
} filter_t;

// STRUCTURE
typedef struct {
  char *input;
  char *output;
  filter_t filter;
} arguments_t;

typedef struct {
  pid_t pid;
  char path[PATH_MAX];
  filter_t filter;
} filter_request_t;

typedef struct {
  int write;
  filter_request_t buffer[REQUEST_FIFO_SIZE];
} request_t;

// process_options_to_request: traite les arguments de la liste de chaine de
// carractère pointé par argv de taille argc et remplit la strucuture pointé par
// arg en focntion des arguments lue dans argv. La focntion est en partie
// générer automatiquement à l'aide des macros définisant la liste des
// filtres/arguments disponible. Renvoit 0 en cas de succes, -1 dans tout les
// autres cas.
int process_options_to_request(int argc, char *argv[], arguments_t *arg);

// print_help: écrit sur la sortie standard l'aide pour l'utilisation du
// programme. Le paramètre exec_name correspond au nom de l'exécutable affiché
// dans l'aide. L'aide est générer et formaté automatiquement à l'aide des
// macros définisant la liste des filtres/arguments disponible
void print_help(const char *exec_name);

#endif
#ifndef CONFIG_H
#define CONFIG_H

#include <semaphore.h>
#include <stdbool.h>

#define CONFIG_FILE_PATH_LOCAL "./bmp_server.conf"
#define CONFIG_FILE_PATH_SYSTEM "/etc/bmp_server.conf"

#define DEFAULT_MAX_WORKERS 10
#define DEFAULT_MIN_THREADS 4
#define DEFAULT_MAX_THREADS 8

#define ABSOLUTE_MIN_THREADS 1
#define ABSOLUTE_MAX_THREADS 32
#define ABSOLUTE_MAX_WORKERS 100

typedef struct {
  int max_workers;
  int min_threads;
  int max_threads;
  bool is_valid;
} server_config_t;

// Initialise la configuration avec des valeurs par défaut

// config_init_default: Initialise la configuration du server avec les valeurs
// par défaut définit par les constante nomé DEFAULT_
void config_init_default(server_config_t *config);

// config_load: charge la configuration du server en lisant la configuration
// définit par le fichier de chemin pointé par filepath. Retourne 0 en cas de
// succès sinon -1
int config_load(server_config_t *config, const char *filepath);

// config_validate: vérifie la validité de la configuration courament définit en
// affectant à config->is_valid la valeur true si la configuration est valide,
// false sinon
bool config_validate(server_config_t *config);

// Calcule le nombre de thread optimal en focntion des valeurs définit pour le
// nombre maximum ou minimum de threads selon la taille file_size d'un fichier à
// traiter
int config_get_thread_count(const server_config_t *config, off_t file_size);

#endif

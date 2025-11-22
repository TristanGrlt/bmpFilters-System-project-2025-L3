#ifndef FULL_IO_H
#define FULL_IO_H

#include <stddef.h>
#include <sys/types.h>

// full_read: Tente de lire count octets depuis le descripteur de fichier pointé
// par fd et les place dans l'espace mémoire pointé par buf en supposant qu'il
// est au moins de taille count
ssize_t full_read(int fd, void *buf, size_t count);

// full_read: Tente de d'écrire count octets dans le fichier référencé par le
// descripteur de fichier pointé par fd depuis  l'espace mémoire pointé par buf
// en supposant qu'il est au moins de taille count
ssize_t full_write(int fd, const void *buf, size_t count);

#endif

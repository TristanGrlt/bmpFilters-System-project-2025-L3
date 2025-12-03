`./server -f` lance le server en foreground sinon deamon

## Si deamon, quelque commande :

```bash
# Voir les logs
journalctl -t bmp_server

# Voir les logs en temps réel
journalctl -t bmp_server -f

# Voir les logs seulement s'il y a des erreurs
journalctl -t bmp_server -p err
```

```bash
# Envoie SIGINT (arret du server)
kill -SIGINT $(cat /tmp/bmp_server.pid)
```

`ps aux | grep bmp_server` : voire les server bmp qui run

Arrêt propre (SIGTERM)

```bash
kill -SIGINT $(cat /tmp/bmp_server.pid)
```

Arrêt forcé (SIGKILL) - pas propre !

```bash
kill -9 $(cat /tmp/bmp_server.pid)
```

Rafaichissement du fichier de config (SIGHUP)

```bash
kill -SIGHUP $(cat /tmp/bmp_server.pid)
```

## voire nombre de worker et de zombie du server deamon en cour

```bash
watch -n 0.01 "echo 'Workers:' \$(pgrep -P \$(cat /tmp/bmp_server.pid) | wc -l) '| Zombies:' \$(ps --ppid \$(cat /tmp/bmp_server.pid) -o stat= 2>/dev/null | grep -c Z)"
```

- Flous (3 variations)
  - blur (-bl) - Flou en boîte 3x3 simple
  - gaussian-blur (-gb) - Flou gaussien 3x3 (plus doux)
  - gaussian-blur-5x5 (-gb5) - Flou gaussien 5x5 (très fort)
  - Netteté (2 variations)
  - sharpen (-sh) - Accentuation modérée
  - sharpen-intense (-shi) - Accentuation forte
- Détection de contours (4 variations)
  - edge-detect (-ed) - Détection de contours générale
  - sobel-horizontal (-soh) - Contours horizontaux (Sobel)
  - sobel-vertical (-sov) - Contours verticaux (Sobel)
  - laplacian (-lap) - Détection Laplacienne
- Effets 3D (2 variations)
  - emboss (-em) - Embossage modéré
  - emboss-intense (-emi) - Embossage fort
- Flous de mouvement (3 directions)
  - motion-blur (-mb) - Flou diagonal
  - motion-blur-horizontal (-mbh) - Flou horizontal
  - motion-blur-vertical (-mbv) - Flou vertical
- Effets artistiques (2 variations)
  - oil-painting (-oil) - Effet peinture à l'huile (5x5)
  - crosshatch (-ch) - Effet hachures croisées

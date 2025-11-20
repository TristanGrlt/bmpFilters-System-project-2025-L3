`./server -f` lance le server en foreground sinon deamon

## Si deamon, quelque commande importante :

```bash
# Voir tous les logs de votre programme
journalctl -t bmp_server

# Voir les logs en temps réel (comme tail -f)
journalctl -t bmp_server -f

# Voir les logs seulement s'il y a des erreurs
journalctl -t bmp_server -p err
```

```bash
# Envoie SIGINT (ce que votre code attend pour s'arrêter proprement)
kill -SIGINT $(cat /tmp/bmp_server.pid)
```

`ps aux | grep bmp_server` : voire les server bmp qui run

Méthode 1: Arrêt propre (SIGTERM)
`kill $(cat /tmp/bmp_server.pid)`

Méthode 2: Arrêt forcé (SIGKILL) - pas propre !
`kill -9 $(cat /tmp/bmp_server.pid)`

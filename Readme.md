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

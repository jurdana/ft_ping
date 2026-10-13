# ft_ping

Réimplémentation en C de la commande `ping`, projet du cursus **École 42**.
Le programme envoie des paquets **ICMP Echo Request** vers un hôte et mesure
le temps d'aller-retour (RTT) des réponses. Il reproduit le comportement et
la sortie de l'implémentation de référence **inetutils-2.0**.

---

## Sommaire

- [Fonctionnalités](#fonctionnalités)
- [Compilation](#compilation)
- [Utilisation](#utilisation)
- [Options](#options)
- [Architecture du code](#architecture-du-code)
- [Fonctionnement technique](#fonctionnement-technique)
- [Gestion mémoire & Valgrind](#gestion-mémoire--valgrind)
- [Exemples de tests](#exemples-de-tests)
- [Scripts de test fournis](#scripts-de-test-fournis)
- [Notes & choix d'implémentation](#notes--choix-dimplémentation)

---

## Fonctionnalités

### Partie obligatoire
- Envoi de paquets ICMP Echo Request et réception des Echo Reply
- Résolution d'adresse IPv4 et de nom d'hôte (FQDN) via `getaddrinfo`
- Calcul du RTT (min / avg / max / stddev) et statistiques de fin
- Arrêt propre sur `Ctrl-C` (SIGINT) avec affichage des statistiques
- Gestion des erreurs sans crash (pas de segfault, bus error, double free…)
- Option `-v` (verbose) : affiche l'identifiant ICMP et le détail des
  paquets d'erreur (Time Exceeded, Destination Unreachable) avec dump
  complet de l'en-tête IP/ICMP
- Option `-?` : affiche l'aide

### Bonus implémentés
- `-c` : nombre de paquets à envoyer
- `-s` : taille du payload
- `-n` : pas de résolution DNS inverse
- `-w` : délai global avant arrêt (timeout)
- `-W` : délai d'attente d'une réponse (linger)

---

## Compilation

```bash
make          # compile le binaire ft_ping
make clean    # supprime les fichiers objets (obj/)
make fclean   # supprime objets + binaire
make re       # recompile tout de zéro
```

Le Makefile ne recompile que les fichiers modifiés. Le binaire est lié avec
`-lm` (fonction `sqrt` pour l'écart-type) et compilé avec
`-Wall -Wextra -Werror`.

> **Note :** `ft_ping` utilise des *raw sockets*, réservées au
> superutilisateur. Il doit donc être lancé avec `sudo`.

---

## Utilisation

```bash
sudo ./ft_ping [OPTIONS] HOST
```

`HOST` peut être une adresse IPv4 (`8.8.8.8`) ou un nom de domaine
(`google.com`).

```bash
sudo ./ft_ping 8.8.8.8
sudo ./ft_ping -c 4 google.com
sudo ./ft_ping -v -c 2 10.0.2.99

sudo ~/ref/inetutils-2.0/ping/ping 8.8.8.8
sudo ~/ref/inetutils-2.0/ping/ping -c 4 google.com
sudo ~/ref/inetutils-2.0/ping/ping -v -c 2 10.0.2.99

./ft_ping 8.8.8.8 

sudo ./ft_ping -?

sudo ./ft_ping 8.8.8.8

sudo ./ft_ping -c 3 8.8.8.8

sudo ./ft_ping -c 2 google.com

sudo ./ft_ping nimportequoi.invalid

sudo ./ft_ping -v -c 2 10.0.2.99

sudo ./ft_ping -v google.com

sudo ./ft_ping -s 20 -c 1 8.8.8.8

sudo ./ft_ping -v -c 1 10.0.2.99

sudo ./ft_ping -v -n -c 1 10.0.2.99

time sudo ./ft_ping -w 3 8.8.8.8

time sudo ./ft_ping -W 1 -c 1 10.255.255.1

time sudo ./ft_ping -W 3 -c 1 10.255.255.1

ttl diff
# terminal 1
sudo tcpdump -i any -n -v icmp

# terminal 2
sudo ./ft_ping -c 2 8.8.8.8

sudo ~/ref/inetutils-2.0/ping/ping
```

---

## Options

| Option | Argument | Description |
|--------|----------|-------------|
| `-v`   | —        | Mode verbose : id ICMP + détail des paquets d'erreur |
| `-?`   | —        | Affiche l'aide et quitte |
| `-c`   | NUMBER   | Arrête après NUMBER paquets envoyés |
| `-s`   | NUMBER   | Taille du payload en octets (défaut 56, max 65399) |
| `-n`   | —        | N'effectue pas la résolution DNS inverse |
| `-w`   | N        | Arrête le programme après N secondes |
| `-W`   | N        | Secondes à attendre une réponse (défaut 10) |

---

## Architecture du code

```
ft_ping/
├── Makefile
├── header/
│   └── ping.h          # struct t_ping, prototypes, includes
└── src/
    ├── main.c          # main, boucle principale, handler de signal
    ├── init.c          # init struct, parsing des arguments, résolution, socket
    ├── packet.c        # construction du paquet ICMP, checksum
    ├── send.c          # envoi du paquet (sendto)
    ├── reception.c     # réception (recvfrom), décodage, affichage des erreurs
    └── print.c         # en-tête PING et statistiques finales
```

Toute l'exécution tourne autour d'une unique structure `t_ping` (allouée sur
la pile dans le `main`) qui contient la configuration issue des arguments,
les ressources réseau (socket, id), l'adresse résolue et les accumulateurs de
statistiques.

---

## Fonctionnement technique

1. **Parsing** (`getopt`) — remplit la configuration et valide chaque
   argument numérique (détection de débordement via `strtol`).
2. **Résolution** (`getaddrinfo`) — transforme l'hôte en adresse binaire.
3. **Socket brute** (`socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)`) — configurée
   avec `SO_RCVTIMEO` (timeout de réception) et `SO_BROADCAST` (pour autoriser
   l'envoi vers une adresse de diffusion comme `255.255.255.255`).
4. **Boucle** : construction du paquet → `sendto` → `recvfrom` → décodage →
   attente d'une seconde → répétition.
5. **Construction du paquet** : en-tête ICMP (type 8, id, séquence) + payload.
   Les premiers octets du payload contiennent un **timestamp** (`gettimeofday`)
   servant à calculer le RTT au retour. Le **checksum** Internet est calculé
   à la main sur l'ensemble du paquet.
6. **Décodage** : on saute l'en-tête IP (via le champ IHL) pour atteindre
   l'en-tête ICMP, on vérifie que la réponse porte bien notre id, puis on
   calcule le RTT à partir du timestamp renvoyé.
7. **Statistiques** affichées à l'arrêt (`Ctrl-C`, `-c` ou `-w`), basées sur
   les accumulateurs (somme et somme des carrés des RTT pour l'écart-type).

---

## Gestion mémoire & Valgrind

Le programme ne présente **aucune fuite imputable au code** et **aucune
erreur mémoire**. Sur tous les chemins testés, Valgrind affiche :

```
definitely lost: 0 bytes in 0 blocks
ERROR SUMMARY: 0 errors from 0 contexts
```

### À propos du « still reachable »

Lorsqu'un hôte est résolu par nom ou qu'une résolution DNS inverse a lieu
(`getnameinfo`), Valgrind rapporte un bloc **`still reachable`** d'environ
1,7 Ko :

```
still reachable: 1,769 bytes in 4 blocks
```

**Ceci est normal et n'est pas une fuite du programme.** Ces allocations sont
faites en interne par la **glibc** lors du chargement dynamique des
bibliothèques de résolution de noms (NSS, via `dlopen`). Elles sont hors du
contrôle du programme, ne sont jamais « perdues » (`definitely lost: 0`), et
sont libérées par le système d'exploitation à la fin du processus.
L'implémentation de référence inetutils-2.0 présente exactement le même
comportement (elle en laisse même davantage). Les piles d'appels de ces blocs
montrent uniquement des fonctions internes (`_dl_open`, `_dl_map_object`,
`strdup` dans `dl-cache`), jamais de code de `ft_ping`.

---

## Exemples de tests

### Ping simple vers 8.8.8.8

```
$ sudo ./ft_ping -c 3 8.8.8.8
PING 8.8.8.8 (8.8.8.8): 56 data bytes
64 bytes from 8.8.8.8: icmp_seq=0 ttl=63 time=11.163 ms
64 bytes from 8.8.8.8: icmp_seq=1 ttl=63 time=11.561 ms
64 bytes from 8.8.8.8: icmp_seq=2 ttl=63 time=10.888 ms
--- 8.8.8.8 ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max/stddev = 10.888/11.204/11.561/0.280 ms
```

Comparaison avec la référence (sortie identique au format près, seuls les
RTT et l'id varient naturellement d'une exécution à l'autre) :

```
$ sudo ~/ref/inetutils-2.0/ping/ping -c 3 8.8.8.8
PING 8.8.8.8 (8.8.8.8): 56 data bytes
64 bytes from 8.8.8.8: icmp_seq=0 ttl=63 time=11.491 ms
64 bytes from 8.8.8.8: icmp_seq=1 ttl=63 time=10.884 ms
64 bytes from 8.8.8.8: icmp_seq=2 ttl=63 time=11.345 ms
--- 8.8.8.8 ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max/stddev = 10.884/11.240/11.491/0.259 ms
```

### Erreur ICMP en mode verbose

```
$ sudo ./ft_ping -v -c 1 10.0.2.99
PING 10.0.2.99 (10.0.2.99): 56 data bytes, id 0x139e = 5022
92 bytes from work (10.0.2.15): Destination Host Unreachable
IP Hdr Dump:
 4500 0054 b42c 4000 4001 6e0b 0a00 020f 0a00 0263
Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src	Dst	Data
 4  5  00 0054 b42c   2 0000  40  01 6e0b 10.0.2.15  10.0.2.99
ICMP: type 8, code 0, size 64, id 0x139e, seq 0x0000
--- 10.0.2.99 ping statistics ---
1 packets transmitted, 0 packets received, 100% packet loss
```

La même commande sur la référence produit une sortie identique (hors id et
checksum, qui varient à chaque exécution).

---

## Scripts de test fournis

Deux scripts accompagnent le projet (nécessitent `sudo`) :

- **`compare_ft_ping.sh`** — lance une série de cas sur `ft_ping` puis sur
  inetutils-2.0, l'un sous l'autre, avec `cat -A` pour visualiser espaces et
  tabulations (vérification de l'indentation stricte).
- **`break_ft_ping.sh`** — fuzzing : une quarantaine d'entrées absurdes
  (chaînes vides, valeurs négatives, overflow, IP malformées, options
  collées…) exécutées sous Valgrind pour vérifier qu'aucune ne provoque de
  crash ni de fuite.

> Les chemins `PING` et `REF` en tête de ces scripts sont à adapter à votre
> installation.

---

## Notes & choix d'implémentation

- **Référence** : la sortie reproduit celle de `inetutils-2.0` (et non celle
  d'iputils, qui diffère sur plusieurs points : `icmp_seq` démarre à 0,
  `stddev` au lieu de `mdev`, etc.).
- **Résolution DNS inverse** : conformément au sujet, elle est exemptée de
  l'exigence d'indentation stricte. Les lignes de réponse normales affichent
  l'adresse IP (comme inetutils) ; `-n` désactive la résolution inverse, ce
  qui est visible notamment sur l'affichage de l'hôte dans les messages
  d'erreur (`work (10.0.2.15)` sans `-n` → `10.0.2.15` avec).
- **Codes de sortie** : `0` si au moins un paquet reçu, `1` si aucune réponse,
  `64` pour une erreur d'usage (arguments), `1` pour une valeur invalide ou un
  hôte inconnu — alignés sur inetutils-2.0.
- **Timestamp dans le payload** : écrit uniquement si la taille (`-s`) est
  suffisante (≥ 16 octets) ; en deçà, la ligne de réponse omet le champ
  `time=`, comme la référence.
- **Adresse de diffusion** : `SO_BROADCAST` est activé pour autoriser le ping
  de `255.255.255.255`.

---

*Projet réalisé dans le cadre du cursus École 42.*

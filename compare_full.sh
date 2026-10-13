#!/bin/bash

# ============================================================
#  Comparaison exhaustive ft_ping vs inetutils-2.0
#  Lance chaque cas sur TON ping puis sur la REFERENCE.
#  cat -A rend visibles espaces ($) et tabulations (^I).
#  Usage : sudo ./compare_full.sh  (ou > out.txt 2>&1)
# ============================================================

PING=./ft_ping
REF=/home/trans/ref/inetutils-2.0/ping/ping
GOOD=8.8.8.8            # ip qui repond
DEAD=192.0.2.1         # ip non routable (selon reseau)
BADHOST=nimportequoi.invalid
GOODHOST=google.com
LAN=10.0.2.99          # ip LAN inexistante -> erreur ICMP (adapte)

# --- resolution de ~ pour REF ---
REF=$(eval echo "$REF")

if [ ! -f "$PING" ]; then echo "Erreur: $PING introuvable."; exit 1; fi
if [ ! -f "$REF" ]; then echo "Erreur: reference introuvable ($REF). Adapte REF."; exit 1; fi
if [ "$EUID" -ne 0 ]; then echo "Erreur: lance avec sudo."; exit 1; fi

sep() { echo "============================================================"; }

# run <description> <timeout_s> <args...>
run() {
    local desc="$1"; shift
    local tmout="$1"; shift
    echo ""
    sep
    echo "### $desc"
    echo "    args: $*"
    sep
    echo "----- [MON ft_ping] -----"
    if [ "$tmout" -gt 0 ]; then
        timeout --signal=INT "$tmout" "$PING" "$@" 2>&1 | cat -A
    else
        "$PING" "$@" 2>&1 | cat -A
    fi
    echo "(exit: ${PIPESTATUS[0]})"
    echo ""
    echo "----- [REFERENCE inetutils] -----"
    if [ "$tmout" -gt 0 ]; then
        timeout --signal=INT "$tmout" "$REF" "$@" 2>&1 | cat -A
    else
        "$REF" "$@" 2>&1 | cat -A
    fi
    echo "(exit: ${PIPESTATUS[0]})"
}

echo "######## COMPARAISON EXHAUSTIVE ft_ping vs inetutils-2.0 ########"
echo "Note: id, checksum, RTT, temps varient : normal."
echo "A matcher: format, texte, indentation, codes de sortie, comportement."

# =======================================================
echo ""; echo "########## 1. USAGE / AIDE / PERMISSIONS ##########"
# =======================================================
run "Aucun argument"                 0
run "Aide -?"                         0  -?
run "Option inconnue -z"              0  -z "$GOOD"
run "Option inconnue -x au milieu"    0  -c 1 -x "$GOOD"

# =======================================================
echo ""; echo "########## 2. PING DE BASE (bonne/mauvaise ip) ##########"
# =======================================================
run "Good ip -c 3"                    0  -c 3 "$GOOD"
run "Bad ip (non routable) -c 2"      0  -c 2 "$DEAD"
run "Good hostname -c 2"              0  -c 2 "$GOODHOST"
run "Bad hostname"                    0  "$BADHOST"
run "Host vide"                       0  ""

# =======================================================
echo ""; echo "########## 3. OPTION -v ##########"
# =======================================================
run "-v good ip -c 2"                 0  -v -c 2 "$GOOD"
run "-v bad ip -c 2"                  0  -v -c 2 "$DEAD"
run "-v bad hostname"                 0  -v "$BADHOST"
run "-v erreur ICMP LAN -c 2"         0  -v -c 2 "$LAN"
run "-v -n erreur ICMP LAN -c 2"      0  -v -n -c 2 "$LAN"

# =======================================================
echo ""; echo "########## 4. OPTION -n ##########"
# =======================================================
run "-n good ip -c 2"                 0  -n -c 2 "$GOOD"
run "-n good hostname -c 2"           0  -n -c 2 "$GOODHOST"

# =======================================================
echo ""; echo "########## 5. OPTION -c (count) ##########"
# =======================================================
run "-c 1"                            0  -c 1 "$GOOD"
run "-c 3"                            0  -c 3 "$GOOD"
run "-c abc (lettres)"                0  -c abc "$GOOD"
run "-c 5abc (residu)"                0  -c 5abc "$GOOD"
run "-c 3.5 (virgule)"                0  -c 3.5 "$GOOD"
run "-c +5 (plus)"                    8  -c +5 "$GOOD"
run "-c -5 (negatif)"                 6  -c -5 "$GOOD"
run "-c sans argument"                0  -c
run "-c double (-c 2 -c 1)"           8  -c 2 -c 1 "$GOOD"

# =======================================================
echo ""; echo "########## 6. OPTION -s (size) ##########"
# =======================================================
run "-s 0"                            6  -s 0 -c 1 "$GOOD"
run "-s 8 (sous timeval)"             6  -s 8 -c 1 "$GOOD"
run "-s 20"                           6  -s 20 -c 1 "$GOOD"
run "-s 100"                          6  -s 100 -c 1 "$GOOD"
run "-s 65399 (max)"                  8  -s 65399 -c 1 "$GOOD"
run "-s 65400 (au-dessus max)"        0  -s 65400 "$GOOD"
run "-s abc (lettres)"                0  -s abc "$GOOD"
run "-s 100abc (residu)"              0  -s 100abc "$GOOD"
run "-s +100 (plus)"                  6  -s +100 -c 1 "$GOOD"
run "-s -100 (negatif)"               0  -s -100 "$GOOD"
run "-s overflow positif"            0  -s 999999999999999999999 "$GOOD"
run "-s sans argument"                0  -s

# =======================================================
echo ""; echo "########## 7. OPTION -w (timeout global) ##########"
# =======================================================
run "-w 2"                            6  -w 2 "$GOOD"
run "-w 0"                            6  -w 0 "$GOOD"
run "-w +3"                           8  -w +3 "$GOOD"
run "-w -5 (negatif)"                 0  -w -5 "$GOOD"
run "-w abc"                          0  -w abc "$GOOD"
run "-w overflow (INT_MAX+1)"         0  -w 2147483648 "$GOOD"
run "-w sans argument"                0  -w

# =======================================================
echo ""; echo "########## 8. OPTION -W (linger) ##########"
# =======================================================
run "-W 1 bad ip -c 1"                6  -W 1 -c 1 "$DEAD"
run "-W 0"                            6  -W 0 -c 1 "$GOOD"
run "-W +2"                           8  -W 2 -c 1 "$GOOD"
run "-W -5 (negatif)"                 0  -W -5 "$GOOD"
run "-W abc"                          0  -W abc "$GOOD"
run "-W overflow"                     0  -W 2147483648 "$GOOD"
run "-W sans argument"                0  -W

# =======================================================
echo ""; echo "########## 9. OPTIONS MELANGEES / COMBINEES ##########"
# =======================================================
run "-vn collees"                     6  -vn -c 1 "$GOOD"
run "-nvc collees"                    6  -nvc 1 "$GOOD"
run "Toutes: -v -n -c -s -w -W"       8  -v -n -c 2 -s 20 -w 5 -W 1 "$GOOD"
run "Flags apres host"                6  "$GOOD" -v -c 1
run "Flags des deux cotes"            6  -v "$GOOD" -c 1
run "Separateur --"                   6  -c 1 -- "$GOOD"
run "-c 1 -s 20 hostname"             6  -c 1 -s 20 "$GOODHOST"
run "-v -c 1 bad ip (erreur)"         0  -v -c 1 "$DEAD"

# =======================================================
echo ""; echo "########## 10. IP / HOSTS MALFORMES ##########"
# =======================================================
run "IP incomplete"                   6  -c 1 8.8.8
run "IP hors limites"                 6  -c 1 999.999.999.999
run "IP trop de champs"               6  -c 1 1.2.3.4.5
run "localhost"                       6  -c 1 127.0.0.1
run "IP broadcast 255.255.255.255"    6  -c 2 255.255.255.255
run "Deux hosts"                      6  -c 1 "$GOOD" 1.1.1.1

echo ""
sep
echo "Fin. Copie toute la sortie pour verification."
sep

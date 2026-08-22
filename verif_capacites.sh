#!/usr/bin/env bash
#
# verif_capacites.sh — LA GARDE DES CAPACITES ANNONCEES (2026-08-22)
# =============================================================================
# POURQUOI IL EXISTE
#
#   « Un drapeau a `true` n'est pas une intention : c'est une reponse que du
#     code va CROIRE, et sur laquelle il va choisir un chemin. »
#
#   Neuf capacites de NKRHI/NKRenderer etaient annoncees et creuses : un champ
#   qu'on regle et qui ne fait rien, une virtuelle qui rend un chiffre credible
#   au lieu d'une erreur, une texture chargee que personne n'echantillonne.
#   Aucune ne casse une compilation. Aucune ne fait rougir un banc. Elles ne se
#   voient que si quelqu'un les cherche — donc jamais.
#
# LE DESSIN, ET IL EST LE POINT ENTIER
#
#   ⚠️ CE DETECTEUR N'A PAS LE DROIT DE JUGER. Il n'a le droit que d'EXIGER UN
#   CLASSEMENT. C'est exactement le dessin de config/bancs.list, et pour la
#   meme raison mesuree : la premiere passe sort des dizaines de candidats. Un
#   outil qui les CRIE est desactive avant vendredi — et on aurait reconstruit
#   le probleme qu'il devait resoudre.
#
#   Jour 1  : on classe les candidats, et l'outil est VERT.
#   Jour 2+ : il devient ROUGE sur le candidat suivant — c'est-a-dire exactement
#             le jour ou il sert.
#
#   Un detecteur qui pretend savoir si une capacite est implementee a tort une
#   fois sur trois, et on l'eteint. Un detecteur qui dit « ce symbole n'est pas
#   classe » a toujours raison : c'est un fait sur le fichier de classement,
#   pas un jugement sur le code.
#
# CE QU'IL DIT, ET CE QU'IL NE DIRA JAMAIS
#   il dit        « ce champ est ecrit et jamais lu »    — exact, verifiable
#   il ne dit PAS « cette capacite n'est pas implementee » — ce n'est pas le
#                 meme enonce, et le second n'est pas mesurable par du texte.
#
# IL NE REFUSE PAS LE COMMIT (arbitrage de Rodolf, R3-b)
#   Un controle qui bloque se contourne par `--no-verify`, en silence, par
#   quelqu'un de presse — et personne ne sait jamais qu'il a ete contourne. Un
#   banc rouge, lui, reste rouge et SE VOIT. On echange un blocage qu'on peut
#   faire taire contre un signal qu'on ne peut pas.
#   => code de sortie 4 ; gitcommit.sh CRIE le 4 et laisse passer le commit.
#
# LA REGLE (a) — LA LIGNE DE PARTAGE DE `vision-assumee`
#   capacite documentee, sans drapeau ............... `vision-assumee` autorise
#   capacite dont un drapeau annonce `true` a du code  `vision-assumee` INTERDIT
#   Un `true` est une promesse faite au CODE, pas au lecteur. Deux sorties
#   seulement : le drapeau rend `false`, ou la capacite s'implemente. La classer
#   et passer n'est pas un troisieme choix — c'est la classe de mensonge qu'on
#   traque, rendue acceptable par un fichier de configuration.
#   ⚠️ Cette regle porte sur le CLASSEMENT, pas sur le code : le detecteur
#   refuse un classement interdit, il ne dit toujours rien sur l'implementation.
#
# LES DETECTEURS
#   D1  virtuelle CREUSE (corps vide, ou un unique `return <litteral>;`), zero
#       surcharge et zero appel dans le depot, APPARIEE a un drapeau
#       `mCaps.<x> = true` dont le nom partage un jeton avec le sien.
#       ⚠️ C'est l'APPARIEMENT qui fait le filtre, pas le corps vide. Sans lui,
#       `SetDebugName` (trois surcharges, corps vides, zero surcharge) serait
#       rapporte a tort : un corps vide qui ne promet rien a personne n'est pas
#       un mensonge, c'est un point d'extension.
#   D2  champ scalaire a valeur par defaut dont AUCUNE occurrence du depot n'est
#       une lecture — que des ecritures, la declaration, des commentaires.
#       Enonce exact : « quelqu'un peut regler ce champ et il ne se passe rien ».
#   D3  NON LIVRE, et la raison est mesuree : `./verif_capacites.sh --pourquoi-pas-d3`
#
# USAGE
#   ./verif_capacites.sh                 # detecte, garde, rapporte
#   ./verif_capacites.sh --liste         # les candidats detectes, sans garde
#   ./verif_capacites.sh --nouveaux      # seulement les non classes, un par ligne
#   ./verif_capacites.sh --pourquoi-pas-d3
#
# CODES DE SORTIE
#   0  tout candidat detecte est classe, et aucun classement n'est interdit
#   2  ECHEC D'INSTRUMENT : temoin non retrouve, corpus absent, liste illisible.
#      Rien n'est conclu sur le depot.
#   4  ECHEC DE CLASSEMENT. NE BLOQUE PAS LE COMMIT — rougit le verificateur.
# =============================================================================
set -uo pipefail

dire()  { printf '%s\n' "$*"; }
dire2() { printf '%s\n' "$*" >&2; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT" || { dire2 "[cap] cd $ROOT impossible"; exit 2; }

LISTE="config/capacites.list"
TMP="$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/nkcap.$$")"
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

LISTE_SEULE=0
NOUVEAUX_SEULS=0

# Les en-tetes examines sont une DONNEE, pas une heuristique : on ne devine pas
# « les fichiers qui ressemblent a de la configuration ».
ENTETES_D2=(
  "Kernel/Runtime/NKRHI/src/NKRHI/Core/NkDescs.h"
  "Kernel/Runtime/NKRHI/src/NKRHI/Core/NkIDevice.h"
  "Kernel/Runtime/NKRenderer/src/NKRenderer/Core/NkRendererConfig.h"
)
ENTETES_D1=( "Kernel/Runtime/NKRHI/src/NKRHI/Core/NkIDevice.h" )

# -- Controle positif d'instrument -------------------------------------------
# Deux temoins MESURES le 2026-08-22, un par detecteur. Un detecteur qui ne
# trouve plus rien ressemble trait pour trait a un depot sain : sans temoin, on
# ne peut pas distinguer « rien a signaler » de « je ne lis plus le fichier ».
TEMOIN_D1="NkIDevice::GetTimestampPeriodNs"
TEMOIN_D2="NkDeviceCaps::timestampQueries"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --liste)           LISTE_SEULE=1; shift ;;
    --nouveaux)        NOUVEAUX_SEULS=1; shift ;;
    --pourquoi-pas-d3) sed -n '/^# ---- D3 : POURQUOI/,/^# ---- FIN D3/p' "$0" | sed 's/^#\{1,2\} \{0,1\}//'; exit 0 ;;
    -h|--help)         sed -n '2,80p' "$0"; exit 0 ;;
    *) dire2 "[cap] option inconnue : $1 (voir --help)"; exit 2 ;;
  esac
done

T0=$(date +%s)

# =============================================================================
# 1. PRECONDITIONS — ne rien conclure depuis un montage casse
# =============================================================================
for f in "${ENTETES_D2[@]}" "${ENTETES_D1[@]}"; do
  if [ ! -f "$f" ]; then
    dire2 "[cap] ECHEC D'INSTRUMENT : en-tete examine introuvable : $f"
    dire2 "[cap]   Il a ete deplace ou renomme. Mets ENTETES_D1/ENTETES_D2 a jour."
    dire2 "[cap]   Ne rien conclure : « je n'ai rien trouve » et « je n'ai rien lu »"
    dire2 "[cap]   produisent la meme sortie verte."
    exit 2
  fi
done
if [ ! -f "$LISTE" ]; then
  dire2 "[cap] ECHEC D'INSTRUMENT : $LISTE absent."
  exit 2
fi

# =============================================================================
# 2. EXTRACTION D2 — les champs candidats, avec leur struct englobante
# =============================================================================
cat > "$TMP/champs.awk" <<'AWKCHAMPS'
BEGIN { prof = 0; attente = "" }
{
  ligne = $0; sub(/\r$/, "", ligne)
  if (match(ligne, /^[[:space:]]*(struct|class)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*/)) {
    t = substr(ligne, RSTART, RLENGTH); sub(/^[[:space:]]*(struct|class)[[:space:]]+/, "", t)
    attente = t
  }
  tmp = ligne
  n = gsub(/\{/, "{", tmp); m = gsub(/\}/, "}", tmp)
  if (n > 0 && attente != "") { prof++; pile[prof] = attente; attente = "" }
  else if (n > 0) { prof++; pile[prof] = (prof > 1 ? pile[prof-1] : "") }
  if (m > 0 && prof > 0) { prof -= m; if (prof < 0) prof = 0 }
  if (prof < 1) next
  st = pile[prof]; if (st == "") next
  if (ligne ~ /^[[:space:]]*(static|virtual|return|\/\/|\/\*|\*)/) next
  if (ligne ~ /\(/) next
  if (match(ligne, /^[[:space:]]*[A-Za-z_][A-Za-z0-9_:<>, \t*&]*[ \t*&]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*=[^=]/)) {
    d = ligne; sub(/[[:space:]]*=.*$/, "", d)
    nom = d; sub(/^.*[ \t*&]/, "", nom); if (nom == "") next
    val = ligne; sub(/^[^=]*=[[:space:]]*/, "", val); sub(/;.*$/, "", val)
    gsub(/[[:space:]]+$/, "", val)
    printf "%s|%s|%s|%d|%s\n", st, nom, FILENAME, FNR, val
  }
}
AWKCHAMPS
awk -f "$TMP/champs.awk" "${ENTETES_D2[@]}" > "$TMP/champs.txt"

# (les voies d'acces sont extraites en section 4 : elles vivent dans TOUT le
#  depot, pas dans les trois en-tetes examines. Defaut trouve et corrige le
#  2026-08-22 -- voir le bloc VOIES D'ACCES plus bas.)

# =============================================================================
# 3. EXTRACTION D1 — virtuelles creuses, et drapeaux reellement mis a `true`
# =============================================================================
cat > "$TMP/virt.awk" <<'AWKVIRT'
BEGIN { prof = 0; attente = ""; att = 0 }
{
  ligne = $0; sub(/\r$/, "", ligne)
  # -- suivi de la classe englobante (meme mecanique que champs.awk) ----------
  if (att == 0 && match(ligne, /^[[:space:]]*(struct|class)[[:space:]]+[A-Za-z_][A-Za-z0-9_]*/)) {
    t = substr(ligne, RSTART, RLENGTH); sub(/^[[:space:]]*(struct|class)[[:space:]]+/, "", t)
    attente = t
  }
  if (att == 0) {
    tmp = ligne
    n = gsub(/\{/, "{", tmp); m = gsub(/\}/, "}", tmp)
    if (n > 0 && attente != "") { prof++; pile[prof] = attente; attente = "" }
    else if (n > 0) { prof++; pile[prof] = (prof > 1 ? pile[prof-1] : "") }
    if (m > 0 && prof > 0) { prof -= m; if (prof < 0) prof = 0 }
  }
  # -- detection de la virtuelle ---------------------------------------------
  if (att == 0) {
    if (ligne ~ /^[[:space:]]*virtual[[:space:]]/ && ligne ~ /\(/ && ligne ~ /\{[[:space:]]*$/) {
      d = ligne; sub(/\(.*$/, "", d)
      nom = d; sub(/^.*[ \t*&]/, "", nom)
      if (nom != "") { att = 1; enom = nom; eligne = FNR; ecl = (prof >= 1 ? pile[prof] : ""); corps = 0; ret = "" }
    }
    next
  }
  if (ligne ~ /^[[:space:]]*\}/) {
    if (corps == 0)                   printf "%s|%s|%d|vide|\n", ecl, enom, eligne
    else if (corps == 1 && ret != "") printf "%s|%s|%d|constant|%s\n", ecl, enom, eligne, ret
    att = 0; next
  }
  if (ligne ~ /^[[:space:]]*$/) next
  if (ligne ~ /^[[:space:]]*(\/\/|\/\*|\*)/) next
  corps++
  ret = ""
  if (match(ligne, /^[[:space:]]*return[[:space:]]+[^;]*;[[:space:]]*$/)) {
    r = ligne; sub(/^[[:space:]]*return[[:space:]]+/, "", r); sub(/;[[:space:]]*$/, "", r)
    # LITTERAL seulement. `return mThing;` n'est pas creux : il rend un etat.
    if (r ~ /^(-?[0-9]+(\.[0-9]*)?[fFuUlL]*|true|false|nullptr|\{\})$/) ret = r
  }
}
AWKVIRT
awk -f "$TMP/virt.awk" "${ENTETES_D1[@]}" > "$TMP/virtuelles.txt"

# Les drapeaux de capacite REELLEMENT ecrits par du code (pas la declaration).
grep -rhoP 'mCaps\.\K[A-Za-z_][A-Za-z0-9_]*(?=[ \t]*=[^=])' Kernel Engine --include='*.cpp' 2>/dev/null \
  | sort -u > "$TMP/drapeaux.txt"

# =============================================================================
# 4. LES PASSES SUR LE CORPUS — deux greps, et pas un par symbole
# =============================================================================
# Mesure du 2026-08-22 : les prototypes relisaient tout le depot UNE FOIS PAR
# SYMBOLE — 30 s pour D2 seul. Ici : deux greps a motifs fixes, et l'awk trie.
# L'exigence « il doit etre lancable » se perd dans ce genre de detail, jamais
# dans les grands choix.
#
# POURQUOI DEUX ET NON UN. Le premier grep sort les NOMS DE TYPE avec `-o` : il
# repond exactement a « quel fichier nomme quel type », en quelques milliers de
# lignes minuscules. Le second sort les LIGNES ENTIERES des champs, parce qu'il
# faut le texte pour distinguer une ecriture d'une lecture. Les fondre en un
# seul obligerait a re-tester chaque nom de type contre chaque ligne : 340 x
# 40 000 comparaisons pour une information que `-o` donne directement.

INCL=(--include='*.h' --include='*.hpp' --include='*.cpp' --include='*.inl')

cut -d'|' -f1 "$TMP/champs.txt" | sort -u | grep -v '^[[:space:]]*$' > "$TMP/types.txt"
cut -d'|' -f2 "$TMP/champs.txt" | sort -u | grep -v '^[[:space:]]*$' > "$TMP/noms.txt"
cut -d'|' -f2 "$TMP/virtuelles.txt" | sort -u | grep -v '^[[:space:]]*$' > "$TMP/methodes.txt"

# -- VOIES D'ACCES ------------------------------------------------------------
# DEFAUT DE MON PROPRE OUTIL, TROUVE PAR LA MESURE (2026-08-22).
# Premiere version : les voies d'acces etaient cherchees dans les TROIS EN-TETES
# EXAMINES. Or `NkDeviceCaps mCaps{}` est declare dans les SIX en-tetes de
# backend (NkVulkanDevice.h, NkOpenglDevice.h...), jamais dans NkIDevice.h. La
# voie `mCaps` n'existait donc pas, `mCaps.maxDescriptorSets = ...` n'etait
# attribue a personne, et le champ ressortait « jamais mentionne ».
# L'outil transformait « ecrit ailleurs » en « jamais touche » : un instrument
# qui accuse le sujet a la place de son propre montage, pour la troisieme fois
# de ce chantier.
# Correction : les voies se cherchent dans TOUT LE CORPUS, et la reconnaissance
# n'exige plus un `.` ou un `->` devant le membre — `mCaps.x` s'ecrit nu.
#
# Deux formes, et pas une de plus :
#   membre     `NkDeviceCaps mCaps;`            -> mCaps.<champ> / mCaps-><champ>
#   accesseur  `const NkDeviceCaps &GetCaps()`  -> GetCaps().<champ>
# Un accesseur est une voie de LECTURE au meme titre qu'un membre. Ignorer les
# deux, c'est appeler « jamais lu » un champ que tout le moteur lit.
TYPES_ALT=$(paste -sd'|' "$TMP/types.txt")
grep -rhoP "(?<![A-Za-z0-9_])(?:${TYPES_ALT})[ \t]*[&*]?[ \t]+[a-zA-Z_][A-Za-z0-9_]*(?=[ \t]*[;={(])" \
     "${INCL[@]}" Kernel Engine Applications 2>/dev/null \
  | sed 's/[&*]/ /g' \
  | awk 'NF >= 2 { printf "%s|%s\n", $1, $NF }' \
  | sort -u > "$TMP/voies.txt"

NB_MOTIFS=$(cat "$TMP/types.txt" "$TMP/noms.txt" "$TMP/methodes.txt" | grep -c '')
if [ "$NB_MOTIFS" -lt 20 ]; then
  dire2 "[cap] ECHEC D'INSTRUMENT : $NB_MOTIFS symbole(s) extrait(s) des en-tetes."
  dire2 "[cap]   L'extraction ne lit plus les en-tetes (forme changee ?). Ne rien conclure."
  exit 2
fi

# (a) quel fichier NOMME quel type — attribution des homonymes
grep -rnwoF -f "$TMP/types.txt" "${INCL[@]}" Kernel Engine Applications 2>/dev/null \
  | awk -F: '{ t=$0; sub(/^[^:]*:[^:]*:/, "", t); printf "%s|%s\n", $1, t }' \
  | sort -u > "$TMP/nomme.txt"

# (b) les lignes entieres qui portent un champ ou une methode suivie
cat "$TMP/noms.txt" "$TMP/methodes.txt" | sort -u > "$TMP/motifs.txt"
grep -rnwF -f "$TMP/motifs.txt" "${INCL[@]}" Kernel Engine Applications 2>/dev/null > "$TMP/hits.txt"

NB_HITS=$(grep -c '' < "$TMP/hits.txt")
if [ "$NB_HITS" -lt 100 ]; then
  dire2 "[cap] ECHEC D'INSTRUMENT : $NB_HITS occurrence(s) sur tout le corpus."
  dire2 "[cap]   Le depot n'est pas la, ou grep n'a rien lu. Ne rien conclure."
  exit 2
fi

# =============================================================================
# 5. CLASSIFICATION — le coeur, et la seule chose qui ait le droit d'etre exacte
# =============================================================================
awk -v fchamps="$TMP/champs.txt" -v fvoies="$TMP/voies.txt" \
    -v fvirt="$TMP/virtuelles.txt" -v fdrap="$TMP/drapeaux.txt" \
    -v fnomme="$TMP/nomme.txt" -v fentete1="${ENTETES_D1[0]}" '
  # ---- decoupage camelCase / snake_case en jetons minuscules ---------------
  function jetons(s,   i, c, mot, out) {
    out = " "; mot = ""
    for (i = 1; i <= length(s); i++) {
      c = substr(s, i, 1)
      if (c ~ /[A-Z]/) { if (mot != "") out = out tolower(mot) " "; mot = c }
      else if (c == "_") { if (mot != "") out = out tolower(mot) " "; mot = "" }
      else mot = mot c
    }
    if (mot != "") out = out tolower(mot) " "
    return out
  }
  # Un jeton partage doit etre PORTEUR. « get », « set », « is », « begin »
  # n existent que pour faire du bruit : cinq caracteres est le seuil mesure qui
  # garde « timestamp » et jette « begin ».
  function partage(a, b,   n, i, t, ja, jb) {
    ja = jetons(a); jb = jetons(b)
    n = split(ja, t, " ")
    for (i = 1; i <= n; i++) {
      if (t[i] == "" || length(t[i]) < 5) continue
      if (index(jb, " " t[i] " ") > 0) return t[i]
      # pluriel simple : « query » contre « queries » n est pas un jeton nouveau
      if (index(jb, " " t[i] "s ") > 0) return t[i]
      if (index(jb, " " substr(t[i], 1, length(t[i]) - 1) "ies ") > 0) return t[i]
    }
    return ""
  }
  BEGIN {
    while ((getline l < fchamps) > 0) {
      split(l, a, "|")
      cle = a[1] "::" a[2]
      ch_struct[cle] = a[1]; ch_nom[cle] = a[2]
      ch_fic[cle] = a[3]; ch_lig[cle] = a[4] + 0; ch_def[cle] = a[5]
      champs[cle] = 1
      parnom[a[2]] = parnom[a[2]] " " cle
    }
    close(fchamps)
    while ((getline l < fvoies) > 0) { split(l, a, "|"); voie[a[1]] = voie[a[1]] " " a[2] }
    close(fvoies)
    while ((getline l < fvirt) > 0) {
      split(l, a, "|")
      cle = a[1] "::" a[2]
      vi_cl[cle] = a[1]; vi_nom[cle] = a[2]; vi_lig[cle] = a[3] + 0
      vi_forme[cle] = a[4]; vi_ret[cle] = a[5]
      virt[cle] = 1
      parmeth[a[2]] = parmeth[a[2]] " " cle
    }
    close(fvirt)
    nbdrap = 0
    while ((getline l < fdrap) > 0) { gsub(/[ \t\r]/, "", l); if (l != "") { nbdrap++; drap[nbdrap] = l } }
    close(fdrap)
    while ((getline l < fnomme) > 0) { gsub(/\r/, "", l); nomme[l] = 1 }
    close(fnomme)
  }
  # ---- une passe sur les occurrences, un seul parcours par ligne -----------
  {
    ligne = $0; sub(/\r$/, "", ligne)
    p1 = index(ligne, ":"); if (p1 == 0) next
    fic = substr(ligne, 1, p1 - 1)
    reste = substr(ligne, p1 + 1)
    p2 = index(reste, ":"); if (p2 == 0) next
    num = substr(reste, 1, p2 - 1) + 0
    txt = substr(reste, p2 + 1)

    # le commentaire de fin de ligne est ampute AVANT toute decision : une
    # mention dans un commentaire n est pas une lecture, et prendre l une pour
    # l autre est exactement « chercher un mot au lieu d un etat ».
    code = txt; sub(/\/\/.*$/, "", code)
    nu = (code ~ /^[[:space:]]*$/)

    # les identifiants de la ligne, extraits UNE fois. Ce qui suit est en
    # O(jetons de la ligne), jamais en O(symboles suivis).
    ntok = split(code, tk, /[^A-Za-z0-9_]+/)
    if (nu) ntok = split(txt, tk, /[^A-Za-z0-9_]+/)
    # DEUX BOUCLES, pas une. La premiere connait TOUS les jetons de la ligne
    # avant que la seconde ne decide quoi que ce soit. C est ce qui permet de
    # ne lancer une regex de voie d acces que si le nom de la voie est
    # REELLEMENT sur la ligne : sans ce filtre, l outil lancait trois regex par
    # voie declaree et par candidat, sur 80 000 lignes — mesure : > 120 s.
    delete vus
    for (j = 1; j <= ntok; j++) if (tk[j] != "") vus[tk[j]] = 1
    delete traite
    for (j = 1; j <= ntok; j++) {
      nom = tk[j]
      if (nom == "" || traite[nom]) continue
      traite[nom] = 1

      # ---- D2 : un champ -----------------------------------------------
      if (parnom[nom] != "") {
        nc = split(parnom[nom], cles, " ")
        # ATTRIBUTION, EN DEUX TEMPS ET DANS CET ORDRE.
        #
        # 1) LA VOIE D ACCES PRIME. Si la ligne ecrit `mCfg.shadow.enabled`, ce
        #    `enabled` appartient a NkShadowConfig et a lui seul.
        # 2) A DEFAUT, le fichier qui nomme le type.
        #
        # ⚠️ DEFAUT MESURE LE 2026-08-22, corrige ici. Avec le seul critere (2),
        #    `NkRendererImpl.cpp` nomme NkIBLConfig dans un COMMENTAIRE, et sa
        #    lecture de `mCfg.shadow.enabled` etait comptee comme une lecture de
        #    NkIBLConfig::enabled — qui n en a aucune. Un homonyme lu FAIT
        #    MANQUER un defaut : l outil se taisait sur un champ creux en
        #    croyant le voir lu. C est la direction d erreur la plus dangereuse
        #    des deux, parce qu elle est silencieuse.
        nvoie = 0
        for (k = 1; k <= nc; k++) {
          cle = cles[k]; if (cle == "") continue
          # UNE LECTURE SUFFIT A CLORE LE DOSSIER. Des qu un candidat a une
          # lecture ou une prise d adresse, il n est plus candidat : continuer a
          # classer ses 400 occurrences suivantes ne change plus rien au verdict.
          # Mesure : 17 s -> 5 s, et pas un chiffre du rapport ne bouge (les
          # candidats RAPPORTES ont lectures=0 par definition).
          if (lec[cle] > 0 || adr[cle] > 0) continue
          s2 = ch_struct[cle]
          parvoie[cle] = 0
          if (voie[s2] == "") continue
          nv = split(voie[s2], vv, " ")
          for (v = 1; v <= nv; v++) {
            if (vv[v] == "" || !vus[vv[v]]) continue
            # membre nu, membre pointeur, ou accesseur : les trois formes.
            # La premiere version exigeait un « . » ou un « > » DEVANT le
            # membre. Or `mCaps.timestampQueries` s ecrit nu : la voie n etait
            # jamais reconnue, et le champ ressortait « jamais mentionne ».
            if (code ~ ("(^|[^A-Za-z0-9_])" vv[v] "[[:space:]]*[.][[:space:]]*" nom "([^A-Za-z0-9_]|$)") || \
                code ~ ("(^|[^A-Za-z0-9_])" vv[v] "[[:space:]]*->[[:space:]]*" nom "([^A-Za-z0-9_]|$)") || \
                code ~ ("(^|[^A-Za-z0-9_])" vv[v] "[[:space:]]*[(][^)]*[)][[:space:]]*[.][[:space:]]*" nom "([^A-Za-z0-9_]|$)")) {
              parvoie[cle] = 1; nvoie++; break
            }
          }
        }
        for (k = 1; k <= nc; k++) {
          cle = cles[k]; if (cle == "") continue
          if (lec[cle] > 0 || adr[cle] > 0) continue
          s2 = ch_struct[cle]
          if (nvoie > 0) att = parvoie[cle]
          else           att = nomme[fic "|" s2]
          if (!att) { horstype[cle]++; continue }
          if (fic == ch_fic[cle] && num == ch_lig[cle]) { decl[cle]++; continue }
          if (nu) { comm[cle]++; continue }
          # prise d adresse : ce n est pas une lecture, mais ce n est pas rien
          if (code ~ ("&[-A-Za-z0-9_.>()]*" nom "([^A-Za-z0-9_]|$)")) { adr[cle]++; continue }
          if (code ~ (nom "[[:space:]]*([+][+]|--)")) { ecr[cle]++; continue }
          if (code ~ (nom "[[:space:]]*([-+*/%|&^]|<<|>>)?=[^=]")) {
            ecr[cle]++
            if (code ~ (nom "[[:space:]]*=[[:space:]]*true")) prom[cle] = 1
            continue
          }
          lec[cle]++
        }
      }

      # ---- D1 : une methode virtuelle suivie ----------------------------
      if (parmeth[nom] != "") {
        nc = split(parmeth[nom], cles, " ")
        for (k = 1; k <= nc; k++) {
          cle = cles[k]; if (cle == "") continue
          # une DECLARATION (ici ou ailleurs) : virtual ... nom(   ou  ... nom(...) override
          if (code ~ ("virtual[[:space:]].*[^A-Za-z0-9_]" nom "[[:space:]]*\\(") || \
              code ~ (nom "[[:space:]]*\\(.*\\)[^;]*override")) { m_decl[cle]++; continue }
          # un APPEL : nom( precede d un non-identifiant
          if (code ~ ("(^|[^A-Za-z0-9_])" nom "[[:space:]]*\\(")) m_appel[cle]++
        }
      }
    }
  }
  END {
    # -- D1 : creuse, une seule declaration, zero appel, APPARIEE ------------
    for (cle in virt) {
      nom = vi_nom[cle]
      if (m_decl[cle] > 1) continue     # surchargee ailleurs : ce n est pas creux
      if (m_appel[cle] > 0) continue    # appelee quelque part
      jet = ""; dr = ""
      for (d = 1; d <= nbdrap; d++) { j = partage(nom, drap[d]); if (j != "") { jet = j; dr = drap[d]; break } }
      # ⚠️ C EST L APPARIEMENT QUI FAIT LE FILTRE. Sans lui, SetDebugName —
      # corps vides, zero surcharge, zero appel — serait rapporte a tort. Un
      # corps vide qui ne promet rien a personne est un point d extension, pas
      # un mensonge.
      if (jet == "") continue
      printf "D1|%s|%s:%d|%s|%s|corps %s, retour %s ; drapeau mCaps.%s mis a true par du code (jeton partage: %s)\n",
             cle, fentete1, vi_lig[cle], "promesse-true",
             (vi_ret[cle] == "" ? "-" : vi_ret[cle]),
             vi_forme[cle], (vi_ret[cle] == "" ? "(aucun)" : vi_ret[cle]), dr, jet
    }
    # -- D2 : ecrit, jamais lu -----------------------------------------------
    for (cle in champs) {
      if (lec[cle] > 0) continue
      if (adr[cle] > 0) continue
      if (ecr[cle] + decl[cle] == 0) continue   # symbole jamais vu : extraction douteuse
      # DEUX ENONCES DIFFERENTS, et les confondre serait mentir sur ce qu on sait :
      #   ecrit-jamais-lu   quelqu un REGLE ce champ, et il ne se passe rien.
      #   jamais-mentionne  personne ne le nomme nulle part, ni en ecriture ni
      #                     en lecture. Enonce plus FAIBLE : il peut aussi vouloir
      #                     dire que l attribution n a pas trouve la voie d acces.
      # Il est dit tel quel, jamais deguise en le premier.
      forme = (ecr[cle] > 0 ? "ecrit-jamais-lu" : "jamais-mentionne")
      printf "D2|%s|%s:%d|%s|%s|%s ; ecritures=%d lectures=0 adresse=0 commentaires=%d hors-type=%d\n",
             cle, ch_fic[cle], ch_lig[cle],
             (prom[cle] ? "promesse-true" : "sans-promesse"),
             (ch_def[cle] == "" ? "-" : ch_def[cle]),
             forme, ecr[cle], comm[cle], horstype[cle]
    }
  }
' "$TMP/hits.txt" | sort > "$TMP/detectes.txt"

NB_DET=$(grep -c '' < "$TMP/detectes.txt")

# -- Controle positif d'instrument -------------------------------------------
manque=""
grep -q "^D1|$TEMOIN_D1|" "$TMP/detectes.txt" || manque="$manque $TEMOIN_D1(D1)"
grep -q "^D2|$TEMOIN_D2|" "$TMP/detectes.txt" || manque="$manque $TEMOIN_D2(D2)"
if [ -n "$manque" ]; then
  dire2 "[cap] ECHEC D'INSTRUMENT : temoin(s) non retrouve(s) :$manque"
  dire2 "[cap]   Ces deux symboles ont ete MESURES creux le 2026-08-22. Leur"
  dire2 "[cap]   disparition veut dire l'une de deux choses :"
  dire2 "[cap]     - ils ont ete implementes  -> mets le temoin a jour, c'est une bonne nouvelle ;"
  dire2 "[cap]     - le detecteur ne detecte plus -> tout le reste de cette sortie est faux."
  dire2 "[cap]   Sans temoin, ces deux cas produisent la MEME sortie verte."
  exit 2
fi

# =============================================================================
# 6. LECTURE DE config/capacites.list
# =============================================================================
# Format : <symbole> | <detecteur> | <classe> | <date> | <note>
declare -A CL_CLASSE=() CL_DATE=() CL_NOTE=()
declare -a CL_ORDRE=()
ROGNE=""
rogner() { local s="${1:-}"; s="${s#"${s%%[![:space:]]*}"}"; s="${s%"${s##*[![:space:]]}"}"; ROGNE="$s"; }

# -- LE CLIQUET DE `A-DATER` (arbitrage de Rodolf, R4-1) ---------------------
# `A-DATER` a le droit d'exister, mais SON COMPTE NE PEUT QUE DESCENDRE.
#
# Le raisonnement, et il n'est pas de moi : dater a la place de Rodolf est
# impossible, et passer 24 drapeaux a `false` sans lui serait une decision
# PRODUIT deguisee en hygiene. Le cliquet resout les deux : le stock est un
# HERITAGE qui ne se renouvelle pas, et toute capacite NOUVELLE est datee a la
# naissance, sans exception. La dette ne grossit pas pendant qu'on la date.
#
# Le plafond est une DONNEE, declaree dans le fichier de classement :
#     # PLAFOND-A-DATER = 24
# Le modifier est un geste delibere qui apparait dans un diff. C'est le point.
#
# ⚠️ SON ABSENCE EST UN ECHEC D'INSTRUMENT, pas un defaut : un cliquet absent et
# un cliquet satisfait produisent la meme sortie verte. C'est la meme phrase que
# pour les temoins, et elle vaut ici aussi.
PLAFOND_A_DATER=""
while IFS= read -r ligne; do
  case "$ligne" in
    \#*PLAFOND-A-DATER*)
      PLAFOND_A_DATER="${ligne##*=}"
      PLAFOND_A_DATER="${PLAFOND_A_DATER//[![:digit:]]/}"
      ;;
  esac
done < "$LISTE"
if [ -z "$PLAFOND_A_DATER" ]; then
  dire2 "[cap] ECHEC D'INSTRUMENT : aucune directive « # PLAFOND-A-DATER = N » dans $LISTE."
  dire2 "[cap]   Le cliquet de A-DATER n'existe donc pas, et un cliquet absent produit"
  dire2 "[cap]   exactement la meme sortie verte qu'un cliquet satisfait."
  dire2 "[cap]   Ajoute la ligne avec le compte actuel, puis fais-la DESCENDRE."
  exit 2
fi

num=0
while IFS= read -r ligne; do
  num=$((num + 1))
  case "$ligne" in ''|\#*) continue ;; esac
  IFS='|' read -r sym det classe date note <<< "$ligne"
  rogner "${sym:-}";    sym="$ROGNE"
  rogner "${det:-}";    det="$ROGNE"
  rogner "${classe:-}"; classe="$ROGNE"
  rogner "${date:-}";   date="$ROGNE"
  rogner "${note:-}";   note="$ROGNE"
  [ -z "$sym" ] && continue
  cle="$det|$sym"
  if [ -n "${CL_CLASSE[$cle]:-}" ]; then
    dire2 "[cap] $LISTE ligne $num : $cle declare deux fois."
    exit 2
  fi
  case "$classe" in
    implementee|vision-assumee|dette-datee|morte-a-retirer) ;;
    *) dire2 "[cap] $LISTE ligne $num : classe inconnue « $classe » pour $sym"
       dire2 "[cap]   attendu : implementee | vision-assumee | dette-datee | morte-a-retirer"
       exit 2 ;;
  esac
  CL_CLASSE["$cle"]="$classe"; CL_DATE["$cle"]="$date"; CL_NOTE["$cle"]="$note"
  CL_ORDRE+=("$cle")
done < "$LISTE"

# =============================================================================
# 7. LA GARDE
# =============================================================================
declare -a NON_CLASSES=() INTERDITS=() SANS_DATE=() ORPHELINES=() A_DATER=()
declare -A VU=()
declare -A NB_PAR_CLASSE=()

while IFS='|' read -r det sym ou promesse val note; do
  cle="$det|$sym"
  VU["$cle"]=1
  cl="${CL_CLASSE[$cle]:-}"
  if [ -z "$cl" ]; then
    NON_CLASSES+=("$det|$sym|$ou|$promesse|$val|$note")
    continue
  fi
  NB_PAR_CLASSE["$cl"]=$(( ${NB_PAR_CLASSE[$cl]:-0} + 1 ))
  # -- REGLE (a) : `vision-assumee` interdit des qu'un drapeau annonce `true`
  if [ "$cl" = "vision-assumee" ]; then
    case "$det:$promesse" in
      D1:*|*:promesse-true) INTERDITS+=("$det|$sym|$promesse|$note") ;;
    esac
  fi
  # -- Une dette sans echeance n'est pas une dette datee.
  #    VIDE = un oubli -> ECHEC.
  #    A-DATER = une echeance qui appartient a Rodolf et que je n'invente pas.
  #    Les deux se ressemblent et ne veulent PAS dire la meme chose : inventer
  #    une date pour faire taire l'outil serait produire un chiffre credible et
  #    faux, precisement ce qu'on traque. Mais une dette qu'on ne mentionne plus
  #    est une dette oubliee : A-DATER est NOMMEE a chaque passage.
  if [ "$cl" = "dette-datee" ]; then
    case "${CL_DATE[$cle]}" in
      "")        SANS_DATE+=("$det|$sym") ;;
      A-DATER)   A_DATER+=("$det|$sym") ;;
    esac
  fi
done < "$TMP/detectes.txt"

for cle in "${CL_ORDRE[@]}"; do
  [ -z "${VU[$cle]:-}" ] && ORPHELINES+=("$cle")
done

# -- LE CLIQUET SE SERRE, IL NE SE DESSERRE PAS ------------------------------
CLIQUET_ROMPU=0
CLIQUET_LACHE=0
if [ "${#A_DATER[@]}" -gt "$PLAFOND_A_DATER" ]; then
  CLIQUET_ROMPU=1
elif [ "${#A_DATER[@]}" -lt "$PLAFOND_A_DATER" ]; then
  CLIQUET_LACHE=1
fi

# =============================================================================
# 8. RAPPORT
# =============================================================================
if [ "$NOUVEAUX_SEULS" -eq 1 ]; then
  for l in "${NON_CLASSES[@]:-}"; do [ -n "$l" ] && dire "$l"; done
  [ "${#NON_CLASSES[@]}" -gt 0 ] && exit 4
  exit 0
fi

dire "-- Capacites : detection ---------------------------------------------"
dire "[cap]     corpus : $(grep -c '' < "$TMP/motifs.txt") symbole(s) suivis, $NB_HITS occurrence(s) lues en une passe"
dire "[cap]     temoins : $TEMOIN_D1 (D1) et $TEMOIN_D2 (D2) toujours detectes"
dire "[cap]     $NB_DET candidat(s) detecte(s), ${#CL_ORDRE[@]} ligne(s) de classement"

if [ "$LISTE_SEULE" -eq 1 ]; then
  dire ""
  dire "-- Candidats detectes (--liste) --------------------------------------"
  while IFS='|' read -r det sym ou promesse val note; do
    printf '  %-3s %-46s %-14s %s\n' "$det" "$sym" "${CL_CLASSE[$det|$sym]:-NON-CLASSE}" "$note"
  done < "$TMP/detectes.txt"
  dire ""
  dire " duree : $(( $(date +%s) - T0 ))s"
  exit 0
fi

RC=0

if [ "${#ORPHELINES[@]}" -gt 0 ]; then
  dire "[cap]     info ${#ORPHELINES[@]} ligne(s) dont le symbole n'est plus detecte —"
  dire "[cap]          il a ete implemente ou retire. Information, jamais un echec :"
  for c in "${ORPHELINES[@]}"; do dire "[cap]          - ${c#*|}   (${c%%|*})"; done
fi

if [ "${#NON_CLASSES[@]}" -gt 0 ]; then
  dire2 ""
  dire2 "[cap]     ECHEC DE CLASSEMENT : ${#NON_CLASSES[@]} capacite(s) detectee(s) SANS LIGNE :"
  for l in "${NON_CLASSES[@]}"; do
    IFS='|' read -r det sym ou promesse val note <<< "$l"
    dire2 "[cap]       - $sym"
    dire2 "[cap]           detecteur $det, $ou"
    dire2 "[cap]           $note"
    if [ "$det" = "D1" ] || [ "$promesse" = "promesse-true" ]; then
      dire2 "[cap]           ⚠️ un drapeau annonce « true » a du code : « vision-assumee » est INTERDIT ici."
    fi
  done
  dire2 "[cap]     Ajoute une ligne dans $LISTE pour chacune :"
  dire2 "[cap]       <symbole> | <detecteur> | <classe> | <date> | <note>"
  dire2 "[cap]     classe : implementee | vision-assumee | dette-datee | morte-a-retirer"
  dire2 "[cap]     Dire « le detecteur se trompe, la lecture est ici » est une reponse"
  dire2 "[cap]     valide — ne rien dire n'en est pas une. Le detecteur ne juge pas."
  RC=4
fi

if [ "${#INTERDITS[@]}" -gt 0 ]; then
  dire2 ""
  dire2 "[cap]     CLASSEMENT INTERDIT : ${#INTERDITS[@]} symbole(s) classe(s) « vision-assumee »"
  dire2 "[cap]     alors qu'un drapeau annonce « true » a du code :"
  for l in "${INTERDITS[@]}"; do
    IFS='|' read -r det sym promesse note <<< "$l"
    dire2 "[cap]       - $sym   ($det — $note)"
  done
  dire2 "[cap]     Un « true » n'est pas une declaration d'intention : c'est une reponse"
  dire2 "[cap]     a une question que le code pose a l'execution, et dont il va se"
  dire2 "[cap]     servir pour choisir un chemin. DEUX SORTIES SEULEMENT :"
  dire2 "[cap]       - le drapeau rend « false » ;"
  dire2 "[cap]       - ou la capacite s'implemente."
  dire2 "[cap]     La classer « vision » et passer rendrait acceptable exactement la"
  dire2 "[cap]     classe de mensonge que ce chantier existe pour traquer."
  RC=4
fi

if [ "${#SANS_DATE[@]}" -gt 0 ]; then
  dire2 ""
  dire2 "[cap]     DETTE SANS DATE : ${#SANS_DATE[@]} symbole(s) classe(s) « dette-datee »"
  dire2 "[cap]     avec un champ date vide. Une dette sans echeance n'est pas datee ;"
  dire2 "[cap]     c'est une vision qui n'ose pas dire son nom."
  for l in "${SANS_DATE[@]}"; do dire2 "[cap]       - ${l#*|}"; done
  RC=4
fi

if [ "${#A_DATER[@]}" -gt 0 ] || [ "$CLIQUET_ROMPU" -eq 1 ]; then
  dire ""
  dire "-- Dettes en attente d'echeance (A-DATER) — CLIQUET $PLAFOND_A_DATER ------------------"
  dire "  ${#A_DATER[@]} capacite(s) reconnue(s) comme dette, dont la date appartient a"
  dire "  Rodolf. Nommees a chaque passage : une dette qu'on ne mentionne plus est"
  dire "  une dette oubliee. Ce n'est PAS un echec — c'est une question ouverte."
  for l in "${A_DATER[@]}"; do dire "    ${l#*|}   (${l%%|*})"; done
fi

if [ "$CLIQUET_ROMPU" -eq 1 ]; then
  dire2 ""
  dire2 "[cap]     CLIQUET ROMPU : ${#A_DATER[@]} dette(s) « A-DATER » pour un plafond de $PLAFOND_A_DATER."
  dire2 "[cap]     Le stock d'echeances non fixees est un HERITAGE : il peut descendre,"
  dire2 "[cap]     il ne peut pas MONTER. Toute capacite nouvelle se date a la naissance,"
  dire2 "[cap]     sans exception — sinon la dette grossit exactement pendant qu'on la date."
  dire2 "[cap]     Deux issues, et pas une troisieme :"
  dire2 "[cap]       - donne une date reelle a la (aux) nouvelle(s) ;"
  dire2 "[cap]       - ou classe-la autrement (implementee / morte-a-retirer)."
  dire2 "[cap]     Relever le plafond dans $LISTE est possible — et c'est un geste"
  dire2 "[cap]     DELIBERE qui apparait dans un diff. C'est tout l'interet du cliquet :"
  dire2 "[cap]     il ne s'oppose pas a la decision, il s'oppose a la derive silencieuse."
  RC=4
fi

if [ "$CLIQUET_LACHE" -eq 1 ]; then
  dire ""
  dire "[cap]     info le cliquet peut DESCENDRE : ${#A_DATER[@]} dette(s) « A-DATER » pour un"
  dire "[cap]          plafond de $PLAFOND_A_DATER. Mets « # PLAFOND-A-DATER = ${#A_DATER[@]} » dans $LISTE."
  dire "[cap]          Un cliquet qu'on ne resserre jamais n'est plus un cliquet — il"
  dire "[cap]          redevient une marge, et une marge se remplit. Je ne le resserre"
  dire "[cap]          PAS tout seul : un controle qui modifie sa propre donnee sans que"
  dire "[cap]          personne ne le voie a cesse d'etre un controle."
fi

# On compte les LIGNES DE DONNEE, pas les mentions : l'en-tete du fichier parle
# lui aussi de « non-examine », et le compter serait se tromper de trois.
NB_NON_EXAMINE=$(grep -v '^[[:space:]]*#' "$LISTE" | grep -c 'non-examine')

dire ""
resume=""
for c in implementee vision-assumee dette-datee morte-a-retirer; do
  resume="$resume $c=${NB_PAR_CLASSE[$c]:-0}"
done
if [ "$RC" -eq 0 ]; then
  dire "[cap]     ok   $NB_DET candidat(s), tous classes :$resume"
  dire "[cap]          dont $NB_NON_EXAMINE porte(nt) la note « non-examine » : classes pour que la"
  dire "[cap]          garde soit totale, PAS examines un par un. Ne pas lire ce"
  dire "[cap]          remplissage comme un jugement."
else
  dire "[cap]     ECHEC —$resume ; voir ci-dessus"
fi
dire "[cap]     duree : $(( $(date +%s) - T0 ))s"
exit "$RC"

# ---- D3 : POURQUOI IL N'EST PAS LIVRE (mesure du 2026-08-22) ----------------
#
# D3 devait comparer les numeros de binding LIES cote C++ aux `layout(binding=N)`
# ECHANTILLONNES cote shader. C'est le seul detecteur inter-langage des trois, et
# celui qui vaut le plus cher a l'usage : aucun compilateur ne regarde les deux
# cotes ensemble.
#
# CE QUE J'AI MESURE, ET QUI L'ARRETE
#
#   Cote shader, tout est exact et qualifie par son set :
#     51 x layout(set=0, binding=0), 31 x layout(set=1, binding=1), etc.
#     Un binding sans son set ne veut rien dire : binding=5 existe dans les
#     quatre sets a la fois.
#
#   Cote C++, l'index de set N'EXISTE NULLE PART DANS LE CODE.
#     NkResources.h declare quatre enums — NkFrameBinding, NkObjectBinding,
#     NkMaterialBinding — et NkResources.cpp construit quatre layouts. Le lien
#     « NkFrameBinding appartient au set 0 » n'est ecrit que dans un COMMENTAIRE
#     (`// Bindings dans le set Frame (set=0)`) et suggere par un nom de membre
#     (mFrameLayout). Aucune declaration ne le porte.
#
# DONC : un detecteur qui comparerait les deux cotes devrait DEVINER
# l'appariement enum -> set. Un detecteur qui devine est un generateur de faux
# positifs, et un generateur de faux positifs se desactive le vendredi. Je
# prefere ne pas le livrer que le livrer devin.
#
# ⚠️ ET C'EST DEJA UN RESULTAT. Le fait mesure est celui-ci :
#
#     L'INDEX DE SET DES LAYOUTS STANDARD N'EST PORTE PAR AUCUNE DECLARATION.
#     Il vit dans un commentaire. Un commentaire ne peut pas se tromper — il
#     peut seulement etre faux sans que rien ne le dise.
#
# Et un indice va exactement dans ce sens, que je NE presente PAS comme un
# resultat parce que je ne l'ai pas mesure de bout en bout :
#   NkResources.cpp:205 porte « binding 5 : IBL irradiance » ; NK_BIND_IBL_IRRADIANCE
#   vaut bien 5 ; mais `tEnvIrradiance` est declare `binding=8` dans les six
#   familles de shaders. C'est un commentaire contre un litteral. Il faut le
#   mesurer, pas l'affirmer.
#
# CE QUI REND D3 LIVRABLE, ET C'EST UNE LIGNE DE CODE :
#   que chaque layout standard porte son set dans une DECLARATION —
#   par exemple `mFrameLayout` enregistre avec `NK_SET_FRAME`, ou les enums
#   regroupes dans une table `{ NK_SET_FRAME, NkFrameBinding }`. Le jour ou
#   l'appariement est une donnee du code, D3 se calcule exactement et je l'ecris.
#
# ---- FIN D3 ------------------------------------------------------------------

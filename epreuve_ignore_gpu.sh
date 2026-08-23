#!/usr/bin/env bash
#
# epreuve_ignore_gpu.sh — IGNORE EST-IL VRAIMENT UN TROISIEME ETAT ? (2026-08-23)
# =============================================================================
# NkMsaaDeviceCheck exige un peripherique GPU. Sur la machine ou il a ete ecrit,
# il en trouve un — donc son chemin IGNORE n'a JAMAIS tourne. Un chemin d'erreur
# que rien n'a jamais emprunte est du code dont on ne sait pas s'il est juste :
# c'est exactement la lecon de la garde « puissance de deux », redondante et donc
# improuvable, supprimee le 22/08.
#
# Cette epreuve injecte deux defauts REELS dans le banc et exige que :
#   1. le banc rende 77 et DISE pourquoi (une ligne [IGNORE] distincte par cas) ;
#   2. verif_bancs.sh le classe IGNORE — ni OK, ni ECHEC ;
#   3. la passe le COMPTE et le NOMME (« ignore(s) » sur la ligne de verdict,
#      section IGNORES, avertissement sous le verdict).
# Le point 3 est le mandat : un banc ignore qui rapporte vert est le mensonge que
# ce chantier traque.
#
# LES DEUX DEFAUTS SONT SUR DES CHEMINS DISTINCTS, ET C'EST LE POINT
#   A  Ouvrir() : le pilote refuse le peripherique  -> IGNORE « aucune carte »
#   B  le TEMOIN a 1 echantillon est refuse         -> IGNORE « rien de mesurable »
# Ils doivent produire des RAISONS DIFFERENTES. Si les deux donnaient la meme
# ligne, la raison ne serait pas lue dans la sortie du banc mais fabriquee.
#
# ⚠️ ORDRE CRITIQUE — LE CONTROLE NEGATIF D'ABORD, LE PIEGE APRES.
# Un `trap restaurer EXIT` arme AVANT le controle « l'arbre est-il propre ? »
# transforme le refus de ce controle en `git checkout` qui DETRUIT la
# modification non commitee qu'il refusait justement d'ecraser. C'est arrive
# dans ce depot le 22/08, et ca a coute 49 lignes plus un depot ou un banc
# referencait une fonction absente. Un filet ne s'arme jamais avant son controle.
#
# CODES DE SORTIE
#   0  les deux defauts produisent un IGNORE correctement compte et nomme,
#      et l'arbre revient a l'identique
#   1  au moins une exigence non tenue
#   2  refus de demarrer (arbre sale), ou ancre d'injection introuvable
# =============================================================================
set -uo pipefail
cd /d/Projets/2026/Nkentseu/Nkentseu-verif || exit 2

SRC="Applications/NkMsaaDeviceCheck/src/main.cpp"
JOURNAL="Build/Verif/NkMsaaDeviceCheck.run.log"
ECHECS=0

restaurer() { git checkout -- "$SRC" 2>/dev/null; touch "$SRC"; }

# --- CONTROLE NEGATIF, AVANT TOUT PIEGE --------------------------------------
sale=$(git status --porcelain -- "$SRC")
if [ -n "$sale" ]; then
  printf '%s\n' "REFUS : $SRC n'est pas propre — RIEN n'a ete touche."
  printf '%s\n' "$sale"
  exit 2
fi
# --- SEULEMENT MAINTENANT le filet -------------------------------------------
trap 'restaurer' EXIT INT TERM HUP

injecter() {
  python - "$SRC" "$1" <<'PY'
import io, sys
p, quoi = sys.argv[1], sys.argv[2]
s = io.open(p, encoding='utf-8', newline='').read()
if quoi == 'A':
    # Le pilote refuse le peripherique : chemin reel de Ouvrir(), pas un
    # court-circuit du main. Create() est bien appele, Destroy() aussi.
    a = "\t\tif (!dev->IsValid()) {"
    assert a in s, "ancre A"
    s = s.replace(a, "\t\tif (true) { // [DEFAUT INJECTE A] le pilote refuse", 1)
elif quoi == 'B':
    # Le temoin a 1 echantillon est refuse : le banc doit se TAIRE sur le MSAA
    # au lieu de produire huit accusations pour zero mesure.
    a = "\tif (!Obtenir(dev, 1u)) {"
    assert a in s, "ancre B"
    s = s.replace(a, "\tif (true) { // [DEFAUT INJECTE B] le temoin est refuse", 1)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
PY
}

exiger() {
  # exiger <intitule> <attendu> <obtenu>
  if [ "$2" = "$3" ]; then
    printf '  [OK]   %s : %s\n' "$1" "$3"
  else
    printf '  [FAIL] %s : attendu « %s », obtenu « %s »\n' "$1" "$2" "$3"
    ECHECS=$((ECHECS + 1))
  fi
}

passe() {
  # Lance la passe sur CE banc seul. Elle construit depuis la source injectee :
  # pas de risque d'ancien binaire.
  ./verif_bancs.sh --mode complet --banc NkMsaaDeviceCheck --sans-capacites \
    > /tmp/epr_ignore_passe.log 2>&1
  printf '%s' "$?"
}

essai() {
  # essai <A|B> <motif attendu dans la raison>
  local quoi="$1" motif="$2"
  printf '\n=== DEFAUT %s ===\n' "$quoi"
  restaurer
  injecter "$quoi" || { printf 'ancre introuvable\n'; exit 2; }

  local rc; rc=$(passe)
  exiger "code de la passe (un IGNORE ne rougit pas)" "0" "$rc"

  # Le banc lui-meme a-t-il rendu 77 et dit pourquoi ?
  local ligne_tab
  ligne_tab=$(grep -aE '^  NkMsaaDeviceCheck ' /tmp/epr_ignore_passe.log | head -1)
  printf '  tableau : %s\n' "$ligne_tab"
  case "$ligne_tab" in
    *IGNORE*) exiger "verdict du banc" "IGNORE" "IGNORE" ;;
    *)        exiger "verdict du banc" "IGNORE" "$(printf '%s' "$ligne_tab" | awk '{print $NF}')" ;;
  esac
  case "$ligne_tab" in
    *" 77 "*) exiger "code de sortie du banc" "77" "77" ;;
    *)        exiger "code de sortie du banc" "77" "autre — voir le tableau" ;;
  esac

  # La passe le COMPTE-T-ELLE et le NOMME-T-ELLE ? (le mandat)
  if grep -aq '1 ignore(s)' /tmp/epr_ignore_passe.log; then
    exiger "compte sur la ligne de verdict" "present" "present"
  else
    exiger "compte sur la ligne de verdict" "present" "ABSENT"
  fi
  if grep -aq 'IGNORES : ces bancs N ONT RIEN MESURE' /tmp/epr_ignore_passe.log; then
    exiger "section IGNORES" "presente" "presente"
  else
    exiger "section IGNORES" "presente" "ABSENTE"
  fi
  if grep -aq "banc(s) IGNORE(S) : leur question n a pas ete posee" /tmp/epr_ignore_passe.log; then
    exiger "avertissement sous le verdict" "present" "present"
  else
    exiger "avertissement sous le verdict" "present" "ABSENT"
  fi

  # La RAISON est-elle celle de CE chemin-la, lue dans la sortie du banc ?
  if grep -aq "$motif" /tmp/epr_ignore_passe.log; then
    exiger "raison propre au defaut $quoi" "trouvee" "trouvee"
  else
    exiger "raison propre au defaut $quoi" "trouvee" "ABSENTE (/$motif/)"
  fi
}

printf '=== epreuve_ignore_gpu.sh — IGNORE est-il un troisieme etat ? ===\n'

essai A 'aucun peripherique headless'
essai B 'le temoin a 1 echantillon a ete REFUSE'

# --- RESTAURATION, ET ON EXIGE QU'ELLE SOIT EXACTE ---------------------------
printf '\n=== RESTAURATION ===\n'
restaurer
apres=$(git status --porcelain -- "$SRC")
if [ -z "$apres" ]; then
  printf '  [OK]   %s est revenu a l identique\n' "$SRC"
else
  printf '  [FAIL] %s n est PAS revenu a l identique :\n%s\n' "$SRC" "$apres"
  ECHECS=$((ECHECS + 1))
fi

printf '\n=== Resultat : %d exigence(s) non tenue(s) ===\n' "$ECHECS"
if [ "$ECHECS" -eq 0 ]; then
  printf 'IGNORE est bien un troisieme etat : compte a part, nomme, et sa raison\n'
  printf 'est LUE dans la sortie du banc — deux chemins, deux raisons distinctes.\n'
fi
exit $([ "$ECHECS" -eq 0 ] && printf 0 || printf 1)

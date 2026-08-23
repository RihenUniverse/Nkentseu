#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""matgraph_mutation.py -- campagne de mutation du graphe de materiaux.

POURQUOI CE FICHIER EXISTE, ET POURQUOI IL EST COMMITE
======================================================
Deux nuits de suite, la campagne a EFFACE du travail non commite : sa
restauration est un `git checkout`, qui ne distingue pas la mutation du travail
voisin. La lecon « commiter avant la premiere mutation » etait juste et
INSUFFISANTE -- elle comptait sur la vigilance. Elle est ici INSTRUMENTEE :

  GARDE 1 -- l'arbre doit etre PROPRE avant de muter. Sinon on refuse, on ne
             mute pas, et on dit quoi commiter.
  GARDE 2 -- l'ancre doit apparaitre EXACTEMENT UNE FOIS. Zero ancre = la
             mutation muterait dans le vide et rendrait un vert pour rien ;
             plusieurs = on ne sait pas ce qu'on a mute.
  GARDE 3 -- la restauration porte sur LES FICHIERS MUTES, jamais sur un
             dossier entier. Et l'arbre est reverifie propre APRES.
  GARDE 4 -- le compte d'erreurs de CONSTRUCTION se lit AVANT le banc. Un banc
             qui rend « 0 echec » sur un binaire perime ne mesure rien ; c'est
             la seule sortie qui ment sans qu'on puisse le voir.

La troisieme campagne l'a paye a son tour : le script precedent n'existait plus
sur le disque quand l'agent suivant a repris. Il est donc DANS LE DEPOT, avec
ses mutations, la ou quelqu'un le chercherait (regle 5).

EMPLOI
    python scripts/matgraph_mutation.py --liste
    python scripts/matgraph_mutation.py M11 M12 M13
"""

import os
import re
import subprocess
import sys

RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPILE_H = "Kernel/Runtime/NKRenderer/src/NKRenderer/Materials/Graph/NkMatGraphCompile.h"
BANC = "Build/Bin/Debug-Windows/NkMatGraphCheck/NkMatGraphCheck.exe"

# --------------------------------------------------------------------------
# LES MUTATIONS. Une entree = un defaut DELIBERE, et le cas qui DOIT rougir.
# Chaque `edits` est une liste de (ancre, remplacement) ; l'ancre doit etre
# unique dans le fichier.
# --------------------------------------------------------------------------

# La premiere passe de refus (~ligne 1322) : la boucle qui verifie les noms de
# canaux AVANT d'emettre. La neutraliser laisse passer les noms inconnus.
_ANCRE_PASSE1 = """				{
					for (uint32 i = 0; i < g.RawNodeCount(); ++i) {
						const graph::NkNode *n = g.RawNodeAt(i);
						if (!n || !n->alive)
							continue;
						const bool estUV = n->type == NkString(NK_MN_UV_MAP);"""
_MUTE_PASSE1 = """				{
					for (uint32 i = 0; i < 0u; ++i) {
						const graph::NkNode *n = g.RawNodeAt(i);
						if (!n || !n->alive)
							continue;
						const bool estUV = n->type == NkString(NK_MN_UV_MAP);"""

# Le second filet, au SITE D'EMISSION de UV Map.
_ANCRE_FILET_UV = """						if (!c) {
							r.error = NkString("canal UV non valide a l emission (le filet du rang 4 a repris "
											   "la main : la passe de refus ne l a pas attrape)");
							r.source = NkString("");
							return r;
						}
						s.Append(c->varying);
						s.Append(", 0.0);\\n");"""
_MUTE_FILET_UV = """						s.Append(c ? c->varying : "vUV");
						s.Append(", 0.0);\\n");"""

# Le second filet, au SITE D'EMISSION d'Attribute.
_ANCRE_FILET_ATTR = """						if (!c) {
							r.error = NkString("attribut non valide a l emission (le filet du rang 4 a repris "
											   "la main : la passe de refus ne l a pas attrape)");
							r.source = NkString("");
							return r;
						}
						s.Append(c->varying);
						s.Append(".rgb;\\n");"""
_MUTE_FILET_ATTR = """						s.Append(c ? c->varying : "vColor");
						s.Append(".rgb;\\n");"""

# La version D AVANT LE CORRECTIF du second filet : au lieu de refuser, il emet
# un nom qui n existe dans aucun backend. Sert a MESURER ce que le correctif du
# 23/08 a change -- et il ne se mesure qu en compagnie de M12, sinon le site est
# inatteignable.
_MUTE_FILET_UV_JETON = """						s.Append(c ? c->varying : "nkCANAL_UV_NON_VALIDE");
						s.Append(", 0.0);\n");"""
_MUTE_FILET_ATTR_JETON = """						s.Append(c ? c->varying : "nkATTRIBUT_NON_VALIDE");
						s.Append(".rgb;\n");"""

MUTATIONS = {
	"M11": {
		"quoi": "le second filet (site d emission) REMET le repli plausible : vUV / vColor pour n importe quel nom",
		"cas": "rang4/canal-nomme-refuse-en-se-nommant",
		"attendu": "SURVIT -- le site est inatteignable tant que la passe 1 refuse. C est le fait mesure du 23/08.",
		"edits": [(COMPILE_H, _ANCRE_FILET_UV, _MUTE_FILET_UV), (COMPILE_H, _ANCRE_FILET_ATTR, _MUTE_FILET_ATTR)],
	},
	"M12": {
		"quoi": "retirer la PREMIERE passe de refus (les noms de canaux ne sont plus verifies avant emission)",
		"cas": "rang4/canal-nomme-refuse-en-se-nommant",
		"attendu": "ATTRAPEE -- le second filet refuse, mais son message perd le nom demande et la liste.",
		"edits": [(COMPILE_H, _ANCRE_PASSE1, _MUTE_PASSE1)],
	},
	"M13": {
		"quoi": "M11 + M12 : les DEUX lignes de defense tombent en meme temps",
		"cas": "rang4/canal-nomme-refuse-en-se-nommant",
		"attendu": "ATTRAPEE -- et c est le SEUL etat qui rend une image credible et fausse. "
				   "Il mesure ce que le second filet achete vraiment.",
		"edits": [
			(COMPILE_H, _ANCRE_PASSE1, _MUTE_PASSE1),
			(COMPILE_H, _ANCRE_FILET_UV, _MUTE_FILET_UV),
			(COMPILE_H, _ANCRE_FILET_ATTR, _MUTE_FILET_ATTR),
		],
	},
	"M14": {
		"quoi": "M12 + le second filet DANS SA VERSION D AVANT LE CORRECTIF (jeton imprononcable au lieu de refus)",
		"cas": "rang4/canal-nomme-refuse-en-se-nommant",
		"attendu": "ATTRAPEE, mais A COMPARER A M12 : c est le seul couple qui mesure ce que le correctif a achete.",
		"edits": [
			(COMPILE_H, _ANCRE_PASSE1, _MUTE_PASSE1),
			(COMPILE_H, _ANCRE_FILET_UV, _MUTE_FILET_UV_JETON),
			(COMPILE_H, _ANCRE_FILET_ATTR, _MUTE_FILET_ATTR_JETON),
		],
	},
}


def _git(*args):
	return subprocess.run(["git"] + list(args), cwd=RACINE, capture_output=True, text=True)


def arbre_propre():
	r = _git("status", "--porcelain")
	return r.stdout.strip() == "", r.stdout.strip()


def construis():
	"""Rend (ok, nb_erreurs_lues, sortie)."""
	# ⚠️ jenga imprime des cadres UTF-8 ; la console Windows decode en cp1252 et
	# LEVE. Un decodage qui echoue ici tuerait la campagne APRES la mutation --
	# c est exactement comme ca que la session precedente s est arretee.
	r = subprocess.run(["jenga", "build", "--target", "NkMatGraphCheck"],
					   cwd=RACINE, capture_output=True, text=True,
					   encoding="utf-8", errors="replace", shell=(os.name == "nt"))
	txt = (r.stdout or "") + (r.stderr or "")
	nb = txt.lower().count("error:") + txt.lower().count(" error ")
	return (r.returncode == 0 and "BUILD COMPLETED" in txt and "SUCCESS" in txt), nb, txt


def joue_le_banc():
	r = subprocess.run([os.path.join(RACINE, BANC.replace("/", os.sep))],
					   cwd=RACINE, capture_output=True, text=True,
					   encoding="utf-8", errors="replace")
	return r.returncode, (r.stdout or "") + (r.stderr or "")


# ⚠️ « ECHEC » apparait AUSSI dans le detail des cas (« GLSLANG=ECHEC »), ou il
# est parfois ATTENDU. Le verdict est le CHAMP D ETAT, pas le mot : il se lit
# entre le nom du cas et la premiere barre.
_LIGNE_CAS = re.compile(r"^(\S+)\s+(OK|ECHEC)\s+\|")


def _etat(ligne):
	m = _LIGNE_CAS.match(ligne.strip())
	return (m.group(1), m.group(2)) if m else (None, None)


def verdict_du_cas(sortie, nom_cas):
	for ligne in sortie.splitlines():
		nom, etat = _etat(ligne)
		if nom == nom_cas:
			return ligne.strip(), etat
	return None, None


def compte_echecs(sortie):
	for ligne in sortie.splitlines():
		if ligne.startswith("-- ") and " cas, " in ligne:
			return ligne.strip()
	return "(compte introuvable -- LE BANC N A PAS FINI)"


def applique(edits):
	"""Rend (ok, message). Refuse si une ancre n'apparait pas exactement une fois.

	⚠️ LES SOURCES DU DEPOT SONT EN CRLF, LES ANCRES DE CE FICHIER EN LF. Une
	comparaison naive rend « ancrage x0 » sur une ancre parfaitement juste --
	et la garde 2 le dirait, mais en accusant l'ancre au lieu du saut de ligne.
	On lit donc le fichier en le NORMALISANT, et on REECRIT avec la fin de
	ligne d'origine : muter un fichier ne doit pas le reformater.
	"""
	contenus = {}
	crlf = {}
	# Verification de TOUTES les ancres AVANT d'ecrire quoi que ce soit.
	for (fic, ancre, _) in edits:
		chemin = os.path.join(RACINE, fic.replace("/", os.sep))
		if fic not in contenus:
			with open(chemin, "rb") as f:
				brut = f.read().decode("utf-8")
			crlf[fic] = "\r\n" in brut
			contenus[fic] = brut.replace("\r\n", "\n")
		n = contenus[fic].count(ancre)
		if n != 1:
			return False, "ancrage x%d (1 attendu) dans %s -- ON NE MUTE PAS" % (n, fic)
	for (fic, ancre, remp) in edits:
		contenus[fic] = contenus[fic].replace(ancre, remp, 1)
	for fic, txt in contenus.items():
		if crlf[fic]:
			txt = txt.replace("\n", "\r\n")
		with open(os.path.join(RACINE, fic.replace("/", os.sep)), "wb") as f:
			f.write(txt.encode("utf-8"))
	return True, "ancrages x1"


def restaure(edits):
	fics = sorted({fic for (fic, _, _) in edits})
	_git("checkout", "--", *fics)
	propre, sale = arbre_propre()
	return propre, sale


def joue(cle):
	m = MUTATIONS[cle]
	print("\n" + "=" * 78)
	print("%s : %s" % (cle, m["quoi"]))
	print("   cas vise : %s" % m["cas"])
	print("   attendu  : %s" % m["attendu"])
	print("=" * 78)

	propre, sale = arbre_propre()
	if not propre:
		print("REFUS -- L ARBRE N EST PAS PROPRE. La restauration effacerait ce travail :")
		print(sale)
		print("Commite d abord. (C est la garde qui a coute deux nuits.)")
		return "REFUS"

	ok, msg = applique(m["edits"])
	print("   mutation : %s" % msg)
	if not ok:
		return "REFUS"

	try:
		bien, nberr, _ = construis()
		# GARDE 4 : le compte d'erreurs AVANT le banc.
		print("   construction : %s (%d erreur(s) lue(s))" % ("OK" if bien else "ECHEC", nberr))
		if not bien:
			print("   => la mutation NE COMPILE PAS : elle ne mesure rien. Verdict indisponible.")
			return "NE COMPILE PAS"
		code, sortie = joue_le_banc()
		if code != 0 and "cas, " not in sortie:
			print("   => LE BANC EST MORT (code %s). Une mutation qui fait tomber le banc ne" % hex(code & 0xFFFFFFFF))
			print("      prouve pas que le cas mesure ; elle prouve qu on ne peut plus le lire.")
			return "BANC MORT"
		print("   %s" % compte_echecs(sortie))
		ligne, etat = verdict_du_cas(sortie, m["cas"])
		print("   cas vise : %s" % (ligne if ligne else "(ABSENT DE LA SORTIE)"))
		attrapee = etat == "ECHEC"
		# une mutation est aussi attrapee si un AUTRE cas rougit
		autres = []
		for l in sortie.splitlines():
			nom, e = _etat(l)
			if e == "ECHEC" and nom != m["cas"]:
				autres.append(l.strip())
		if autres:
			print("   autres cas rouges : %d" % len(autres))
			for l in autres[:6]:
				print("      %s" % l[:150])
		verdict = "ATTRAPEE" if (attrapee or autres) else "A SURVECU"
		print("   VERDICT : %s%s" % (verdict, "" if attrapee else (" (par un autre cas)" if autres else "")))
		return verdict
	finally:
		propre, sale = restaure(m["edits"])
		print("   restauration : %s" % ("arbre propre" if propre else "⚠️ ARBRE SALE : " + sale))


def main():
	if "--liste" in sys.argv[1:] or not sys.argv[1:]:
		for k in sorted(MUTATIONS):
			print("%s  %s" % (k, MUTATIONS[k]["quoi"]))
			print("      cas : %s" % MUTATIONS[k]["cas"])
			print("      attendu : %s" % MUTATIONS[k]["attendu"])
		return 0
	resultats = []
	for cle in sys.argv[1:]:
		if cle not in MUTATIONS:
			print("mutation inconnue : %s" % cle)
			return 2
		resultats.append((cle, joue(cle)))
	print("\n" + "=" * 78)
	print("RECAPITULATIF")
	for cle, v in resultats:
		print("   %-5s %-14s (attendu : %s)" % (cle, v, MUTATIONS[cle]["attendu"].split(" -- ")[0]))
	print("=" * 78)
	return 0


if __name__ == "__main__":
	sys.exit(main())

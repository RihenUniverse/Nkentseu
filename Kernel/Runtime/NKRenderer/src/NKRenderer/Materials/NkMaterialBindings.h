#pragma once
// -----------------------------------------------------------------------------
// @File    NkMaterialBindings.h
// @Brief   Les numeros de binding du SET MATERIAU (set 2), en un seul endroit.
// @Author  Rihen
// @License Proprietary - All Rights Reserved (see LICENSE)
//
// POURQUOI CE FICHIER EXISTE — un defaut MESURE le 2026-08-22
//   En ajoutant le masque par texture, j'ai voulu verifier que le temoin
//   Demo4..Demo8 saurait attraper une erreur de binding. J'ai donc ecrit
//   volontairement le descripteur sur le binding 10, ABSENT du layout.
//
//   Resultat : **aucune erreur, aucun avertissement, aucune ligne de journal, et
//   les cinq signatures identiques.** Le moteur ecrit sans broncher un
//   descripteur sur un binding que son propre layout ne declare pas. La texture
//   n'arrive jamais au shader, et rien ne le dit.
//
//   C'est la forme exacte de la regle deja inscrite au CLAUDE.md parent — « un
//   parametre qui n'est pas honore est pire qu'un parametre absent ». Un
//   parametre absent casse ; un parametre ignore rend une image plausible.
//
//   Tant que le RHI ne saura pas refuser ce cas, la parade est ici : **UN SEUL
//   nombre, cite par les DEUX cotes**. Le C++ construit son layout et ecrit ses
//   descripteurs avec ces constantes ; le banc `NkMatGraphCheck` lit le SHADER
//   sur le disque et compare le binding qu'il y trouve a la constante. Les deux
//   chemins se valident l'un l'autre — changer un seul cote met le banc au
//   rouge, ce que le temoin d'images ne sait pas faire.
//
// AUCUNE DEPENDANCE, VOLONTAIREMENT. Ce fichier doit pouvoir etre inclus par un
//   banc console qui ne lie ni NKRenderer ni NKRHI. C'est ce qui rend la
//   verification croisee possible sans trainer un device.
// -----------------------------------------------------------------------------

namespace nkentseu {
	namespace renderer {

		// Set 2 = les ressources PAR INSTANCE de materiau. Le layout est UNIQUE et
		// partage par tous les archetypes : ajouter un binding ici le donne a
		// tout le monde, y compris aux gabarits qui n'en veulent pas.
		//
		// ⚠️ 0, 1, 2 sont pris par les sets globaux (camera, objet, lumieres) et
		// le binding 8 par l'UBO du materiau — d'ou le saut de 7 a 9. L'histoire
		// est dans NkMaterialSystem.cpp : un second Add(4, SAMPLER) ecrasait
		// l'Add(4, UBO) dans le tableau de descripteurs GL.
		enum : unsigned int {
			NK_MATBIND_ALBEDO = 3,
			NK_MATBIND_NORMAL_OR_MATCAP = 4,
			NK_MATBIND_ORM = 5,
			NK_MATBIND_EMISSIVE = 6,
			NK_MATBIND_HEIGHT = 7,
			NK_MATBIND_UBO = 8,
			// Carte de masque des couches (LayeredV1). Quatre canaux = quatre
			// masques dans une seule texture.
			NK_MATBIND_LAYER_MASK = 9,
		};

		// Nom du sampler tel qu'il est ECRIT dans le shader. Le banc le cherche
		// par ce nom : renommer d'un cote sans l'autre met le banc au rouge.
		static const char *const NK_MATBIND_LAYER_MASK_SAMPLER = "tMask";

	} // namespace renderer
} // namespace nkentseu

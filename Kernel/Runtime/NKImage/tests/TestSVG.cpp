// =============================================================================
// @File    TestSVG.cpp
// @Brief   Le banc du codec SVG maison (NkSVGCodec) : un SVG par palier, ECRIT
//          PAR LE TEST, decode, puis compare EN PIXELS a des valeurs attendues.
// @Author  TEUGUIA TADJUIDJE Rodolf Séderis — Rihen
// @License Proprietary - All Rights Reserved (see LICENSE)
//
// =============================================================================
//  POURQUOI LE TEST ECRIT SES PROPRES FICHIERS
// =============================================================================
//  Un banc qui lit un .svg versionne prouve deux choses a la fois (le fichier
//  et le decodeur) et ment le jour ou le fichier bouge. Ici chaque cas ECRIT le
//  SVG qu'il decode : le contenu attendu est SOUS LES YEUX, a cote des pixels
//  qu'on exige. Le seul fichier venu d'ailleurs est le temoin croise (l'export
//  de NkUIDesign), et c'est precisement son role.
//
//  CHAQUE CAS EXIGE DES PIXELS, jamais « ca a decode » : au moins quatre points
//  par element, choisis pour que la panne se voie (un coin qui doit etre VIDE
//  vaut mieux qu'un centre qui doit etre plein — le centre est plein meme quand
//  le rayon est ignore).
// =============================================================================

#include "NKImage/Codecs/SVG/NkSVGCodec.h"
#include "NKImage/Core/NkImage.h"

#include <cstdio>
#include <cstring>
#include <cmath>

using namespace nkentseu;

// ─────────────────────────────────────────────────────────────────────────────
// Le cadre : compter, dire, et rendre un code de sortie honnete
// ─────────────────────────────────────────────────────────────────────────────
namespace {

	int32 gPass = 0;
	int32 gTotal = 0;

	void Verifier(const char *nom, bool ok, const char *detail) {
		++gTotal;
		if (ok)
			++gPass;
		std::printf("%s %s\n", ok ? "  [ok]  " : "  [FAIL]", nom);
		if (detail && *detail)
			std::printf("         %s\n", detail);
	}

	/// Un pixel RGBA de l'image (0,0,0,0 hors bornes).
	void Pixel(const NkImage &im, int32 x, int32 y, uint8 out[4]) {
		out[0] = out[1] = out[2] = out[3] = 0;
		if (!im.IsValid() || !im.Pixels() || x < 0 || y < 0 || x >= im.Width() || y >= im.Height())
			return;
		const int32 ch = im.Channels();
		const uint8 *p = im.Pixels() + (usize)y * (usize)im.Stride() + (usize)x * (usize)ch;
		out[0] = p[0];
		out[1] = ch > 1 ? p[1] : p[0];
		out[2] = ch > 2 ? p[2] : p[0];
		out[3] = ch > 3 ? p[3] : 255;
	}

	/// Le pixel (x,y) est-il proche de (r,g,b) a @p tol pres, et OPAQUE ?
	bool Proche(const NkImage &im, int32 x, int32 y, int32 r, int32 g, int32 b, int32 tol) {
		uint8 c[4];
		Pixel(im, x, y, c);
		if (c[3] < 200)
			return false;
		return std::abs((int32)c[0] - r) <= tol && std::abs((int32)c[1] - g) <= tol &&
			   std::abs((int32)c[2] - b) <= tol;
	}

	/// Couverture : alpha du pixel (ce que « vide » veut dire, sans supposer une couleur).
	int32 AlphaDe(const NkImage &im, int32 x, int32 y) {
		uint8 c[4];
		Pixel(im, x, y, c);
		return (int32)c[3];
	}

	int32 Gris(const NkImage &im, int32 x, int32 y) {
		uint8 c[4];
		Pixel(im, x, y, c);
		return ((int32)c[0] + (int32)c[1] + (int32)c[2]) / 3;
	}

	/// Decode un SVG depuis une chaine C (la taille = strlen), a la taille voulue.
	NkImage Decoder(const char *svg, int32 w = 0, int32 h = 0) {
		return NkSVGCodec::Decode((const uint8 *)svg, std::strlen(svg), w, h);
	}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// PALIER 1 — rx / ry sur <rect> : la regle CSS, et des ARCS
// ─────────────────────────────────────────────────────────────────────────────
static void TestRxRy() {
	std::printf("\n== PALIER 1 : rx / ry sur <rect> ==\n");

	// (a) rx=20 seul : ry doit VALOIR 20 (regle CSS), les quatre coins evides.
	//     Le coin (11,11) est le point qui rougit si rx est ignore : sans arrondi
	//     il est rouge, avec arrondi il est vide. C'est LA mutation du palier.
	static const char *kRx =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
		"<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" rx=\"20\" fill=\"#ff0000\"/>"
		"</svg>";
	NkImage a = Decoder(kRx);
	char det[512];
	const bool taille = a.IsValid() && a.Width() == 100 && a.Height() == 100;
	// centre plein ; milieux des quatre cotes pleins ; QUATRE COINS VIDES
	const bool plein = taille && Proche(a, 50, 50, 255, 0, 0, 2) && Proche(a, 50, 12, 255, 0, 0, 2) &&
					   Proche(a, 12, 50, 255, 0, 0, 2) && Proche(a, 87, 50, 255, 0, 0, 2) &&
					   Proche(a, 50, 87, 255, 0, 0, 2);
	const int32 c0 = AlphaDe(a, 11, 11), c1 = AlphaDe(a, 88, 11), c2 = AlphaDe(a, 11, 88), c3 = AlphaDe(a, 88, 88);
	const bool coinsVides = taille && c0 < 40 && c1 < 40 && c2 < 40 && c3 < 40;
	// et le coin n'est pas COUPE en biais : (25,25) est DANS le disque de l'arc
	// (centre 30,30 rayon 20) alors qu'un chanfrein droit l'aurait exclu
	const bool arcPasChanfrein = taille && Proche(a, 25, 25, 255, 0, 0, 2);
	std::snprintf(det, sizeof(det),
				  "100x100=%d ; centre et milieux pleins=%d ; alpha des quatre coins=%d/%d/%d/%d (vides=%d) ; "
				  "(25,25) dans l'arc=%d",
				  taille ? 1 : 0, plein ? 1 : 0, c0, c1, c2, c3, coinsVides ? 1 : 0, arcPasChanfrein ? 1 : 0);
	Verifier("1a. rx=\"20\" seul : ry en herite (regle CSS), les quatre coins sont EVIDES, et le coin est un ARC "
			 "(le point (25,25) y est, un chanfrein l'aurait coupe)",
			 taille && plein && coinsVides && arcPasChanfrein, det);

	// (b) le bornage a la moitie du cote : rx=999 sur un carre 80 -> 40 = un disque.
	static const char *kBorne =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
		"<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" rx=\"999\" ry=\"999\" fill=\"#00ff00\"/>"
		"</svg>";
	NkImage b = Decoder(kBorne);
	// un disque de centre (50,50) rayon 40 : (50,50) plein, (50,12) plein (bord),
	// (20,20) VIDE (distance 42.4 > 40), (25,25) plein (distance 35.3 < 40)
	const bool disque = b.IsValid() && Proche(b, 50, 50, 0, 255, 0, 2) && Proche(b, 50, 12, 0, 255, 0, 2) &&
						AlphaDe(b, 20, 20) < 40 && Proche(b, 25, 25, 0, 255, 0, 2);
	std::snprintf(det, sizeof(det), "centre plein=%d ; (20,20) alpha=%d (hors du disque) ; (25,25) alpha=%d (dedans)",
				  b.IsValid() && Proche(b, 50, 50, 0, 255, 0, 2) ? 1 : 0, AlphaDe(b, 20, 20), AlphaDe(b, 25, 25));
	Verifier("1b. rx/ry BORNES a la moitie du cote : rx=999 sur un carre de 80 donne un disque de rayon 40", disque,
			 det);

	// (c) rx et ry DIFFERENTS : l'arrondi est elliptique, pas circulaire.
	static const char *kEllip =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
		"<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" rx=\"40\" ry=\"8\" fill=\"#0000ff\"/>"
		"</svg>";
	NkImage c = Decoder(kEllip);
	// coin haut-gauche : l'ellipse a pour centre (50,18), demi-axes (40,8).
	//   (12,12) : ((12-50)/40)^2 + ((12-18)/8)^2 = 0.90 + 0.56 = 1.46 > 1 -> VIDE
	//   (12,25) : sous le centre de l'ellipse, dans la bande droite -> PLEIN
	const bool ellipse = c.IsValid() && AlphaDe(c, 12, 12) < 60 && Proche(c, 12, 25, 0, 0, 255, 2) &&
						 Proche(c, 50, 12, 0, 0, 255, 2);
	std::snprintf(det, sizeof(det), "(12,12) alpha=%d (hors de l'ellipse) ; (12,25) bleu=%d ; (50,12) bleu=%d",
				  AlphaDe(c, 12, 12), Proche(c, 12, 25, 0, 0, 255, 2) ? 1 : 0, Proche(c, 50, 12, 0, 0, 255, 2) ? 1 : 0);
	Verifier("1c. rx=40 ry=8 : l'arrondi est ELLIPTIQUE (le coin est vide la ou un arrondi circulaire serait plein)",
			 ellipse, det);
}

// ─────────────────────────────────────────────────────────────────────────────
// CE QUE LE CODEC SAUTE : il doit le DIRE, une fois par nom
// ─────────────────────────────────────────────────────────────────────────────
static void TestNonGere() {
	std::printf("\n== CE QUI EST SAUTE SE DIT ==\n");
	static const char *kInconnu =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\" viewBox=\"0 0 40 40\">"
		"<defs><symbol id=\"s\"><rect width=\"5\" height=\"5\"/></symbol></defs>"
		"<use href=\"#s\"/><use href=\"#s\"/>"
		"<clipPath id=\"c\"><rect width=\"5\" height=\"5\"/></clipPath>"
		"<rect x=\"0\" y=\"0\" width=\"40\" height=\"40\" fill=\"#808080\" stroke=\"#000\" "
		"stroke-dasharray=\"4 2\"/>"
		"</svg>";
	NkSVGImage *img = NkSVGImage::LoadFromMemory((const uint8 *)kInconnu, std::strlen(kInconnu));
	char det[512] = {0};
	bool useDit = false, clipDit = false, dashDit = false, useUneFois = true;
	int32 nb = 0;
	if (img) {
		nb = img->SkippedCount();
		int32 nbUse = 0;
		for (int32 i = 0; i < nb; ++i) {
			const char *n = img->SkippedAt(i);
			if (!n)
				continue;
			if (std::strcmp(n, "use") == 0) {
				useDit = true;
				++nbUse;
			}
			if (std::strcmp(n, "clipPath") == 0)
				clipDit = true;
			if (std::strcmp(n, "stroke-dasharray") == 0)
				dashDit = true;
		}
		useUneFois = (nbUse == 1); // DEUX <use> dans le fichier, UNE seule mention
		std::snprintf(det, sizeof(det), "%d nom(s) saute(s) : ", nb);
		for (int32 i = 0; i < nb; ++i) {
			std::strncat(det, img->SkippedAt(i) ? img->SkippedAt(i) : "?", sizeof(det) - std::strlen(det) - 1);
			std::strncat(det, " ", sizeof(det) - std::strlen(det) - 1);
		}
		img->Free();
	}
	Verifier("Les elements et attributs NON GERES sont nommes (use, clipPath, stroke-dasharray), et DEUX <use> ne "
			 "donnent QU'UNE mention (une fois par nom, jamais en silence, jamais en boucle)",
			 img != nullptr && useDit && clipDit && dashDit && useUneFois, det);
}

// ─────────────────────────────────────────────────────────────────────────────
int TestSVG_Run() {
	gPass = 0;
	gTotal = 0;
	std::printf("\n===== BANC DU CODEC SVG (NkSVGCodec) =====\n");
	TestRxRy();
	TestNonGere();
	std::printf("\n===== SVG : %d / %d =====\n", gPass, gTotal);
	return (gPass == gTotal) ? 0 : 1;
}

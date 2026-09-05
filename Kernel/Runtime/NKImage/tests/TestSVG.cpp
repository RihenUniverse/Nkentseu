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
#include "NKContainers/String/Encoding/NkBase64.h"
#include "NKMemory/NkAllocator.h"
// LE BANC lie NKFont pour eprouver l'INJECTION -- NKImage, lui, ne le connait
// pas (aucune arete dans les .jenga ; le cas « sans source » ci-dessous le prouve
// autrement : sans injection, le codec ne peut PAS peindre un glyphe).
#include "NKFont/Text/NkFontGlyphSource.h"

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
// PALIER 2 — <image> : « data: » base64 ET href relatif au fichier SVG
// ─────────────────────────────────────────────────────────────────────────────

/// Un damier 2x2 : magenta / vert / vert / magenta. Ses quatre texels sont des
/// couleurs PURES, donc reconnaissables apres n'importe quel etirement (le bord
/// est repete : les quatre coins de la boite rendent les quatre texels tels quels).
static NkImage Damier2x2() {
	NkImage im = NkImage::Create(2u, 2u, 4);
	if (!im.IsValid() || !im.Pixels())
		return im;
	static const uint8 kM[4] = {255, 0, 255, 255}, kV[4] = {0, 255, 0, 255};
	uint8 *px = im.Pixels();
	for (int32 y = 0; y < 2; ++y)
		for (int32 x = 0; x < 2; ++x) {
			const uint8 *c = ((x + y) & 1) ? kV : kM;
			for (int32 k = 0; k < 4; ++k)
				px[(y * 2 + x) * 4 + k] = c[k];
		}
	return im;
}

static void TestImage() {
	std::printf("\n== PALIER 2 : <image> ==\n");
	char det[640];

	NkImage src = Damier2x2();
	// le PNG du damier, en memoire, puis en base64 : la source des deux cas
	uint8 *pngOctets = nullptr;
	usize pngTaille = 0;
	const bool encode = src.IsValid() && src.SaveToMemory(pngOctets, pngTaille) && pngOctets && pngTaille > 0;
	NkString b64;
	if (encode)
		b64 = nkentseu::encoding::base64::NkEncode(pngOctets, pngTaille);

	// (a) data:image/png;base64 — etire (« none ») sur 40x40 : les quatre texels
	//     aux quatre coins, purs.
	static char svg[8192];
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\" viewBox=\"0 0 40 40\">"
				  "<image x=\"0\" y=\"0\" width=\"40\" height=\"40\" preserveAspectRatio=\"none\" "
				  "href=\"data:image/png;base64,%s\"/></svg>",
				  encode ? b64.Data() : "");
	NkImage a = Decoder(svg);
	const bool quatreTexels = a.IsValid() && a.Width() == 40 && Proche(a, 3, 3, 255, 0, 255, 4) &&
							  Proche(a, 36, 3, 0, 255, 0, 4) && Proche(a, 3, 36, 0, 255, 0, 4) &&
							  Proche(a, 36, 36, 255, 0, 255, 4);
	std::snprintf(det, sizeof(det), "PNG encode=%d (%u octets, base64 %u car.) ; 40x40=%d ; quatre texels purs=%d",
				  encode ? 1 : 0, (uint32)pngTaille, (uint32)b64.Length(), a.IsValid() && a.Width() == 40 ? 1 : 0,
				  quatreTexels ? 1 : 0);
	Verifier("2a. <image href=\"data:image/png;base64,...\"> : le base64 est decode, le PNG relu par les codecs "
			 "NKImage, et les quatre texels du damier arrivent PURS aux quatre coins (preserveAspectRatio=none)",
			 encode && quatreTexels, det);

	// (b) un base64 ABIME ne doit RIEN peindre — et le dire. Un decodeur qui
	//     « devine » peindrait du bruit ; ici la boite reste vide.
	static char svgKo[8192];
	{
		NkString abime = b64;
		char *d = abime.Data();
		if (d && abime.Length() > 8)
			d[4] = '@'; // caractere hors alphabet base64
		std::snprintf(svgKo, sizeof(svgKo),
					  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\" viewBox=\"0 0 40 40\">"
					  "<image x=\"0\" y=\"0\" width=\"40\" height=\"40\" href=\"data:image/png;base64,%s\"/></svg>",
					  d ? d : "");
	}
	NkImage ko = Decoder(svgKo);
	const bool rienPeint = ko.IsValid() && AlphaDe(ko, 3, 3) < 20 && AlphaDe(ko, 20, 20) < 20 &&
						   AlphaDe(ko, 36, 36) < 20;
	std::snprintf(det, sizeof(det), "alpha (3,3)=%d (20,20)=%d (36,36)=%d -- la boite reste VIDE",
				  AlphaDe(ko, 3, 3), AlphaDe(ko, 20, 20), AlphaDe(ko, 36, 36));
	Verifier("2b. un base64 ABIME ne peint RIEN (le decodeur relaie l'echec au lieu de deviner des octets)", rienPeint,
			 det);

	// (c) href RELATIF, resolu par rapport au dossier du .svg — pas au repertoire
	//     courant. On ecrit le PNG a cote, et on decode en disant ce dossier.
	const char *dossier = "Build";
	const char *chemin = "Build/nksvg_test_damier.png";
	const bool ecrit = src.IsValid() && src.SavePNG(chemin);
	static const char *kRel =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\" viewBox=\"0 0 40 40\">"
		"<image x=\"0\" y=\"0\" width=\"40\" height=\"40\" preserveAspectRatio=\"none\" "
		"href=\"nksvg_test_damier.png\"/></svg>";
	NkSVGImage *rel = NkSVGImage::LoadFromMemory((const uint8 *)kRel, std::strlen(kRel), dossier);
	NkImage r = rel ? rel->Rasterize(0, 0) : NkImage();
	const bool relOk = ecrit && rel && Proche(r, 3, 3, 255, 0, 255, 4) && Proche(r, 36, 3, 0, 255, 0, 4) &&
					   Proche(r, 3, 36, 0, 255, 0, 4) && Proche(r, 36, 36, 255, 0, 255, 4);
	if (rel)
		rel->Free();
	std::snprintf(det, sizeof(det), "PNG ecrit dans %s=%d ; decode avec baseDir=\"%s\" -> quatre texels=%d", chemin,
				  ecrit ? 1 : 0, dossier, relOk ? 1 : 0);
	Verifier("2c. <image href=\"...png\"> RELATIF : resolu depuis le DOSSIER DU SVG (baseDir), pas depuis le "
			 "repertoire courant du processus",
			 relOk, det);

	// (d) preserveAspectRatio=\"xMidYMid meet\" : une image 4x2 dans une boite
	//     carree laisse des BANDES VIDES en haut et en bas, et ne se deforme pas.
	NkImage large = NkImage::Create(4u, 2u, 4, 0xFF0000FFu); // rouge opaque
	uint8 *lo = nullptr;
	usize lt = 0;
	NkString b64Large;
	if (large.IsValid() && large.SaveToMemory(lo, lt) && lo && lt)
		b64Large = nkentseu::encoding::base64::NkEncode(lo, lt);
	static char svgMeet[8192];
	std::snprintf(svgMeet, sizeof(svgMeet),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\" viewBox=\"0 0 40 40\">"
				  "<image x=\"0\" y=\"0\" width=\"40\" height=\"40\" preserveAspectRatio=\"xMidYMid meet\" "
				  "href=\"data:image/png;base64,%s\"/></svg>",
				  b64Large.Length() ? b64Large.Data() : "");
	NkImage m = Decoder(svgMeet);
	// echelle = min(40/4, 40/2) = 10 -> l'image occupe 40 x 20, centree : y de 10 a 30
	const bool bandes = m.IsValid() && AlphaDe(m, 20, 3) < 20 && AlphaDe(m, 20, 36) < 20 &&
						Proche(m, 20, 20, 255, 0, 0, 4) && Proche(m, 3, 20, 255, 0, 0, 4);
	std::snprintf(det, sizeof(det), "bande haute alpha=%d ; bande basse alpha=%d ; centre rouge=%d ; bord gauche a "
								   "mi-hauteur rouge=%d",
				  AlphaDe(m, 20, 3), AlphaDe(m, 20, 36), Proche(m, 20, 20, 255, 0, 0, 4) ? 1 : 0,
				  Proche(m, 3, 20, 255, 0, 0, 4) ? 1 : 0);
	Verifier("2d. preserveAspectRatio=\"xMidYMid meet\" : une image 4x2 dans une boite carree garde son rapport et "
			 "laisse des BANDES VIDES (elle n'est pas etiree)",
			 bandes, det);

	// (e) l'image suit le TRANSFORM du groupe parent, et son `opacity`.
	static char svgRot[8192];
	std::snprintf(svgRot, sizeof(svgRot),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"80\" viewBox=\"0 0 80 80\">"
				  "<g transform=\"rotate(45 40 40)\">"
				  "<image x=\"20\" y=\"20\" width=\"40\" height=\"40\" preserveAspectRatio=\"none\" opacity=\"0.5\" "
				  "href=\"data:image/png;base64,%s\"/></g></svg>",
				  encode ? b64.Data() : "");
	NkImage rot = Decoder(svgRot);
	// tournee de 45 deg autour du centre, la boite devient un losange : le centre
	// reste couvert, mais le COIN de la boite droite (22,22) sort du losange.
	const int32 aCentre = AlphaDe(rot, 40, 40), aCoin = AlphaDe(rot, 22, 22);
	const bool tourne = rot.IsValid() && aCentre > 100 && aCentre < 160 && aCoin < 40;
	std::snprintf(det, sizeof(det),
				  "alpha au centre=%d (opacity 0.5 -> ~128, pas 255) ; alpha au coin de la boite DROITE (22,22)=%d "
				  "(hors du losange)",
				  aCentre, aCoin);
	Verifier("2e. l'<image> suit le transform du groupe parent (rotation : sa boite devient un losange) et son "
			 "`opacity` (alpha ~128, pas 255)",
			 tourne, det);

	if (pngOctets)
		nkentseu::memory::NkFree(pngOctets);
	if (lo)
		nkentseu::memory::NkFree(lo);
}

// ─────────────────────────────────────────────────────────────────────────────
// PALIER 3 — <text> : les contours de NKFont, la ligne de base, l'ancrage
// ─────────────────────────────────────────────────────────────────────────────

/// L'ENCRE d'une zone : combien de pixels sombres, et ou. C'est la mesure juste
/// pour du texte -- exiger un pixel PRECIS sur un glyphe reviendrait a figer le
/// dessin d'une police, qui n'est pas notre contrat.
struct Encre {
		int32 sombres = 0;
		int32 xmin = 1 << 20, xmax = -1, ymin = 1 << 20, ymax = -1;
};

static Encre EncreDe(const NkImage &im, int32 x0, int32 y0, int32 x1, int32 y1) {
	Encre e;
	for (int32 y = y0; y < y1; ++y)
		for (int32 x = x0; x < x1; ++x) {
			if (Gris(im, x, y) < 128 && AlphaDe(im, x, y) > 128) {
				++e.sombres;
				if (x < e.xmin) e.xmin = x;
				if (x > e.xmax) e.xmax = x;
				if (y < e.ymin) e.ymin = y;
				if (y > e.ymax) e.ymax = y;
			}
		}
	return e;
}

/// Une page blanche de 200x80 avec le <text> demande dedans.
static void PageTexte(char *out, usize n, const char *attrsEtContenu) {
	std::snprintf(out, n,
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"80\" viewBox=\"0 0 200 80\">"
				  "<rect x=\"0\" y=\"0\" width=\"200\" height=\"80\" fill=\"#ffffff\"/>"
				  "%s</svg>",
				  attrsEtContenu);
}

static void TestTexte() {
	std::printf("\n== PALIER 3 : <text> ==\n");
	char det[640];
	static char svg[4096];

	// (a) LA LIGNE DE BASE. « y » est la ligne de base, pas le haut de la boite :
	//     un « H » (sans jambage) doit poser TOUTE son encre AU-DESSUS de y.
	PageTexte(svg, sizeof(svg),
			  "<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"40\" fill=\"#000000\">H</text>");
	NkImage a = Decoder(svg);
	Encre ea = a.IsValid() ? EncreDe(a, 0, 0, 200, 80) : Encre();
	const bool auDessus = ea.sombres > 60 && ea.ymax <= 61 && ea.ymin > 20 && ea.xmin >= 19;
	std::snprintf(det, sizeof(det),
				  "encre=%d px ; boite x[%d..%d] y[%d..%d] ; la ligne de base est y=60 -> rien sous 61=%d",
				  ea.sombres, ea.xmin, ea.xmax, ea.ymin, ea.ymax, ea.ymax <= 61 ? 1 : 0);
	Verifier("3a. <text> est PEINT (des glyphes, pas une boite) et « y » est bien la LIGNE DE BASE : un H pose "
			 "toute son encre au-dessus",
			 auDessus, det);

	// (b) font-size = le CADRATIN : doubler le corps double la hauteur et
	//     quadruple (a peu pres) la quantite d'encre.
	PageTexte(svg, sizeof(svg),
			  "<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"20\" fill=\"#000000\">H</text>");
	NkImage b = Decoder(svg);
	Encre eb = b.IsValid() ? EncreDe(b, 0, 0, 200, 80) : Encre();
	const int32 h40 = ea.ymax - ea.ymin + 1, h20 = eb.ymax - eb.ymin + 1;
	const bool proportionnel = eb.sombres > 10 && h20 > 0 && h40 >= 2 * h20 - 3 && h40 <= 2 * h20 + 3 &&
							   ea.sombres > (eb.sombres * 5) / 2 && ea.sombres < eb.sombres * 6;
	std::snprintf(det, sizeof(det), "corps 20 : %d px de haut, %d d'encre ; corps 40 : %d px de haut, %d d'encre",
				  h20, eb.sombres, h40, ea.sombres);
	Verifier("3b. font-size est une ECHELLE reelle : le corps double donne deux fois la hauteur et ~quatre fois "
			 "l'encre (les contours suivent, rien n'est etire depuis un atlas)",
			 proportionnel, det);

	// (c) text-anchor : start / middle / end deplacent le texte AUTOUR de x.
	PageTexte(svg, sizeof(svg), "<text x=\"100\" y=\"60\" font-family=\"Inter\" font-size=\"30\" fill=\"#000000\" "
								"text-anchor=\"start\">Ab</text>");
	Encre s0 = EncreDe(Decoder(svg), 0, 0, 200, 80);
	PageTexte(svg, sizeof(svg), "<text x=\"100\" y=\"60\" font-family=\"Inter\" font-size=\"30\" fill=\"#000000\" "
								"text-anchor=\"middle\">Ab</text>");
	Encre s1 = EncreDe(Decoder(svg), 0, 0, 200, 80);
	PageTexte(svg, sizeof(svg), "<text x=\"100\" y=\"60\" font-family=\"Inter\" font-size=\"30\" fill=\"#000000\" "
								"text-anchor=\"end\">Ab</text>");
	Encre s2 = EncreDe(Decoder(svg), 0, 0, 200, 80);
	const int32 largeur = s0.xmax - s0.xmin;
	const bool ancrage = s0.sombres > 40 && s1.sombres > 40 && s2.sombres > 40 && s0.xmin >= 99 && s2.xmax <= 101 &&
						 s1.xmin < s0.xmin && s1.xmin > s2.xmin &&
						 std::abs((s0.xmin + s0.xmax) / 2 - (s1.xmin + s1.xmax) / 2 - largeur / 2) <= 3;
	std::snprintf(det, sizeof(det), "start x[%d..%d] ; middle x[%d..%d] ; end x[%d..%d] (x=100, largeur %d)", s0.xmin,
				  s0.xmax, s1.xmin, s1.xmax, s2.xmin, s2.xmax, largeur);
	Verifier("3c. text-anchor : « start » commence a x, « end » y finit, « middle » centre dessus -- la largeur "
			 "totale est donc bien mesuree AVANT de poser les glyphes",
			 ancrage, det);

	// (d) fill : le texte prend sa couleur (et pas le noir par defaut).
	PageTexte(svg, sizeof(svg),
			  "<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"40\" fill=\"#ff0000\">H</text>");
	NkImage d = Decoder(svg);
	int32 rouges = 0;
	for (int32 y = 10; y < 62; ++y)
		for (int32 x = 18; x < 70; ++x)
			if (Proche(d, x, y, 255, 0, 0, 6))
				++rouges;
	std::snprintf(det, sizeof(det), "%d pixels rouges purs dans la boite du glyphe", rouges);
	Verifier("3d. <text fill=\"#ff0000\"> : les glyphes prennent la couleur demandee", rouges > 40, det);

	// (e) <tspan> : il continue le texte, et un x/y explicite REPOSITIONNE.
	PageTexte(svg, sizeof(svg), "<text x=\"20\" y=\"40\" font-family=\"Inter\" font-size=\"20\" fill=\"#000000\">"
								"H<tspan x=\"120\" y=\"70\">H</tspan></text>");
	NkImage e = Decoder(svg);
	Encre haut = EncreDe(e, 0, 0, 200, 50);
	Encre bas = EncreDe(e, 0, 50, 200, 80);
	const bool tspan = haut.sombres > 10 && bas.sombres > 10 && haut.xmin >= 19 && haut.xmin < 40 &&
					   bas.xmin >= 119 && bas.xmin < 145;
	std::snprintf(det, sizeof(det), "premier H : x[%d..%d] y[%d..%d] ; le <tspan x=120 y=70> : x[%d..%d] y[%d..%d]",
				  haut.xmin, haut.xmax, haut.ymin, haut.ymax, bas.xmin, bas.xmax, bas.ymin, bas.ymax);
	Verifier("3e. <tspan x= y=> REPOSITIONNE le curseur (les deux H sont a deux endroits distincts, pas empiles)",
			 tspan, det);

	// (f) une police INTROUVABLE : on rend quand meme, avec Inter, et ON LE DIT.
	static const char *kInconnue =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"80\" viewBox=\"0 0 200 80\">"
		"<rect x=\"0\" y=\"0\" width=\"200\" height=\"80\" fill=\"#ffffff\"/>"
		"<text x=\"20\" y=\"60\" font-family=\"UnePoliceQuiNExistePas\" font-size=\"40\">H</text></svg>";
	NkSVGImage *img = NkSVGImage::LoadFromMemory((const uint8 *)kInconnue, std::strlen(kInconnue));
	bool repliDit = false;
	int32 encreRepli = 0;
	if (img) {
		for (int32 i = 0; i < img->SkippedCount(); ++i) {
			const char *n = img->SkippedAt(i);
			if (n && std::strncmp(n, "police:", 7) == 0)
				repliDit = true;
		}
		NkImage r = img->Rasterize(0, 0);
		encreRepli = EncreDe(r, 0, 0, 200, 80).sombres;
		img->Free();
	}
	std::snprintf(det, sizeof(det), "repli nomme=%d ; encre du texte replie=%d px", repliDit ? 1 : 0, encreRepli);
	Verifier("3f. une font-family INTROUVABLE : le texte est peint avec la police de repli (Inter) ET le repli est "
			 "NOMME -- rendre autre chose que ce qui est demande sans le dire serait le vrai defaut",
			 repliDit && encreRepli > 60, det);

	// (g) l'INDENTATION d'un fichier ne devient pas du texte : sans
	//     xml:space=\"preserve\", les blancs sont reduits et les bords rognes.
	static const char *kBlancs =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"80\" viewBox=\"0 0 200 80\">"
		"<rect x=\"0\" y=\"0\" width=\"200\" height=\"80\" fill=\"#ffffff\"/>"
		"<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"30\" fill=\"#000000\">\n      H\n   </text></svg>";
	Encre blancs = EncreDe(Decoder(kBlancs), 0, 0, 200, 80);
	const bool rogne = blancs.sombres > 20 && blancs.xmin >= 19 && blancs.xmin < 32;
	std::snprintf(det, sizeof(det), "le H indente commence a x=%d (x demande : 20 -- les blancs de tete sont rognes)",
				  blancs.xmin);
	Verifier("3g. les blancs de mise en forme du FICHIER ne deviennent pas du texte : sans xml:space=\"preserve\" "
			 "ils sont rognes, le glyphe commence bien a x",
			 rogne, det);
	// (i) AUCUNE SOURCE DE GLYPHES : le texte est SAUTE **ET DIT**, jamais rendu
	//     vide en silence. C'est la contrepartie de « NKImage ne depend pas de
	//     NKFont » : qui ne fournit pas de police n'a pas de texte, et l'apprend.
	{
		NkIGlyphSource *garde = NkSVGCodec::GetDefaultGlyphSource();
		NkSVGCodec::SetDefaultGlyphSource(nullptr);
		static const char *kSansSource =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"80\" viewBox=\"0 0 200 80\">"
			"<rect x=\"0\" y=\"0\" width=\"200\" height=\"80\" fill=\"#ffffff\"/>"
			"<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"40\" fill=\"#000000\">H</text></svg>";
		NkSVGImage *sans = NkSVGImage::LoadFromMemory((const uint8 *)kSansSource, std::strlen(kSansSource));
		bool texteDit = false;
		int32 encreSans = -1;
		if (sans) {
			for (int32 k = 0; k < sans->SkippedCount(); ++k) {
				const char *n = sans->SkippedAt(k);
				if (n && std::strcmp(n, "text") == 0)
					texteDit = true;
			}
			NkImage r = sans->Rasterize(0, 0);
			encreSans = EncreDe(r, 0, 0, 200, 80).sombres;
			sans->Free();
		}
		// et AVEC la source rendue, le meme document peint de nouveau
		NkSVGCodec::SetDefaultGlyphSource(garde);
		NkImage avec = Decoder(kSansSource);
		const int32 encreAvec = EncreDe(avec, 0, 0, 200, 80).sombres;
		std::snprintf(det, sizeof(det),
					  "sans source : « text » nomme=%d, encre=%d (le fond blanc est peint quand meme) ; avec la "
					  "source injectee : encre=%d",
					  texteDit ? 1 : 0, encreSans, encreAvec);
		Verifier("3i. SANS source de glyphes injectee, <text> est SAUTE ET NOMME (le reste du document est peint "
				 "normalement) ; la meme source injectee, il est peint -- l'injection est le seul chemin",
				 texteDit && encreSans == 0 && encreAvec > 60, det);
	}

	// (h) L'AVANCE : trois H cote a cote occupent ~trois fois la largeur d'un seul.
	//     Sans avance, les glyphes s'EMPILENT au meme x -- la boite resterait
	//     celle d'un seul glyphe, et l'encre serait a peine plus dense.
	PageTexte(svg, sizeof(svg),
			  "<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"30\" fill=\"#000000\">H</text>");
	Encre un = EncreDe(Decoder(svg), 0, 0, 200, 80);
	PageTexte(svg, sizeof(svg),
			  "<text x=\"20\" y=\"60\" font-family=\"Inter\" font-size=\"30\" fill=\"#000000\">HHH</text>");
	Encre trois = EncreDe(Decoder(svg), 0, 0, 200, 80);
	const int32 l1 = un.xmax - un.xmin + 1, l3 = trois.xmax - trois.xmin + 1;
	const bool avance = un.sombres > 20 && l1 > 0 && l3 >= 2 * l1 && trois.sombres >= 2 * un.sombres &&
						trois.ymin == un.ymin && trois.ymax == un.ymax;
	std::snprintf(det, sizeof(det), "un H : %d px de large, %d d'encre ; trois H : %d px de large, %d d'encre "
								   "(meme hauteur y[%d..%d] contre y[%d..%d])",
				  l1, un.sombres, l3, trois.sombres, trois.ymin, trois.ymax, un.ymin, un.ymax);
	Verifier("3h. L'AVANCE d'un glyphe a l'autre : trois H occupent au moins deux fois la largeur d'un seul et "
			 "portent au moins deux fois l'encre -- ils ne s'empilent pas",
			 avance, det);
}


// ─────────────────────────────────────────────────────────────────────────────
// PALIER 4 — les degrades COMPLETS : gradientTransform partout, fx/fy, href
// ─────────────────────────────────────────────────────────────────────────────

/// Une page 100x100 avec des <defs> et un rect qui les utilise.
static void PageDegrade(char *out, usize n, const char *defs, const char *fill) {
	std::snprintf(out, n,
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
				  "<defs>%s</defs>"
				  "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"%s\"/></svg>",
				  defs, fill);
}

static void TestDegrades() {
	std::printf("\n== PALIER 4 : degrades ==\n");
	char det[640];
	static char svg[4096];

	// (a) gradientTransform en objectBoundingBox. Un degrade HORIZONTAL tourne de
	//     90 degres doit devenir VERTICAL. C'etait le defaut : gradientTransform
	//     n'etait honore qu'en userSpaceOnUse, et IGNORE EN SILENCE ici.
	PageDegrade(svg, sizeof(svg),
				"<linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\" "
				"gradientTransform=\"rotate(90 0.5 0.5)\">"
				"<stop offset=\"0\" stop-color=\"#000000\"/><stop offset=\"1\" stop-color=\"#ffffff\"/>"
				"</linearGradient>",
				"url(#g)");
	NkImage a = Decoder(svg);
	const int32 hautG = Gris(a, 10, 10), hautD = Gris(a, 90, 10);
	const int32 basG = Gris(a, 10, 90), basD = Gris(a, 90, 90);
	// vertical : ca varie du haut vers le bas, et PAS de gauche a droite
	const bool vertical = a.IsValid() && hautG < basG - 100 && std::abs(hautG - hautD) < 12 &&
						  std::abs(basG - basD) < 12;
	std::snprintf(det, sizeof(det), "gris : haut %d/%d, bas %d/%d -- vertical (haut != bas, gauche == droite)=%d",
				  hautG, hautD, basG, basD, vertical ? 1 : 0);
	Verifier("4a. gradientTransform est honore AUSSI en objectBoundingBox : « rotate(90) » sur un degrade "
			 "horizontal le rend VERTICAL (il etait ignore en silence dans ce mode)",
			 vertical, det);

	// (b) le FOYER d'un radial (fx/fy) : le blanc est AU FOYER, pas au centre.
	PageDegrade(svg, sizeof(svg),
				"<radialGradient id=\"r\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\" fx=\"0.2\" fy=\"0.2\">"
				"<stop offset=\"0\" stop-color=\"#ffffff\"/><stop offset=\"1\" stop-color=\"#000000\"/>"
				"</radialGradient>",
				"url(#r)");
	NkImage b = Decoder(svg);
	const int32 auFoyer = Gris(b, 20, 20), auCentre = Gris(b, 50, 50), oppose = Gris(b, 80, 80);
	const bool foyer = b.IsValid() && auFoyer > auCentre + 20 && auCentre > oppose;
	std::snprintf(det, sizeof(det), "gris au foyer (20,20)=%d ; au centre (50,50)=%d ; a l'oppose (80,80)=%d",
				  auFoyer, auCentre, oppose);
	Verifier("4b. <radialGradient fx fy> : le point le plus clair est LE FOYER, pas le centre du cercle", foyer, det);

	// (c) sans fx/fy le foyer EST le centre : le degrade est concentrique.
	PageDegrade(svg, sizeof(svg),
				"<radialGradient id=\"r2\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\">"
				"<stop offset=\"0\" stop-color=\"#ffffff\"/><stop offset=\"1\" stop-color=\"#000000\"/>"
				"</radialGradient>",
				"url(#r2)");
	NkImage c = Decoder(svg);
	const int32 c0 = Gris(c, 50, 50), c1 = Gris(c, 20, 20), c2 = Gris(c, 80, 80), c3 = Gris(c, 80, 20);
	const bool concentrique = c.IsValid() && c0 > c1 + 20 && std::abs(c1 - c2) < 12 && std::abs(c1 - c3) < 12;
	std::snprintf(det, sizeof(det), "centre=%d ; les trois coins a egale distance : %d / %d / %d", c0, c1, c2, c3);
	Verifier("4c. sans fx/fy, le degrade radial est CONCENTRIQUE : trois points a egale distance du centre ont la "
			 "meme valeur",
			 concentrique, det);

	// (d) href : un degrade herite la GEOMETRIE de celui qu'il reference, pas
	//     seulement ses arrets. Ici la base est VERTICALE ; le fils ne dit rien.
	PageDegrade(svg, sizeof(svg),
				"<linearGradient id=\"base\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\">"
				"<stop offset=\"0\" stop-color=\"#000000\"/><stop offset=\"1\" stop-color=\"#ffffff\"/>"
				"</linearGradient>"
				"<linearGradient id=\"fils\" href=\"#base\"/>",
				"url(#fils)");
	NkImage d = Decoder(svg);
	const int32 dh = Gris(d, 50, 10), db = Gris(d, 50, 90), dg = Gris(d, 10, 50), dd = Gris(d, 90, 50);
	const bool herite = d.IsValid() && dh < db - 100 && std::abs(dg - dd) < 12;
	std::snprintf(det, sizeof(det), "haut=%d bas=%d (la base est verticale) ; gauche=%d droite=%d (egales)", dh, db,
				  dg, dd);
	Verifier("4d. href herite la GEOMETRIE, pas seulement les arrets : un fils muet reprend le x1/y1/x2/y2 de sa "
			 "base (verticale) au lieu de retomber sur l'horizontale par defaut",
			 herite, det);

	// (e) spreadMethod=\"repeat\" en userSpaceOnUse : le motif se REPETE.
	PageDegrade(svg, sizeof(svg),
				"<linearGradient id=\"rep\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" y1=\"0\" x2=\"25\" y2=\"0\" "
				"spreadMethod=\"repeat\">"
				"<stop offset=\"0\" stop-color=\"#000000\"/><stop offset=\"1\" stop-color=\"#ffffff\"/>"
				"</linearGradient>",
				"url(#rep)");
	NkImage e = Decoder(svg);
	const int32 p0 = Gris(e, 5, 50), p1 = Gris(e, 30, 50), p2 = Gris(e, 55, 50), p3 = Gris(e, 80, 50);
	const bool repete = e.IsValid() && std::abs(p0 - p1) < 14 && std::abs(p1 - p2) < 14 && std::abs(p2 - p3) < 14 &&
						Gris(e, 20, 50) > p0 + 100;
	std::snprintf(det, sizeof(det), "meme phase tous les 25 px : %d / %d / %d / %d ; fin de periode (20,50)=%d", p0,
				  p1, p2, p3, Gris(e, 20, 50));
	Verifier("4e. spreadMethod=\"repeat\" (userSpaceOnUse) : le motif se repete tous les 25 px, la meme phase rend "
			 "la meme valeur",
			 repete, det);

	// (f) stop-opacity : un arret transparent laisse voir CE QU'IL Y A DESSOUS.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
				  "<defs><linearGradient id=\"o\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">"
				  "<stop offset=\"0\" stop-color=\"#ff0000\" stop-opacity=\"1\"/>"
				  "<stop offset=\"1\" stop-color=\"#ff0000\" stop-opacity=\"0\"/></linearGradient></defs>"
				  "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#o)\"/></svg>");
	NkImage f = Decoder(svg);
	const int32 aG = AlphaDe(f, 5, 50), aD = AlphaDe(f, 95, 50);
	const bool opacite = f.IsValid() && aG > 230 && aD < 25;
	std::snprintf(det, sizeof(det), "alpha a gauche (stop-opacity 1) = %d ; a droite (stop-opacity 0) = %d", aG, aD);
	Verifier("4f. stop-opacity : l'arret transparent rend le degrade transparent de son cote (l'alpha varie, pas "
			 "seulement la couleur)",
			 opacite, det);
}

// ─────────────────────────────────────────────────────────────────────────────
// PALIER 5 — <filter><feDropShadow> : l'ombre est l'alpha du GROUPE
// ─────────────────────────────────────────────────────────────────────────────
static void TestOmbre() {
	std::printf("\n== PALIER 5 : <feDropShadow> ==\n");
	char det[640];
	static char svg[4096];

	// (a) une ombre NETTE (stdDeviation=0) decalee de +12,+12 : elle est SOUS la
	//     forme, decalee, et de la couleur demandee.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"120\" viewBox=\"0 0 120 120\">"
				  "<defs><filter id=\"o\" x=\"-50%%\" y=\"-50%%\" width=\"200%%\" height=\"200%%\">"
				  "<feDropShadow dx=\"12\" dy=\"12\" stdDeviation=\"0\" flood-color=\"#ff0000\" "
				  "flood-opacity=\"1\"/></filter></defs>"
				  "<g filter=\"url(#o)\"><rect x=\"20\" y=\"20\" width=\"50\" height=\"50\" fill=\"#000000\"/></g>"
				  "</svg>");
	NkImage a = Decoder(svg);
	// la forme : (20..70) ; l'ombre : (32..82). En (75,75) il n'y a QUE l'ombre.
	const bool forme = Proche(a, 45, 45, 0, 0, 0, 4);
	const bool ombreLa = Proche(a, 75, 75, 255, 0, 0, 6);
	const bool ombreDessous = Proche(a, 45, 45, 0, 0, 0, 4); // au centre : la forme, pas l'ombre
	const bool rienAvant = AlphaDe(a, 10, 10) < 20;			 // avant la forme : rien
	const bool net = a.IsValid() && forme && ombreLa && ombreDessous && rienAvant;
	std::snprintf(det, sizeof(det), "forme noire au centre=%d ; ombre rouge en (75,75)=%d ; alpha en (10,10)=%d",
				  forme ? 1 : 0, ombreLa ? 1 : 0, AlphaDe(a, 10, 10));
	Verifier("5a. <feDropShadow dx dy> : une ombre de la couleur demandee, DECALEE, et posee SOUS la forme (le "
			 "centre reste noir, pas rouge)",
			 net, det);

	// (b) le FLOU : avec stdDeviation, le bord de l'ombre n'est plus franc --
	//     il existe des valeurs INTERMEDIAIRES la ou l'ombre nette n'en a pas.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"120\" viewBox=\"0 0 120 120\">"
				  "<defs><filter id=\"o\"><feDropShadow dx=\"12\" dy=\"12\" stdDeviation=\"4\" "
				  "flood-color=\"#ff0000\"/></filter></defs>"
				  "<g filter=\"url(#o)\"><rect x=\"20\" y=\"20\" width=\"50\" height=\"50\" fill=\"#000000\"/></g>"
				  "</svg>");
	NkImage b = Decoder(svg);
	// LE BORD DE L'OMBRE EST A x=82 (le rect 20..70, decale de 12). Un flou se
	// mesure DE PART ET D'AUTRE de ce bord, a un demi ecart-type : DEHORS l'alpha
	// apparait, DEDANS il diminue. Une simple dilatation ne ferait que le premier ;
	// exiger les deux, c'est exiger une vraie transition.
	const int32 dehorsNet = AlphaDe(a, 84, 50), dehorsFlou = AlphaDe(b, 84, 50);
	const int32 dedansNet = AlphaDe(a, 80, 50), dedansFlou = AlphaDe(b, 80, 50);
	const int32 coeur = AlphaDe(b, 75, 75);
	const bool floute = b.IsValid() && dehorsNet == 0 && dehorsFlou > 40 && dedansNet > 250 &&
						dedansFlou < dedansNet - 40 && coeur > 150;
	std::snprintf(det, sizeof(det),
				  "a 2 px DEHORS du bord : net=%d flou=%d ; a 2 px DEDANS : net=%d flou=%d ; coeur de l'ombre=%d",
				  dehorsNet, dehorsFlou, dedansNet, dedansFlou, coeur);
	Verifier("5b. stdDeviation FLOUTE vraiment l'ombre : l'alpha apparait dehors ET diminue dedans (une "
			 "transition, pas une dilatation), sans vider le coeur",
			 floute, det);

	// (c) flood-opacity : l'ombre est plus transparente, la forme ne bouge pas.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"120\" viewBox=\"0 0 120 120\">"
				  "<defs><filter id=\"o\"><feDropShadow dx=\"12\" dy=\"12\" stdDeviation=\"0\" "
				  "flood-color=\"#ff0000\" flood-opacity=\"0.25\"/></filter></defs>"
				  "<g filter=\"url(#o)\"><rect x=\"20\" y=\"20\" width=\"50\" height=\"50\" fill=\"#000000\"/></g>"
				  "</svg>");
	NkImage c = Decoder(svg);
	const int32 a25 = AlphaDe(c, 75, 75);
	const bool opac = c.IsValid() && a25 > 45 && a25 < 80 && Proche(c, 45, 45, 0, 0, 0, 4);
	std::snprintf(det, sizeof(det), "alpha de l'ombre a flood-opacity=0.25 : %d (attendu ~64) ; la forme est "
								   "intacte=%d",
				  a25, Proche(c, 45, 45, 0, 0, 0, 4) ? 1 : 0);
	Verifier("5c. flood-opacity attenue l'ombre SANS toucher a la forme", opac, det);

	// (d) une primitive de filtre non geree est NOMMEE (elle change l'image :
	//     la sauter en silence ferait croire que le filtre a ete applique).
	static const char *kAutre =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
		"<defs><filter id=\"f\"><feGaussianBlur stdDeviation=\"3\"/><feTurbulence baseFrequency=\"0.1\"/>"
		"</filter></defs><g filter=\"url(#f)\"><rect x=\"5\" y=\"5\" width=\"30\" height=\"30\" fill=\"#000\"/></g>"
		"</svg>";
	NkSVGImage *img = NkSVGImage::LoadFromMemory((const uint8 *)kAutre, std::strlen(kAutre));
	bool blurDit = false, turbDit = false;
	if (img) {
		for (int32 i = 0; i < img->SkippedCount(); ++i) {
			const char *n = img->SkippedAt(i);
			if (n && std::strcmp(n, "feGaussianBlur") == 0)
				blurDit = true;
			if (n && std::strcmp(n, "feTurbulence") == 0)
				turbDit = true;
		}
		// et le groupe est peint QUAND MEME : perdre l'effet vaut mieux que perdre
		// la forme
		NkImage r = img->Rasterize(0, 0);
		blurDit = blurDit && Proche(r, 20, 20, 0, 0, 0, 4);
		img->Free();
	}
	std::snprintf(det, sizeof(det), "feGaussianBlur nomme (et la forme peinte quand meme)=%d ; feTurbulence "
								   "nomme=%d",
				  blurDit ? 1 : 0, turbDit ? 1 : 0);
	Verifier("5d. les primitives de filtre AUTRES que feDropShadow sont NOMMEES, et le groupe est peint quand "
			 "meme (perdre l'effet vaut mieux que perdre la forme)",
			 blurDit && turbDit, det);
}

// ─────────────────────────────────────────────────────────────────────────────
// PALIER 6 — opacites composees et mix-blend-mode
// ─────────────────────────────────────────────────────────────────────────────
static void TestOpacites() {
	std::printf("\n== PALIER 6 : opacites et fusion ==\n");
	char det[640];
	static char svg[4096];

	// (a) opacity ET fill-opacity se COMPOSENT : 0.5 x 0.5 = 0.25.
	//     Un noir a 25 % sur du blanc donne 191.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
				  "<rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" fill=\"#ffffff\"/>"
				  "<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" fill=\"#000000\" opacity=\"0.5\" "
				  "fill-opacity=\"0.5\"/></svg>");
	NkImage a = Decoder(svg);
	const int32 g = Gris(a, 30, 30);
	const bool compose = a.IsValid() && g > 180 && g < 202;
	std::snprintf(det, sizeof(det), "gris obtenu %d (0.5 x 0.5 = 0.25 de noir sur blanc -> ~191 ; une seule des "
								   "deux opacites donnerait ~128)",
				  g);
	Verifier("6a. opacity ET fill-opacity se COMPOSENT (0,25 au total, pas 0,5)", compose, det);

	// (b) mix-blend-mode:multiply -- un gris a 50 % sur du rouge donne du rouge
	//     sombre : le vert et le bleu restent a zero, le rouge est divise par deux.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
				  "<rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" fill=\"#ff0000\"/>"
				  "<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" fill=\"#808080\" "
				  "style=\"mix-blend-mode:multiply\"/></svg>");
	NkImage b = Decoder(svg);
	uint8 pb[4];
	{
		uint8 tmp[4];
		Pixel(b, 30, 30, tmp);
		pb[0] = tmp[0];
		pb[1] = tmp[1];
		pb[2] = tmp[2];
		pb[3] = tmp[3];
	}
	const bool multiplie = b.IsValid() && pb[0] > 118 && pb[0] < 138 && pb[1] < 8 && pb[2] < 8;
	std::snprintf(det, sizeof(det), "(%u,%u,%u) -- multiply de #808080 sur #ff0000 attend ~(128,0,0), le mode "
								   "ignore aurait laisse (128,128,128)",
				  pb[0], pb[1], pb[2]);
	Verifier("6b. mix-blend-mode:multiply est PEINT (le fond rouge assombrit la source grise)", multiplie, det);

	// (c) screen : la meme source ECLAIRCIT au lieu d'assombrir.
	std::snprintf(svg, sizeof(svg),
				  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
				  "<rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" fill=\"#ff0000\"/>"
				  "<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" fill=\"#808080\" "
				  "style=\"mix-blend-mode:screen\"/></svg>");
	NkImage c = Decoder(svg);
	uint8 pc[4];
	Pixel(c, 30, 30, pc);
	const bool ecran = c.IsValid() && pc[0] > 245 && pc[1] > 118 && pc[1] < 138 && pc[2] > 118 && pc[2] < 138;
	std::snprintf(det, sizeof(det), "(%u,%u,%u) -- screen attend ~(255,128,128)", pc[0], pc[1], pc[2]);
	Verifier("6c. mix-blend-mode:screen est PEINT (la source eclaircit le fond)", ecran, det);

	// (d) un mode NON gere est nomme et rendu en « normal » -- pas devine.
	static const char *kAutre =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
		"<rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" fill=\"#ff0000\"/>"
		"<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" fill=\"#808080\" "
		"style=\"mix-blend-mode:overlay\"/></svg>";
	NkImage d = Decoder(kAutre);
	uint8 pd[4];
	Pixel(d, 30, 30, pd);
	const bool normal = d.IsValid() && pd[0] > 118 && pd[0] < 138 && pd[1] > 118 && pd[1] < 138;
	std::snprintf(det, sizeof(det), "(%u,%u,%u) -- « overlay » n'est pas peint : la source est posee telle quelle",
				  pd[0], pd[1], pd[2]);
	Verifier("6d. un mix-blend-mode non gere (overlay) est rendu en NORMAL, pas approche par un autre mode", normal,
			 det);
}

// ─────────────────────────────────────────────────────────────────────────────
// LE TEMOIN CROISE — deux lecteurs du MEME document, compares en pixels
// -----------------------------------------------------------------------------
//  `tests/temoins/` porte trois fichiers ECRITS PAR NkUIDesign (sa sonde 82, le
//  2026-09-05) : la page exportee en PNG par son peintre, la MEME page exportee
//  en SVG par son ecrivain, et l'image 2x2 que le document reference.
//
//  Ces trois fichiers sont le seul endroit du banc ou l'entree ne vient pas du
//  test -- et c'est precisement le point : ils viennent d'un AUTRE programme.
//  Le PNG est la verite du peintre ; le SVG re-rasterise par ce codec doit
//  tomber au meme endroit. Quand les deux different, l'un des deux a tort, et le
//  banc dit lequel au lieu de moyenner.
//
//  AVANT CE LOT, la sonde 82 de NkUIDesign ne pouvait comparer que les formes et
//  les degrades : « texte et image : STRUCTURE seulement, le parseur maison ne
//  sait ni <text> ni <image> ». C'est ce qui change ici.
// ─────────────────────────────────────────────────────────────────────────────
static void TestTemoinCroise() {
	std::printf("\n== TEMOIN CROISE : l'export de NkUIDesign ==\n");
	char det[900];
	const char *dossier = "Kernel/Runtime/NKImage/tests/temoins";
	const char *cheminSvg = "Kernel/Runtime/NKImage/tests/temoins/sonde_export_page.svg";
	const char *cheminPng = "Kernel/Runtime/NKImage/tests/temoins/sonde_export_page.png";

	NkImage png;
	const bool pngLu = png.Load(cheminPng, 4) && png.Width() == 200 && png.Height() == 120;
	NkSVGImage *doc = NkSVGImage::LoadFromFile(cheminSvg);
	NkImage svg = doc ? doc->Rasterize(0, 0) : NkImage();
	const bool svgLu = svg.IsValid() && svg.Width() == 200 && svg.Height() == 120;
	if (!pngLu || !svgLu) {
		std::snprintf(det, sizeof(det), "PNG lu=%d ; SVG lu=%d -- les temoins sont dans %s", pngLu ? 1 : 0,
					  svgLu ? 1 : 0, dossier);
		Verifier("T. les deux temoins de NkUIDesign sont lisibles", false, det);
		if (doc)
			doc->Free();
		return;
	}
	(void)dossier;

	// ── (1) LES FORMES : ce que la sonde 82 comparait deja ────────────────
	const bool rouge = Proche(svg, 15, 15, 255, 0, 0, 3) && Proche(svg, 64, 15, 255, 0, 0, 3) &&
					   Proche(svg, 15, 44, 255, 0, 0, 3) && Proche(svg, 64, 44, 255, 0, 0, 3);
	const int32 q15 = Gris(svg, 110, 15), q25 = Gris(svg, 110, 25), q35 = Gris(svg, 110, 35),
				q45 = Gris(svg, 110, 45);
	const int32 g15 = Gris(png, 110, 15), g25 = Gris(png, 110, 25), g35 = Gris(png, 110, 35),
				g45 = Gris(png, 110, 45);
	auto ecart24 = [](int32 a, int32 b) { return std::abs(a - b) <= 24; };
	const bool degrade = ecart24(q15, g15) && ecart24(q25, g25) && ecart24(q35, g35) && ecart24(q45, g45);
	const bool tourne = Proche(svg, 155, 80, 0, 0, 255, 3) && Proche(svg, 155, 63, 0, 0, 255, 3);
	std::snprintf(det, sizeof(det),
				  "rect rouge aux quatre points=%d ; degrade SVG %d/%d/%d/%d contre PNG %d/%d/%d/%d (le peintre "
				  "peint 24 bandes, le codec interpole : ecart <= 24)=%d ; carre tourne=%d",
				  rouge ? 1 : 0, q15, q25, q35, q45, g15, g25, g35, g45, degrade ? 1 : 0, tourne ? 1 : 0);
	Verifier("T1. FORMES ET DEGRADES : le SVG re-rasterise tombe sur les memes pixels que le PNG du peintre",
			 rouge && degrade && tourne, det);

	// ── (2) L'IMAGE : c'est ICI que « structure » devient « pixels » ──────
	//     Le document pose une image 2x2 (magenta / vert) etiree dans 40x40 en
	//     preserveAspectRatio="none". Les quatre texels doivent tomber aux memes
	//     quatre points que dans le PNG -- et etre les MEMES couleurs.
	uint8 pp[4][4], ps[4][4];
	const int32 pts[4][2] = {{82, 62}, {117, 62}, {82, 97}, {117, 97}};
	bool memeImage = true;
	for (int32 i = 0; i < 4; ++i) {
		Pixel(png, pts[i][0], pts[i][1], pp[i]);
		Pixel(svg, pts[i][0], pts[i][1], ps[i]);
		for (int32 k = 0; k < 3; ++k)
			if (std::abs((int32)pp[i][k] - (int32)ps[i][k]) > 6)
				memeImage = false;
	}
	std::snprintf(det, sizeof(det),
				  "PNG (%u,%u,%u) (%u,%u,%u) (%u,%u,%u) (%u,%u,%u) | SVG (%u,%u,%u) (%u,%u,%u) (%u,%u,%u) "
				  "(%u,%u,%u)",
				  pp[0][0], pp[0][1], pp[0][2], pp[1][0], pp[1][1], pp[1][2], pp[2][0], pp[2][1], pp[2][2], pp[3][0],
				  pp[3][1], pp[3][2], ps[0][0], ps[0][1], ps[0][2], ps[1][0], ps[1][1], ps[1][2], ps[2][0], ps[2][1],
				  ps[2][2], ps[3][0], ps[3][1], ps[3][2]);
	Verifier("T2. L'IMAGE, EN PIXELS (elle n'etait que « structure ») : les quatre texels du damier 2x2 tombent "
			 "aux memes points et aux memes couleurs que dans le PNG du peintre",
			 memeImage, det);

	// ── (3) LE TEXTE : de l'encre au meme endroit ─────────────────────────
	//     Le document pose « Ab » a 14 px sur un fond blanc, boite (10,60)-(70,90).
	Encre ep = EncreDe(png, 8, 58, 72, 92);
	Encre es = EncreDe(svg, 8, 58, 72, 92);
	const bool encrePresente = es.sombres > 20;
	// la ligne de base est la MEME : le bas de l'encre doit coincider a 2 px pres
	const bool memeBase = encrePresente && std::abs(es.ymax - ep.ymax) <= 2;
	// et le texte commence au meme x
	const bool memeDepart = encrePresente && std::abs(es.xmin - ep.xmin) <= 3;
	std::snprintf(det, sizeof(det),
				  "PNG : %d px d'encre, x[%d..%d] y[%d..%d] | SVG : %d px d'encre, x[%d..%d] y[%d..%d] -- meme "
				  "ligne de base (%d) et meme depart (%d)",
				  ep.sombres, ep.xmin, ep.xmax, ep.ymin, ep.ymax, es.sombres, es.xmin, es.xmax, es.ymin, es.ymax,
				  memeBase ? 1 : 0, memeDepart ? 1 : 0);
	Verifier("T3. LE TEXTE, EN PIXELS (il n'etait que « structure ») : de l'encre dans la meme boite, posee sur la "
			 "MEME ligne de base et au meme depart que le peintre",
			 encrePresente && memeBase && memeDepart, det);

	// ── (4) CE QUE LE TEMOIN MESURE ET QUI N'EST PAS EGAL : LA TAILLE ─────
	//     `font-size` vaut le CADRATIN (em) en SVG ; NkFontAtlas, lui, echelonne
	//     par (ascender - descender). Pour Inter les deux different, donc un
	//     `font-size="14"` ecrit par l'export ne rend PAS la meme hauteur que le
	//     peintre a 14 px. On MESURE l'ecart et on le NOMME plutot que de plier le
	//     codec : c'est le SVG qui doit etre juste pour un navigateur.
	const int32 hp = ep.ymax - ep.ymin + 1, hs = es.ymax - es.ymin + 1;
	const float32 rapport = (hp > 0) ? (float32)hs / (float32)hp : 0.f;
	const bool ecartConnu = encrePresente && hp > 0 && rapport > 1.05f && rapport < 1.45f;
	std::snprintf(det, sizeof(det),
				  "hauteur d'encre : peintre %d px, codec %d px -> rapport %.3f. LE CODEC SUIT LA NORME (echelle "
				  "= font-size / unitsPerEm) ; le peintre echelonne par (ascender - descender). L'export ecrit la "
				  "taille DU PEINTRE dans un attribut qui, en SVG, est un cadratin : un navigateur rendra ce "
				  "texte comme nous, plus grand que le PNG.",
				  hp, hs, (double)rapport);
	Verifier("T4. L'ECART DE TAILLE EST MESURE ET NOMME, pas masque : le texte du SVG est plus haut que celui du "
			 "PNG parce que l'export ecrit une taille de peintre dans un attribut qui vaut un cadratin",
			 ecartConnu, det);

	// ── (5) L'OMBRE PORTEE ET LE FOND DE PAGE ────────────────────────────
	uint8 fp[4], fs[4];
	Pixel(png, 5, 115, fp);
	Pixel(svg, 5, 115, fs);
	bool memeFond = true;
	for (int32 k = 0; k < 4; ++k)
		if (std::abs((int32)fp[k] - (int32)fs[k]) > 3)
			memeFond = false;
	// l'ombre du rect rouge (dy=4, flou 4) : juste SOUS lui, plus sombre que la page
	const int32 ombrePng = Gris(png, 40, 53), ombreSvg = Gris(svg, 40, 53);
	const bool ombre = ombreSvg < 250 && std::abs(ombrePng - ombreSvg) <= 40;
	std::snprintf(det, sizeof(det),
				  "fond de page PNG (%u,%u,%u,%u) = SVG (%u,%u,%u,%u), alpha compris -> %d ; sous le rect rouge "
				  "(l'ombre) : PNG %d, SVG %d",
				  fp[0], fp[1], fp[2], fp[3], fs[0], fs[1], fs[2], fs[3], memeFond ? 1 : 0, ombrePng, ombreSvg);
	Verifier("T5. le FOND DE PAGE est identique alpha compris, et l'OMBRE PORTEE assombrit le meme endroit dans "
			 "les deux lecteurs",
			 memeFond && ombre, det);

	// ── (6) et ce que le codec a saute sur un fichier REEL ────────────────
	std::snprintf(det, sizeof(det), "%d chose(s) sautee(s) : ", doc->SkippedCount());
	for (int32 i = 0; i < doc->SkippedCount(); ++i) {
		std::strncat(det, doc->SkippedAt(i) ? doc->SkippedAt(i) : "?", sizeof(det) - std::strlen(det) - 1);
		std::strncat(det, " ", sizeof(det) - std::strlen(det) - 1);
	}
	Verifier("T6. sur un fichier REEL produit par une autre application, le codec ne saute plus rien en silence "
			 "(la liste ci-dessous est vide, ou nommee)",
			 doc->SkippedCount() == 0, det);

	doc->Free();
}

// ─────────────────────────────────────────────────────────────────────────────
// PALIER <use> / <symbol> — instancier par reference
// ─────────────────────────────────────────────────────────────────────────────

/// Combien de pixels different entre deux images (au-dela de @p tol par canal) ?
/// C'est LE critere d'une instanciation : le fichier a <use> doit rendre CE QUE
/// RENDRAIT sa version aplatie -- pas « a peu pres », pas « aux points qu'on a
/// choisi de regarder ». On compare TOUT.
static int32 PixelsDifferents(const NkImage &a, const NkImage &b, int32 tol) {
	if (!a.IsValid() || !b.IsValid() || a.Width() != b.Width() || a.Height() != b.Height())
		return -1;
	int32 n = 0;
	for (int32 y = 0; y < a.Height(); ++y)
		for (int32 x = 0; x < a.Width(); ++x) {
			uint8 pa[4], pb[4];
			Pixel(a, x, y, pa);
			Pixel(b, x, y, pb);
			for (int32 k = 0; k < 4; ++k)
				if (std::abs((int32)pa[k] - (int32)pb[k]) > tol) {
					++n;
					break;
				}
		}
	return n;
}

static void TestUse() {
	std::printf("\n== PALIER <use> / <symbol> ==\n");
	char det[640];

	// (a) LE TEMOIN DU PALIER : un fichier a <use> IMBRIQUES contre sa version
	//     APLATIE, ecrite a la main juste en dessous. Les deux doivent rendre les
	//     memes pixels -- tous.
	//     « brique » = un carre rouge et un carre bleu ; « paire » = deux briques
	//     dont une decalee ; la page instancie « paire » deux fois, a deux places.
	static const char *kUse =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"120\" viewBox=\"0 0 120 120\">"
		"<defs>"
		"<g id=\"brique\">"
		"<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"#ff0000\"/>"
		"<rect x=\"10\" y=\"0\" width=\"10\" height=\"10\" fill=\"#0000ff\"/>"
		"</g>"
		"<g id=\"paire\">"
		"<use href=\"#brique\"/>"
		"<use href=\"#brique\" x=\"0\" y=\"20\"/>"
		"</g>"
		"</defs>"
		"<use href=\"#paire\" x=\"10\" y=\"10\"/>"
		"<use href=\"#paire\" x=\"60\" y=\"60\"/>"
		"</svg>";
	static const char *kPlat =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"120\" viewBox=\"0 0 120 120\">"
		"<g><g><rect x=\"10\" y=\"10\" width=\"10\" height=\"10\" fill=\"#ff0000\"/>"
		"<rect x=\"20\" y=\"10\" width=\"10\" height=\"10\" fill=\"#0000ff\"/></g>"
		"<g><rect x=\"10\" y=\"30\" width=\"10\" height=\"10\" fill=\"#ff0000\"/>"
		"<rect x=\"20\" y=\"30\" width=\"10\" height=\"10\" fill=\"#0000ff\"/></g></g>"
		"<g><g><rect x=\"60\" y=\"60\" width=\"10\" height=\"10\" fill=\"#ff0000\"/>"
		"<rect x=\"70\" y=\"60\" width=\"10\" height=\"10\" fill=\"#0000ff\"/></g>"
		"<g><rect x=\"60\" y=\"80\" width=\"10\" height=\"10\" fill=\"#ff0000\"/>"
		"<rect x=\"70\" y=\"80\" width=\"10\" height=\"10\" fill=\"#0000ff\"/></g></g>"
		"</svg>";
	NkImage avecUse = Decoder(kUse);
	NkImage aplati = Decoder(kPlat);
	const int32 diff = PixelsDifferents(avecUse, aplati, 2);
	// et le rendu n'est pas vide : les quatre briques sont bien la
	const bool contenu = Proche(avecUse, 15, 15, 255, 0, 0, 3) && Proche(avecUse, 25, 15, 0, 0, 255, 3) &&
						 Proche(avecUse, 15, 35, 255, 0, 0, 3) && Proche(avecUse, 65, 85, 255, 0, 0, 3) &&
						 Proche(avecUse, 75, 65, 0, 0, 255, 3);
	std::snprintf(det, sizeof(det), "%d pixel(s) different(s) sur %d ; les quatre briques instanciees sont la=%d",
				  diff, avecUse.IsValid() ? avecUse.Width() * avecUse.Height() : 0, contenu ? 1 : 0);
	Verifier("U1. <use> IMBRIQUES (un <use> dans un <g> lui-meme instancie par <use>) rend EXACTEMENT la meme "
			 "image que la version aplatie ecrite a la main -- tous les pixels compares",
			 diff == 0 && contenu, det);

	// (b) un <defs> ne se rend PAS la ou il est defini : sans les <use>, la page
	//     est vide. C'est ce qui distingue « instancier » de « dessiner deux fois ».
	static const char *kDefsSeul =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
		"<defs><g id=\"b\"><rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" fill=\"#ff0000\"/></g></defs>"
		"<symbol id=\"s\"><rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" fill=\"#00ff00\"/></symbol>"
		"</svg>";
	NkImage vide = Decoder(kDefsSeul);
	const bool rienRendu = vide.IsValid() && AlphaDe(vide, 30, 30) < 20 && AlphaDe(vide, 5, 5) < 20;
	std::snprintf(det, sizeof(det), "alpha au centre=%d -- ni le <defs> ni le <symbol> ne se peignent d'eux-memes",
				  AlphaDe(vide, 30, 30));
	Verifier("U2. <defs> ET <symbol> ne se rendent PAS la ou ils sont definis (seul <use> les instancie)", rienRendu,
			 det);

	// (c) <symbol viewBox> instancie avec width/height : il est MIS A L'ECHELLE.
	static const char *kSymbole =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
		"<symbol id=\"carre\" viewBox=\"0 0 10 10\">"
		"<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"#ff0000\"/></symbol>"
		"<use href=\"#carre\" x=\"20\" y=\"20\" width=\"60\" height=\"60\"/></svg>";
	NkImage sym = Decoder(kSymbole);
	// le carre de 10 devient 60x60 a partir de (20,20) : plein a (50,50) et (25,25),
	// vide juste a cote (85,85) et avant (15,15)
	const bool echelle = sym.IsValid() && Proche(sym, 50, 50, 255, 0, 0, 3) && Proche(sym, 25, 25, 255, 0, 0, 3) &&
						 Proche(sym, 75, 75, 255, 0, 0, 3) && AlphaDe(sym, 85, 85) < 20 && AlphaDe(sym, 15, 15) < 20;
	std::snprintf(det, sizeof(det),
				  "rouge en (25,25) (50,50) (75,75) ; alpha en (15,15)=%d et (85,85)=%d -- le symbole de 10 unites "
				  "occupe bien 60 px a partir de (20,20)",
				  AlphaDe(sym, 15, 15), AlphaDe(sym, 85, 85));
	Verifier("U3. <symbol viewBox> instancie avec width/height est MIS A L'ECHELLE (sa viewBox devient le repere "
			 "de la boite du <use>)",
			 echelle, det);

	// (d) le <use> herite du STYLE et compose les TRANSFORMATIONS.
	static const char *kStyle =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">"
		"<defs><g id=\"c\"><rect x=\"0\" y=\"0\" width=\"20\" height=\"20\"/></g></defs>"
		"<g transform=\"translate(50 0)\"><use href=\"#c\" x=\"10\" y=\"10\" fill=\"#00ff00\"/></g></svg>";
	NkImage st = Decoder(kStyle);
	// translate(50) du parent + x/y du use : le carre va de (60,10) a (80,30), en VERT
	const bool compose = st.IsValid() && Proche(st, 70, 20, 0, 255, 0, 3) && AlphaDe(st, 20, 20) < 20;
	std::snprintf(det, sizeof(det), "vert en (70,20)=%d (translate du parent + x/y du use) ; rien en (20,20) : "
								   "alpha=%d",
				  Proche(st, 70, 20, 0, 255, 0, 3) ? 1 : 0, AlphaDe(st, 20, 20));
	Verifier("U4. <use> COMPOSE la transformation de son parent, la sienne et ses x/y, et transmet son style "
			 "(fill herite jusqu'au contenu instancie)",
			 compose, det);

	// (e) UNE REFERENCE CIRCULAIRE ne fait pas exploser la pile : elle est bornee
	//     ET NOMMEE. Un decodeur qui plante sur un fichier tordu est un decodeur
	//     qu'on ne peut pas exposer a des fichiers venus d'ailleurs.
	static const char *kBoucle =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
		"<defs><g id=\"boucle\"><rect x=\"0\" y=\"0\" width=\"5\" height=\"5\" fill=\"#000\"/>"
		"<use href=\"#boucle\" x=\"1\" y=\"1\"/></g></defs>"
		"<use href=\"#boucle\"/></svg>";
	NkSVGImage *b = NkSVGImage::LoadFromMemory((const uint8 *)kBoucle, std::strlen(kBoucle));
	bool recursionDite = false;
	int32 formes = 0;
	if (b) {
		formes = b->ShapeCount();
		for (int32 i = 0; i < b->SkippedCount(); ++i) {
			const char *n = b->SkippedAt(i);
			if (n && std::strcmp(n, "use-recursion") == 0)
				recursionDite = true;
		}
		b->Free();
	}
	std::snprintf(det, sizeof(det), "%d forme(s) produites, la garde a arrete l'instanciation et l'a NOMMEE=%d",
				  formes, recursionDite ? 1 : 0);
	Verifier("U5. une REFERENCE CIRCULAIRE est bornee ET NOMMEE (le decodeur ne plante pas, et ne se tait pas)",
			 b != nullptr && recursionDite && formes > 0 && formes <= 16, det);

	// (f) une cible INTROUVABLE : rien n'est instancie, et c'est dit.
	static const char *kAbsent =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\" viewBox=\"0 0 60 60\">"
		"<use href=\"#jamaisDefini\"/></svg>";
	NkSVGImage *a2 = NkSVGImage::LoadFromMemory((const uint8 *)kAbsent, std::strlen(kAbsent));
	bool absentDit = false;
	if (a2) {
		for (int32 i = 0; i < a2->SkippedCount(); ++i) {
			const char *n = a2->SkippedAt(i);
			if (n && std::strcmp(n, "use-cible-absente") == 0)
				absentDit = true;
		}
		a2->Free();
	}
	std::snprintf(det, sizeof(det), "cible absente nommee=%d", absentDit ? 1 : 0);
	Verifier("U6. <use> vers une cible INTROUVABLE : rien n'est instancie, et c'est dit", absentDit, det);
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
	// LE REGISTRE SUIT LES PALIERS : ce qui vient d'etre implemente le QUITTE, ce
	// qui reste a faire y demeure. Verifier les deux sens, c'est empecher deux
	// mensonges opposes -- annoncer comme saute ce qu'on peint, et taire ce qu'on
	// saute vraiment.
	bool clipDit = false, dashDit = false, useEncoreDit = false;
	int32 nbClip = 0, nb = 0;
	if (img) {
		nb = img->SkippedCount();
		for (int32 i = 0; i < nb; ++i) {
			const char *n = img->SkippedAt(i);
			if (!n)
				continue;
			if (std::strcmp(n, "use") == 0)
				useEncoreDit = true; // <use> est GERE depuis son palier : plus ici
			if (std::strcmp(n, "clipPath") == 0) {
				clipDit = true;
				++nbClip;
			}
			if (std::strcmp(n, "stroke-dasharray") == 0)
				dashDit = true;
		}
		std::snprintf(det, sizeof(det), "%d nom(s) saute(s) : ", nb);
		for (int32 i = 0; i < nb; ++i) {
			std::strncat(det, img->SkippedAt(i) ? img->SkippedAt(i) : "?", sizeof(det) - std::strlen(det) - 1);
			std::strncat(det, " ", sizeof(det) - std::strlen(det) - 1);
		}
		std::strncat(det, "| <use> n'y est plus (il est peint) ", sizeof(det) - std::strlen(det) - 1);
		img->Free();
	}
	Verifier("Le REGISTRE DES SAUTS suit les paliers : ce qui reste a faire est nomme (clipPath, "
			 "stroke-dasharray), une seule fois par nom, et <use> N'Y EST PLUS depuis qu'il est peint",
			 img != nullptr && clipDit && dashDit && !useEncoreDit && nbClip == 1, det);
}

// ─────────────────────────────────────────────────────────────────────────────
int TestSVG_Run() {
	gPass = 0;
	gTotal = 0;
	std::printf("\n===== BANC DU CODEC SVG (NkSVGCodec) =====\n");
	// L'INJECTION, telle qu'une application la ferait : une ligne, au demarrage.
	// NKImage ne connait aucune police ; NKFont en fournit une source ; c'est
	// l'appelant qui les met en presence.
	static NkFontGlyphSource glyphes;
	NkSVGCodec::SetDefaultGlyphSource(&glyphes);
	TestRxRy();
	TestImage();
	TestTexte();
	TestDegrades();
	TestOmbre();
	TestOpacites();
	TestUse();
	TestNonGere();
	TestTemoinCroise();
	std::printf("\n===== SVG : %d / %d =====\n", gPass, gTotal);
	return (gPass == gTotal) ? 0 : 1;
}

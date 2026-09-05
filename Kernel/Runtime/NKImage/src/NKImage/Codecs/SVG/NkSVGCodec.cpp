// =============================================================================
// NkSVGCodec.cpp
// -----------------------------------------------------------------------------
// Reecriture complete from scratch 2026-05-19, inspiree de NanoSVG par
// Mikko Mononen (sans inclure la lib). Single-file rasterizer SVG -> NkImage RGBA.
//
// Architecture :
//   1. Parser XML stream-based avec stack pour <g> (push/pop style + transform).
//   2. Chaque shape produit une Shape interne (path polyligne apres flatten,
//      style cumule, CTM cumule deja applique aux points).
//   3. Rasterizer scanline avec supersample 2x AA, fill-rule nonzero/evenodd,
//      alpha blending sur l'image RGBA32 destination.
//
// Features supportees :
//   - <svg> avec viewBox + width/height
//   - <g> avec transform cascade correcte (BUG fixe vs ancienne version)
//   - <path> M L H V C S Q T A Z (absolu/relatif) avec text-to-path 24+ KB OK
//   - <rect> <circle> <ellipse> <line> <polyline> <polygon>
//   - Cubic + quadratic Bezier flatten adaptatif
//   - Arc elliptique conversion W3C SVG 1.1 Appendix F.6
//   - fill, opacity, fill-opacity, fill-rule
//   - STROKE RASTERISE : stroke, stroke-width, stroke-opacity, stroke-linecap
//     (butt/round/square), stroke-linejoin (miter/round/bevel), stroke-miterlimit.
//     Le trait est converti en contour plein (BuildStrokeShape) puis rempli.
//   - Gradients lineaires et radiaux (objectBoundingBox / userSpaceOnUse)
//   - transform: translate, scale, rotate, matrix
//   - Couleurs : #RGB #RRGGBB rgb() rgba() + 148 noms CSS
//   - Attribut style="..." CSS inline
//
// Non implemente :
//   - <text> <tspan> (necessite font system)
//   - <defs><style> avec class CSS
//   - stroke-dasharray (le trait est toujours continu)
//   - patterns, masks, clip-path, filters
//   - <use> <symbol> <image>
//
// ATTENTION : cette liste a menti pendant un temps. Elle annoncait « Stroke
// (uniquement fill rendu pour Phase 1) » et « Gradients » comme absents, alors
// que les deux etaient implementes -- au point de faire dessiner des icones
// entieres en contours evides pour contourner une limite qui n'existait plus
// (13 aout 2026). Une capacite qu'on ajoute se declare ICI dans le meme
// changement, sans quoi personne ne s'en sert.
//
// AUTEUR : TEUGUIA TADJUIDJE Rodolf Séderis — Rihen
// =============================================================================

#include "NKImage/Codecs/SVG/NkSVGCodec.h"
#include "NKImage/Core/NkImage.h"
#include "NKFileSystem/NkFile.h"
#include "NKMemory/NkAllocator.h"
#include "NKContainers/Sequential/NkVector.h"
#include "NKContainers/String/Encoding/NkBase64.h"
#include "NKFont/Core/NkFontParser.h"
#include "NKFont/Embedded/NkFontEmbedded.h"
#include "NKLogger/NkLog.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <utility>
#include <new>

namespace nkentseu {

	using namespace nkentseu::memory;

	namespace { // file-local helpers (anonymous namespace)

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 1 — Helpers parsing texte
		// ─────────────────────────────────────────────────────────────────────────────

		inline bool IsSpace(char c) noexcept {
			return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
		}

		inline bool IsDigit(char c) noexcept {
			return c >= '0' && c <= '9';
		}

		inline const char *SkipWS(const char *p) noexcept {
			while (*p && IsSpace(*p))
				++p;
			return p;
		}

		inline const char *SkipWSComma(const char *p) noexcept {
			while (*p && (IsSpace(*p) || *p == ','))
				++p;
			return p;
		}

		/// Parse un float style strtod (sans locale). Avance @p end si fourni.
		float32 ParseFloat(const char *s, const char **endOut = nullptr) noexcept {
			const char *p = s ? SkipWS(s) : nullptr;
			if (!p) {
				if (endOut)
					*endOut = nullptr;
				return 0.f;
			}
			float32 sign = 1.f;
			if (*p == '+')
				++p;
			else if (*p == '-') {
				sign = -1.f;
				++p;
			}
			float64 v = 0.0;
			while (IsDigit(*p)) {
				v = v * 10.0 + (float64)(*p - '0');
				++p;
			}
			if (*p == '.') {
				++p;
				float64 f = 0.1;
				while (IsDigit(*p)) {
					v += (float64)(*p - '0') * f;
					f *= 0.1;
					++p;
				}
			}
			if (*p == 'e' || *p == 'E') {
				++p;
				int32 esign = 1, e = 0;
				if (*p == '+')
					++p;
				else if (*p == '-') {
					esign = -1;
					++p;
				}
				while (IsDigit(*p)) {
					e = e * 10 + (*p - '0');
					++p;
				}
				v *= std::pow(10.0, (float64)(esign * e));
			}
			if (endOut)
				*endOut = p;
			return (float32)(sign * v);
		}

		/// Strcasecmp portable (les noms CSS sont insensibles a la casse).
		int32 StrCaseCmp(const char *a, const char *b) noexcept {
			while (*a && *b) {
				const char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
				const char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
				if (ca != cb)
					return ca - cb;
				++a;
				++b;
			}
			return *a - *b;
		}

		int32 HexDigit(char c) noexcept {
			if (c >= '0' && c <= '9')
				return c - '0';
			if (c >= 'a' && c <= 'f')
				return c - 'a' + 10;
			if (c >= 'A' && c <= 'F')
				return c - 'A' + 10;
			return -1;
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 2 — Table des noms de couleurs CSS (148 noms standards)
		// ─────────────────────────────────────────────────────────────────────────────

		struct NamedColor {
				const char *name;
				uint8 r, g, b;
		};

		const NamedColor kNamedColors[] = {
			{"aliceblue", 240, 248, 255},
			{"antiquewhite", 250, 235, 215},
			{"aqua", 0, 255, 255},
			{"aquamarine", 127, 255, 212},
			{"azure", 240, 255, 255},
			{"beige", 245, 245, 220},
			{"bisque", 255, 228, 196},
			{"black", 0, 0, 0},
			{"blanchedalmond", 255, 235, 205},
			{"blue", 0, 0, 255},
			{"blueviolet", 138, 43, 226},
			{"brown", 165, 42, 42},
			{"burlywood", 222, 184, 135},
			{"cadetblue", 95, 158, 160},
			{"chartreuse", 127, 255, 0},
			{"chocolate", 210, 105, 30},
			{"coral", 255, 127, 80},
			{"cornflowerblue", 100, 149, 237},
			{"cornsilk", 255, 248, 220},
			{"crimson", 220, 20, 60},
			{"cyan", 0, 255, 255},
			{"darkblue", 0, 0, 139},
			{"darkcyan", 0, 139, 139},
			{"darkgoldenrod", 184, 134, 11},
			{"darkgray", 169, 169, 169},
			{"darkgreen", 0, 100, 0},
			{"darkgrey", 169, 169, 169},
			{"darkkhaki", 189, 183, 107},
			{"darkmagenta", 139, 0, 139},
			{"darkolivegreen", 85, 107, 47},
			{"darkorange", 255, 140, 0},
			{"darkorchid", 153, 50, 204},
			{"darkred", 139, 0, 0},
			{"darksalmon", 233, 150, 122},
			{"darkseagreen", 143, 188, 143},
			{"darkslateblue", 72, 61, 139},
			{"darkslategray", 47, 79, 79},
			{"darkslategrey", 47, 79, 79},
			{"darkturquoise", 0, 206, 209},
			{"darkviolet", 148, 0, 211},
			{"deeppink", 255, 20, 147},
			{"deepskyblue", 0, 191, 255},
			{"dimgray", 105, 105, 105},
			{"dimgrey", 105, 105, 105},
			{"dodgerblue", 30, 144, 255},
			{"firebrick", 178, 34, 34},
			{"floralwhite", 255, 250, 240},
			{"forestgreen", 34, 139, 34},
			{"fuchsia", 255, 0, 255},
			{"gainsboro", 220, 220, 220},
			{"ghostwhite", 248, 248, 255},
			{"gold", 255, 215, 0},
			{"goldenrod", 218, 165, 32},
			{"gray", 128, 128, 128},
			{"green", 0, 128, 0},
			{"greenyellow", 173, 255, 47},
			{"grey", 128, 128, 128},
			{"honeydew", 240, 255, 240},
			{"hotpink", 255, 105, 180},
			{"indianred", 205, 92, 92},
			{"indigo", 75, 0, 130},
			{"ivory", 255, 255, 240},
			{"khaki", 240, 230, 140},
			{"lavender", 230, 230, 250},
			{"lavenderblush", 255, 240, 245},
			{"lawngreen", 124, 252, 0},
			{"lemonchiffon", 255, 250, 205},
			{"lightblue", 173, 216, 230},
			{"lightcoral", 240, 128, 128},
			{"lightcyan", 224, 255, 255},
			{"lightgoldenrodyellow", 250, 250, 210},
			{"lightgray", 211, 211, 211},
			{"lightgreen", 144, 238, 144},
			{"lightgrey", 211, 211, 211},
			{"lightpink", 255, 182, 193},
			{"lightsalmon", 255, 160, 122},
			{"lightseagreen", 32, 178, 170},
			{"lightskyblue", 135, 206, 250},
			{"lightslategray", 119, 136, 153},
			{"lightslategrey", 119, 136, 153},
			{"lightsteelblue", 176, 196, 222},
			{"lightyellow", 255, 255, 224},
			{"lime", 0, 255, 0},
			{"limegreen", 50, 205, 50},
			{"linen", 250, 240, 230},
			{"magenta", 255, 0, 255},
			{"maroon", 128, 0, 0},
			{"mediumaquamarine", 102, 205, 170},
			{"mediumblue", 0, 0, 205},
			{"mediumorchid", 186, 85, 211},
			{"mediumpurple", 147, 112, 219},
			{"mediumseagreen", 60, 179, 113},
			{"mediumslateblue", 123, 104, 238},
			{"mediumspringgreen", 0, 250, 154},
			{"mediumturquoise", 72, 209, 204},
			{"mediumvioletred", 199, 21, 133},
			{"midnightblue", 25, 25, 112},
			{"mintcream", 245, 255, 250},
			{"mistyrose", 255, 228, 225},
			{"moccasin", 255, 228, 181},
			{"navajowhite", 255, 222, 173},
			{"navy", 0, 0, 128},
			{"oldlace", 253, 245, 230},
			{"olive", 128, 128, 0},
			{"olivedrab", 107, 142, 35},
			{"orange", 255, 165, 0},
			{"orangered", 255, 69, 0},
			{"orchid", 218, 112, 214},
			{"palegoldenrod", 238, 232, 170},
			{"palegreen", 152, 251, 152},
			{"paleturquoise", 175, 238, 238},
			{"palevioletred", 219, 112, 147},
			{"papayawhip", 255, 239, 213},
			{"peachpuff", 255, 218, 185},
			{"peru", 205, 133, 63},
			{"pink", 255, 192, 203},
			{"plum", 221, 160, 221},
			{"powderblue", 176, 224, 230},
			{"purple", 128, 0, 128},
			{"red", 255, 0, 0},
			{"rosybrown", 188, 143, 143},
			{"royalblue", 65, 105, 225},
			{"saddlebrown", 139, 69, 19},
			{"salmon", 250, 128, 114},
			{"sandybrown", 244, 164, 96},
			{"seagreen", 46, 139, 87},
			{"seashell", 255, 245, 238},
			{"sienna", 160, 82, 45},
			{"silver", 192, 192, 192},
			{"skyblue", 135, 206, 235},
			{"slateblue", 106, 90, 205},
			{"slategray", 112, 128, 144},
			{"slategrey", 112, 128, 144},
			{"snow", 255, 250, 250},
			{"springgreen", 0, 255, 127},
			{"steelblue", 70, 130, 180},
			{"tan", 210, 180, 140},
			{"teal", 0, 128, 128},
			{"thistle", 216, 191, 216},
			{"tomato", 255, 99, 71},
			{"turquoise", 64, 224, 208},
			{"violet", 238, 130, 238},
			{"wheat", 245, 222, 179},
			{"white", 255, 255, 255},
			{"whitesmoke", 245, 245, 245},
			{"yellow", 255, 255, 0},
			{"yellowgreen", 154, 205, 50},
		};
		constexpr int32 kNamedColorsCount = (int32)(sizeof(kNamedColors) / sizeof(kNamedColors[0]));

	} // anonymous namespace

	// ─────────────────────────────────────────────────────────────────────────────
	// SECTION 3 — NkSVGColor / NkSVGTransform (API publique du header)
	// ─────────────────────────────────────────────────────────────────────────────

	NkSVGColor NkSVGColor::Parse(const char *str) noexcept {
		if (!str)
			return Black();
		while (*str && IsSpace(*str))
			++str;
		if (!*str)
			return Black();
		if (StrCaseCmp(str, "none") == 0)
			return None();
		if (StrCaseCmp(str, "transparent") == 0)
			return Transparent();
		if (StrCaseCmp(str, "currentColor") == 0)
			return Black(); // pas de cascade currentColor

		// #RGB / #RRGGBB / #RRGGBBAA
		if (*str == '#') {
			++str;
			int32 len = 0;
			while (str[len] && HexDigit(str[len]) >= 0)
				++len;
			NkSVGColor c;
			c.a = 255;
			if (len == 3) {
				c.r = (uint8)(HexDigit(str[0]) * 17);
				c.g = (uint8)(HexDigit(str[1]) * 17);
				c.b = (uint8)(HexDigit(str[2]) * 17);
			} else if (len == 6 || len == 8) {
				c.r = (uint8)(HexDigit(str[0]) * 16 + HexDigit(str[1]));
				c.g = (uint8)(HexDigit(str[2]) * 16 + HexDigit(str[3]));
				c.b = (uint8)(HexDigit(str[4]) * 16 + HexDigit(str[5]));
				if (len == 8) {
					c.a = (uint8)(HexDigit(str[6]) * 16 + HexDigit(str[7]));
				}
			}
			return c;
		}

		// rgb(r,g,b) / rgba(r,g,b,a)
		if (str[0] == 'r' && str[1] == 'g' && str[2] == 'b') {
			const char *p = str + 3;
			if (*p == 'a')
				++p;
			while (*p && *p != '(')
				++p;
			if (*p == '(')
				++p;
			const float32 r = ParseFloat(p, &p);
			p = SkipWSComma(p);
			const float32 g = ParseFloat(p, &p);
			p = SkipWSComma(p);
			const float32 b = ParseFloat(p, &p);
			p = SkipWSComma(p);
			float32 a = 1.f;
			if (*p && *p != ')')
				a = ParseFloat(p, &p);
			const auto clamp255 = [](float32 v) -> uint8 {
				if (v < 0.f)
					return 0;
				if (v > 255.f)
					return 255;
				return (uint8)v;
			};
			NkSVGColor c;
			c.r = clamp255(r);
			c.g = clamp255(g);
			c.b = clamp255(b);
			c.a = (a <= 1.f) ? clamp255(a * 255.f) : clamp255(a);
			return c;
		}

		// Nom CSS (recherche lineaire, ~150 entrees).
		for (int32 i = 0; i < kNamedColorsCount; ++i) {
			if (StrCaseCmp(str, kNamedColors[i].name) == 0) {
				return {kNamedColors[i].r, kNamedColors[i].g, kNamedColors[i].b, 255, false};
			}
		}
		return Black();
	}

	NkSVGTransform NkSVGTransform::Rotate(float32 deg) noexcept {
		const float32 r = deg * 0.017453292519943295f;
		const float32 c = std::cos(r), s = std::sin(r);
		return {c, s, -s, c, 0, 0};
	}

	NkSVGTransform NkSVGTransform::operator*(const NkSVGTransform &o) const noexcept {
		// (*this) * o : applique o d'abord, puis (*this).
		// En forme matricielle :
		//   [a c e]   [oa oc oe]
		//   [b d f] x [ob od of]
		//   [0 0 1]   [0  0  1 ]
		NkSVGTransform r;
		r.a = a * o.a + c * o.b;
		r.b = b * o.a + d * o.b;
		r.c = a * o.c + c * o.d;
		r.d = b * o.c + d * o.d;
		r.e = a * o.e + c * o.f + e;
		r.f = b * o.e + d * o.f + f;
		return r;
	}

	NkSVGTransform NkSVGTransform::Parse(const char *str) noexcept {
		NkSVGTransform result = Identity();
		if (!str)
			return result;
		const char *p = str;
		while (*p) {
			p = SkipWS(p);
			if (!*p)
				break;
			// Lit le nom de l'op (matrix/translate/scale/rotate/skewX/skewY).
			char op[16] = {0};
			int32 oi = 0;
			while (*p && *p != '(' && !IsSpace(*p) && oi < 15)
				op[oi++] = *p++;
			op[oi] = 0;
			p = SkipWS(p);
			if (*p != '(') {
				if (*p)
					++p;
				continue;
			}
			++p;
			// Lit jusqu'a 6 floats.
			float32 args[6] = {0};
			int32 nargs = 0;
			while (*p && *p != ')' && nargs < 6) {
				p = SkipWSComma(p);
				if (*p == ')')
					break;
				args[nargs++] = ParseFloat(p, &p);
			}
			if (*p == ')')
				++p;

			NkSVGTransform t = Identity();
			if (std::strcmp(op, "matrix") == 0 && nargs >= 6) {
				t.a = args[0];
				t.b = args[1];
				t.c = args[2];
				t.d = args[3];
				t.e = args[4];
				t.f = args[5];
			} else if (std::strcmp(op, "translate") == 0) {
				t = Translate(args[0], nargs > 1 ? args[1] : 0.f);
			} else if (std::strcmp(op, "scale") == 0) {
				t = Scale(args[0], nargs > 1 ? args[1] : args[0]);
			} else if (std::strcmp(op, "rotate") == 0) {
				if (nargs >= 3) {
					// rotate(angle cx cy) = T(cx,cy) * R(angle) * T(-cx,-cy)
					t = Translate(args[1], args[2]) * Rotate(args[0]) * Translate(-args[1], -args[2]);
				} else {
					t = Rotate(args[0]);
				}
			} else if (std::strcmp(op, "skewX") == 0) {
				const float32 rd = args[0] * 0.017453292519943295f;
				t.a = 1.f;
				t.b = 0.f;
				t.c = std::tan(rd);
				t.d = 1.f;
			} else if (std::strcmp(op, "skewY") == 0) {
				const float32 rd = args[0] * 0.017453292519943295f;
				t.a = 1.f;
				t.b = std::tan(rd);
				t.c = 0.f;
				t.d = 1.f;
			}
			// SVG : "transform=A B C" applique A puis B puis C, lecture
			// gauche-droite. Donc result = result * t.
			result = result * t;
		}
		return result;
	}

	// ═════════════════════════════════════════════════════════════════════════════
	// SECTION 4 — Structures internes : Shape + PathBuilder + ParseState
	// ═════════════════════════════════════════════════════════════════════════════

	namespace {

		// ── Gradients (linear / radial) ───────────────────────────────────────────────
		enum class GradKind : uint8 { Linear = 0, Radial };
		enum class GradSpread : uint8 { Pad = 0, Reflect, Repeat };

		struct GradStop {
				float32 offset = 0.f; // 0..1
				NkSVGColor color;	  // color.a inclut deja stop-opacity
		};

		struct Gradient {
				char id[64] = {0};
				char href[64] = {0}; // gradient reference (stops herites)
				GradKind kind = GradKind::Linear;
				GradSpread spread = GradSpread::Pad;
				bool userSpace = false;							   // false = objectBoundingBox (defaut)
				NkSVGTransform xform = NkSVGTransform::Identity(); // gradientTransform
				float32 x1 = 0.f, y1 = 0.f, x2 = 1.f, y2 = 0.f;	   // linear (defaut bbox 0..1)
				float32 cx = 0.5f, cy = 0.5f, r = 0.5f;			   // radial
				float32 fx = 0.5f, fy = 0.5f;					   // radial : le FOYER (defaut = centre)
				bool hasFocal = false;							   // fx/fy ecrits par l'auteur ?
				// CE QUI A ETE ECRIT, attribut par attribut. Un gradient qui en
				// reference un autre (href) n'herite QUE ce qu'il n'a pas dit lui-meme :
				// sans ces drapeaux on ne peut pas distinguer « absent » de « pose a la
				// valeur par defaut », et l'heritage devient faux dans un cas sur deux.
				bool hasX1 = false, hasY1 = false, hasX2 = false, hasY2 = false;
				bool hasCx = false, hasCy = false, hasR = false;
				bool hasUnits = false, hasSpread = false, hasXform = false;
				NkVector<GradStop> stops;

				Gradient() = default;
				Gradient(Gradient &&) noexcept = default;
				Gradient &operator=(Gradient &&) noexcept = default;
				Gradient(const Gradient &) = delete;
				Gradient &operator=(const Gradient &) = delete;
		};

		// ── L'AJUSTEMENT D'UNE IMAGE DANS SA BOITE (preserveAspectRatio) ──────
		enum class FitKind : uint8 {
			Meet = 0, ///< tient EN ENTIER dans la boite, des bandes vides restent
			Slice,	  ///< COUVRE la boite, ce qui depasse est coupe
			None	  ///< etire aux deux dimensions, le rapport est perdu
		};

		/// Shape interne = un path flatten en polyligne(s), + style cumule + CTM applique.
		/// Les xs/ys sont en COORDONNEES DESTINATION (apres ctm + view->out scaling).
		/// Une shape peut aussi porter une IMAGE (<image>) : ses quatre coins sont
		/// alors le contour, et `img` les pixels a poser dedans. Les deux vivent dans
		/// LE MEME vecteur, dans l'ordre du document -- c'est ce qui garde l'ordre de
		/// peinture entre formes et images (deux listes l'auraient perdu).
		struct Shape {
				NkSVGStyle style;
				NkVector<float32> xs;
				NkVector<float32> ys;
				NkVector<int32> contourStart; // index dans xs/ys
				NkVector<int32> contourLen;
				NkSVGTransform ctm = NkSVGTransform::Identity(); // CTM applique (gradients userSpace)
				char fillRef[64] = {0};							 // id de gradient si fill="url(#id)"
				char strokeRef[64] = {0};						 // id de gradient si stroke="url(#id)"

				// ── <image> ──────────────────────────────────────────────────
				NkImage img;						 ///< pixels decodes ; invalide = ce n'est pas une image
				float32 ix = 0, iy = 0, iw = 0, ih = 0; ///< la boite, en espace UTILISATEUR (avant ctm)
				FitKind fit = FitKind::Meet;

				bool EstImage() const noexcept {
					return img.IsValid();
				}

				Shape() = default;
				// Move uniquement (les NkVector peuvent etre couteux a copier).
				Shape(Shape &&) noexcept = default;
				Shape &operator=(Shape &&) noexcept = default;
				Shape(const Shape &) = delete;
				Shape &operator=(const Shape &) = delete;
		};

		// ─────────────────────────────────────────────────────────────────────────────
		// CE QUE LE CODEC SAUTE SE DIT — UNE FOIS PAR NOM, JAMAIS EN SILENCE.
		// Un decodeur qui ignore sans le dire fait croire que le fichier est rendu.
		// Le nom est garde ici (relisible par NkSVGImage::SkippedAt) ET journalise :
		// le test lit la liste, l'humain lit le journal.
		// ─────────────────────────────────────────────────────────────────────────────
		struct SkipList {
				static constexpr int32 kMax = 32;
				char noms[kMax][40] = {};
				int32 nb = 0;

				bool Connu(const char *nom) const noexcept {
					for (int32 i = 0; i < nb; ++i)
						if (std::strcmp(noms[i], nom) == 0)
							return true;
					return false;
				}

				/// @return true si c'est la PREMIERE fois qu'on voit ce nom.
				bool Noter(const char *nom) noexcept {
					if (!nom || !*nom || Connu(nom))
						return false;
					if (nb >= kMax)
						return false;
					std::strncpy(noms[nb], nom, 39);
					noms[nb][39] = 0;
					++nb;
					return true;
				}
		};

		/// State courant pendant le parsing (push/pop sur <g>).
		struct ParseState {
				NkSVGStyle style = {};
				NkSVGTransform xform = NkSVGTransform::Identity();
				char fillRef[64] = {0}; // herite via cascade <g>
				char strokeRef[64] = {0};
		};

		/// Paire (nom, valeur) d'attribut XML. value pointe dans le pool partage.
		struct AttrPair {
				const char *name;
				const char *value;
		};

		/// Trouve l'attribut par nom dans la liste (case sensitive).
		const char *FindAttr(const AttrPair *attrs, int32 n, const char *name) noexcept {
			for (int32 i = 0; i < n; ++i) {
				if (std::strcmp(attrs[i].name, name) == 0)
					return attrs[i].value;
			}
			return nullptr;
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 5 — Construction des paths : PathBuilder + Bezier flatten + Arc
		// ─────────────────────────────────────────────────────────────────────────────

		/// Helper qui pousse des segments dans une Shape. Gere les contours (M ouvre,
		/// Z ferme) + curseur courant + reflexion ctrl points (S/T).
		struct PathBuilder {
				Shape &sh;
				float32 curX = 0, curY = 0;		// position du curseur
				float32 startX = 0, startY = 0; // debut du contour courant
				bool inContour = false;

				explicit PathBuilder(Shape &s) noexcept : sh(s) {
				}

				void StartContour(float32 x, float32 y) noexcept {
					if (inContour)
						EndContour();
					sh.contourStart.PushBack((int32)sh.xs.Size());
					sh.contourLen.PushBack(1);
					sh.xs.PushBack(x);
					sh.ys.PushBack(y);
					startX = curX = x;
					startY = curY = y;
					inContour = true;
				}

				void LineTo(float32 x, float32 y) noexcept {
					if (!inContour) {
						StartContour(x, y);
						return;
					}
					sh.xs.PushBack(x);
					sh.ys.PushBack(y);
					sh.contourLen[sh.contourLen.Size() - 1]++;
					curX = x;
					curY = y;
				}

				void EndContour() noexcept {
					inContour = false;
				}

				void Close() noexcept {
					if (!inContour)
						return;
					if (std::fabs(curX - startX) > 1e-5f || std::fabs(curY - startY) > 1e-5f) {
						sh.xs.PushBack(startX);
						sh.ys.PushBack(startY);
						sh.contourLen[sh.contourLen.Size() - 1]++;
					}
					curX = startX;
					curY = startY;
					inContour = false;
				}
		};

		/// Flatten cubique Bezier par subdivision adaptative (De Casteljau).
		/// Tolerance flatness = 0.5 px max d'erreur.
		void FlattenCubic(PathBuilder &pb, float32 x0, float32 y0, float32 x1, float32 y1, float32 x2, float32 y2,
						  float32 x3, float32 y3, int32 depth = 0) noexcept {
			constexpr float32 kTol2 = 0.25f; // (0.5 px)^2
			constexpr int32 kMaxDepth = 12;
			if (depth >= kMaxDepth) {
				pb.LineTo(x3, y3);
				return;
			}
			// Distance des points de controle a la ligne (x0,y0)-(x3,y3).
			const float32 dx = x3 - x0, dy = y3 - y0;
			const float32 d1 = std::fabs((x1 - x3) * dy - (y1 - y3) * dx);
			const float32 d2 = std::fabs((x2 - x3) * dy - (y2 - y3) * dx);
			const float32 len2 = dx * dx + dy * dy;
			if ((d1 + d2) * (d1 + d2) <= kTol2 * len2) {
				pb.LineTo(x3, y3);
				return;
			}
			// Subdivise en 2 par De Casteljau.
			const float32 x01 = (x0 + x1) * 0.5f, y01 = (y0 + y1) * 0.5f;
			const float32 x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
			const float32 x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
			const float32 x012 = (x01 + x12) * 0.5f, y012 = (y01 + y12) * 0.5f;
			const float32 x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
			const float32 xC = (x012 + x123) * 0.5f, yC = (y012 + y123) * 0.5f;
			FlattenCubic(pb, x0, y0, x01, y01, x012, y012, xC, yC, depth + 1);
			FlattenCubic(pb, xC, yC, x123, y123, x23, y23, x3, y3, depth + 1);
		}

		/// Flatten quadratique Bezier (eleve en cubique : c1 = p0+2/3(p1-p0), c2 = p2+2/3(p1-p2)).
		void FlattenQuad(PathBuilder &pb, float32 x0, float32 y0, float32 x1, float32 y1, float32 x2,
						 float32 y2) noexcept {
			const float32 cx1 = x0 + (2.f / 3.f) * (x1 - x0);
			const float32 cy1 = y0 + (2.f / 3.f) * (y1 - y0);
			const float32 cx2 = x2 + (2.f / 3.f) * (x1 - x2);
			const float32 cy2 = y2 + (2.f / 3.f) * (y1 - y2);
			FlattenCubic(pb, x0, y0, cx1, cy1, cx2, cy2, x2, y2);
		}

		/// Convertit un arc elliptique en sequence de Beziers cubiques (W3C SVG 1.1).
		/// @p x1,y1 = position depart, @p x2,y2 = position arrivee.
		void FlattenArc(PathBuilder &pb, float32 x1, float32 y1, float32 rx, float32 ry, float32 angleDeg,
						bool largeArc, bool sweep, float32 x2, float32 y2) noexcept {
			if (rx == 0.f || ry == 0.f) {
				pb.LineTo(x2, y2);
				return;
			}
			rx = std::fabs(rx);
			ry = std::fabs(ry);
			const float32 rad = angleDeg * 0.017453292519943295f;
			const float32 cosA = std::cos(rad), sinA = std::sin(rad);
			// Step 1 : (x1', y1')
			const float32 dx2 = (x1 - x2) * 0.5f, dy2 = (y1 - y2) * 0.5f;
			const float32 x1p = cosA * dx2 + sinA * dy2;
			const float32 y1p = -sinA * dx2 + cosA * dy2;
			// Step 2 : (cx', cy')
			float32 rxs = rx * rx, rys = ry * ry;
			const float32 x1ps = x1p * x1p, y1ps = y1p * y1p;
			const float32 cr = x1ps / rxs + y1ps / rys;
			if (cr > 1.f) {
				const float32 s = std::sqrt(cr);
				rx *= s;
				ry *= s;
				rxs = rx * rx;
				rys = ry * ry;
			}
			float32 sq = (rxs * rys - rxs * y1ps - rys * x1ps) / (rxs * y1ps + rys * x1ps);
			sq = (sq < 0.f) ? 0.f : sq;
			const float32 coef = (largeArc == sweep ? -1.f : 1.f) * std::sqrt(sq);
			const float32 cxp = coef * (rx * y1p) / ry;
			const float32 cyp = -coef * (ry * x1p) / rx;
			// Step 3 : (cx, cy)
			const float32 cx = cosA * cxp - sinA * cyp + (x1 + x2) * 0.5f;
			const float32 cy = sinA * cxp + cosA * cyp + (y1 + y2) * 0.5f;
			// Step 4 : theta1 + dtheta
			auto angleFn = [](float32 ux, float32 uy, float32 vx, float32 vy) {
				const float32 dot = ux * vx + uy * vy;
				const float32 lenSq = (ux * ux + uy * uy) * (vx * vx + vy * vy);
				if (lenSq <= 0.f)
					return 0.f;
				float32 a = std::acos(std::fmax(-1.f, std::fmin(1.f, dot / std::sqrt(lenSq))));
				if (ux * vy - uy * vx < 0.f)
					a = -a;
				return a;
			};
			const float32 ux1 = (x1p - cxp) / rx, uy1 = (y1p - cyp) / ry;
			const float32 ux2 = (-x1p - cxp) / rx, uy2 = (-y1p - cyp) / ry;
			const float32 theta1 = angleFn(1, 0, ux1, uy1);
			float32 dtheta = angleFn(ux1, uy1, ux2, uy2);
			if (!sweep && dtheta > 0)
				dtheta -= 6.283185307179586f;
			else if (sweep && dtheta < 0)
				dtheta += 6.283185307179586f;
			// Step 5 : approxime par 1 Bezier par quart de tour max.
			const int32 nSegs = (int32)std::ceil(std::fabs(dtheta) / 1.5707963267948966f);
			if (nSegs <= 0)
				return;
			const float32 dt = dtheta / (float32)nSegs;
			const float32 t = (4.f / 3.f) * std::tan(dt * 0.25f);
			float32 cxLast = x1, cyLast = y1;
			for (int32 i = 0; i < nSegs; ++i) {
				const float32 a0 = theta1 + dt * (float32)i;
				const float32 a1 = theta1 + dt * (float32)(i + 1);
				const float32 ca0 = std::cos(a0), sa0 = std::sin(a0);
				const float32 ca1 = std::cos(a1), sa1 = std::sin(a1);
				const float32 ex = cosA * (rx * ca1) - sinA * (ry * sa1) + cx;
				const float32 ey = sinA * (rx * ca1) + cosA * (ry * sa1) + cy;
				const float32 c1x = cosA * (rx * (ca0 - t * sa0)) - sinA * (ry * (sa0 + t * ca0)) + cx;
				const float32 c1y = sinA * (rx * (ca0 - t * sa0)) + cosA * (ry * (sa0 + t * ca0)) + cy;
				const float32 c2x = cosA * (rx * (ca1 + t * sa1)) - sinA * (ry * (sa1 - t * ca1)) + cx;
				const float32 c2y = sinA * (rx * (ca1 + t * sa1)) + cosA * (ry * (sa1 - t * ca1)) + cy;
				FlattenCubic(pb, cxLast, cyLast, c1x, c1y, c2x, c2y, ex, ey);
				cxLast = ex;
				cyLast = ey;
			}
		}

		/// Parser de l'attribut d="..." pour <path>.
		/// Decode toutes les commandes SVG, applique relative/absolue, gere les
		/// repetitions implicites (M suivi de coords = ML, M etc.).
		void ParsePathD(const char *d, PathBuilder &pb) noexcept {
			if (!d)
				return;
			const char *p = d;
			float32 prevCtrlX = 0, prevCtrlY = 0;
			char lastCmd = 0;
			while (*p) {
				p = SkipWSComma(p);
				if (!*p)
					break;
				char cmd = *p;
				const bool isCmd = (cmd >= 'A' && cmd <= 'Z') || (cmd >= 'a' && cmd <= 'z');
				if (isCmd) {
					++p;
				} else {
					// Repetition implicite : M -> L (M devient L pour les sets suivants),
					// m -> l. Les autres conservent leur commande.
					cmd = lastCmd;
					if (cmd == 'M')
						cmd = 'L';
					else if (cmd == 'm')
						cmd = 'l';
				}
				p = SkipWSComma(p);
				const bool rel = (cmd >= 'a' && cmd <= 'z');
				auto X = [&](float32 x) { return rel ? pb.curX + x : x; };
				auto Y = [&](float32 y) { return rel ? pb.curY + y : y; };
				switch (cmd) {
					case 'M':
					case 'm': {
						const float32 x = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y = ParseFloat(p, &p);
						pb.StartContour(X(x), Y(y));
						prevCtrlX = pb.curX;
						prevCtrlY = pb.curY;
						break;
					}
					case 'L':
					case 'l': {
						const float32 x = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y = ParseFloat(p, &p);
						pb.LineTo(X(x), Y(y));
						prevCtrlX = pb.curX;
						prevCtrlY = pb.curY;
						break;
					}
					case 'H':
					case 'h': {
						const float32 x = ParseFloat(p, &p);
						pb.LineTo(X(x), pb.curY);
						prevCtrlX = pb.curX;
						prevCtrlY = pb.curY;
						break;
					}
					case 'V':
					case 'v': {
						const float32 y = ParseFloat(p, &p);
						pb.LineTo(pb.curX, Y(y));
						prevCtrlX = pb.curX;
						prevCtrlY = pb.curY;
						break;
					}
					case 'C':
					case 'c': {
						const float32 x1 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y1 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 x2 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y2 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 x3 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y3 = ParseFloat(p, &p);
						const float32 sx = pb.curX, sy = pb.curY;
						FlattenCubic(pb, sx, sy, X(x1), Y(y1), X(x2), Y(y2), X(x3), Y(y3));
						prevCtrlX = X(x2);
						prevCtrlY = Y(y2);
						break;
					}
					case 'S':
					case 's': {
						const float32 x2 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y2 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 x3 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y3 = ParseFloat(p, &p);
						const float32 sx = pb.curX, sy = pb.curY;
						const bool reflect = (lastCmd == 'C' || lastCmd == 'c' || lastCmd == 'S' || lastCmd == 's');
						const float32 c1x = reflect ? (2.f * sx - prevCtrlX) : sx;
						const float32 c1y = reflect ? (2.f * sy - prevCtrlY) : sy;
						FlattenCubic(pb, sx, sy, c1x, c1y, X(x2), Y(y2), X(x3), Y(y3));
						prevCtrlX = X(x2);
						prevCtrlY = Y(y2);
						break;
					}
					case 'Q':
					case 'q': {
						const float32 x1 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y1 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 x2 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y2 = ParseFloat(p, &p);
						const float32 sx = pb.curX, sy = pb.curY;
						FlattenQuad(pb, sx, sy, X(x1), Y(y1), X(x2), Y(y2));
						prevCtrlX = X(x1);
						prevCtrlY = Y(y1);
						break;
					}
					case 'T':
					case 't': {
						const float32 x2 = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y2 = ParseFloat(p, &p);
						const float32 sx = pb.curX, sy = pb.curY;
						const bool reflect = (lastCmd == 'Q' || lastCmd == 'q' || lastCmd == 'T' || lastCmd == 't');
						const float32 c1x = reflect ? (2.f * sx - prevCtrlX) : sx;
						const float32 c1y = reflect ? (2.f * sy - prevCtrlY) : sy;
						FlattenQuad(pb, sx, sy, c1x, c1y, X(x2), Y(y2));
						prevCtrlX = c1x;
						prevCtrlY = c1y;
						break;
					}
					case 'A':
					case 'a': {
						const float32 rx = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 ry = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 ang = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const int32 large = (int32)ParseFloat(p, &p);
						p = SkipWSComma(p);
						const int32 sweep = (int32)ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 x = ParseFloat(p, &p);
						p = SkipWSComma(p);
						const float32 y = ParseFloat(p, &p);
						const float32 sx = pb.curX, sy = pb.curY;
						const float32 ex = X(x), ey = Y(y);
						FlattenArc(pb, sx, sy, rx, ry, ang, large != 0, sweep != 0, ex, ey);
						pb.curX = ex;
						pb.curY = ey;
						if (pb.sh.contourLen.Size() > 0) {
							// Update du contourLen pour le dernier point d'arc.
							// FlattenArc -> FlattenCubic -> LineTo qui incremente contourLen,
							// donc rien a faire ici. Mais on met a jour curX/curY au-dessus
							// pour les commandes relatives suivantes.
						}
						prevCtrlX = pb.curX;
						prevCtrlY = pb.curY;
						break;
					}
					case 'Z':
					case 'z': {
						pb.Close();
						break;
					}
					default:
						// commande inconnue : on saute un caractere et continue
						++p;
						break;
				}
				lastCmd = cmd;
			}
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 6 — Style : application des attributs CSS / individuels
		// ─────────────────────────────────────────────────────────────────────────────

		void ApplyAttrToStyle(NkSVGStyle &s, const char *name, const char *value) noexcept {
			if (!name || !value)
				return;
			if (std::strcmp(name, "fill") == 0)
				s.fill = NkSVGColor::Parse(value);
			else if (std::strcmp(name, "stroke") == 0)
				s.stroke = NkSVGColor::Parse(value);
			else if (std::strcmp(name, "stroke-width") == 0)
				s.strokeWidth = ParseFloat(value);
			else if (std::strcmp(name, "opacity") == 0)
				s.opacity = ParseFloat(value);
			else if (std::strcmp(name, "fill-opacity") == 0)
				s.fillOpacity = ParseFloat(value);
			else if (std::strcmp(name, "stroke-opacity") == 0)
				s.strokeOpacity = ParseFloat(value);
			else if (std::strcmp(name, "fill-rule") == 0)
				s.fillEvenOdd = (std::strcmp(value, "evenodd") == 0);
			else if (std::strcmp(name, "display") == 0)
				s.visible = std::strcmp(value, "none") != 0;
			else if (std::strcmp(name, "visibility") == 0)
				s.visible = std::strcmp(value, "hidden") != 0;
			else if (std::strcmp(name, "stroke-linecap") == 0) {
				if (std::strcmp(value, "round") == 0)
					s.strokeLineCap = NkSVGLineCap::Round;
				else if (std::strcmp(value, "square") == 0)
					s.strokeLineCap = NkSVGLineCap::Square;
				else
					s.strokeLineCap = NkSVGLineCap::Butt;
			} else if (std::strcmp(name, "stroke-linejoin") == 0) {
				if (std::strcmp(value, "round") == 0)
					s.strokeLineJoin = NkSVGLineJoin::Round;
				else if (std::strcmp(value, "bevel") == 0)
					s.strokeLineJoin = NkSVGLineJoin::Bevel;
				else
					s.strokeLineJoin = NkSVGLineJoin::Miter;
			} else if (std::strcmp(name, "stroke-miterlimit") == 0) {
				const float32 ml = ParseFloat(value);
				s.strokeMiterLimit = (ml >= 1.f) ? ml : 1.f;
			}
		}

		/// Parse l'attribut "style" CSS inline (key:value;key:value).
		void ApplyCSSStyle(NkSVGStyle &s, const char *css) noexcept {
			if (!css)
				return;
			const char *p = css;
			while (*p) {
				// Lit le nom de la propriete.
				char name[64];
				int32 ni = 0;
				while (*p && *p != ':' && *p != ';' && ni + 1 < (int32)sizeof(name)) {
					if (!IsSpace(*p))
						name[ni++] = *p;
					++p;
				}
				name[ni] = 0;
				if (*p != ':') {
					while (*p && *p != ';')
						++p;
					if (*p)
						++p;
					continue;
				}
				++p;
				while (IsSpace(*p))
					++p;
				// Lit la valeur (peut etre longue : rgb(...) etc., on prend 255 chars).
				char val[256];
				int32 vi = 0;
				while (*p && *p != ';' && vi + 1 < (int32)sizeof(val))
					val[vi++] = *p++;
				val[vi] = 0;
				// Trim trailing whitespace.
				while (vi > 0 && IsSpace(val[vi - 1]))
					val[--vi] = 0;
				ApplyAttrToStyle(s, name, val);
				if (*p == ';')
					++p;
			}
		}

		/// Fusionne les attributs d'un element sur un style herite.
		/// Ordre : (1) parent, (2) attribut "style" inline, (3) attributs individuels.
		NkSVGStyle MergeStyle(const NkSVGStyle &parent, const AttrPair *attrs, int32 n) noexcept {
			NkSVGStyle s = parent;
			const char *css = FindAttr(attrs, n, "style");
			if (css)
				ApplyCSSStyle(s, css);
			for (int32 i = 0; i < n; ++i) {
				if (std::strcmp(attrs[i].name, "style") == 0)
					continue;
				ApplyAttrToStyle(s, attrs[i].name, attrs[i].value);
			}
			return s;
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// Helpers gradients : url(#id), props CSS, refs fill/stroke, lecture de <stop>
		// ─────────────────────────────────────────────────────────────────────────────

		/// Extrait l'id d'une valeur "url(#id)" -> out. @return true si match.
		bool ParseUrlRef(const char *v, char *out, usize outSz) noexcept {
			if (!v)
				return false;
			const char *p = SkipWS(v);
			if (std::strncmp(p, "url(", 4) != 0)
				return false;
			p += 4;
			p = SkipWS(p);
			if (*p == '#')
				++p;
			usize i = 0;
			while (*p && *p != ')' && i + 1 < outSz) {
				if (!IsSpace(*p))
					out[i++] = *p;
				++p;
			}
			out[i] = 0;
			return i > 0;
		}

		/// Recupere une propriete d'un attribut style="a:b;c:d". @return true si trouvee.
		bool GetCSSProp(const char *css, const char *prop, char *out, usize outSz) noexcept {
			if (!css)
				return false;
			const usize plen = std::strlen(prop);
			const char *p = css;
			while (*p) {
				const char *ns = p;
				while (*p && *p != ':' && *p != ';')
					++p;
				const char *n0 = ns;
				const char *nend = p;
				while (n0 < nend && IsSpace(*n0))
					++n0;
				while (nend > n0 && IsSpace(nend[-1]))
					--nend;
				const bool match = ((usize)(nend - n0) == plen) && std::strncmp(n0, prop, plen) == 0;
				if (*p == ':') {
					++p;
					while (*p && IsSpace(*p))
						++p;
					const char *vs = p;
					while (*p && *p != ';')
						++p;
					const char *ve = p;
					while (ve > vs && IsSpace(ve[-1]))
						--ve;
					if (match) {
						usize i = 0;
						for (const char *q = vs; q < ve && i + 1 < outSz; ++q)
							out[i++] = *q;
						out[i] = 0;
						return true;
					}
				}
				if (*p == ';')
					++p;
			}
			return false;
		}

		/// Met a jour un ref (fill/stroke) selon une valeur : url() -> set, sinon clear.
		void UpdateRefFromValue(char *ref, const char *value) noexcept {
			char tmp[64];
			if (value && ParseUrlRef(value, tmp, sizeof(tmp))) {
				std::strncpy(ref, tmp, 63);
				ref[63] = 0;
			} else if (value) {
				ref[0] = 0; // couleur solide ou "none" : annule un gradient herite
			}
		}

		/// Met a jour fillRef/strokeRef (deja herites) selon style="" puis attrs individuels.
		void UpdateRefs(char *fillRef, char *strokeRef, const AttrPair *attrs, int32 n) noexcept {
			const char *css = FindAttr(attrs, n, "style");
			if (css) {
				char buf[80];
				if (GetCSSProp(css, "fill", buf, sizeof(buf)))
					UpdateRefFromValue(fillRef, buf);
				if (GetCSSProp(css, "stroke", buf, sizeof(buf)))
					UpdateRefFromValue(strokeRef, buf);
			}
			for (int32 i = 0; i < n; ++i) {
				if (std::strcmp(attrs[i].name, "fill") == 0)
					UpdateRefFromValue(fillRef, attrs[i].value);
				else if (std::strcmp(attrs[i].name, "stroke") == 0)
					UpdateRefFromValue(strokeRef, attrs[i].value);
			}
		}

		/// Parse un nombre pouvant finir par '%' (=> /100). @p def si str nul.
		float32 ParsePct(const char *s, float32 def) noexcept {
			if (!s)
				return def;
			const char *e = nullptr;
			float32 v = ParseFloat(s, &e);
			if (e) {
				while (*e && IsSpace(*e))
					++e;
				if (*e == '%')
					v *= 0.01f;
			}
			return v;
		}

		/// Lit un <stop> -> GradStop (offset + stop-color * stop-opacity).
		GradStop ReadStop(const AttrPair *a, int32 n) noexcept {
			GradStop st;
			const char *css = FindAttr(a, n, "style");
			char cbuf[80], obuf[32];

			const char *sc = FindAttr(a, n, "stop-color");
			if (!sc && css && GetCSSProp(css, "stop-color", cbuf, sizeof(cbuf)))
				sc = cbuf;
			NkSVGColor col = sc ? NkSVGColor::Parse(sc) : NkSVGColor::Black();

			const char *so = FindAttr(a, n, "stop-opacity");
			if (!so && css && GetCSSProp(css, "stop-opacity", obuf, sizeof(obuf)))
				so = obuf;
			if (so) {
				float32 op = ParseFloat(so);
				if (op < 0.f)
					op = 0.f;
				if (op > 1.f)
					op = 1.f;
				col.a = (uint8)((float32)col.a * op);
			}
			col.none = false;
			st.color = col;
			st.offset = ParsePct(FindAttr(a, n, "offset"), 0.f);
			if (st.offset < 0.f)
				st.offset = 0.f;
			if (st.offset > 1.f)
				st.offset = 1.f;
			return st;
		}

		/// Applique une matrice a tous les points d'une Shape (in-place).
		void ApplyTransform(Shape &sh, const NkSVGTransform &m) noexcept {
			for (uint32 i = 0; i < sh.xs.Size(); ++i) {
				float32 x = sh.xs[i], y = sh.ys[i];
				m.Apply(x, y);
				sh.xs[i] = x;
				sh.ys[i] = y;
			}
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 7 — Constructeurs de Shape par tag SVG
		// ─────────────────────────────────────────────────────────────────────────────

		void ShapeFromRect(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st) noexcept {
			const char *sw = FindAttr(a, n, "width");
			const char *sh = FindAttr(a, n, "height");
			if (!sw || !sh)
				return;
			const float32 x = ParseFloat(FindAttr(a, n, "x"));
			const float32 y = ParseFloat(FindAttr(a, n, "y"));
			const float32 w = ParseFloat(sw);
			const float32 h = ParseFloat(sh);
			if (w <= 0 || h <= 0)
				return;

			// ── rx / ry : la regle du W3C (SVG 1.1 §9.2), pas une approximation ──
			//   - « auto » ou absent des DEUX cotes : coins droits ;
			//   - un seul des deux donne : l'autre prend sa valeur ;
			//   - chacun borne a la MOITIE du cote correspondant.
			// Le coin est un ARC (quart d'ellipse par une Bezier cubique, k = 4/3
			// tan(pi/8)), jamais une suite de segments : a l'oeil comme au test,
			// un octogone n'est pas un arrondi.
			const char *srx = FindAttr(a, n, "rx");
			const char *sry = FindAttr(a, n, "ry");
			const bool rxAuto = (!srx || StrCaseCmp(srx, "auto") == 0);
			const bool ryAuto = (!sry || StrCaseCmp(sry, "auto") == 0);
			float32 rx = rxAuto ? 0.f : ParseFloat(srx);
			float32 ry = ryAuto ? 0.f : ParseFloat(sry);
			if (rxAuto && !ryAuto)
				rx = ry;
			else if (ryAuto && !rxAuto)
				ry = rx;
			if (rx < 0.f)
				rx = 0.f;
			if (ry < 0.f)
				ry = 0.f;
			if (rx > w * 0.5f)
				rx = w * 0.5f;
			if (ry > h * 0.5f)
				ry = h * 0.5f;

			Shape s2;
			s2.style = st.style;
			PathBuilder pb(s2);
			if (rx > 0.f && ry > 0.f) {
				constexpr float32 k = 0.5522847498307933f;
				const float32 cx = rx * k, cy = ry * k;
				const float32 x0 = x, x1 = x + w, y0 = y, y1 = y + h;
				pb.StartContour(x0 + rx, y0);
				pb.LineTo(x1 - rx, y0);
				FlattenCubic(pb, x1 - rx, y0, x1 - rx + cx, y0, x1, y0 + ry - cy, x1, y0 + ry);
				pb.LineTo(x1, y1 - ry);
				FlattenCubic(pb, x1, y1 - ry, x1, y1 - ry + cy, x1 - rx + cx, y1, x1 - rx, y1);
				pb.LineTo(x0 + rx, y1);
				FlattenCubic(pb, x0 + rx, y1, x0 + rx - cx, y1, x0, y1 - ry + cy, x0, y1 - ry);
				pb.LineTo(x0, y0 + ry);
				FlattenCubic(pb, x0, y0 + ry, x0, y0 + ry - cy, x0 + rx - cx, y0, x0 + rx, y0);
			} else {
				pb.StartContour(x, y);
				pb.LineTo(x + w, y);
				pb.LineTo(x + w, y + h);
				pb.LineTo(x, y + h);
			}
			pb.Close();
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		/// Cercle : approxime par 4 Beziers cubiques (k = (4/3)*tan(pi/8) ≈ 0.5523).
		void ShapeFromCircle(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st) noexcept {
			const char *sr = FindAttr(a, n, "r");
			if (!sr)
				return;
			const float32 cx = ParseFloat(FindAttr(a, n, "cx"));
			const float32 cy = ParseFloat(FindAttr(a, n, "cy"));
			const float32 r = ParseFloat(sr);
			if (r <= 0)
				return;
			Shape s2;
			s2.style = st.style;
			PathBuilder pb(s2);
			const float32 k = 0.5522847498307933f;
			pb.StartContour(cx - r, cy);
			FlattenCubic(pb, cx - r, cy, cx - r, cy - r * k, cx - r * k, cy - r, cx, cy - r);
			FlattenCubic(pb, cx, cy - r, cx + r * k, cy - r, cx + r, cy - r * k, cx + r, cy);
			FlattenCubic(pb, cx + r, cy, cx + r, cy + r * k, cx + r * k, cy + r, cx, cy + r);
			FlattenCubic(pb, cx, cy + r, cx - r * k, cy + r, cx - r, cy + r * k, cx - r, cy);
			pb.Close();
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		void ShapeFromEllipse(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st) noexcept {
			const char *srx = FindAttr(a, n, "rx");
			const char *sry = FindAttr(a, n, "ry");
			if (!srx || !sry)
				return;
			const float32 cx = ParseFloat(FindAttr(a, n, "cx"));
			const float32 cy = ParseFloat(FindAttr(a, n, "cy"));
			const float32 rx = ParseFloat(srx), ry = ParseFloat(sry);
			if (rx <= 0 || ry <= 0)
				return;
			Shape s2;
			s2.style = st.style;
			PathBuilder pb(s2);
			const float32 k = 0.5522847498307933f;
			pb.StartContour(cx - rx, cy);
			FlattenCubic(pb, cx - rx, cy, cx - rx, cy - ry * k, cx - rx * k, cy - ry, cx, cy - ry);
			FlattenCubic(pb, cx, cy - ry, cx + rx * k, cy - ry, cx + rx, cy - ry * k, cx + rx, cy);
			FlattenCubic(pb, cx + rx, cy, cx + rx, cy + ry * k, cx + rx * k, cy + ry, cx, cy + ry);
			FlattenCubic(pb, cx, cy + ry, cx - rx * k, cy + ry, cx - rx, cy + ry * k, cx - rx, cy);
			pb.Close();
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		void ShapeFromLine(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st) noexcept {
			Shape s2;
			s2.style = st.style;
			PathBuilder pb(s2);
			pb.StartContour(ParseFloat(FindAttr(a, n, "x1")), ParseFloat(FindAttr(a, n, "y1")));
			pb.LineTo(ParseFloat(FindAttr(a, n, "x2")), ParseFloat(FindAttr(a, n, "y2")));
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		void ShapeFromPolygon(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st,
							  bool closed) noexcept {
			const char *pts = FindAttr(a, n, "points");
			if (!pts)
				return;
			Shape s2;
			s2.style = st.style;
			PathBuilder pb(s2);
			const char *p = pts;
			bool first = true;
			while (*p) {
				p = SkipWSComma(p);
				if (!*p)
					break;
				const float32 x = ParseFloat(p, &p);
				p = SkipWSComma(p);
				if (!*p && first)
					break;
				const float32 y = ParseFloat(p, &p);
				if (first) {
					pb.StartContour(x, y);
					first = false;
				} else
					pb.LineTo(x, y);
			}
			if (closed)
				pb.Close();
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// <image> — LA SOURCE : « data:image/...;base64,... » ou un chemin RELATIF AU
		// FICHIER SVG. Un href relatif ne veut rien dire sans savoir d'ou l'on lit :
		// le dossier du .svg est donc porte jusqu'ici (nullptr = on ne sait pas, et
		// on le DIT au lieu de deviner un chemin depuis le repertoire courant).
		// ─────────────────────────────────────────────────────────────────────────────

		/// Decode « data:<type>;base64,<charge> ». @return image invalide si ce n'est
		/// pas une URL de donnees, ou si la charge est illisible (dit par l'appelant).
		NkImage ImageDepuisDataURL(const char *href, bool &etaitDataURL) noexcept {
			etaitDataURL = false;
			NkImage vide;
			if (!href || std::strncmp(href, "data:", 5) != 0)
				return vide;
			etaitDataURL = true;
			// on saute jusqu'a la virgule ; on exige base64 (le pourcent-encodage
			// d'un SVG imbrique n'est pas lu -- ce serait un autre palier, et il est dit)
			const char *virgule = href;
			while (*virgule && *virgule != ',')
				++virgule;
			if (*virgule != ',')
				return vide;
			bool b64 = false;
			for (const char *p = href + 5; p < virgule; ++p) {
				if (std::strncmp(p, "base64", 6) == 0) {
					b64 = true;
					break;
				}
			}
			if (!b64) {
				logger.Warn("[SVG] <image> : data: sans « base64 » -- charge non lue.");
				return vide;
			}
			const char *charge = virgule + 1;
			const usize nChars = std::strlen(charge);
			if (nChars == 0)
				return vide;
			// 4 caracteres -> 3 octets ; on alloue large, NkDecode rend la taille exacte
			const usize capacite = (nChars / 4u + 2u) * 3u;
			uint8 *brut = (uint8 *)NkAlloc(capacite);
			if (!brut)
				return vide;
			usize ecrits = 0;
			const NkStringView vue(charge, nChars);
			// NkDecode ECHOUE sur un caractere invalide (il ne devine pas) : on relaie
			// cet echec au lieu de peindre du bruit.
			if (!encoding::base64::NkDecode(vue, brut, &ecrits) || ecrits == 0) {
				NkFree(brut);
				logger.Warn("[SVG] <image> : base64 illisible -- image ignoree.");
				return vide;
			}
			NkImage im;
			// 4 canaux exiges : le rasteriseur compose en RGBA, un PNG gris ou sans
			// alpha doit arriver au meme format que les autres.
			if (!im.LoadFromMemory(brut, ecrits, 4)) {
				NkFree(brut);
				logger.Warn("[SVG] <image> : les octets base64 ne sont AUCUN des formats connus de NKImage.");
				return NkImage();
			}
			NkFree(brut);
			return im;
		}

		/// Joint le dossier du .svg et un href relatif. Un href ABSOLU (C:/..., /...)
		/// est pris tel quel.
		void JoindreChemin(char *out, usize outSz, const char *base, const char *href) noexcept {
			out[0] = 0;
			if (!href)
				return;
			const bool absolu = (href[0] == '/' || href[0] == '\\' ||
								 (href[0] && href[1] == ':' && (href[2] == '/' || href[2] == '\\')));
			if (absolu || !base || !*base) {
				std::strncpy(out, href, outSz - 1);
				out[outSz - 1] = 0;
				return;
			}
			std::strncpy(out, base, outSz - 1);
			out[outSz - 1] = 0;
			usize l = std::strlen(out);
			if (l > 0 && out[l - 1] != '/' && out[l - 1] != '\\' && l + 1 < outSz) {
				out[l++] = '/';
				out[l] = 0;
			}
			std::strncat(out, href, outSz - l - 1);
		}

		FitKind LireFit(const char *par) noexcept {
			if (!par)
				return FitKind::Meet; // le defaut SVG : xMidYMid meet
			const char *p = SkipWS(par);
			if (std::strncmp(p, "none", 4) == 0)
				return FitKind::None;
			// « <align> [meet|slice] » : l'alignement autre que Mid n'est pas honore
			// (l'image est CENTREE) -- dit par l'appelant, une fois.
			for (const char *q = p; *q; ++q) {
				if (std::strncmp(q, "slice", 5) == 0)
					return FitKind::Slice;
			}
			return FitKind::Meet;
		}

		void ShapeFromImage(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st,
							const char *baseDir, SkipList &skips) noexcept {
			const char *href = FindAttr(a, n, "href");
			if (!href)
				href = FindAttr(a, n, "xlink:href");
			if (!href || !*href)
				return;
			const float32 w = ParseFloat(FindAttr(a, n, "width"));
			const float32 h = ParseFloat(FindAttr(a, n, "height"));
			if (w <= 0.f || h <= 0.f)
				return;
			const float32 x = ParseFloat(FindAttr(a, n, "x"));
			const float32 y = ParseFloat(FindAttr(a, n, "y"));

			bool dataURL = false;
			NkImage im = ImageDepuisDataURL(href, dataURL);
			if (!dataURL) {
				char chemin[1024];
				JoindreChemin(chemin, sizeof(chemin), baseDir, href);
				if (!chemin[0])
					return;
				if (!baseDir && href[0] != '/' && !(href[0] && href[1] == ':')) {
					if (skips.Noter("image-href-relatif"))
						logger.Warn("[SVG] <image href=\"{0}\"> : le SVG a ete decode SANS son dossier "
									"(Decode d'un buffer) -- le chemin est resolu depuis le repertoire courant.",
									href);
				}
				if (!im.Load(chemin, 4)) {
					logger.Warn("[SVG] <image> : « {0} » illisible -- image ignoree (le cadre reste vide).", chemin);
					return;
				}
			}
			if (!im.IsValid())
				return;

			const char *par = FindAttr(a, n, "preserveAspectRatio");
			if (par) {
				const char *p = SkipWS(par);
				if (std::strncmp(p, "none", 4) != 0 && std::strncmp(p, "xMidYMid", 8) != 0 &&
					skips.Noter("preserveAspectRatio-align"))
					logger.Warn("[SVG] preserveAspectRatio « {0} » : l'ajustement (meet / slice) est honore, "
								"l'ALIGNEMENT ne l'est pas -- l'image est centree.",
								par);
			}

			Shape s2;
			s2.style = st.style;
			s2.img = std::move(im);
			s2.ix = x;
			s2.iy = y;
			s2.iw = w;
			s2.ih = h;
			s2.fit = LireFit(par);
			// les quatre coins de la boite, comme pour toute shape : ils donnent la
			// bbox de balayage, et ils suivent le transform du groupe parent.
			PathBuilder pb(s2);
			pb.StartContour(x, y);
			pb.LineTo(x + w, y);
			pb.LineTo(x + w, y + h);
			pb.LineTo(x, y + h);
			pb.Close();
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		void ShapeFromPath(NkVector<Shape> &shapes, const AttrPair *a, int32 n, const ParseState &st) noexcept {
			const char *d = FindAttr(a, n, "d");
			if (!d)
				return;
			Shape s2;
			s2.style = st.style;
			PathBuilder pb(s2);
			ParsePathD(d, pb);
			ApplyTransform(s2, st.xform);
			shapes.PushBack(std::move(s2));
		}

		// ═════════════════════════════════════════════════════════════════════════════
		// SECTION 7bis — <text> : LES CONTOURS DES GLYPHES, par NKFont
		// -----------------------------------------------------------------------------
		// CHERCHER AVANT D'ECRIRE (porte du 28/08) : NKFont porte deja tout ce qu'il
		// faut, une couche plus bas -- `NkGetGlyphShape` rend les contours d'un glyphe
		// (TrueType et CFF), `NkScaleForEmToPixels` l'echelle, `NkGetGlyphHMetrics`
		// l'avance. On ne rasterise donc AUCUN glyphe ici : on les convertit en
		// contours et on les donne au rasteriseur qui remplit deja tout le reste.
		//
		// POURQUOI LES CONTOURS ET PAS L'ATLAS. L'atlas de NkFontAtlas est rasterise
		// A UNE TAILLE en pixels ; un SVG se re-rasterise a n'importe quelle taille
		// (c'est son interet) et porte des matrices qui tournent. Un atlas etire par
		// une matrice rend flou -- c'est exactement ce que la sonde 81 de NkUIDesign
		// mesure et refuse. Les contours, eux, suivent la matrice sans perte, passent
		// par le meme anticrenelage que les formes, et acceptent un degrade.
		//
		// ⚠️ CE QUE `font-size` VEUT DIRE. En SVG c'est le CADRATIN (em) : l'echelle
		// est `font-size / unitsPerEm`. NkFontAtlas, lui, appelle
		// `NkScaleForPixelHeight` = px / (ascender - descender). Pour Inter les deux
		// different d'environ 21 % ; on suit LA NORME, et l'ecart avec le peintre est
		// mesure et nomme par le banc du temoin croise plutot que masque.
		// ═════════════════════════════════════════════════════════════════════════════

		/// Une police ouverte pendant le temps d'un decodage.
		struct FaceChargee {
				char famille[64] = {0};
				uint8 *ttf = nullptr; ///< buffer decompresse (nous appartient)
				nkfont::NkFontFaceInfo info;
				bool ok = false;
		};

		/// Les polices ouvertes pour CE document. Ouvrir une police coute une
		/// decompression et un parsing de tables : on ne le fait qu'une fois par
		/// famille, et on rend tout a la fin du decodage.
		struct FontCache {
				static constexpr int32 kMax = 4;
				FaceChargee faces[kMax];
				int32 nb = 0;

				/// Le premier nom d'une liste « Inter, sans-serif » -> "Inter",
				/// guillemets et espaces retires.
				static void PremiereFamille(const char *liste, char *out, usize outSz) noexcept {
					out[0] = 0;
					if (!liste)
						return;
					const char *p = SkipWS(liste);
					usize i = 0;
					while (*p && *p != ',' && i + 1 < outSz) {
						if (*p != '"' && *p != '\'')
							out[i++] = *p;
						++p;
					}
					while (i > 0 && IsSpace(out[i - 1]))
						--i;
					out[i] = 0;
				}

				/// La police embarquee dont le nom correspond, sinon Inter -- et on DIT
				/// le repli : un texte rendu dans une autre police que celle demandee
				/// n'est pas une erreur, mais le taire en serait une.
				nkfont::NkFontFaceInfo *Obtenir(const char *fontFamily, SkipList &skips) noexcept {
					char fam[64];
					PremiereFamille(fontFamily, fam, sizeof(fam));
					if (!fam[0])
						std::strncpy(fam, "Inter", sizeof(fam) - 1);

					for (int32 i = 0; i < nb; ++i)
						if (StrCaseCmp(faces[i].famille, fam) == 0)
							return faces[i].ok ? &faces[i].info : nullptr;
					if (nb >= kMax)
						return nb > 0 && faces[0].ok ? &faces[0].info : nullptr;

					// la police embarquee du meme nom, sinon Inter
					int32 nbEmb = 0;
					const NkEmbeddedFontData *toutes = NkFontEmbedded::GetAll(&nbEmb);
					NkEmbeddedFontId choisie = NkEmbeddedFontId::Inter;
					bool trouvee = false;
					for (int32 i = 0; i < nbEmb && toutes; ++i) {
						if (toutes[i].name && StrCaseCmp(toutes[i].name, fam) == 0) {
							choisie = (NkEmbeddedFontId)i;
							trouvee = true;
							break;
						}
					}
					if (!trouvee) {
						char cle[80];
						std::snprintf(cle, sizeof(cle), "police:%s", fam);
						if (skips.Noter(cle))
							logger.Warn("[SVG] <text font-family=\"{0}\"> : police indisponible -- repli sur Inter "
										"(embarquee). La forme des glyphes n'est PAS celle demandee.",
										fam);
					}
					const NkEmbeddedFontData *d = NkFontEmbedded::GetData(choisie);
					FaceChargee &f = faces[nb++];
					std::strncpy(f.famille, fam, sizeof(f.famille) - 1);
					if (!d) {
						if (skips.Noter("police-absente"))
							logger.Warn("[SVG] <text> : AUCUNE police embarquee dans ce binaire -- le texte n'est "
										"pas peint.");
						return nullptr;
					}
					uint32 taille = 0;
					f.ttf = NkFontEmbedded::DecompressData(*d, &taille);
					if (!f.ttf || taille == 0) {
						logger.Warn("[SVG] <text> : decompression de la police « {0} » echouee.", d->name);
						return nullptr;
					}
					f.ok = nkfont::NkInitFontFace(&f.info, f.ttf, (nkft_size)taille, 0);
					if (!f.ok) {
						logger.Warn("[SVG] <text> : police « {0} » illisible (tables).", d->name);
						return nullptr;
					}
					return &f.info;
				}

				void Liberer() noexcept {
					for (int32 i = 0; i < nb; ++i) {
						if (faces[i].ok)
							nkfont::NkFreeFontFace(&faces[i].info);
						if (faces[i].ttf)
							NkFontEmbedded::FreeDecompressedData(faces[i].ttf);
						faces[i].ttf = nullptr;
						faces[i].ok = false;
					}
					nb = 0;
				}
		};

		/// Un morceau de texte : le contenu direct d'un <text>, ou celui d'un <tspan>.
		struct TextFragment {
				char txt[512] = {0};
				float32 fontSize = 16.f;
				char famille[64] = {0};
				int32 weight = 400;
				NkSVGStyle style;
				char fillRef[64] = {0};
				bool posX = false, posY = false; ///< le fragment repositionne le curseur
				float32 x = 0.f, y = 0.f;
		};

		/// L'etat d'un <text> en cours : on accumule les fragments jusqu'a </text>,
		/// PARCE QUE `text-anchor` a besoin de la largeur TOTALE, qu'on ne connait
		/// qu'a la fermeture. Poser les glyphes au fil de l'eau aurait interdit
		/// « middle » et « end », ou impose une seconde passe.
		struct TextState {
				bool actif = false;
				float32 x = 0.f, y = 0.f;
				int32 anchor = 0; ///< 0 start, 1 middle, 2 end
				bool preserve = false;
				NkSVGTransform xform = NkSVGTransform::Identity();
				NkVector<TextFragment> frags;
		};

		/// Decode les entites XML et applique la regle des blancs de SVG :
		/// sans xml:space="preserve", les blancs sont reduits a UN espace et les
		/// bords sont rognes -- sans quoi l'INDENTATION du fichier deviendrait du
		/// texte visible.
		void NettoyerTexte(const char *src, usize n, bool preserve, char *out, usize outSz) noexcept {
			usize o = 0;
			bool blancPrec = true; // rogne les blancs de tete
			for (usize i = 0; i < n && o + 1 < outSz; ++i) {
				char c = src[i];
				if (c == '&') { // entites
					if (std::strncmp(src + i, "&amp;", 5) == 0) { c = '&'; i += 4; }
					else if (std::strncmp(src + i, "&lt;", 4) == 0) { c = '<'; i += 3; }
					else if (std::strncmp(src + i, "&gt;", 4) == 0) { c = '>'; i += 3; }
					else if (std::strncmp(src + i, "&quot;", 6) == 0) { c = '"'; i += 5; }
					else if (std::strncmp(src + i, "&apos;", 6) == 0) { c = '\''; i += 5; }
					else if (src[i + 1] == '#') {
						usize j = i + 2;
						int32 code = 0;
						if (src[j] == 'x' || src[j] == 'X') {
							++j;
							while (j < n && HexDigit(src[j]) >= 0)
								code = code * 16 + HexDigit(src[j++]);
						} else {
							while (j < n && IsDigit(src[j]))
								code = code * 10 + (src[j++] - '0');
						}
						if (j < n && src[j] == ';' && code > 0) {
							char buf[8];
							const int32 nb = NkFontEncodeUTF8((NkFontCodepoint)code, buf, (nkft_int32)sizeof(buf));
							for (int32 k = 0; k < nb && o + 1 < outSz; ++k)
								out[o++] = buf[k];
							i = j;
							blancPrec = false;
							continue;
						}
					}
				}
				if (!preserve && IsSpace(c)) {
					if (blancPrec)
						continue;
					c = ' ';
					blancPrec = true;
				} else {
					blancPrec = false;
				}
				out[o++] = c;
			}
			if (!preserve)
				while (o > 0 && out[o - 1] == ' ')
					--o;
			out[o] = 0;
		}

		/// La largeur d'un fragment, en unites utilisateur (somme des avances).
		/// Le CRENAGE n'est PAS applique : le peintre de NKGui ne l'applique pas non
		/// plus (`x += g->advanceX`), et le temoin croise compare les deux.
		float32 LargeurFragment(nkfont::NkFontFaceInfo *face, const TextFragment &fr) noexcept {
			if (!face)
				return 0.f;
			const float32 ech = nkfont::NkScaleForEmToPixels(face, fr.fontSize);
			const char *p = fr.txt;
			const char *end = p + std::strlen(p);
			float32 w = 0.f;
			while (p < end) {
				const NkFontCodepoint cp = NkFontDecodeUTF8(&p, end);
				if (cp == 0u)
					break;
				const nkft_int32 gid = (nkft_int32)nkfont::NkFindGlyphIndex(face, cp);
				nkft_int32 aw = 0, lsb = 0;
				nkfont::NkGetGlyphHMetrics(face, (NkGlyphId)gid, &aw, &lsb);
				w += (float32)aw * ech;
			}
			return w;
		}

		/// Pose les contours d'un fragment dans @p sh, a partir de (penX, penY).
		/// penX avance. Y est INVERSE : la police monte, l'ecran descend.
		void ContoursDuFragment(Shape &sh, nkfont::NkFontFaceInfo *face, const TextFragment &fr, float32 &penX,
								float32 penY) noexcept {
			if (!face)
				return;
			const float32 ech = nkfont::NkScaleForEmToPixels(face, fr.fontSize);
			PathBuilder pb(sh);
			const char *p = fr.txt;
			const char *end = p + std::strlen(p);
			while (p < end) {
				const NkFontCodepoint cp = NkFontDecodeUTF8(&p, end);
				if (cp == 0u)
					break;
				const NkGlyphId gid = nkfont::NkFindGlyphIndex(face, cp);
				nkft_int32 aw = 0, lsb = 0;
				nkfont::NkGetGlyphHMetrics(face, gid, &aw, &lsb);
				nkfont::NkFontVertexBuffer vb;
				if (gid != 0 && nkfont::NkGetGlyphShape(face, gid, &vb)) {
					float32 cx = 0.f, cy = 0.f;
					bool ouvert = false;
					for (uint32 i = 0; i < vb.count; ++i) {
						const nkfont::NkFontVertex &v = vb.verts[i];
						const float32 vx = penX + (float32)v.x * ech;
						const float32 vy = penY - (float32)v.y * ech;
						switch (v.type) {
							case nkfont::NK_FONT_VERTEX_MOVE:
								if (ouvert)
									pb.Close(); // un contour de glyphe est TOUJOURS ferme :
												// sans ca, le remplissage nonzero fuit
								pb.StartContour(vx, vy);
								ouvert = true;
								break;
							case nkfont::NK_FONT_VERTEX_LINE:
								pb.LineTo(vx, vy);
								break;
							case nkfont::NK_FONT_VERTEX_CURVE:
								FlattenQuad(pb, cx, cy, penX + (float32)v.cx * ech, penY - (float32)v.cy * ech, vx, vy);
								break;
							case nkfont::NK_FONT_VERTEX_CUBIC:
								FlattenCubic(pb, cx, cy, penX + (float32)v.cx * ech, penY - (float32)v.cy * ech,
											 penX + (float32)v.cx1 * ech, penY - (float32)v.cy1 * ech, vx, vy);
								break;
							default:
								break;
						}
						cx = vx;
						cy = vy;
					}
					if (ouvert)
						pb.Close();
				}
				penX += (float32)aw * ech;
			}
		}

		/// Ferme un <text> : c'est ICI qu'on connait la largeur totale, donc l'ancrage.
		void FinirTexte(NkVector<Shape> &shapes, TextState &ts, FontCache &polices, SkipList &skips) noexcept {
			if (!ts.actif)
				return;
			ts.actif = false;
			if (ts.frags.IsEmpty())
				return;

			// largeur totale (les fragments qui repositionnent le curseur ouvrent une
			// nouvelle sequence : l'ancrage ne vaut que pour celle en cours)
			float32 largeur = 0.f;
			for (uint32 i = 0; i < ts.frags.Size(); ++i) {
				nkfont::NkFontFaceInfo *face = polices.Obtenir(ts.frags[i].famille, skips);
				largeur += LargeurFragment(face, ts.frags[i]);
			}
			float32 penX = ts.x;
			if (ts.anchor == 1)
				penX -= largeur * 0.5f;
			else if (ts.anchor == 2)
				penX -= largeur;
			float32 penY = ts.y;

			Shape sh;
			sh.style = ts.frags[0].style;
			// LES GLYPHES SE REMPLISSENT EN NONZERO. Un `fill-rule="evenodd"` herite
			// d'un ancetre creverait les contre-formes (le O deviendrait plein, le e
			// perdrait son oeil) : la regle du texte n'est pas celle du dessin.
			sh.style.fillEvenOdd = false;
			std::strncpy(sh.fillRef, ts.frags[0].fillRef, 63);

			bool grasSimule = false;
			for (uint32 i = 0; i < ts.frags.Size(); ++i) {
				TextFragment &fr = ts.frags[i];
				nkfont::NkFontFaceInfo *face = polices.Obtenir(fr.famille, skips);
				if (fr.posX)
					penX = fr.x;
				if (fr.posY)
					penY = fr.y;
				ContoursDuFragment(sh, face, fr, penX, penY);
				if (fr.weight >= 600)
					grasSimule = true;
			}
			if (sh.contourStart.IsEmpty())
				return;
			// FAUSSE GRAISSE : les polices embarquees n'ont qu'une coupe. Un
			// font-weight >= 600 est rendu par un TRAIT de la couleur du remplissage
			// autour du glyphe -- plus d'encre, la meme forme. Ce n'est pas un vrai
			// Bold dessine par un typographe, et c'est dit.
			if (grasSimule) {
				sh.style.stroke = sh.style.fill;
				sh.style.strokeOpacity = sh.style.fillOpacity;
				sh.style.strokeWidth = ts.frags[0].fontSize * 0.03f;
				if (skips.Noter("font-weight"))
					logger.Warn("[SVG] font-weight >= 600 : la police embarquee n'a qu'une coupe -- graisse "
								"SIMULEE par un trait (ce n'est pas un vrai Bold).");
			}
			ApplyTransform(sh, ts.xform);
			sh.ctm = ts.xform;
			shapes.PushBack(std::move(sh));
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 8 — Parser XML stream-based avec stack <g>
		// ─────────────────────────────────────────────────────────────────────────────

		/// Parse les attributs d'une balise jusqu'a '>'. Les noms et valeurs sont
		/// stockes dans des buffers fournis par le caller (nameBuf + attrPool partage).
		int32 ParseTagAttrs(const char *&p, const char *end, AttrPair *attrs, char *nameBuf, usize &nameOff,
							usize nameBufSize, char *attrPool, usize &poolOff, usize attrPoolSize) noexcept {
			int32 num = 0;
			while (p < end && *p != '>' && *p != '/') {
				while (p < end && IsSpace(*p))
					++p;
				if (p >= end || *p == '>' || *p == '/')
					break;
				// Nom de l'attribut.
				const usize nameStart = nameOff;
				while (p < end && *p != '=' && !IsSpace(*p) && *p != '>' && *p != '/' && nameOff + 1 < nameBufSize) {
					nameBuf[nameOff++] = *p++;
				}
				if (nameOff < nameBufSize)
					nameBuf[nameOff++] = 0;
				while (p < end && IsSpace(*p))
					++p;
				// Valeur.
				const char *valPtr = "";
				if (p < end && *p == '=') {
					++p;
					while (p < end && IsSpace(*p))
						++p;
					const char q = (*p == '"' || *p == '\'') ? *p++ : ' ';
					valPtr = (const char *)(attrPool + poolOff);
					usize vi = 0;
					while (p < end && *p != q && *p != '>' && poolOff + vi + 1 < attrPoolSize) {
						attrPool[poolOff + vi++] = *p++;
					}
					// Si pool sature, skip le reste pour avancer correctement.
					while (p < end && *p != q && *p != '>')
						++p;
					attrPool[poolOff + vi] = 0;
					poolOff += vi + 1;
					if (*p == q)
						++p;
				}
				if (num < 64) {
					attrs[num].name = nameBuf + nameStart;
					attrs[num].value = valPtr;
					++num;
				}
			}
			return num;
		}

		/// Lit le prochain tag dans le flux XML.
		/// @return 0 = fin, 1 = balise ouvrante, 2 = balise self-close, 3 = balise fermante.
		int32 ReadNextTag(const char *&p, const char *end, char *tagBuf, usize tagBufSize, AttrPair *attrs,
						  int32 &outNumAttrs, char *nameBuf, usize &nameOff, usize nameBufSize, char *attrPool,
						  usize &poolOff, usize attrPoolSize) noexcept {
			while (p < end) {
				// Skip texte / espace entre tags.
				while (p < end && *p != '<')
					++p;
				if (p >= end)
					return 0;
				++p;
				// Commentaire <!-- ... -->
				if (p + 2 < end && p[0] == '!' && p[1] == '-' && p[2] == '-') {
					p += 3;
					while (p + 2 < end && !(p[0] == '-' && p[1] == '-' && p[2] == '>'))
						++p;
					if (p + 2 < end)
						p += 3;
					continue;
				}
				// PI <?...?> ou DOCTYPE <!...>
				if (p < end && (*p == '?' || *p == '!')) {
					while (p < end && *p != '>')
						++p;
					if (p < end)
						++p;
					continue;
				}
				// Closing tag </tag>
				if (p < end && *p == '/') {
					++p;
					usize ti = 0;
					while (p < end && *p != '>' && !IsSpace(*p) && ti + 1 < tagBufSize) {
						tagBuf[ti++] = *p++;
					}
					tagBuf[ti] = 0;
					while (p < end && *p != '>')
						++p;
					if (p < end)
						++p;
					outNumAttrs = 0;
					return 3;
				}
				// Opening tag <tag attr=...>
				usize ti = 0;
				while (p < end && *p != '>' && *p != '/' && !IsSpace(*p) && ti + 1 < tagBufSize) {
					tagBuf[ti++] = *p++;
				}
				tagBuf[ti] = 0;
				// On reset les offsets de pool a chaque tag (les valeurs precedentes
				// ne sont plus utilisees apres traitement du tag).
				nameOff = 0;
				poolOff = 0;
				outNumAttrs =
					ParseTagAttrs(p, end, attrs, nameBuf, nameOff, nameBufSize, attrPool, poolOff, attrPoolSize);
				const bool self = (p < end && *p == '/');
				while (p < end && *p != '>')
					++p;
				if (p < end)
					++p;
				return self ? 2 : 1;
			}
			return 0;
		}

		/// Parse l'ensemble du document SVG -> liste de shapes + gradients + viewBox + dim.
		void ParseSVGDocument(const char *xml, usize xmlLen, NkVector<Shape> &shapes, NkVector<Gradient> &gradients,
							  float32 &vbX, float32 &vbY, float32 &vbW, float32 &vbH, float32 &svgW, float32 &svgH,
							  SkipList &skips, const char *baseDir) noexcept {
			vbX = vbY = 0.f;
			vbW = 0.f;
			vbH = 0.f;
			svgW = 0.f;
			svgH = 0.f;

			constexpr usize kAttrPoolSize = 1024 * 1024; // 1 MB par tag
			constexpr usize kNameBufSize = 64 * 1024;	 // 64 KB noms d'attrs
			char *nameBuf = (char *)NkAlloc(kNameBufSize);
			char *attrPool = (char *)NkAlloc(kAttrPoolSize);
			if (!nameBuf || !attrPool) {
				if (nameBuf)
					NkFree(nameBuf);
				if (attrPool)
					NkFree(attrPool);
				return;
			}
			usize nameOff = 0, poolOff = 0;

			char tagBuf[64];
			AttrPair attrs[64];

			// Stack <g> avec depth max raisonnable.
			constexpr int32 kMaxDepth = 64;
			ParseState stack[kMaxDepth];
			int32 depth = 0;
			stack[0] = {};

			int32 numAttrs = 0;
			bool gotSvg = false;
			TextState texte;
			FontCache polices;
			int32 defsDepth = 0; // > 0 = on est dans <defs> : shapes non rendues
			bool buildingGrad = false;
			Gradient curGrad; // gradient en cours de construction (stops)
			// BUG FIX : end est calcule UNE seule fois. xml est passe par reference et
			// est avance par le parser ; si on recalcule "xml + xmlLen" a chaque tour,
			// end se decale avec xml et finit par sortir du buffer (SEGV sur memoire
			// non mappee lors du parsing de longs SVG).
			const char *const xmlEnd = xml + xmlLen;
			while (true) {
				const int32 kind =
					ReadNextTag(reinterpret_cast<const char *&>(xml), xmlEnd, tagBuf, sizeof(tagBuf), attrs, numAttrs,
								nameBuf, nameOff, kNameBufSize, attrPool, poolOff, kAttrPoolSize);
				if (kind == 0)
					break;

				if (kind == 3) {
					// Closing tag : depile la stack / la profondeur defs / finalise un gradient.
					if (std::strcmp(tagBuf, "text") == 0) {
						FinirTexte(shapes, texte, polices, skips);
						continue;
					}
					if (std::strcmp(tagBuf, "tspan") == 0)
						continue;
					if (std::strcmp(tagBuf, "g") == 0 && depth > 0) {
						--depth;
					} else if (std::strcmp(tagBuf, "defs") == 0 && defsDepth > 0) {
						--defsDepth;
					} else if ((std::strcmp(tagBuf, "linearGradient") == 0 ||
								std::strcmp(tagBuf, "radialGradient") == 0) &&
							   buildingGrad) {
						gradients.PushBack(std::move(curGrad));
						curGrad = Gradient();
						buildingGrad = false;
					}
					continue;
				}

				ParseState &cur = stack[depth];

				if (std::strcmp(tagBuf, "svg") == 0 && !gotSvg) {
					const char *vb = FindAttr(attrs, numAttrs, "viewBox");
					if (vb) {
						const char *pp = vb;
						vbX = ParseFloat(pp, &pp);
						pp = SkipWSComma(pp);
						vbY = ParseFloat(pp, &pp);
						pp = SkipWSComma(pp);
						vbW = ParseFloat(pp, &pp);
						pp = SkipWSComma(pp);
						vbH = ParseFloat(pp, &pp);
					}
					const char *w = FindAttr(attrs, numAttrs, "width");
					const char *h = FindAttr(attrs, numAttrs, "height");
					if (w)
						svgW = ParseFloat(w);
					if (h)
						svgH = ParseFloat(h);
					// <svg> peut aussi porter fill/stroke par defaut + transform.
					cur.style = MergeStyle(cur.style, attrs, numAttrs);
					UpdateRefs(cur.fillRef, cur.strokeRef, attrs, numAttrs);
					const char *tr = FindAttr(attrs, numAttrs, "transform");
					if (tr)
						cur.xform = cur.xform * NkSVGTransform::Parse(tr);
					gotSvg = true;
					continue;
				}

				if (std::strcmp(tagBuf, "g") == 0) {
					// Ouverture <g> : push un nouveau state derive de cur.
					// BUG FIX : on ne pop QU'A la fermeture </g>, pas a la sortie du
					// tag courant. Les <path> enfants heritent ainsi du transform/style.
					if (kind == 1 && depth + 1 < kMaxDepth) {
						ParseState next = cur;
						next.style = MergeStyle(cur.style, attrs, numAttrs);
						UpdateRefs(next.fillRef, next.strokeRef, attrs, numAttrs);
						const char *tr = FindAttr(attrs, numAttrs, "transform");
						if (tr)
							next.xform = cur.xform * NkSVGTransform::Parse(tr);
						stack[++depth] = next;
					}
					// <g/> self-closed = groupe vide, on ignore.
					continue;
				}

				// <defs> : on N'ignore PLUS le bloc -> on entre dedans pour capter les
				// gradients ; un compteur empeche le rendu des shapes qu'il contient.
				if (std::strcmp(tagBuf, "defs") == 0) {
					if (kind == 1)
						++defsDepth;
					continue;
				}

				// Gradients : <linearGradient> / <radialGradient> (+ <stop> enfants).
				if (std::strcmp(tagBuf, "linearGradient") == 0 || std::strcmp(tagBuf, "radialGradient") == 0) {
					curGrad = Gradient();
					curGrad.kind = (tagBuf[0] == 'l') ? GradKind::Linear : GradKind::Radial;
					const char *id = FindAttr(attrs, numAttrs, "id");
					if (id) {
						std::strncpy(curGrad.id, id, 63);
						curGrad.id[63] = 0;
					}
					const char *href = FindAttr(attrs, numAttrs, "xlink:href");
					if (!href)
						href = FindAttr(attrs, numAttrs, "href");
					if (href) {
						const char *hp = (*href == '#') ? href + 1 : href;
						std::strncpy(curGrad.href, hp, 63);
						curGrad.href[63] = 0;
					}
					const char *gu = FindAttr(attrs, numAttrs, "gradientUnits");
					curGrad.userSpace = (gu && std::strcmp(gu, "userSpaceOnUse") == 0);
					curGrad.hasUnits = (gu != nullptr);
					const char *gt = FindAttr(attrs, numAttrs, "gradientTransform");
					if (gt) {
						curGrad.xform = NkSVGTransform::Parse(gt);
						curGrad.hasXform = true;
					}
					const char *sp = FindAttr(attrs, numAttrs, "spreadMethod");
					curGrad.hasSpread = (sp != nullptr);
					if (sp) {
						if (std::strcmp(sp, "reflect") == 0)
							curGrad.spread = GradSpread::Reflect;
						else if (std::strcmp(sp, "repeat") == 0)
							curGrad.spread = GradSpread::Repeat;
					}
					if (curGrad.kind == GradKind::Linear) {
						const char *a1 = FindAttr(attrs, numAttrs, "x1");
						const char *b1 = FindAttr(attrs, numAttrs, "y1");
						const char *a2 = FindAttr(attrs, numAttrs, "x2");
						const char *b2 = FindAttr(attrs, numAttrs, "y2");
						curGrad.x1 = ParsePct(a1, 0.f);
						curGrad.y1 = ParsePct(b1, 0.f);
						curGrad.x2 = ParsePct(a2, 1.f);
						curGrad.y2 = ParsePct(b2, 0.f);
						curGrad.hasX1 = (a1 != nullptr);
						curGrad.hasY1 = (b1 != nullptr);
						curGrad.hasX2 = (a2 != nullptr);
						curGrad.hasY2 = (b2 != nullptr);
					} else {
						const char *ac = FindAttr(attrs, numAttrs, "cx");
						const char *bc = FindAttr(attrs, numAttrs, "cy");
						const char *rr = FindAttr(attrs, numAttrs, "r");
						curGrad.cx = ParsePct(ac, 0.5f);
						curGrad.cy = ParsePct(bc, 0.5f);
						curGrad.r = ParsePct(rr, 0.5f);
						curGrad.hasCx = (ac != nullptr);
						curGrad.hasCy = (bc != nullptr);
						curGrad.hasR = (rr != nullptr);
						// LE FOYER : par defaut il est AU CENTRE (le degrade est alors
						// concentrique) ; fx/fy le deplacent, et la lumiere se decale.
						const char *sfx = FindAttr(attrs, numAttrs, "fx");
						const char *sfy = FindAttr(attrs, numAttrs, "fy");
						curGrad.fx = sfx ? ParsePct(sfx, curGrad.cx) : curGrad.cx;
						curGrad.fy = sfy ? ParsePct(sfy, curGrad.cy) : curGrad.cy;
						curGrad.hasFocal = (sfx != nullptr || sfy != nullptr);
					}
					buildingGrad = true;
					if (kind == 2) { // self-close (ex. gradient referencant un href, sans stops)
						gradients.PushBack(std::move(curGrad));
						curGrad = Gradient();
						buildingGrad = false;
					}
					continue;
				}
				if (std::strcmp(tagBuf, "stop") == 0) {
					if (buildingGrad)
						curGrad.stops.PushBack(ReadStop(attrs, numAttrs));
					continue;
				}

				// Tags structurels ignores (contenu sans shape direct). <style> est
				// SAUTE, pas applique : les classes CSS ne sont pas resolues -> on le dit.
				if (std::strcmp(tagBuf, "title") == 0 || std::strcmp(tagBuf, "desc") == 0 ||
					std::strcmp(tagBuf, "metadata") == 0 || std::strcmp(tagBuf, "style") == 0) {
					if (std::strcmp(tagBuf, "style") == 0 && skips.Noter("style"))
						logger.Warn("[SVG] <style> saute : les classes CSS ne sont pas resolues (attributs et "
									"style=\"...\" inline seuls).");
					// Skip jusqu'au closing equivalent.
					if (kind == 1) {
						int32 nested = 1;
						while (nested > 0) {
							const int32 k2 = ReadNextTag(reinterpret_cast<const char *&>(xml), xml + xmlLen, tagBuf,
														 sizeof(tagBuf), attrs, numAttrs, nameBuf, nameOff,
														 kNameBufSize, attrPool, poolOff, kAttrPoolSize);
							if (k2 == 0)
								break;
							if (k2 == 1)
								++nested;
							else if (k2 == 3)
								--nested;
						}
					}
					continue;
				}

				// Contenu de <defs> : definitions seulement, pas de rendu de shape.
				if (defsDepth > 0)
					continue;

				// ── <text> / <tspan> : le contenu VIT ENTRE LES BALISES ──────────
				//    Le lecteur de tags saute le texte ; ici on le prend a la source,
				//    depuis la position juste apres le « > » de la balise ouvrante.
				if (std::strcmp(tagBuf, "text") == 0 || std::strcmp(tagBuf, "tspan") == 0) {
					const bool estText = (tagBuf[0] == 't' && tagBuf[1] == 'e');
					ParseState loc = cur;
					loc.style = MergeStyle(cur.style, attrs, numAttrs);
					UpdateRefs(loc.fillRef, loc.strokeRef, attrs, numAttrs);
					const char *tr2 = FindAttr(attrs, numAttrs, "transform");
					if (tr2)
						loc.xform = cur.xform * NkSVGTransform::Parse(tr2);

					if (estText) {
						FinirTexte(shapes, texte, polices, skips); // un <text> non ferme
						texte = TextState();
						texte.actif = true;
						texte.x = ParseFloat(FindAttr(attrs, numAttrs, "x"));
						texte.y = ParseFloat(FindAttr(attrs, numAttrs, "y"));
						texte.xform = loc.xform;
						const char *anc = FindAttr(attrs, numAttrs, "text-anchor");
						if (anc && std::strcmp(anc, "middle") == 0)
							texte.anchor = 1;
						else if (anc && (std::strcmp(anc, "end") == 0))
							texte.anchor = 2;
						const char *esp = FindAttr(attrs, numAttrs, "xml:space");
						texte.preserve = (esp && std::strcmp(esp, "preserve") == 0);
						if (FindAttr(attrs, numAttrs, "dominant-baseline") && skips.Noter("dominant-baseline"))
							logger.Warn("[SVG] dominant-baseline non honore : y est la LIGNE DE BASE.");
						if (FindAttr(attrs, numAttrs, "textLength") && skips.Noter("textLength"))
							logger.Warn("[SVG] textLength non honore : le texte n'est ni etire ni comprime.");
					}
					if (!texte.actif) {
						// un <tspan> hors de tout <text> : rien a poser
						continue;
					}

					TextFragment fr;
					fr.style = loc.style;
					std::strncpy(fr.fillRef, loc.fillRef, 63);
					const char *fs = FindAttr(attrs, numAttrs, "font-size");
					if (!fs && estText)
						fs = nullptr;
					fr.fontSize = fs ? ParseFloat(fs)
									 : (texte.frags.IsEmpty() ? 16.f : texte.frags[texte.frags.Size() - 1].fontSize);
					const char *ff = FindAttr(attrs, numAttrs, "font-family");
					if (!ff && !texte.frags.IsEmpty())
						ff = texte.frags[texte.frags.Size() - 1].famille;
					if (ff) {
						std::strncpy(fr.famille, ff, sizeof(fr.famille) - 1);
						fr.famille[sizeof(fr.famille) - 1] = 0;
					}
					const char *fw = FindAttr(attrs, numAttrs, "font-weight");
					if (fw) {
						if (StrCaseCmp(fw, "bold") == 0)
							fr.weight = 700;
						else if (StrCaseCmp(fw, "normal") == 0)
							fr.weight = 400;
						else
							fr.weight = (int32)ParseFloat(fw);
					} else if (!texte.frags.IsEmpty()) {
						fr.weight = texte.frags[texte.frags.Size() - 1].weight;
					}
					if (!estText) { // un <tspan> peut repositionner le curseur
						const char *tx = FindAttr(attrs, numAttrs, "x");
						const char *ty = FindAttr(attrs, numAttrs, "y");
						if (tx) {
							fr.posX = true;
							fr.x = ParseFloat(tx);
						}
						if (ty) {
							fr.posY = true;
							fr.y = ParseFloat(ty);
						}
					}
					if (kind == 1) { // le contenu court jusqu'au prochain '<'
						const char *deb = xml;
						const char *fin = deb;
						while (fin < xmlEnd && *fin != '<')
							++fin;
						if (fin > deb)
							NettoyerTexte(deb, (usize)(fin - deb), texte.preserve, fr.txt, sizeof(fr.txt));
					}
					if (fr.txt[0])
						texte.frags.PushBack(fr);
					continue;
				}

				// Element shape : compose le state local = cur + attrs propres.
				if (FindAttr(attrs, numAttrs, "stroke-dasharray") && skips.Noter("stroke-dasharray"))
					logger.Warn("[SVG] stroke-dasharray non honore : le trait est rendu CONTINU.");

				ParseState local = cur;
				local.style = MergeStyle(cur.style, attrs, numAttrs);
				UpdateRefs(local.fillRef, local.strokeRef, attrs, numAttrs);
				const char *tr = FindAttr(attrs, numAttrs, "transform");
				if (tr)
					local.xform = cur.xform * NkSVGTransform::Parse(tr);

				const uint32 shapeBefore = shapes.Size();
				if (std::strcmp(tagBuf, "rect") == 0)
					ShapeFromRect(shapes, attrs, numAttrs, local);
				else if (std::strcmp(tagBuf, "circle") == 0)
					ShapeFromCircle(shapes, attrs, numAttrs, local);
				else if (std::strcmp(tagBuf, "ellipse") == 0)
					ShapeFromEllipse(shapes, attrs, numAttrs, local);
				else if (std::strcmp(tagBuf, "line") == 0)
					ShapeFromLine(shapes, attrs, numAttrs, local);
				else if (std::strcmp(tagBuf, "polyline") == 0)
					ShapeFromPolygon(shapes, attrs, numAttrs, local, false);
				else if (std::strcmp(tagBuf, "polygon") == 0)
					ShapeFromPolygon(shapes, attrs, numAttrs, local, true);
				else if (std::strcmp(tagBuf, "path") == 0)
					ShapeFromPath(shapes, attrs, numAttrs, local);
				else if (std::strcmp(tagBuf, "image") == 0)
					ShapeFromImage(shapes, attrs, numAttrs, local, baseDir, skips);
				else if (skips.Noter(tagBuf)) {
					// L'ELEMENT QU'ON NE SAIT PAS SE DIT, une fois par nom. Le silence
					// d'un decodeur est ce qui fait croire qu'un fichier est rendu.
					logger.Warn("[SVG] element non gere, saute : <{0}>", tagBuf);
				}

				// Propage le CTM + les refs de gradient aux shapes nouvellement creees.
				for (uint32 si = shapeBefore; si < shapes.Size(); ++si) {
					shapes[si].ctm = local.xform;
					std::strncpy(shapes[si].fillRef, local.fillRef, 63);
					shapes[si].fillRef[63] = 0;
					std::strncpy(shapes[si].strokeRef, local.strokeRef, 63);
					shapes[si].strokeRef[63] = 0;
				}
			}

			FinirTexte(shapes, texte, polices, skips); // un <text> jamais ferme se pose quand meme
			polices.Liberer();
			NkFree(nameBuf);
			NkFree(attrPool);
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// SECTION 9 — Rasterizer scanline AA (supersample 2x)
		// ─────────────────────────────────────────────────────────────────────────────

		struct ScanEdge {
				float32 x;
				int32 dir;
		};

		// ─────────────────────────────────────────────────────────────────────────────
		// LA PEINTURE D'UN DEGRADE — ON VA DU PIXEL VERS LE DEGRADE, jamais l'inverse
		// -----------------------------------------------------------------------------
		// L'ancienne version transportait la GEOMETRIE du degrade (deux points, ou un
		// centre et un rayon) vers l'espace des pixels. Ca marche tant que la matrice
		// ne fait que translater et mettre a l'echelle -- et ca ment des qu'elle
		// tourne ou cisaille : un cercle transporte par trois nombres redevient un
		// cercle, alors qu'il doit devenir une ELLIPSE. C'est aussi pour ca que
		// `gradientTransform` etait silencieusement IGNORE en objectBoundingBox.
		//
		// Ici on garde le degrade dans SON espace et on y ramene chaque pixel par une
		// matrice inverse. Consequences, toutes gratuites : gradientTransform vaut
		// dans les DEUX modes d'unites, une ellipse est une ellipse, et le foyer
		// (fx/fy) se pose sans cas particulier.
		// ─────────────────────────────────────────────────────────────────────────────
		struct GradPaint {
				GradKind kind = GradKind::Linear;
				GradSpread spread = GradSpread::Pad;
				const GradStop *stops = nullptr;
				int32 nStops = 0;
				NkSVGTransform inv;						  ///< pixel -> espace du degrade
				float32 x1 = 0, y1 = 0, x2 = 1, y2 = 0;	  // linear, dans SON espace
				float32 cx = 0, cy = 0, r = 1;			  // radial, dans SON espace
				float32 fx = 0, fy = 0;					  // le foyer
		};

		/// Lerp deux couleurs (composantes + alpha).
		NkSVGColor LerpStopColor(const NkSVGColor &a, const NkSVGColor &b, float32 f) noexcept {
			if (f < 0.f)
				f = 0.f;
			if (f > 1.f)
				f = 1.f;
			NkSVGColor c;
			c.r = (uint8)((float32)a.r + ((float32)b.r - (float32)a.r) * f);
			c.g = (uint8)((float32)a.g + ((float32)b.g - (float32)a.g) * f);
			c.b = (uint8)((float32)a.b + ((float32)b.b - (float32)a.b) * f);
			c.a = (uint8)((float32)a.a + ((float32)b.a - (float32)a.a) * f);
			c.none = false;
			return c;
		}

		/// Evalue la couleur du gradient au pixel (px,py).
		NkSVGColor EvalGrad(const GradPaint &gp, float32 px, float32 py) noexcept {
			if (gp.nStops <= 0)
				return NkSVGColor::Transparent();
			// le pixel, ramene dans l'espace ou le degrade est decrit
			gp.inv.Apply(px, py);
			float32 t;
			if (gp.kind == GradKind::Linear) {
				const float32 dx = gp.x2 - gp.x1, dy = gp.y2 - gp.y1;
				const float32 l2 = dx * dx + dy * dy;
				t = (l2 > 1e-9f) ? ((px - gp.x1) * dx + (py - gp.y1) * dy) / l2 : 0.f;
			} else if (gp.r <= 1e-6f) {
				t = 1.f;
			} else {
				// LE FOYER (SVG 1.1 §13.2.3) : le degrade court du foyer F vers le
				// cercle, le long du rayon qui passe par le pixel. On cherche le
				// scalaire k tel que F + k (P - F) touche le cercle ; la valeur du
				// degrade est alors 1/k. Foyer au centre -> on retombe exactement sur
				// la distance normalisee, sans cas particulier.
				const float32 dx = px - gp.fx, dy = py - gp.fy;
				const float32 fcx = gp.fx - gp.cx, fcy = gp.fy - gp.cy;
				const float32 a = dx * dx + dy * dy;
				if (a <= 1e-12f) {
					t = 0.f;
				} else {
					const float32 b = dx * fcx + dy * fcy;
					const float32 c = fcx * fcx + fcy * fcy - gp.r * gp.r;
					float32 disc = b * b - a * c;
					if (disc < 0.f)
						disc = 0.f;
					const float32 k = (-b + std::sqrt(disc)) / a;
					t = (k > 1e-6f) ? (1.f / k) : 1.f;
				}
			}
			// spreadMethod.
			if (gp.spread == GradSpread::Pad) {
				if (t < 0.f)
					t = 0.f;
				if (t > 1.f)
					t = 1.f;
			} else if (gp.spread == GradSpread::Repeat) {
				t = t - std::floor(t);
			} else { // Reflect
				float32 u = std::fabs(t);
				u = u - 2.f * std::floor(u * 0.5f);
				if (u > 1.f)
					u = 2.f - u;
				t = u;
			}
			// Echantillonne les stops (supposes tries par offset croissant).
			if (t <= gp.stops[0].offset)
				return gp.stops[0].color;
			if (t >= gp.stops[gp.nStops - 1].offset)
				return gp.stops[gp.nStops - 1].color;
			for (int32 i = 0; i < gp.nStops - 1; ++i) {
				const float32 o0 = gp.stops[i].offset;
				const float32 o1 = gp.stops[i + 1].offset;
				if (t <= o1) {
					const float32 seg = o1 - o0;
					const float32 f = (seg > 1e-6f) ? (t - o0) / seg : 0.f;
					return LerpStopColor(gp.stops[i].color, gp.stops[i + 1].color, f);
				}
			}
			return gp.stops[gp.nStops - 1].color;
		}

		/// Inverse d'une affine [a c e ; b d f]. @return false si degeneree (det ~ 0).
		bool Inverser(const NkSVGTransform &m, NkSVGTransform &out) noexcept {
			const float32 det = m.a * m.d - m.b * m.c;
			if (std::fabs(det) < 1e-12f)
				return false;
			const float32 inv = 1.f / det;
			out.a = m.d * inv;
			out.b = -m.b * inv;
			out.c = -m.c * inv;
			out.d = m.a * inv;
			out.e = (m.c * m.f - m.d * m.e) * inv;
			out.f = (m.b * m.e - m.a * m.f) * inv;
			return true;
		}

		/// Echantillon BILINEAIRE (bord repete) d'une image RGBA, en pixels source.
		void EchantillonBilineaire(const NkImage &src, float32 fx, float32 fy, float32 out[4]) noexcept {
			const int32 w = src.Width(), h = src.Height(), ch = src.Channels();
			const uint8 *px = src.Pixels();
			float32 sx = fx - 0.5f, sy = fy - 0.5f;
			int32 x0 = (int32)std::floor(sx), y0 = (int32)std::floor(sy);
			const float32 tx = sx - (float32)x0, ty = sy - (float32)y0;
			int32 x1 = x0 + 1, y1 = y0 + 1;
			auto borne = [](int32 v, int32 hi) { return v < 0 ? 0 : (v >= hi ? hi - 1 : v); };
			x0 = borne(x0, w);
			x1 = borne(x1, w);
			y0 = borne(y0, h);
			y1 = borne(y1, h);
			const int32 st = src.Stride();
			auto lire = [&](int32 x, int32 y, float32 c[4]) {
				const uint8 *p = px + (usize)y * (usize)st + (usize)x * (usize)ch;
				c[0] = (float32)p[0];
				c[1] = (float32)(ch > 1 ? p[1] : p[0]);
				c[2] = (float32)(ch > 2 ? p[2] : p[0]);
				c[3] = (float32)(ch > 3 ? p[3] : 255);
			};
			float32 c00[4], c10[4], c01[4], c11[4];
			lire(x0, y0, c00);
			lire(x1, y0, c10);
			lire(x0, y1, c01);
			lire(x1, y1, c11);
			for (int32 k = 0; k < 4; ++k) {
				const float32 h0 = c00[k] + (c10[k] - c00[k]) * tx;
				const float32 h1 = c01[k] + (c11[k] - c01[k]) * tx;
				out[k] = h0 + (h1 - h0) * ty;
			}
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// LA POSE D'UNE <image> : on balaie la BOITE TRANSFORMEE et, pour chaque pixel,
		// on revient dans l'espace utilisateur par l'INVERSE de la matrice totale. Un
		// aller (image -> ecran) aurait laisse des trous des que la matrice tourne ou
		// agrandit ; le retour n'en laisse aucun, et il donne la rotation gratuitement.
		// Les bords sont anticreneles par 2x2 sous-echantillons -- le seul endroit ou
		// une image posee de biais se juge.
		// ─────────────────────────────────────────────────────────────────────────────
		void RasterizeImage(NkImage &dst, const Shape &sh, const NkImage &src,
							const NkSVGTransform &totale) noexcept {
			if (!src.IsValid() || !sh.style.visible)
				return;
			NkSVGTransform inv;
			if (!Inverser(totale, inv))
				return;
			const float32 alphaGlobal = sh.style.opacity * sh.style.fillOpacity;
			if (alphaGlobal <= 0.f)
				return;

			const int32 W = dst.Width(), H = dst.Height();
			// bbox du quad deja transforme (les points de la shape sont en destination)
			float32 minx = 1e30f, miny = 1e30f, maxx = -1e30f, maxy = -1e30f;
			for (uint32 i = 0; i < sh.xs.Size(); ++i) {
				if (sh.xs[i] < minx) minx = sh.xs[i];
				if (sh.xs[i] > maxx) maxx = sh.xs[i];
				if (sh.ys[i] < miny) miny = sh.ys[i];
				if (sh.ys[i] > maxy) maxy = sh.ys[i];
			}
			int32 x0 = (int32)std::floor(minx), x1 = (int32)std::ceil(maxx);
			int32 y0 = (int32)std::floor(miny), y1 = (int32)std::ceil(maxy);
			if (x0 < 0) x0 = 0;
			if (y0 < 0) y0 = 0;
			if (x1 > W) x1 = W;
			if (y1 > H) y1 = H;
			if (x0 >= x1 || y0 >= y1)
				return;

			const float32 nw = (float32)src.Width(), nh = (float32)src.Height();
			if (nw <= 0.f || nh <= 0.f)
				return;
			// preserveAspectRatio : l'echelle image -> boite, et le centrage.
			float32 ech = 1.f, offx = 0.f, offy = 0.f, echY = 1.f;
			if (sh.fit == FitKind::None) {
				ech = sh.iw / nw;
				echY = sh.ih / nh;
			} else {
				const float32 sx = sh.iw / nw, sy = sh.ih / nh;
				ech = (sh.fit == FitKind::Meet) ? (sx < sy ? sx : sy) : (sx > sy ? sx : sy);
				echY = ech;
				offx = (sh.iw - nw * ech) * 0.5f; // xMidYMid : centre
				offy = (sh.ih - nh * echY) * 0.5f;
			}

			uint8 *pixels = dst.Pixels();
			const int32 stride = dst.Stride();
			for (int32 py = y0; py < y1; ++py) {
				for (int32 px = x0; px < x1; ++px) {
					// 2x2 sous-echantillons : couverture ET couleur moyennee
					float32 acc[4] = {0.f, 0.f, 0.f, 0.f};
					int32 dedans = 0;
					for (int32 sy2 = 0; sy2 < 2; ++sy2) {
						for (int32 sx2 = 0; sx2 < 2; ++sx2) {
							float32 ux = (float32)px + 0.25f + 0.5f * (float32)sx2;
							float32 uy = (float32)py + 0.25f + 0.5f * (float32)sy2;
							inv.Apply(ux, uy); // -> espace utilisateur
							const float32 bx = ux - sh.ix, by = uy - sh.iy;
							if (bx < 0.f || by < 0.f || bx > sh.iw || by > sh.ih)
								continue; // hors de la boite <image>
							const float32 fx = (bx - offx) / ech;
							const float32 fy = (by - offy) / echY;
							if (fx < 0.f || fy < 0.f || fx > nw || fy > nh)
								continue; // « meet » : la bande vide autour de l'image
							float32 c[4];
							EchantillonBilineaire(src, fx, fy, c);
							for (int32 k = 0; k < 4; ++k)
								acc[k] += c[k];
							++dedans;
						}
					}
					if (dedans == 0)
						continue;
					const float32 inv4 = 1.f / (float32)dedans;
					const float32 couverture = (float32)dedans * 0.25f;
					const float32 sa = (acc[3] * inv4 / 255.f) * alphaGlobal * couverture;
					if (sa <= 0.f)
						continue;
					const int32 isa = (int32)(sa * 255.f + 0.5f);
					if (isa <= 0)
						continue;
					const int32 invA = 255 - isa;
					uint8 *p = pixels + (usize)py * (usize)stride + (usize)px * 4u;
					for (int32 k = 0; k < 3; ++k) {
						const int32 sc = (int32)(acc[k] * inv4 + 0.5f);
						p[k] = (uint8)((sc * isa + (int32)p[k] * invA + 127) / 255);
					}
					const int32 a2 = (int32)p[3] + isa - ((int32)p[3] * isa + 127) / 255;
					p[3] = (uint8)(a2 > 255 ? 255 : (a2 < 0 ? 0 : a2));
				}
			}
		}

		void RasterizeShape(NkImage &img, const Shape &sh, const GradPaint *paint = nullptr) noexcept {
			if (sh.contourStart.IsEmpty())
				return;
			const NkSVGColor &fill = sh.style.fill;
			if (fill.none)
				return;
			if (!sh.style.visible)
				return;

			// Alpha effectif = fill.a * opacity * fill-opacity (range 0..1).
			// En mode gradient, l'alpha varie par pixel -> on ne pre-calcule que le facteur global.
			const float32 styleA = sh.style.opacity * sh.style.fillOpacity;
			const float32 alphaF = (float32)fill.a / 255.f * styleA;
			if (!paint && alphaF <= 0.f)
				return;

			const int32 W = img.Width();
			const int32 H = img.Height();
			if (W <= 0 || H <= 0)
				return;

			// BBox du shape pour limiter les scanlines.
			float32 minY = 1e30f, maxY = -1e30f;
			for (uint32 i = 0; i < sh.ys.Size(); ++i) {
				if (sh.ys[i] < minY)
					minY = sh.ys[i];
				if (sh.ys[i] > maxY)
					maxY = sh.ys[i];
			}
			int32 y0 = (int32)std::floor(minY);
			int32 y1 = (int32)std::ceil(maxY);
			if (y0 < 0)
				y0 = 0;
			if (y1 > H)
				y1 = H;
			if (y0 >= y1)
				return;

			// Supersample factor vertical pour AA edges.
			constexpr int32 kSS = 2;

			// Buffer de couverture par pixel (0..255) pour la ligne courante.
			uint8 *covRow = (uint8 *)NkAlloc((usize)W);
			if (!covRow)
				return;

			uint8 *pixels = img.Pixels();
			const int32 stride = img.Stride();
			const bool evenOdd = sh.style.fillEvenOdd;

			constexpr int32 kMaxEdges = 4096;
			ScanEdge edges[kMaxEdges];

			for (int32 py = y0; py < y1; ++py) {
				std::memset(covRow, 0, (usize)W);

				// Pour chaque sub-scanline (2x AA).
				for (int32 ss = 0; ss < kSS; ++ss) {
					const float32 ySub = (float32)py + ((float32)ss + 0.5f) / (float32)kSS;
					int32 nEdges = 0;
					// Collecte les intersections de tous les segments.
					for (uint32 ci = 0; ci < sh.contourStart.Size(); ++ci) {
						const int32 start = sh.contourStart[ci];
						const int32 len = sh.contourLen[ci];
						if (len < 2)
							continue;
						for (int32 k = 0; k < len - 1; ++k) {
							const float32 xA = sh.xs[start + k];
							const float32 yA = sh.ys[start + k];
							const float32 xB = sh.xs[start + k + 1];
							const float32 yB = sh.ys[start + k + 1];
							// Segment cross la sub-scanline ?
							if ((yA <= ySub && yB > ySub) || (yB <= ySub && yA > ySub)) {
								const float32 t = (ySub - yA) / (yB - yA);
								const float32 x = xA + (xB - xA) * t;
								if (nEdges < kMaxEdges) {
									edges[nEdges].x = x;
									edges[nEdges].dir = (yA <= ySub) ? +1 : -1;
									++nEdges;
								}
							}
						}
					}
					if (nEdges < 2)
						continue;
					// Tri par x croissant (insertion sort, nEdges typiquement petit).
					for (int32 i = 1; i < nEdges; ++i) {
						ScanEdge tmp = edges[i];
						int32 j = i - 1;
						while (j >= 0 && edges[j].x > tmp.x) {
							edges[j + 1] = edges[j];
							--j;
						}
						edges[j + 1] = tmp;
					}

					// Remplit selon fill-rule.
					if (evenOdd) {
						for (int32 i = 0; i + 1 < nEdges; i += 2) {
							float32 xL = edges[i].x;
							float32 xR = edges[i + 1].x;
							if (xL > xR)
								std::swap(xL, xR);
							int32 ixL = (int32)std::floor(xL);
							int32 ixR = (int32)std::ceil(xR);
							if (ixL < 0)
								ixL = 0;
							if (ixR > W)
								ixR = W;
							for (int32 x = ixL; x < ixR; ++x) {
								const float32 pL = (float32)x;
								const float32 pR = (float32)(x + 1);
								float32 cov = std::fmin(pR, xR) - std::fmax(pL, xL);
								if (cov <= 0.f)
									continue;
								if (cov > 1.f)
									cov = 1.f;
								const int32 add = (int32)(cov * (255.f / (float32)kSS));
								const int32 s = (int32)covRow[x] + add;
								covRow[x] = (uint8)(s > 255 ? 255 : s);
							}
						}
					} else {
						// Non-zero : winding counter.
						int32 winding = 0;
						float32 spanStart = 0.f;
						bool open = false;
						for (int32 i = 0; i < nEdges; ++i) {
							const int32 prev = winding;
							winding += edges[i].dir;
							if (prev == 0 && winding != 0) {
								spanStart = edges[i].x;
								open = true;
							} else if (prev != 0 && winding == 0 && open) {
								float32 xL = spanStart;
								float32 xR = edges[i].x;
								if (xL > xR)
									std::swap(xL, xR);
								int32 ixL = (int32)std::floor(xL);
								int32 ixR = (int32)std::ceil(xR);
								if (ixL < 0)
									ixL = 0;
								if (ixR > W)
									ixR = W;
								for (int32 x = ixL; x < ixR; ++x) {
									const float32 pL = (float32)x;
									const float32 pR = (float32)(x + 1);
									float32 cov = std::fmin(pR, xR) - std::fmax(pL, xL);
									if (cov <= 0.f)
										continue;
									if (cov > 1.f)
										cov = 1.f;
									const int32 add = (int32)(cov * (255.f / (float32)kSS));
									const int32 s = (int32)covRow[x] + add;
									covRow[x] = (uint8)(s > 255 ? 255 : s);
								}
								open = false;
							}
						}
					}
				}

				// Blend la row dans l'image (premultiplied alpha src-over).
				uint8 *dst = pixels + (usize)py * (usize)stride;
				for (int32 x = 0; x < W; ++x) {
					const uint8 c = covRow[x];
					if (c == 0)
						continue;
					// Couleur source : solide ou echantillonnee dans le gradient.
					int32 srcR, srcG, srcB;
					float32 baseA;
					if (paint) {
						const NkSVGColor gc = EvalGrad(*paint, (float32)x + 0.5f, (float32)py + 0.5f);
						srcR = gc.r;
						srcG = gc.g;
						srcB = gc.b;
						baseA = (float32)gc.a / 255.f * styleA;
					} else {
						srcR = fill.r;
						srcG = fill.g;
						srcB = fill.b;
						baseA = alphaF;
					}
					const float32 alpha = baseA * (float32)c / 255.f;
					const int32 sa = (int32)(alpha * 255.f);
					if (sa <= 0)
						continue;
					const int32 invA = 255 - sa;
					uint8 *px = dst + (usize)x * 4;
					px[0] = (uint8)((srcR * sa + (int32)px[0] * invA + 127) / 255);
					px[1] = (uint8)((srcG * sa + (int32)px[1] * invA + 127) / 255);
					px[2] = (uint8)((srcB * sa + (int32)px[2] * invA + 127) / 255);
					const int32 a2 = (int32)px[3] + sa - ((int32)px[3] * sa + 127) / 255;
					px[3] = (uint8)(a2 > 255 ? 255 : (a2 < 0 ? 0 : a2));
				}
			}

			NkFree(covRow);
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// Pousse un polygone dans la Shape destination en FORCANT l'orientation CCW puis
		// en fermant le contour. Indispensable : le stroke est rasterise en union de
		// multiples sous-polygones (quads de segment + disques/triangles de jointure) ;
		// si certains etaient CW et d'autres CCW, la regle nonzero les annulerait
		// (winding +1/-1 = 0 -> trous). Tout CCW -> winding >= 1 partout -> union pleine.
		// ─────────────────────────────────────────────────────────────────────────────
		void PushPolyCCW(Shape &dst, const float32 *px, const float32 *py, int32 n) noexcept {
			if (n < 3)
				return;
			float32 area = 0.f;
			for (int32 i = 0; i < n; ++i) {
				const int32 j = (i + 1) % n;
				area += px[i] * py[j] - px[j] * py[i];
			}
			const bool ccw = area > 0.f;
			const int32 cStart = (int32)dst.xs.Size();
			if (ccw) {
				for (int32 i = 0; i < n; ++i) {
					dst.xs.PushBack(px[i]);
					dst.ys.PushBack(py[i]);
				}
			} else {
				for (int32 i = n - 1; i >= 0; --i) {
					dst.xs.PushBack(px[i]);
					dst.ys.PushBack(py[i]);
				}
			}
			// Ferme (re-pousse le premier point pousse).
			dst.xs.PushBack(dst.xs[(uint32)cStart]);
			dst.ys.PushBack(dst.ys[(uint32)cStart]);
			dst.contourStart.PushBack(cStart);
			dst.contourLen.PushBack((int32)dst.xs.Size() - cStart);
		}

		/// Pousse un disque approxime (20 cotes) — pour cap/join "round".
		void PushDisc(Shape &dst, float32 cx, float32 cy, float32 r) noexcept {
			if (r <= 0.f)
				return;
			constexpr int32 N = 20;
			float32 px[N], py[N];
			for (int32 i = 0; i < N; ++i) {
				const float32 a = (float32)i * (6.283185307179586f / (float32)N);
				px[i] = cx + r * std::cos(a);
				py[i] = cy + r * std::sin(a);
			}
			PushPolyCCW(dst, px, py, N);
		}

		// ─────────────────────────────────────────────────────────────────────────────
		// Construit une "shape ruban" autour du chemin source pour rasteriser le stroke.
		// Honore stroke-linecap (butt/round/square) et stroke-linejoin (miter/round/bevel)
		// + stroke-miterlimit. Approche par tampons : un quad par segment, une jointure
		// par vertex interieur, un cap par extremite ouverte. Tout passe par PushPolyCCW
		// (union nonzero correcte). Le resultat est rempli par RasterizeShape avec la
		// couleur stroke en "fill".
		// ─────────────────────────────────────────────────────────────────────────────
		void BuildStrokeShape(const Shape &src, float32 hwPx, NkSVGLineCap cap, NkSVGLineJoin join, float32 miterLimit,
							  Shape &dst) noexcept {
			if (hwPx <= 0.f)
				return;
			if (miterLimit < 1.f)
				miterLimit = 1.f;

			for (uint32 ci = 0; ci < src.contourStart.Size(); ++ci) {
				const int32 start = src.contourStart[ci];
				const int32 len = src.contourLen[ci];
				if (len < 2)
					continue;

				const bool closed = (std::fabs(src.xs[start] - src.xs[start + len - 1]) < 1e-4f &&
									 std::fabs(src.ys[start] - src.ys[start + len - 1]) < 1e-4f);
				const int32 nPts = closed ? (len - 1) : len;
				if (nPts < 2)
					continue;

				auto PX = [&](int32 i) { return src.xs[start + i]; };
				auto PY = [&](int32 i) { return src.ys[start + i]; };

				const int32 segCount = closed ? nPts : (nPts - 1);

				// ── Un quad par segment ───────────────────────────────────────────────
				for (int32 s = 0; s < segCount; ++s) {
					const int32 i0 = s;
					const int32 i1 = (s + 1) % nPts;
					float32 ax = PX(i0), ay = PY(i0);
					float32 bx = PX(i1), by = PY(i1);
					float32 dx = bx - ax, dy = by - ay;
					const float32 l = std::sqrt(dx * dx + dy * dy);
					if (l < 1e-6f)
						continue;
					dx /= l;
					dy /= l;
					const float32 nx = -dy * hwPx, ny = dx * hwPx; // perpendiculaire * demi-largeur

					// Cap "square" : etend le segment de hwPx aux extremites ouvertes.
					if (!closed && cap == NkSVGLineCap::Square) {
						if (s == 0) {
							ax -= dx * hwPx;
							ay -= dy * hwPx;
						}
						if (s == segCount - 1) {
							bx += dx * hwPx;
							by += dy * hwPx;
						}
					}
					const float32 qx[4] = {ax + nx, bx + nx, bx - nx, ax - nx};
					const float32 qy[4] = {ay + ny, by + ny, by - ny, ay - ny};
					PushPolyCCW(dst, qx, qy, 4);
				}

				// ── Jointures aux vertices interieurs (et tous pour un contour ferme) ──
				const int32 jStart = closed ? 0 : 1;
				const int32 jEnd = closed ? nPts : (nPts - 1);
				for (int32 v = jStart; v < jEnd; ++v) {
					const int32 prev = (v - 1 + nPts) % nPts;
					const int32 next = (v + 1) % nPts;
					float32 idx = PX(v) - PX(prev), idy = PY(v) - PY(prev);
					float32 odx = PX(next) - PX(v), ody = PY(next) - PY(v);
					const float32 li = std::sqrt(idx * idx + idy * idy);
					const float32 lo = std::sqrt(odx * odx + ody * ody);
					if (li < 1e-6f || lo < 1e-6f)
						continue;
					idx /= li;
					idy /= li;
					odx /= lo;
					ody /= lo;
					const float32 vx = PX(v), vy = PY(v);

					if (join == NkSVGLineJoin::Round) {
						PushDisc(dst, vx, vy, hwPx);
						continue;
					}
					// Perpendiculaires (unitaires) des deux segments.
					const float32 niX = -idy, niY = idx; // perp entrant
					const float32 noX = -ody, noY = odx; // perp sortant

					bool doMiter = (join == NkSVGLineJoin::Miter);
					float32 apX = 0.f, apY = 0.f, amX = 0.f, amY = 0.f;
					if (doMiter) {
						// Bisectrice des perpendiculaires -> direction du miter.
						float32 bx = niX + noX, by = niY + noY;
						const float32 bl = std::sqrt(bx * bx + by * by);
						if (bl < 1e-4f) {
							doMiter = false; // U-turn : pas de miter, bevel.
						} else {
							bx /= bl;
							by /= bl;
							const float32 cosHalf = bx * niX + by * niY; // dot(bisect, perpIn)
							if (cosHalf < 1e-4f) {
								doMiter = false;
							} else {
								const float32 ratio = 1.f / cosHalf; // miterLength / hw
								if (ratio > miterLimit) {
									doMiter = false; // depasse la limite -> bevel
								} else {
									const float32 mlen = hwPx * ratio;
									apX = vx + bx * mlen;
									apY = vy + by * mlen;
									amX = vx - bx * mlen;
									amY = vy - by * mlen;
								}
							}
						}
					}

					if (doMiter) {
						// Triangles de miter des deux cotes (l'interieur est couvert par les quads).
						const float32 tpX[3] = {vx + niX * hwPx, apX, vx + noX * hwPx};
						const float32 tpY[3] = {vy + niY * hwPx, apY, vy + noY * hwPx};
						PushPolyCCW(dst, tpX, tpY, 3);
						const float32 tmX[3] = {vx - niX * hwPx, amX, vx - noX * hwPx};
						const float32 tmY[3] = {vy - niY * hwPx, amY, vy - noY * hwPx};
						PushPolyCCW(dst, tmX, tmY, 3);
					} else {
						// Bevel : comble le coin par un triangle de chaque cote vers le vertex.
						const float32 tpX[3] = {vx + niX * hwPx, vx + noX * hwPx, vx};
						const float32 tpY[3] = {vy + niY * hwPx, vy + noY * hwPx, vy};
						PushPolyCCW(dst, tpX, tpY, 3);
						const float32 tmX[3] = {vx - niX * hwPx, vx - noX * hwPx, vx};
						const float32 tmY[3] = {vy - niY * hwPx, vy - noY * hwPx, vy};
						PushPolyCCW(dst, tmX, tmY, 3);
					}
				}

				// ── Caps "round" aux extremites ouvertes ──────────────────────────────
				if (!closed && cap == NkSVGLineCap::Round) {
					PushDisc(dst, PX(0), PY(0), hwPx);
					PushDisc(dst, PX(nPts - 1), PY(nPts - 1), hwPx);
				}
			}
		}

		/// Donnees opaques d'un NkSVGImage : shapes vectorielles + dimensions natives.
		/// Pointe par NkSVGImage::mImpl (PIMPL).
		struct SVGImageImpl {
				NkVector<Shape> shapes;
				NkVector<Gradient> gradients;
				float32 vbX = 0, vbY = 0, vbW = 0, vbH = 0;
				float32 svgW = 0, svgH = 0;
				SkipList skips; ///< ce que le decodage a saute (dit une fois par nom)
		};

		/// Trouve un gradient par id (recherche lineaire). nullptr si absent.
		const Gradient *FindGradient(const SVGImageImpl *impl, const char *id) noexcept {
			if (!impl || !id || !id[0])
				return nullptr;
			for (uint32 i = 0; i < impl->gradients.Size(); ++i) {
				if (std::strcmp(impl->gradients[i].id, id) == 0)
					return &impl->gradients[i];
			}
			return nullptr;
		}

		/// Construit une GradPaint (geometrie en pixels destination) pour un ref donne.
		/// @p localPts = shape deja transformee (pour le bbox objectBoundingBox).
		/// @p ctm/@p mView = transforms pour le mode userSpaceOnUse.
		/// @return false si le gradient/les stops sont introuvables.
		bool BuildGradPaint(const SVGImageImpl *impl, const char *ref, const Shape &localPts, const NkSVGTransform &ctm,
							const NkSVGTransform &mView, GradPaint &gp) noexcept {
			const Gradient *g = FindGradient(impl, ref);
			if (!g)
				return false;

			// ── L'HERITAGE PAR href ──────────────────────────────────────────
			// Un gradient qui en reference un autre herite TOUT ce qu'il n'a pas dit
			// lui-meme : ses arrets, mais aussi sa geometrie, ses unites, sa matrice
			// et son etalement. N'heriter que les arrets (ce qu'on faisait) donne des
			// degrades geometriquement FAUX des qu'un fichier factorise ses
			// definitions -- une pratique courante des exporteurs.
			const Gradient *base = g->href[0] ? FindGradient(impl, g->href) : nullptr;
			const Gradient *sg = g;
			if (g->stops.IsEmpty() && base)
				sg = base;
			if (sg->stops.IsEmpty())
				return false;
			gp.stops = &sg->stops[0];
			gp.nStops = (int32)sg->stops.Size();
			gp.kind = g->kind;
			gp.spread = (g->hasSpread || !base) ? g->spread : base->spread;
			const bool userSpace = (g->hasUnits || !base) ? g->userSpace : base->userSpace;
			const NkSVGTransform gxf = (g->hasXform || !base) ? g->xform : base->xform;
			const float32 gx1 = (g->hasX1 || !base) ? g->x1 : base->x1;
			const float32 gy1 = (g->hasY1 || !base) ? g->y1 : base->y1;
			const float32 gx2 = (g->hasX2 || !base) ? g->x2 : base->x2;
			const float32 gy2 = (g->hasY2 || !base) ? g->y2 : base->y2;
			const float32 gcx = (g->hasCx || !base) ? g->cx : base->cx;
			const float32 gcy = (g->hasCy || !base) ? g->cy : base->cy;
			const float32 gr = (g->hasR || !base) ? g->r : base->r;
			const bool focal = g->hasFocal || (base && base->hasFocal);
			const float32 gfx = g->hasFocal ? g->fx : (base && base->hasFocal ? base->fx : gcx);
			const float32 gfy = g->hasFocal ? g->fy : (base && base->hasFocal ? base->fy : gcy);

			gp.x1 = gx1;
			gp.y1 = gy1;
			gp.x2 = gx2;
			gp.y2 = gy2;
			gp.cx = gcx;
			gp.cy = gcy;
			gp.r = gr;
			gp.fx = focal ? gfx : gcx;
			gp.fy = focal ? gfy : gcy;
			// LE FOYER DOIT RESTER DANS LE CERCLE (la norme le ramene sur le bord
			// sinon) : au-dehors, le rayon ne coupe plus le cercle et la couleur
			// partirait a l'infini.
			{
				const float32 ddx = gp.fx - gp.cx, ddy = gp.fy - gp.cy;
				const float32 d2 = ddx * ddx + ddy * ddy;
				if (gp.r > 1e-6f && d2 > gp.r * gp.r * 0.9801f) { // 0.99 r
					const float32 d = std::sqrt(d2);
					const float32 k = (gp.r * 0.99f) / d;
					gp.fx = gp.cx + ddx * k;
					gp.fy = gp.cy + ddy * k;
				}
			}

			// LA MATRICE : espace du degrade -> pixels. On l'inverse une fois par
			// forme, pas une fois par pixel.
			NkSVGTransform versPixels;
			if (!userSpace) {
				// objectBoundingBox : le degrade est decrit dans le carre unite de la
				// BOITE de la forme. bbox <- points DEJA transformes (destination).
				float32 minx = 1e30f, miny = 1e30f, maxx = -1e30f, maxy = -1e30f;
				for (uint32 k = 0; k < localPts.xs.Size(); ++k) {
					const float32 x = localPts.xs[k], y = localPts.ys[k];
					if (x < minx)
						minx = x;
					if (x > maxx)
						maxx = x;
					if (y < miny)
						miny = y;
					if (y > maxy)
						maxy = y;
				}
				float32 w = maxx - minx, h = maxy - miny;
				if (w <= 0.f)
					w = 1.f;
				if (h <= 0.f)
					h = 1.f;
				// unite -> boite, PUIS gradientTransform (qui s'applique dans l'espace
				// du degrade) : c'est cette composition qui manquait.
				versPixels = NkSVGTransform::Translate(minx, miny) * NkSVGTransform::Scale(w, h) * gxf;
			} else {
				// userSpaceOnUse : user -> dest, gradientTransform comprise.
				versPixels = mView * (ctm * gxf);
			}
			if (!Inverser(versPixels, gp.inv))
				return false;
			return true;
		}

	} // anonymous namespace

	// ═════════════════════════════════════════════════════════════════════════════
	// SECTION 9.5 — Triangulation : ear-clipping pour acces mesh 3D / collision
	// ═════════════════════════════════════════════════════════════════════════════

	namespace {

		/// Signed area (algorithm shoelace). Positif = CCW, negatif = CW.
		float32 SignedArea(const float32 *xs, const float32 *ys, int32 n) noexcept {
			float32 s = 0.f;
			for (int32 i = 0; i < n; ++i) {
				const int32 j = (i + 1) % n;
				s += xs[i] * ys[j] - xs[j] * ys[i];
			}
			return s * 0.5f;
		}

		/// Test point-in-triangle via signes des aires (barycentric-like).
		bool PointInTriangle(float32 px, float32 py, float32 ax, float32 ay, float32 bx, float32 by, float32 cx,
							 float32 cy) noexcept {
			const float32 d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
			const float32 d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
			const float32 d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);
			const bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
			const bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
			return !(hasNeg && hasPos);
		}

		/// Triangule un contour simple par ear-clipping. Output : indices triangle-list
		/// dans le repere original (les indices pointent dans @p xs / @p ys, decales
		/// par @p baseIndex). @return nombre de triangles produits.
		/// Algo O(N²) -- OK pour contours < ~500 vertices.
		int32 EarClipContour(const float32 *xs, const float32 *ys, int32 n, uint32 baseIndex,
							 NkVector<uint32> &outIndices) noexcept {
			if (n < 3)
				return 0;
			// Detection orientation. Si CW, on inverse l'ordre (ear-clipping veut CCW).
			const bool isCCW = (SignedArea(xs, ys, n) > 0.f);

			// Liste de vertices restants (indices dans xs/ys).
			NkVector<int32> verts;
			verts.Reserve((usize)n);
			if (isCCW) {
				for (int32 i = 0; i < n; ++i)
					verts.PushBack(i);
			} else {
				for (int32 i = n - 1; i >= 0; --i)
					verts.PushBack(i);
			}

			int32 triCount = 0;
			int32 guard = 2 * n; // protection anti-boucle infinie (cas degenere)
			while (verts.Size() >= 3 && guard-- > 0) {
				const int32 nv = (int32)verts.Size();
				bool earFound = false;
				for (int32 i = 0; i < nv; ++i) {
					const int32 i0 = verts[(i - 1 + nv) % nv];
					const int32 i1 = verts[i];
					const int32 i2 = verts[(i + 1) % nv];
					const float32 ax = xs[i0], ay = ys[i0];
					const float32 bx = xs[i1], by = ys[i1];
					const float32 cx = xs[i2], cy = ys[i2];
					// Convexite (cross product positif en CCW).
					const float32 cross = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
					if (cross <= 0.f)
						continue;
					// Aucun autre vertex du polygone a l'interieur du triangle ?
					bool inside = false;
					for (int32 k = 0; k < nv && !inside; ++k) {
						const int32 ki = verts[k];
						if (ki == i0 || ki == i1 || ki == i2)
							continue;
						if (PointInTriangle(xs[ki], ys[ki], ax, ay, bx, by, cx, cy)) {
							inside = true;
						}
					}
					if (inside)
						continue;
					// Ear trouve : output triangle + retire i1.
					outIndices.PushBack(baseIndex + (uint32)i0);
					outIndices.PushBack(baseIndex + (uint32)i1);
					outIndices.PushBack(baseIndex + (uint32)i2);
					++triCount;
					verts.RemoveAt((uint32)i);
					earFound = true;
					break;
				}
				if (!earFound)
					break; // Polygone degenere ou non-simple
			}
			return triCount;
		}

		/// Pack RGBA8 -> uint32 (little-endian : R=byte0, G=byte1, B=byte2, A=byte3).
		uint32 PackRGBA(uint8 r, uint8 g, uint8 b, uint8 a) noexcept {
			return ((uint32)r) | ((uint32)g << 8) | ((uint32)b << 16) | ((uint32)a << 24);
		}

	} // anonymous namespace

	// ═════════════════════════════════════════════════════════════════════════════
	// SECTION 9.6 — NkSVGShapeView : implementation
	// ═════════════════════════════════════════════════════════════════════════════

	int32 NkSVGShapeView::ContourCount() const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		return sh ? (int32)sh->contourStart.Size() : 0;
	}

	int32 NkSVGShapeView::ContourPointCount(int32 ci) const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		if (!sh || ci < 0 || (uint32)ci >= sh->contourLen.Size())
			return 0;
		return sh->contourLen[(uint32)ci];
	}

	const float32 *NkSVGShapeView::ContourXs(int32 ci) const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		if (!sh || ci < 0 || (uint32)ci >= sh->contourStart.Size())
			return nullptr;
		return &sh->xs[(uint32)sh->contourStart[(uint32)ci]];
	}

	const float32 *NkSVGShapeView::ContourYs(int32 ci) const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		if (!sh || ci < 0 || (uint32)ci >= sh->contourStart.Size())
			return nullptr;
		return &sh->ys[(uint32)sh->contourStart[(uint32)ci]];
	}

	NkSVGColor NkSVGShapeView::FillColor() const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		return sh ? sh->style.fill : NkSVGColor::Black();
	}

	NkSVGColor NkSVGShapeView::StrokeColor() const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		return sh ? sh->style.stroke : NkSVGColor::None();
	}

	float32 NkSVGShapeView::StrokeWidth() const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		return sh ? sh->style.strokeWidth : 1.f;
	}

	float32 NkSVGShapeView::Opacity() const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		return sh ? sh->style.opacity : 1.f;
	}

	bool NkSVGShapeView::FillEvenOdd() const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		return sh && sh->style.fillEvenOdd;
	}

	int32 NkSVGShapeView::Triangulate(NkVector<float32> &outXs, NkVector<float32> &outYs, NkVector<uint32> &outIndices,
									  uint32 baseIndex) const noexcept {
		const Shape *sh = (const Shape *)mShapePtr;
		if (!sh)
			return 0;

		// Si baseIndex == 0 et que les buffers de sortie sont vides, on peut
		// commencer a 0. Sinon on utilise baseIndex pour chainer (appel par
		// TriangulateAll en boucle sur les shapes).
		uint32 curBase = baseIndex;
		int32 totalTris = 0;
		for (uint32 ci = 0; ci < sh->contourStart.Size(); ++ci) {
			const int32 start = sh->contourStart[ci];
			const int32 len = sh->contourLen[ci];
			if (len < 3)
				continue;
			// Le contour peut etre ferme (dernier point == premier). On detecte
			// et on l'ignore pour l'ear clipping.
			const float32 fx = sh->xs[(uint32)start];
			const float32 fy = sh->ys[(uint32)start];
			const float32 lx = sh->xs[(uint32)(start + len - 1)];
			const float32 ly = sh->ys[(uint32)(start + len - 1)];
			int32 n = len;
			if (std::fabs(fx - lx) < 1e-5f && std::fabs(fy - ly) < 1e-5f) {
				n = len - 1;
			}
			if (n < 3)
				continue;

			// Copie locale des vertices pour ear-clipping (puis push dans outXs/Ys).
			for (int32 k = 0; k < n; ++k) {
				outXs.PushBack(sh->xs[(uint32)(start + k)]);
				outYs.PushBack(sh->ys[(uint32)(start + k)]);
			}
			const uint32 contourBase = curBase;
			totalTris += EarClipContour(&sh->xs[(uint32)start], &sh->ys[(uint32)start], n, contourBase, outIndices);
			curBase += (uint32)n;
		}
		return totalTris;
	}

	// ═════════════════════════════════════════════════════════════════════════════
	// SECTION 10 — NkSVGImage API publique : stockage vectoriel + rasterize ondemand
	// ═════════════════════════════════════════════════════════════════════════════

	NkSVGImage *NkSVGImage::LoadFromMemory(const uint8 *data, usize size) noexcept {
		return LoadFromMemory(data, size, nullptr);
	}

	NkSVGImage *NkSVGImage::LoadFromMemory(const uint8 *data, usize size, const char *baseDir) noexcept {
		if (!data || size < 5)
			return nullptr;
		const char *xml = reinterpret_cast<const char *>(data);

		// Alloc impl + parse.
		SVGImageImpl *impl = (SVGImageImpl *)NkAlloc(sizeof(SVGImageImpl));
		if (!impl)
			return nullptr;
		new (impl) SVGImageImpl();
		ParseSVGDocument(xml, size, impl->shapes, impl->gradients, impl->vbX, impl->vbY, impl->vbW, impl->vbH,
						 impl->svgW, impl->svgH, impl->skips, baseDir);

		// Si parsing a echoue (aucun shape ET aucune viewBox), on libere.
		if (impl->shapes.IsEmpty() && impl->vbW <= 0.f && impl->svgW <= 0.f) {
			impl->~SVGImageImpl();
			NkFree(impl);
			return nullptr;
		}

		// Defauts raisonnables si l'auteur du SVG a omis viewBox ou width/height.
		if (impl->vbW <= 0.f)
			impl->vbW = (impl->svgW > 0.f) ? impl->svgW : 800.f;
		if (impl->vbH <= 0.f)
			impl->vbH = (impl->svgH > 0.f) ? impl->svgH : 600.f;
		if (impl->svgW <= 0.f)
			impl->svgW = impl->vbW;
		if (impl->svgH <= 0.f)
			impl->svgH = impl->vbH;

		// Alloc l'image vectorielle (objet + impl).
		NkSVGImage *svg = (NkSVGImage *)NkAlloc(sizeof(NkSVGImage));
		if (!svg) {
			impl->~SVGImageImpl();
			NkFree(impl);
			return nullptr;
		}
		new (svg) NkSVGImage();
		svg->mImpl = impl;
		return svg;
	}

	NkSVGImage *NkSVGImage::LoadFromFile(const char *path) noexcept {
		if (!path)
			return nullptr;
		NkFile f;
		if (!f.Open(path, NkFileMode::NK_READ))
			return nullptr;
		const usize sz = (usize)f.Size();
		if (sz == 0) {
			f.Close();
			return nullptr;
		}
		uint8 *buf = (uint8 *)NkAlloc(sz + 1);
		if (!buf) {
			f.Close();
			return nullptr;
		}
		const usize read = f.Read(buf, sz);
		buf[read] = 0;
		f.Close();
		// LE DOSSIER DU FICHIER : un href relatif se resout par rapport au .svg qui
		// le porte, jamais par rapport au repertoire courant du processus.
		char base[1024];
		std::strncpy(base, path, sizeof(base) - 1);
		base[sizeof(base) - 1] = 0;
		usize coupe = 0;
		for (usize i = 0; base[i]; ++i)
			if (base[i] == '/' || base[i] == '\\')
				coupe = i;
		base[coupe] = 0; // "" si le chemin n'a pas de dossier : le courant, et c'est juste
		NkSVGImage *svg = LoadFromMemory(buf, read, base);
		NkFree(buf);
		return svg;
	}

	NkImage NkSVGImage::Rasterize(int32 outW, int32 outH) const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		if (!impl)
			return NkImage();

		// Resolution de la taille de sortie.
		if (outW <= 0 && outH <= 0) {
			outW = (int32)impl->svgW;
			outH = (int32)impl->svgH;
		} else if (outW <= 0) {
			outW = (int32)((float32)outH * impl->vbW / impl->vbH);
		} else if (outH <= 0) {
			outH = (int32)((float32)outW * impl->vbH / impl->vbW);
		}
		if (outW <= 0)
			outW = 1;
		if (outH <= 0)
			outH = 1;

		// Mapping viewBox -> taille de sortie.
		const NkSVGTransform mView = NkSVGTransform::Scale((float32)outW / impl->vbW, (float32)outH / impl->vbH) *
									 NkSVGTransform::Translate(-impl->vbX, -impl->vbY);

		// Alloc image RGBA32 (zero-initialise = fond transparent).
		NkImage img = NkImage::Alloc(outW, outH, NkImagePixelFormat::NK_RGBA32);
		if (!img.IsValid())
			return NkImage();

		// Copie locale des shapes + applique le mapping a leurs points, puis
		// rasterise. On NE modifie PAS l'impl source (pour que Rasterize() puisse
		// etre appele plusieurs fois avec des tailles differentes).
		for (uint32 i = 0; i < impl->shapes.Size(); ++i) {
			const Shape &src = impl->shapes[i];
			// ── UNE IMAGE SE POSE (pas de remplissage, pas de trait) ───────────
			if (src.EstImage()) {
				Shape poseur;
				poseur.style = src.style;
				poseur.ix = src.ix;
				poseur.iy = src.iy;
				poseur.iw = src.iw;
				poseur.ih = src.ih;
				poseur.fit = src.fit;
				for (uint32 k = 0; k < src.xs.Size(); ++k) {
					float32 x = src.xs[k], y = src.ys[k];
					mView.Apply(x, y);
					poseur.xs.PushBack(x);
					poseur.ys.PushBack(y);
				}
				// les pixels sont REFERENCES, jamais copies ni deplaces : Rasterize()
				// reste const et rejouable a plusieurs tailles, comme son contrat le dit.
				RasterizeImage(img, poseur, src.img, mView * src.ctm);
				continue;
			}
			// Construit un Shape transforme localement (sans toucher la source).
			Shape local;
			local.style = src.style;
			local.xs.Reserve(src.xs.Size());
			local.ys.Reserve(src.ys.Size());
			for (uint32 k = 0; k < src.xs.Size(); ++k) {
				float32 x = src.xs[k], y = src.ys[k];
				mView.Apply(x, y);
				local.xs.PushBack(x);
				local.ys.PushBack(y);
			}
			for (uint32 k = 0; k < src.contourStart.Size(); ++k) {
				local.contourStart.PushBack(src.contourStart[k]);
				local.contourLen.PushBack(src.contourLen[k]);
			}
			// ── Rasterise le fill (couleur unie OU gradient) ───────────────────
			GradPaint fillGP;
			bool hasFillGP = false;
			if (src.fillRef[0])
				hasFillGP = BuildGradPaint(impl, src.fillRef, local, src.ctm, mView, fillGP);
			const bool fillRefUnresolved = (src.fillRef[0] && !hasFillGP);
			if (!fillRefUnresolved)
				RasterizeShape(img, local, hasFillGP ? &fillGP : nullptr);

			// ── Rasterise le stroke si present ─────────────────────────────────
			// On construit une nouvelle Shape "ruban" autour des contours et on
			// la rasterise comme un fill avec la couleur stroke. strokeWidth est
			// en unites SVG (avant view->out scale) : on le convertit en pixels
			// via la moyenne des facteurs du mView (mView est diagonal en pratique).
			const bool hasStroke = (!src.style.stroke.none || src.strokeRef[0]) && src.style.strokeWidth > 0.f;
			if (hasStroke) {
				const float32 sx = std::sqrt(mView.a * mView.a + mView.b * mView.b);
				const float32 sy = std::sqrt(mView.c * mView.c + mView.d * mView.d);
				const float32 swPx = src.style.strokeWidth * ((sx + sy) * 0.5f);
				if (swPx >= 0.1f) {
					Shape strokeShape;
					strokeShape.style = src.style;
					// RasterizeShape regarde "fill" -> on copie stroke dedans.
					strokeShape.style.fill = src.style.stroke;
					strokeShape.style.fillOpacity = src.style.strokeOpacity;
					strokeShape.style.fillEvenOdd = false;
					BuildStrokeShape(local, swPx * 0.5f, src.style.strokeLineCap, src.style.strokeLineJoin,
									 src.style.strokeMiterLimit, strokeShape);
					GradPaint strokeGP;
					bool hasStrokeGP = false;
					if (src.strokeRef[0])
						hasStrokeGP = BuildGradPaint(impl, src.strokeRef, strokeShape, src.ctm, mView, strokeGP);
					const bool strokeRefUnresolved = (src.strokeRef[0] && !hasStrokeGP);
					if (strokeShape.contourStart.Size() > 0 && !strokeRefUnresolved) {
						RasterizeShape(img, strokeShape, hasStrokeGP ? &strokeGP : nullptr);
					}
				}
			}
		}
		return img;
	}

	int32 NkSVGImage::NaturalWidth() const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		return impl ? (int32)impl->svgW : 0;
	}

	int32 NkSVGImage::NaturalHeight() const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		return impl ? (int32)impl->svgH : 0;
	}

	int32 NkSVGImage::SkippedCount() const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		return impl ? impl->skips.nb : 0;
	}

	const char *NkSVGImage::SkippedAt(int32 idx) const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		if (!impl || idx < 0 || idx >= impl->skips.nb)
			return nullptr;
		return impl->skips.noms[idx];
	}

	int32 NkSVGImage::ShapeCount() const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		return impl ? (int32)impl->shapes.Size() : 0;
	}

	NkSVGShapeView NkSVGImage::GetShape(int32 idx) const noexcept {
		NkSVGShapeView v;
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		if (!impl || idx < 0 || (uint32)idx >= impl->shapes.Size())
			return v;
		v.mShapePtr = &impl->shapes[(uint32)idx];
		return v;
	}

	bool NkSVGImage::TriangulateAll(NkVector<float32> &outXs, NkVector<float32> &outYs, NkVector<uint32> &outIndices,
									NkVector<uint32> &outTriColors) const noexcept {
		const SVGImageImpl *impl = (const SVGImageImpl *)mImpl;
		if (!impl)
			return false;
		outXs.Clear();
		outYs.Clear();
		outIndices.Clear();
		outTriColors.Clear();
		for (uint32 i = 0; i < impl->shapes.Size(); ++i) {
			const Shape &sh = impl->shapes[i];
			if (sh.style.fill.none)
				continue;
			// Triangulate cette shape : chaque triangle produit hérite la couleur fill.
			NkSVGShapeView v;
			v.mShapePtr = &sh;
			const uint32 baseIdx = (uint32)outXs.Size();
			const int32 trisBefore = (int32)(outIndices.Size() / 3);
			v.Triangulate(outXs, outYs, outIndices, baseIdx);
			const int32 trisAfter = (int32)(outIndices.Size() / 3);
			// Couleur fill premultipliee par opacity * fillOpacity.
			const float32 a = (float32)sh.style.fill.a / 255.f * sh.style.opacity * sh.style.fillOpacity;
			const uint32 col = PackRGBA(sh.style.fill.r, sh.style.fill.g, sh.style.fill.b, (uint8)(a * 255.f));
			for (int32 k = trisBefore; k < trisAfter; ++k) {
				outTriColors.PushBack(col);
			}
		}
		return true;
	}

	void NkSVGImage::Free() noexcept {
		SVGImageImpl *impl = (SVGImageImpl *)mImpl;
		if (impl) {
			impl->~SVGImageImpl();
			NkFree(impl);
			mImpl = nullptr;
		}
		this->~NkSVGImage();
		NkFree(this);
	}

	// ═════════════════════════════════════════════════════════════════════════════
	// SECTION 11 — NkSVGCodec API publique
	// ═════════════════════════════════════════════════════════════════════════════

	NkImage NkSVGCodec::Decode(const uint8 *data, usize size, int32 outW, int32 outH) noexcept {
		// Pipe-through : on parse en NkSVGImage puis on rasterise. Une seule passe
		// si l'appelant n'a pas besoin de conserver la representation vectorielle.
		NkSVGImage *svg = NkSVGImage::LoadFromMemory(data, size);
		if (!svg)
			return NkImage();
		// NkSVGImage reste une ressource tas explicite : on la libere ici.
		// Seule l'image rasterisee est un type valeur, rendue par valeur.
		NkImage img = svg->Rasterize(outW, outH);
		svg->Free();
		return img;
	}

	NkImage NkSVGCodec::DecodeFromFile(const char *path, int32 outW, int32 outH) noexcept {
		NkSVGImage *svg = NkSVGImage::LoadFromFile(path);
		if (!svg)
			return NkImage();
		NkImage img = svg->Rasterize(outW, outH);
		svg->Free();
		return img;
	}

	// L'encodage SVG (NkImage -> SVG enrobant un PNG base64) n'est pas prioritaire.
	// On le laisse stub pour l'instant ; a implementer si besoin.
	bool NkSVGCodec::Encode(const NkImage &, uint8 *&, usize &) noexcept {
		return false;
	}

	bool NkSVGCodec::EncodeToFile(const NkImage &, const char *) noexcept {
		return false;
	}

} // namespace nkentseu

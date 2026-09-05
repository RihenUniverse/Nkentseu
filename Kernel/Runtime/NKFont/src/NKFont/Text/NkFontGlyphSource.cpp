// =============================================================================
// NkFontGlyphSource.cpp — l'adaptateur NKFont -> NkIGlyphSource
// -----------------------------------------------------------------------------
// AUTEUR : TEUGUIA TADJUIDJE Rodolf Séderis — Rihen
// LICENCE : Proprietary - All Rights Reserved (voir LICENSE)
// =============================================================================

#include "NKFont/Text/NkFontGlyphSource.h"
#include "NKFont/NkFont.h"
#include "NKLogger/NkLog.h"

#include <cstring>

namespace nkentseu {

	namespace {

		/// Comparaison insensible a la casse (les noms de fontes le sont).
		int32 CmpSansCasse(const char *a, const char *b) noexcept {
			if (!a || !b)
				return a == b ? 0 : 1;
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

	} // namespace

	NkFontGlyphSource::~NkFontGlyphSource() noexcept {
		for (int32 i = 0; i < mNbFaces; ++i) {
			if (mFaces[i].ok)
				nkfont::NkFreeFontFace(&mFaces[i].info);
			if (mFaces[i].ttf)
				NkFontEmbedded::FreeDecompressedData(mFaces[i].ttf);
			mFaces[i].ttf = nullptr;
			mFaces[i].ok = false;
		}
		mNbFaces = 0;
		mActive = -1;
	}

	int32 NkFontGlyphSource::Ouvrir(const char *famille) noexcept {
		if (!famille || !*famille)
			famille = "Inter";

		for (int32 i = 0; i < mNbFaces; ++i)
			if (CmpSansCasse(mFaces[i].famille, famille) == 0)
				return mFaces[i].ok ? i : -1;
		if (mNbFaces >= kMaxFaces)
			return (mNbFaces > 0 && mFaces[0].ok) ? 0 : -1;

		// La police EMBARQUEE du meme nom, sinon Inter. C'est l'appelant qui dira
		// le repli : cette fonction rend seulement un indice.
		int32 nbEmb = 0;
		const NkEmbeddedFontData *toutes = NkFontEmbedded::GetAll(&nbEmb);
		NkEmbeddedFontId choisie = NkEmbeddedFontId::Inter;
		for (int32 i = 0; i < nbEmb && toutes; ++i) {
			if (toutes[i].name && CmpSansCasse(toutes[i].name, famille) == 0) {
				choisie = (NkEmbeddedFontId)i;
				break;
			}
		}
		const NkEmbeddedFontData *d = NkFontEmbedded::GetData(choisie);
		Face &f = mFaces[mNbFaces];
		std::strncpy(f.famille, famille, sizeof(f.famille) - 1);
		f.famille[sizeof(f.famille) - 1] = 0;
		++mNbFaces;
		if (!d) {
			logger.Warn("[NkFontGlyphSource] aucune police embarquee dans ce binaire.");
			return -1;
		}
		nkft_uint32 taille = 0;
		f.ttf = NkFontEmbedded::DecompressData(*d, &taille);
		if (!f.ttf || taille == 0) {
			logger.Warn("[NkFontGlyphSource] decompression de « {0} » echouee.", d->name);
			return -1;
		}
		f.ok = nkfont::NkInitFontFace(&f.info, f.ttf, (nkft_size)taille, 0);
		if (!f.ok) {
			logger.Warn("[NkFontGlyphSource] police « {0} » illisible (tables).", d->name);
			return -1;
		}
		return mNbFaces - 1;
	}

	bool NkFontGlyphSource::SelectFace(const char *family, int32 weight) noexcept {
		mWeight = weight > 0 ? weight : 400;
		const int32 idx = Ouvrir(family);
		mActive = idx;
		if (idx < 0)
			return false;
		// « trouvee » veut dire : la fonte DEMANDEE, pas un repli. On compare au nom
		// embarque, pas au nom range dans le cache (qui est celui qu'on a demande).
		if (!family || !*family)
			return true;
		int32 nbEmb = 0;
		const NkEmbeddedFontData *toutes = NkFontEmbedded::GetAll(&nbEmb);
		for (int32 i = 0; i < nbEmb && toutes; ++i)
			if (toutes[i].name && CmpSansCasse(toutes[i].name, family) == 0)
				return true;
		return false; // un repli a ete pris : a l'appelant de le DIRE
	}

	const char *NkFontGlyphSource::ActiveFamily() const noexcept {
		if (mActive < 0 || mActive >= mNbFaces)
			return "";
		return mFaces[mActive].famille;
	}

	float32 NkFontGlyphSource::Advance(uint32 codepoint, float32 fontSize) const noexcept {
		if (mActive < 0 || !mFaces[mActive].ok)
			return 0.f;
		const nkfont::NkFontFaceInfo *info = &mFaces[mActive].info;
		// `font-size` EST LE CADRATIN : echelle = taille / unitsPerEm (CSS, SVG).
		const float32 ech = nkfont::NkScaleForEmToPixels(const_cast<nkfont::NkFontFaceInfo *>(info), fontSize);
		const NkGlyphId gid = nkfont::NkFindGlyphIndex(info, (NkFontCodepoint)codepoint);
		nkft_int32 aw = 0, lsb = 0;
		nkfont::NkGetGlyphHMetrics(info, gid, &aw, &lsb);
		return (float32)aw * ech;
	}

	bool NkFontGlyphSource::Metrics(float32 fontSize, float32 &ascent, float32 &descent) const noexcept {
		ascent = 0.f;
		descent = 0.f;
		if (mActive < 0 || !mFaces[mActive].ok)
			return false;
		const nkfont::NkFontFaceInfo *info = &mFaces[mActive].info;
		const float32 ech = nkfont::NkScaleForEmToPixels(const_cast<nkfont::NkFontFaceInfo *>(info), fontSize);
		nkft_int32 asc = 0, desc = 0, gap = 0;
		nkfont::NkGetFontVMetrics(info, &asc, &desc, &gap);
		ascent = (float32)asc * ech;
		descent = -(float32)desc * ech; // `desc` est negatif dans la table : on rend une hauteur
		return true;
	}

	bool NkFontGlyphSource::Outline(uint32 codepoint, float32 fontSize, float32 penX, float32 baselineY,
									NkIGlyphSink &sink) const noexcept {
		if (mActive < 0 || !mFaces[mActive].ok)
			return false;
		const nkfont::NkFontFaceInfo *info = &mFaces[mActive].info;
		const NkGlyphId gid = nkfont::NkFindGlyphIndex(info, (NkFontCodepoint)codepoint);
		if (gid == 0)
			return false;
		nkfont::NkFontVertexBuffer vb;
		if (!nkfont::NkGetGlyphShape(info, gid, &vb) || vb.count == 0)
			return false;

		const float32 ech = nkfont::NkScaleForEmToPixels(const_cast<nkfont::NkFontFaceInfo *>(info), fontSize);
		// L'AXE Y EST RETOURNE ICI, une fois : la police monte, l'image descend.
		// Chaque appelant qui le referait serait un appelant de plus a pouvoir se
		// tromper de signe.
		auto X = [&](nkft_int32 v) { return penX + (float32)v * ech; };
		auto Y = [&](nkft_int32 v) { return baselineY - (float32)v * ech; };

		bool ouvert = false;
		for (nkft_uint32 i = 0; i < vb.count; ++i) {
			const nkfont::NkFontVertex &v = vb.verts[i];
			switch (v.type) {
				case nkfont::NK_FONT_VERTEX_MOVE:
					if (ouvert)
						sink.GlyphClose();
					sink.GlyphMoveTo(X(v.x), Y(v.y));
					ouvert = true;
					break;
				case nkfont::NK_FONT_VERTEX_LINE:
					sink.GlyphLineTo(X(v.x), Y(v.y));
					break;
				case nkfont::NK_FONT_VERTEX_CURVE:
					sink.GlyphQuadTo(X(v.cx), Y(v.cy), X(v.x), Y(v.y));
					break;
				case nkfont::NK_FONT_VERTEX_CUBIC:
					sink.GlyphCubicTo(X(v.cx), Y(v.cy), X(v.cx1), Y(v.cy1), X(v.x), Y(v.y));
					break;
				default:
					break;
			}
		}
		if (ouvert)
			sink.GlyphClose();
		return ouvert;
	}

} // namespace nkentseu

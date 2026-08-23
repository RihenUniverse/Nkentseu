// =============================================================================
// Applications/NkMsaaDeviceCheck/src/main.cpp
// =============================================================================
// LE CONTRAT DIT-IL LA VERITE SUR UNE VRAIE CARTE ? (mode complet)
// -----------------------------------------------------------------------------
// NkMsaaContractCheck mesure le CONTRAT sans peripherique : il prouve que
// demander 3 echantillons est REFUSABLE. Il ne prouve pas que le refus
// correspond a ce que la carte fait vraiment.
//
// Ce banc-ci ferme la moitie manquante. Pour chaque nombre d'echantillons :
//   attendu = caps.SupportsSamples(s)
//   obtenu  = CreateTexture(desc avec samples = s).IsValid()
//   exigence : obtenu == attendu
//
// LES DEUX DIRECTIONS D'ERREUR N'ONT PAS LE MEME SENS
// -----------------------------------------------------------------------------
//   attendu=NON, obtenu=OUI  -> LE MENSONGE VISE. La carte a accepte une valeur
//     que le contrat refuse : elle a tres probablement degrade en silence, et
//     l'appelant croit avoir du MSAA. C'est le defaut des neuf capacites.
//   attendu=OUI, obtenu=NON  -> le contrat est trop optimiste : il promet ce que
//     la carte refuse. Moins dangereux, faux quand meme.
// Les deux sont des echecs. Ils sont NOMMES separement.
//
// ⚠️ IGNORE N'EST PAS VERT, ET C'EST LA RAISON D'ETRE DU CODE 77
// -----------------------------------------------------------------------------
// Un banc qui exige un GPU sera saute sur une machine qui n'en a pas, ou dont le
// pilote refuse. UN BANC IGNORE QUI RAPPORTE VERT EST EXACTEMENT LE MENSONGE QUE
// CE CHANTIER TRAQUE : on croit avoir mesure.
//   0  le contrat dit la verite sur cette carte
//   1  le contrat MENT — voir les lignes [FAIL]
//   77 IGNORE : aucun peripherique. NI succes, NI echec. Le verificateur le
//      compte a part et le NOMME.
//
// ⚠️ Et un precedent a ne pas suivre : NkGpuProbe rend 1 quand aucun device n'est
// disponible — il confond « pas de carte » avec « la carte est cassee ». C'est le
// symetrique du meme defaut : un faux ECHEC au lieu d'un faux vert.
// =============================================================================
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkGraphicsApi.h"
#include "NKRHI/Core/NkIDevice.h"

#include <cstdio>

using namespace nkentseu;

namespace {

	const int kIgnore = 77;

	int32 gPass = 0;
	int32 gFail = 0;

	NkSampleCount Brut(uint32 n) {
		return static_cast<NkSampleCount>(n);
	}

	// Une seule valeur mesuree : on demande, on regarde, on confronte au contrat.
	void Mesurer(NkIDevice *dev, uint32 n) {
		const NkDeviceCaps &caps = dev->GetCaps();
		const bool attendu = caps.SupportsSamples(Brut(n));

		NkTextureDesc d;
		d.type = NkTextureType::NK_TEX2D;
		d.format = NkGPUFormat::NK_RGBA8_UNORM;
		d.width = 64;
		d.height = 64;
		d.mipLevels = 1;
		d.samples = Brut(n);
		d.bindFlags = NkBindFlags::NK_RENDER_TARGET;
		d.debugName = "NkMsaaDeviceCheck";

		NkTextureHandle h = dev->CreateTexture(d);
		const bool obtenu = h.IsValid();
		if (obtenu)
			dev->DestroyTexture(h);

		if (obtenu == attendu) {
			++gPass;
			printf("  [OK]   %2u echantillons : contrat=%s, carte=%s\n", n, attendu ? "OUI" : "NON",
				   obtenu ? "OUI" : "NON");
			return;
		}
		++gFail;
		if (!attendu && obtenu) {
			printf("  [FAIL] %2u echantillons : le contrat REFUSE, la carte ACCEPTE.\n", n);
			printf("         C'est le mensonge vise : la ressource existe, et rien ne dit\n");
			printf("         combien d'echantillons elle porte reellement.\n");
		} else {
			printf("  [FAIL] %2u echantillons : le contrat PROMET, la carte REFUSE.\n", n);
			printf("         Le contrat est trop optimiste — il annonce ce qui n'existe pas.\n");
		}
	}

	NkIDevice *Ouvrir(NkGraphicsApi api) {
		NkDeviceInitInfo di;
		di.api = api; // pas de surface -> headless
		NkIDevice *dev = NkDeviceFactory::Create(di);
		if (dev == nullptr)
			return nullptr;
		if (!dev->Init(di)) {
			NkDeviceFactory::Destroy(dev);
			return nullptr;
		}
		return dev;
	}

} // namespace

int main() {
	printf("=== NkMsaaDeviceCheck — le contrat dit-il la verite sur cette carte ? ===\n");
	fflush(stdout);

	const NkGraphicsApi essais[] = {NkGraphicsApi::NK_GFX_API_DX11, NkGraphicsApi::NK_GFX_API_DX12,
									NkGraphicsApi::NK_GFX_API_OPENGL};
	NkIDevice *dev = nullptr;
	NkGraphicsApi retenue = NkGraphicsApi::NK_GFX_API_NONE;
	for (uint32 i = 0; i < sizeof(essais) / sizeof(essais[0]); ++i) {
		printf("  ... tentative %s\n", NkGraphicsApiName(essais[i]));
		fflush(stdout);
		dev = Ouvrir(essais[i]);
		if (dev != nullptr) {
			retenue = essais[i];
			break;
		}
	}

	if (dev == nullptr) {
		printf("\n[IGNORE] aucun peripherique headless n'a pu etre cree sur cette machine.\n");
		printf("         Ce banc n'a RIEN mesure. Ce n'est pas un succes.\n");
		printf("         Code de sortie %d : le verificateur le compte IGNORE, distinct de OK,\n", kIgnore);
		printf("         et le nomme sur sa ligne de verdict. Un banc ignore qui rapporterait\n");
		printf("         vert ferait croire la question mesuree — c'est le mensonge meme que\n");
		printf("         ce banc existe pour traquer.\n");
		return kIgnore;
	}

	printf("\n--- peripherique : %s ---\n", NkGraphicsApiName(retenue));
	const NkDeviceCaps &caps = dev->GetCaps();
	printf("    drapeaux MSAA de cette carte : 2x=%s 4x=%s 8x=%s 16x=%s   MaxSamples()=%u\n",
		   caps.msaa2x ? "oui" : "non", caps.msaa4x ? "oui" : "non", caps.msaa8x ? "oui" : "non",
		   caps.msaa16x ? "oui" : "non", caps.MaxSamples());
	printf("\n-- confrontation contrat / carte --\n");

	// Les valeurs EXPRIMABLES, plus deux IMPOSSIBLES (le cas du mandat).
	const uint32 valeurs[] = {1u, 2u, 3u, 4u, 7u, 8u, 16u, 32u, 64u};
	for (uint32 i = 0; i < sizeof(valeurs) / sizeof(valeurs[0]); ++i)
		Mesurer(dev, valeurs[i]);

	NkDeviceFactory::Destroy(dev);

	printf("\n=== Resultat : %d OK / %d FAIL ===\n", gPass, gFail);
	if (gFail == 0)
		printf("Le contrat dit la verite sur cette carte, pour les 9 valeurs mesurees.\n");
	return gFail == 0 ? 0 : 1;
}

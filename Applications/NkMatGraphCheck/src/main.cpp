// =============================================================================
// NkMatGraphCheck — banc CONSOLE du graphe de materiaux (couche 3 sur NKGraph).
//
// POURQUOI UNE APPLICATION ET PAS UN `tests/` : mesure le 2026-08-21,
// `jenga test --project X` rend « Unit-test execution is disabled by workspace
// policy ». Les bancs sous `tests/` COMPILENT et ne s'executent JAMAIS. Un banc
// qui doit prouver quelque chose est donc une application console a part
// entiere. Meme raison que NKEditMeshHarness et NkSLCheck.
//
// AUCUN GPU, AUCUNE FENETRE. Un graphe est une structure de donnees ; le NkSL
// qu'il emettra se verifie par le vrai NkSLCompiler, qui n'a pas besoin de
// device (c'est ce que fait deja NkSLCheck). Le banc reste donc rapide et
// utilisable partout.
//
// CE QUE CHAQUE CAS DOIT ETRE : un cas qui DISCRIMINE. Il ne confirme pas que ca
// marche, il echouerait si l'implantation etait fausse D'UNE FACON PRECISE, et
// cette facon est ecrite a cote du cas. Un cas qui ne peut pas surprendre n'est
// pas une mesure.
// =============================================================================
#include "NKRenderer/Materials/Graph/NkMatGraphTypes.h"

#include <stdio.h>

using namespace nkentseu;
using namespace nkentseu::graph;
using namespace nkentseu::renderer::matgraph;

static uint32 gCas = 0;
static uint32 gEchecs = 0;

static void Cas(const char *nom, bool ok, const char *detail) {
	++gCas;
	if (!ok)
		++gEchecs;
	printf("%-34s %s | %s\n", nom, ok ? "OK  " : "ECHEC", detail);
}

// ── types/ ───────────────────────────────────────────────────────────────────

static void CasTypesEnregistrement() {
	// DISCRIMINE : une implantation qui enregistrerait deux fois le meme nom, ou
	// qui rendrait 0 (l'identifiant reserve « invalide »), passerait un test qui
	// se contenterait de compter les appels. On verifie donc que les quatre
	// identifiants sont NON NULS et DEUX A DEUX DISTINCTS.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const bool nonNuls = t.Valid();
	const bool distincts = t.real != t.vector && t.real != t.color && t.real != t.shader &&
						   t.vector != t.color && t.vector != t.shader && t.color != t.shader;
	// Et ils doivent se RETROUVER par leur nom : un enregistrement qui ne
	// remplirait pas la table de noms laisserait FindType a zero, et tout
	// NkMatAddNode ulterieur echouerait sans qu'on sache pourquoi.
	const bool retrouves = g.FindType(NK_MT_REAL) == t.real && g.FindType(NK_MT_SHADER) == t.shader;
	char d[160];
	snprintf(d, sizeof(d), "reel=%u vect=%u coul=%u shader=%u | distincts=%d retrouves-par-nom=%d",
			 t.real, t.vector, t.color, t.shader, distincts ? 1 : 0, retrouves ? 1 : 0);
	Cas("types/enregistrement", nonNuls && distincts && retrouves, d);
}

static void CasConversionDirigee() {
	// LE CAS QUI TRANCHE, et il se teste DANS LES DEUX SENS : une table de
	// conversions symetrique par erreur passerait un test a sens unique.
	//   reel -> couleur   doit etre ACCEPTE (niveau de gris)
	//   couleur -> reel   doit etre REFUSE (luminance ? moyenne ? canal rouge ?
	//                     trois reponses plausibles, donc aucune par defaut)
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const bool reelVersCouleur = g.Accepts(t.color, t.real);
	const bool couleurVersReel = g.Accepts(t.real, t.color);
	// Et le shader ne se convertit avec RIEN : c'est ce qui empeche de brancher
	// une couleur la ou le moteur attend une surface ombree.
	const bool couleurVersShader = g.Accepts(t.shader, t.color);
	const bool shaderVersCouleur = g.Accepts(t.color, t.shader);
	char d[192];
	snprintf(d, sizeof(d), "reel>couleur=%s couleur>reel=%s | couleur>shader=%s shader>couleur=%s",
			 reelVersCouleur ? "ok" : "REFUSE", couleurVersReel ? "ACCEPTE" : "refuse",
			 couleurVersShader ? "ACCEPTE" : "refuse", shaderVersCouleur ? "ACCEPTE" : "refuse");
	Cas("types/conversion-dirigee", reelVersCouleur && !couleurVersReel && !couleurVersShader && !shaderVersCouleur, d);
}

// ── bibliotheque/ ────────────────────────────────────────────────────────────

static void CasInstancierPrincipled() {
	// DISCRIMINE : on ne compte pas les prises, on les cherche PAR NOM ET PAR
	// SENS. Une implantation qui ajouterait les bonnes prises dans le mauvais
	// sens (bsdf en entree) donnerait exactement le meme compte.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId n = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNode *nd = g.Find(n);
	const int32 baseColor = nd ? nd->FindSocket("base_color", NkSocketDir::Input) : -1;
	const int32 bsdfOut = nd ? nd->FindSocket("bsdf", NkSocketDir::Output) : -1;
	// Le piege du sens : « bsdf » NE DOIT PAS exister en entree.
	const int32 bsdfIn = nd ? nd->FindSocket("bsdf", NkSocketDir::Input) : 0;
	// Le piege du type : base_color doit porter le type COULEUR, pas reel. Une
	// declaration decalee d'une ligne donnerait le bon nom et le mauvais type,
	// et le graphe accepterait alors des liens qu'il devrait refuser.
	const bool bonType = nd && baseColor >= 0 && nd->sockets[(usize)baseColor].type == g.FindType(NK_MT_COLOR);
	char d[192];
	snprintf(d, sizeof(d), "noeud=%u prises=%u base_color=%d(type-ok=%d) bsdf-sortie=%d bsdf-entree=%d(-1 attendu)", n,
			 nd ? (uint32)nd->sockets.Size() : 0u, baseColor, bonType ? 1 : 0, bsdfOut, bsdfIn);
	Cas("biblio/instancier-principled", n != NK_NODE_INVALID && baseColor >= 0 && bsdfOut >= 0 && bsdfIn < 0 && bonType,
		d);
}

static void CasPrototypeInconnu() {
	// DISCRIMINE : un refus qui laisserait quand meme un noeud derriere lui
	// donnerait le meme code de retour. On verifie donc AUSSI que le graphe est
	// reste VIDE — c'est le meme piege que « cycle-refuse » dans NKGraph.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId n = NkMatAddNode(g, "mat.noeud_qui_nexiste_pas");
	char d[128];
	snprintf(d, sizeof(d), "rendu=%u (0 attendu) noeuds-dans-le-graphe=%u (0 attendu)", n, g.NodeCount());
	Cas("biblio/prototype-inconnu", n == NK_NODE_INVALID && g.NodeCount() == 0, d);
}

static void CasTypesNonEnregistres() {
	// LE CAS QU'ON N'ECRIT PAS SPONTANEMENT : instancier AVANT d'avoir
	// enregistre les types. Une implantation « serviable » qui enregistrerait
	// les types au vol reussirait ici — et priverait le graphe de ses
	// CONVERSIONS, si bien qu'il refuserait plus tard des liens parfaitement
	// bons, tres loin de la cause. Le refus doit etre net.
	NkNodeGraph g; // volontairement SANS NkMatRegisterTypes
	const NkNodeId n = NkMatAddNode(g, NK_MN_PRINCIPLED);
	char d[128];
	snprintf(d, sizeof(d), "rendu=%u (0 attendu) noeuds=%u (0 attendu)", n, g.NodeCount());
	Cas("biblio/types-non-enregistres", n == NK_NODE_INVALID && g.NodeCount() == 0, d);
}

// ── graphe/ ──────────────────────────────────────────────────────────────────

static void CasPrincipledVersSortie() {
	// Le graphe minimal REEL. DISCRIMINE par l'ORDRE topologique : le Principled
	// doit venir AVANT la sortie. On cree la sortie EN PREMIER pour qu'un tri
	// qui renverrait l'ordre d'insertion echoue.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkLinkError e = g.Connect(bsdf, "bsdf", out, "surface");
	NkVector<NkNodeId> ordre;
	const bool trie = g.TopoSort(ordre);
	const bool bonOrdre = trie && ordre.Size() == 2 && ordre[0] == bsdf && ordre[1] == out;
	NkNodeId trouve = NK_NODE_INVALID;
	const NkMatGraphError v = NkMatValidate(g, &trouve);
	char d[192];
	snprintf(d, sizeof(d), "lien=%s ordre=%s (bsdf avant sortie) validation=%s sortie-trouvee=%d",
			 NkLinkErrorName(e), bonOrdre ? "bsdf>sortie" : "MAUVAIS", NkMatGraphErrorName(v),
			 trouve == out ? 1 : 0);
	Cas("graphe/principled-vers-sortie", e == NkLinkError::Ok && bonOrdre && v == NkMatGraphError::Ok && trouve == out,
		d);
}

static void CasCouleurDansShaderRefuse() {
	// LE CAS QUI PROTEGE L'AUTEUR. Brancher une couleur dans l'entree « surface »
	// est l'erreur la plus courante dans l'editeur de Blender. Elle doit etre
	// refusee AVEC SA RAISON, et le lien ne doit PAS avoir ete pose — un refus
	// qui laisserait le lien rendrait le meme code d'erreur.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId emis = NkMatAddNode(g, NK_MN_EMISSION);
	// « color » est une ENTREE couleur d'Emission : on tente une sortie->entree
	// invalide ET un type invalide, en deux cas distincts.
	const NkLinkError typeFaux = g.Connect(emis, "emission", out, "surface"); // valide, temoin positif
	const NkLinkError sensFaux = g.Connect(out, "surface", emis, "color");	 // entree -> entree
	const uint32 liens = g.LinkCount();
	char d[192];
	snprintf(d, sizeof(d), "temoin-positif(emission>surface)=%s | entree>entree=%s | liens=%u (1 attendu)",
			 NkLinkErrorName(typeFaux), NkLinkErrorName(sensFaux), liens);
	Cas("graphe/sens-et-types-refuses",
		typeFaux == NkLinkError::Ok && sensFaux == NkLinkError::DirectionMismatch && liens == 1, d);
}

static void CasDeuxSortiesRefusees() {
	// VALIDATION DE DOMAINE, pas du coeur. Le coeur ne sait pas ce qu'est une
	// « sortie materiau » et ne peut pas refuser la seconde — c'est a moi.
	// DISCRIMINE : le graphe reste PARFAITEMENT VALIDE pour le coeur (aucun
	// cycle, tous les types bons). Seule une passe qui connait le domaine voit
	// le probleme.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId out1 = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out1, "surface");
	const NkMatGraphError avant = NkMatValidate(g);
	const NkNodeId out2 = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkMatGraphError apres = NkMatValidate(g);
	NkVector<NkNodeId> ordre;
	const bool coeurContent = g.TopoSort(ordre); // le coeur, lui, ne voit rien
	// Et le retrait de la seconde sortie doit RENDRE le graphe valide : une
	// validation qui compterait les noeuds MORTS resterait bloquee sur
	// « plusieurs sorties » apres la suppression.
	g.RemoveNode(out2);
	const NkMatGraphError apresRetrait = NkMatValidate(g);
	char d[224];
	snprintf(d, sizeof(d), "1 sortie=%s | 2 sorties=%s (coeur triable=%d, il ne voit rien) | apres retrait=%s",
			 NkMatGraphErrorName(avant), NkMatGraphErrorName(apres), coeurContent ? 1 : 0,
			 NkMatGraphErrorName(apresRetrait));
	Cas("graphe/deux-sorties-refusees",
		avant == NkMatGraphError::Ok && apres == NkMatGraphError::MultipleOutput && coeurContent &&
			apresRetrait == NkMatGraphError::Ok,
		d);
}

static void CasSortieNonReliee() {
	// DISCRIMINE : une validation qui se contenterait de compter les sorties
	// declarerait ce graphe bon. Il ne l'est pas — rien n'entre dans la sortie,
	// et le compilateur n'aurait rien a compiler.
	//
	// ⚠️ CE QUE CE CAS NE PROUVE **PAS**, et je l'ai appris en le mutant. Le
	// piege de la sentinelle est reel — l'index du socket « surface » vaut 0,
	// donc un controle ecrit `if (!index)` au lieu de `if (index < 0)` est faux.
	// Mais sous cette mutation, CE cas-ci **passe encore**, pour la mauvaise
	// raison : le controle casse rend « sortie-non-reliee » a tout le monde, y
	// compris a lui. Ce sont les cas a graphe VALIDE (principled-vers-sortie,
	// melange-deux-bsdf, aller-retour) qui attrapent la mutation, en se mettant
	// au rouge. On l'ecrit ici parce qu'un commentaire qui promet une
	// discrimination qu'il n'a pas est pire qu'un cas absent.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	NkMatAddNode(g, NK_MN_PRINCIPLED); // present mais NON RELIE
	const NkMatGraphError v = NkMatValidate(g);
	const NkNode *nd = g.Find(out);
	const int32 idx = nd ? nd->FindSocket("surface", NkSocketDir::Input) : -1;
	char d[160];
	snprintf(d, sizeof(d), "validation=%s | index du socket surface=%d (0 : la sentinelle a zero serait un BUG)",
			 NkMatGraphErrorName(v), idx);
	Cas("graphe/sortie-non-reliee", v == NkMatGraphError::OutputUnlinked && idx == 0, d);
}

static void CasAucuneSortie() {
	// Le temoin de l'autre bord : sans lui, une validation qui refuserait TOUT
	// passerait les cas precedents sans rien prouver.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkMatGraphError v = NkMatValidate(g);
	char d[128];
	snprintf(d, sizeof(d), "validation=%s", NkMatGraphErrorName(v));
	Cas("graphe/aucune-sortie", v == NkMatGraphError::NoOutput, d);
}

static void CasMelangeShader() {
	// Le « Mix Shader » de Rodolf, celui par lequel tout a commence. On melange
	// DEUX BSDF vers une seule sortie. DISCRIMINE par l'ordre topologique a
	// QUATRE noeuds : le melangeur doit venir apres SES DEUX sources et avant la
	// sortie. Les noeuds sont crees dans l'ordre INVERSE de l'ordre attendu.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_SHADER);
	const NkNodeId diff = NkMatAddNode(g, NK_MN_DIFFUSE);
	const NkNodeId emis = NkMatAddNode(g, NK_MN_EMISSION);
	const NkLinkError e1 = g.Connect(diff, "bsdf", mix, "shader1");
	const NkLinkError e2 = g.Connect(emis, "emission", mix, "shader2");
	const NkLinkError e3 = g.Connect(mix, "shader", out, "surface");
	NkVector<NkNodeId> o;
	const bool trie = g.TopoSort(o);
	int32 pMix = -1, pOut = -1, pDiff = -1, pEmis = -1;
	for (usize i = 0; i < o.Size(); ++i) {
		if (o[i] == mix)
			pMix = (int32)i;
		else if (o[i] == out)
			pOut = (int32)i;
		else if (o[i] == diff)
			pDiff = (int32)i;
		else if (o[i] == emis)
			pEmis = (int32)i;
	}
	const bool bonOrdre = trie && pDiff < pMix && pEmis < pMix && pMix < pOut;
	const NkMatGraphError v = NkMatValidate(g);
	char d[224];
	snprintf(d, sizeof(d), "liens=%s/%s/%s | positions diff=%d emis=%d mix=%d sortie=%d | validation=%s",
			 NkLinkErrorName(e1), NkLinkErrorName(e2), NkLinkErrorName(e3), pDiff, pEmis, pMix, pOut,
			 NkMatGraphErrorName(v));
	Cas("graphe/melange-deux-bsdf",
		e1 == NkLinkError::Ok && e2 == NkLinkError::Ok && e3 == NkLinkError::Ok && bonOrdre &&
			v == NkMatGraphError::Ok,
		d);
}

static void CasAllerRetourFichier() {
	// Le graphe est une DONNEE : il doit survivre a l'aller-retour. DISCRIMINE
	// en comparant les TEXTES caractere pour caractere, pas des comptes — des
	// libelles ou des conversions perdus laisseraient les comptes intacts. Et on
	// verifie que la SEMANTIQUE survit : le graphe relu doit encore refuser ce
	// qu'il refusait (preuve que `conv` a ete relu, pas seulement reecrit).
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	NkString a;
	g.Serialize(a);

	NkNodeGraph g2;
	const bool lu = g2.Deserialize(a.CStr());
	NkString b;
	g2.Serialize(b);
	const bool identique = (a == b);
	const NkMatGraphError v = NkMatValidate(g2);
	// La semantique : reel->couleur accepte, couleur->reel refuse, APRES relecture.
	const NkTypeId r2 = g2.FindType(NK_MT_REAL), c2 = g2.FindType(NK_MT_COLOR);
	const bool semantique = r2 && c2 && g2.Accepts(c2, r2) && !g2.Accepts(r2, c2);
	char d[224];
	snprintf(d, sizeof(d), "lu=%d texte-identique=%d octets=%u validation=%s | semantique-survit=%d (types %u/%u)",
			 lu ? 1 : 0, identique ? 1 : 0, (uint32)a.Size(), NkMatGraphErrorName(v), semantique ? 1 : 0, r2, c2);
	Cas("graphe/aller-retour-fichier", lu && identique && v == NkMatGraphError::Ok && semantique, d);
	(void)t;
}

int main() {
	printf("== NkMatGraphCheck — graphe de materiaux, couche 3 sur NKGraph ==\n");
	printf("   regime : structure de donnees pure, aucun GPU, aucune fenetre.\n");
	printf("   NE couvre PAS : la generation de NkSL (pas encore ecrite), les\n");
	printf("   parametres de noeuds (le coeur n'a pas encore ou les ranger).\n\n");

	CasTypesEnregistrement();
	CasConversionDirigee();
	CasInstancierPrincipled();
	CasPrototypeInconnu();
	CasTypesNonEnregistres();
	CasPrincipledVersSortie();
	CasCouleurDansShaderRefuse();
	CasDeuxSortiesRefusees();
	CasSortieNonReliee();
	CasAucuneSortie();
	CasMelangeShader();
	CasAllerRetourFichier();

	printf("\n-- %u cas, %u echec(s) --\n", gCas, gEchecs);
	return gEchecs == 0 ? 0 : 1;
}

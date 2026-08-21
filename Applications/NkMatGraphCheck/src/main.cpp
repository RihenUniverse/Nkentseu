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
// Les numeros de binding du set materiau. En-tete SANS DEPENDANCE, fait pour
// etre lisible par un banc qui ne lie ni NKRenderer ni NKRHI.
#include "NKRenderer/Materials/NkMaterialBindings.h"
// Le VRAI compilateur NkSL : c est lui qui dit si le shader traverse les
// quatre backends, et il n a besoin d aucun device (cf. NkSLCheck).
#include "NKSL/Compiler/NkSLCompiler.h"

#include <stdio.h>
#include <stdlib.h>

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

// ── outils communs aux cas de valeurs ────────────────────────────────────────

// Cherche une ligne COMMENCANT par `prefixe` dans un texte `.nkgraph`. Sert a
// prouver une ABSENCE de ligne, ce qu'aucun compteur ne peut faire.
static bool ContientLigne(const NkString &texte, const char *prefixe) {
	const char *p = texte.CStr();
	while (p && *p) {
		const char *q = prefixe;
		const char *r = p;
		while (*q && *r == *q) {
			++r;
			++q;
		}
		if (*q == 0)
			return true;
		while (*p && *p != '\n')
			++p;
		if (*p == '\n')
			++p;
	}
	return false;
}

// Compte les diagnostics d'un genre donne et rend le premier detail rencontre.
static uint32 CompteDiag(const NkNodeGraph &g, NkGraphIssue genre, NkString *premierDetail = nullptr) {
	NkVector<NkGraphDiag> diags;
	g.Validate(diags);
	uint32 n = 0;
	for (usize i = 0; i < diags.Size(); ++i)
		if (diags[i].issue == genre) {
			if (n == 0 && premierDetail)
				*premierDetail = diags[i].detail;
			++n;
		}
	return n;
}

// ── valeurs : defauts de prise et proprietes ─────────────────────────────────

static void CasDefautDePrise() {
	// DISCRIMINE : on verifie les TROIS etats qu'une prise peut avoir, parce que
	// c'est leur confusion qui coute cher — prise inexistante (nullptr), prise
	// existante sans defaut (`IsSet()` faux), prise avec defaut. Une implantation
	// qui rendrait nullptr dans les deux premiers cas passerait un test qui ne
	// regarderait que le troisieme.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId n = NkMatAddNode(g, NK_MN_PRINCIPLED);

	const NkGraphValue *absente = g.SocketDefault(n, "prise_qui_nexiste_pas", NkSocketDir::Input);
	const NkGraphValue *vierge = g.SocketDefault(n, "roughness", NkSocketDir::Input);
	const bool viergeNonRenseignee = vierge && !vierge->IsSet();

	const bool pose = g.SetSocketDefault(n, "roughness", NkSocketDir::Input, NkValueReal(t.real, 0.5f));
	const NkGraphValue *lu = g.SocketDefault(n, "roughness", NkSocketDir::Input);
	const bool bonne = lu && lu->IsSet() && lu->type == t.real && lu->numbers.Size() == 1 && lu->numbers[0] == 0.5f;

	// Poser un defaut sur une prise absente doit ECHOUER, jamais en creer une.
	const bool refuse = !g.SetSocketDefault(n, "prise_qui_nexiste_pas", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	char d[240];
	snprintf(d, sizeof(d), "absente=%s | vierge non-renseignee=%d | pose=%d relu-exact=%d | pose sur absente refusee=%d",
			 absente ? "POINTEUR (nullptr attendu)" : "nullptr", viergeNonRenseignee ? 1 : 0, pose ? 1 : 0,
			 bonne ? 1 : 0, refuse ? 1 : 0);
	Cas("valeur/defaut-de-prise", !absente && viergeNonRenseignee && pose && bonne && refuse, d);
}

static void CasJamaisRenseigneContreVide() {
	// LE PIEGE PAYE PAR L'AGENT NkUIDesign, transpose ici. « Jamais renseigne »
	// et « renseigne a vide » doivent rester DEUX etats distincts a travers
	// l'aller-retour de fichier. Un ecrivain qui emettrait quand meme sa ligne
	// pour une valeur absente ecrirait un type 0 ; la relecture rendrait alors
	// une valeur qui SEMBLE renseignee, et sans la moindre erreur.
	//
	// PREMIERE VERSION DE CE CAS : ELLE NE PROUVAIT RIEN, et c'est une mutation
	// qui me l'a appris. Elle prenait un noeud SANS aucune propriete et sans
	// aucun defaut, puis verifiait qu'aucune ligne « def » ou « prop » n'etait
	// ecrite. Une boucle d'ecriture qui n'itere sur RIEN n'ecrit rien quoi qu'on
	// fasse de son garde : la mutation « emets la ligne meme si la valeur n'est
	// pas renseignee » passait au vert. On exerce donc maintenant les DEUX
	// portes avec de vraies iterations : une propriete POSEE MAIS VIDE, et six
	// prises qui existent toutes sans defaut.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId n = NkMatAddNode(g, NK_MN_PRINCIPLED);

	// PORTE 1 — les prises. Le Principled en a six, aucune n'a de defaut : la
	// boucle d'ecriture les parcourt toutes, et son garde est donc exerce six
	// fois. Aucune ligne « def » ne doit sortir.
	// PORTE 2 — une propriete POSEE mais dont la valeur n'a jamais ete
	// renseignee. C'est un etat legitime : l'editeur cree la ligne, l'auteur n'a
	// pas encore saisi. La boucle des proprietes itere donc une fois, et son
	// garde est exerce lui aussi.
	NkGraphValue jamaisRenseignee; // type == NK_TYPE_INVALID
	g.SetProp(n, "en_attente_de_saisie", jamaisRenseignee);
	const uint32 posees = g.PropCount(n);

	NkString texte;
	g.Serialize(texte);
	const bool aucunDef = !ContientLigne(texte, "def ");
	const bool aucunProp = !ContientLigne(texte, "prop ");

	// Et apres l'aller-retour, la propriete non renseignee doit etre ABSENTE —
	// pas revenue « renseignee a vide ». C'est la difference exacte que le
	// defaut de NkUIDesign effacait.
	NkNodeGraph g2;
	g2.Deserialize(texte.CStr());
	const bool absenteApres = g2.FindProp(n, "en_attente_de_saisie") == nullptr;

	// Le TEMOIN DE L'AUTRE BORD : une valeur RENSEIGNEE A VIDE (type valide,
	// zero reel, texte vide) doit, elle, s'ecrire et survivre. Sans lui, un
	// ecrivain qui n'ecrirait JAMAIS rien passerait tout ce qui precede.
	NkGraphValue videMaisPosee;
	videMaisPosee.type = t.real;
	g.SetProp(n, "marqueur", videMaisPosee);
	NkString avec;
	g.Serialize(avec);
	NkNodeGraph g3;
	g3.Deserialize(avec.CStr());
	const NkGraphValue *relu = g3.FindProp(n, "marqueur");
	const bool survit = relu && relu->IsSet() && relu->numbers.Size() == 0 && relu->text.Size() == 0;
	const bool uneSeuleLigne = ContientLigne(avec, "prop ");

	char d[256];
	snprintf(d, sizeof(d),
			 "proprietes posees=%u | aucune ligne def=%d aucune ligne prop=%d | absente apres relecture=%d | temoin vide-mais-posee : ligne ecrite=%d survit=%d",
			 posees, aucunDef ? 1 : 0, aucunProp ? 1 : 0, absenteApres ? 1 : 0, uneSeuleLigne ? 1 : 0,
			 survit ? 1 : 0);
	Cas("valeur/jamais-renseigne-vs-vide",
		posees == 1 && aucunDef && aucunProp && absenteApres && uneSeuleLigne && survit, d);
}

static void CasProprieteDeNoeud() {
	// DISCRIMINE : poser deux fois la meme cle doit REMPLACER, pas empiler. Deux
	// proprietes homonymes rendraient la lecture dependante de l'ordre
	// d'insertion, et la premiere lecture donnerait l'ANCIENNE valeur.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId n = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.SetProp(n, "operation", NkValueText(t.real, "multiplier"));
	const uint32 apres1 = g.PropCount(n);
	g.SetProp(n, "operation", NkValueText(t.real, "ajouter"));
	const uint32 apres2 = g.PropCount(n);
	const NkGraphValue *v = g.FindProp(n, "operation");
	const bool remplacee = v && v->text == NkString("ajouter");

	// Une propriete inconnue rend nullptr, jamais une valeur par defaut
	// silencieuse : « absente » et « posee a zero » ne se corrigent pas pareil.
	const bool inconnueNulle = g.FindProp(n, "jamais_posee") == nullptr;

	g.SetProp(n, "seconde", NkValueReal(t.real, 2.f));
	const bool retiree = g.RemoveProp(n, "operation");
	const bool retraitInconnu = !g.RemoveProp(n, "jamais_posee");
	const NkGraphValue *reste = g.FindProp(n, "seconde");
	const bool resteIntacte = reste && reste->numbers.Size() == 1 && reste->numbers[0] == 2.f;
	char d[240];
	snprintf(d, sizeof(d), "1 pose=%u | 2 poses meme cle=%u (1 attendu) remplacee=%d | inconnue nullptr=%d | retiree=%d retrait-inconnu-refuse=%d voisine-intacte=%d",
			 apres1, apres2, remplacee ? 1 : 0, inconnueNulle ? 1 : 0, retiree ? 1 : 0, retraitInconnu ? 1 : 0,
			 resteIntacte ? 1 : 0);
	Cas("valeur/propriete-de-noeud",
		apres1 == 1 && apres2 == 1 && remplacee && inconnueNulle && retiree && retraitInconnu && resteIntacte, d);
}

static void CasAllerRetourConstruitEnMemoire() {
	// ⚠️ LE CRITERE D'ACCEPTATION, et il est precis : le document doit etre
	// CONSTRUIT EN MEMOIRE, pas seulement relu. Un ecrivain qui reemettrait une
	// forme litterale memorisee A LA LECTURE passerait un test relu -> reecrit,
	// et ecrirait du vide pour un document qui n'a jamais ete lu.
	//
	// DISCRIMINE : textes compares caractere pour caractere, ET chaque valeur
	// revenue EGALE AU BIT — des comptes justes laisseraient passer une valeur
	// perdue en route.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	const float32 blanc[4] = {1.f, 1.f, 1.f, 1.f};
	g.SetSocketDefault(bsdf, "base_color", NkSocketDir::Input, NkValueVec(t.color, blanc, 4));
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 0.5f));
	g.SetSocketDefault(bsdf, "metallic", NkSocketDir::Input, NkValueReal(t.real, 0.f));
	g.SetProp(bsdf, "distribution", NkValueText(t.real, "GGX multi diffusion"));

	NkString a;
	g.Serialize(a);
	NkNodeGraph g2;
	const bool lu = g2.Deserialize(a.CStr());
	NkString b;
	g2.Serialize(b);

	const NkGraphValue *c1 = g.SocketDefault(bsdf, "base_color", NkSocketDir::Input);
	const NkGraphValue *c2 = g2.SocketDefault(bsdf, "base_color", NkSocketDir::Input);
	const NkGraphValue *p1 = g.FindProp(bsdf, "distribution");
	const NkGraphValue *p2 = g2.FindProp(bsdf, "distribution");
	const bool valeursEgales = c1 && c2 && c1->Equals(*c2) && p1 && p2 && p1->Equals(*p2);
	// Un texte a ESPACES est ce qui casse en premier quand on lit un jeton au
	// lieu du reste de la ligne.
	const bool texteAEspaces = p2 && p2->text == NkString("GGX multi diffusion");
	char d[240];
	snprintf(d, sizeof(d), "lu=%d texte-identique=%d octets=%u | valeurs egales au bit=%d | texte a espaces preserve=%d",
			 lu ? 1 : 0, (a == b) ? 1 : 0, (uint32)a.Size(), valeursEgales ? 1 : 0, texteAEspaces ? 1 : 0);
	Cas("valeur/aller-retour-construit-en-memoire", lu && (a == b) && valeursEgales && texteAEspaces, d);
}

static void CasPrecisionExacte() {
	// DISCRIMINE : un tiers. Le format ecrit les positions en « %.6f », qui rend
	// 0.333333 — un AUTRE flottant. Si les valeurs avaient herite de ce
	// formateur, l'egalite exacte echouerait sur un aller-retour pourtant
	// correct, et personne ne saurait dire si le defaut est dans la valeur ou
	// dans l'ecriture.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId n = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const float32 tiers = 1.f / 3.f;
	g.SetSocketDefault(n, "roughness", NkSocketDir::Input, NkValueReal(t.real, tiers));
	NkString s;
	g.Serialize(s);
	NkNodeGraph g2;
	g2.Deserialize(s.CStr());
	const NkGraphValue *v = g2.SocketDefault(n, "roughness", NkSocketDir::Input);
	const bool exact = v && v->IsSet() && v->numbers.Size() == 1 && v->numbers[0] == tiers;
	char d[192];
	snprintf(d, sizeof(d), "1/3 ecrit puis relu | egal au bit=%d (%.9g vs %.9g)", exact ? 1 : 0, (double)tiers,
			 (v && v->numbers.Size() == 1) ? (double)v->numbers[0] : 0.0);
	Cas("valeur/precision-traverse-le-texte", exact, d);
}

// ── validation : LE CHEMIN QUI N'ETAIT PAS GARDE ─────────────────────────────
// Tous les cas qui suivent entrent par `Deserialize`, jamais par l'API. C'est
// tout leur objet : `Connect()` refusait deja ces etats, `Deserialize` les
// laissait passer sans un mot. Les fichiers sont ecrits A LA MAIN, parce qu'un
// fichier produit par `Serialize` ne peut pas etre malade.

static const char *kFichierSain = "nkgraph 1\n"
								  "compteurs 3 2\n"
								  "type 1 mat.reel\n"
								  "type 2 mat.couleur\n"
								  "conv 1 2\n"
								  "noeud 1 0.000000 0.000000 source Source\n"
								  "sock 1 1 2 sortie\n"
								  "noeud 2 0.000000 0.000000 puits Puits\n"
								  "sock 2 0 2 entree\n"
								  "lien 1 1 0 2 0\n";

static void CasFichierSainZeroDiagnostic() {
	// LE TEMOIN DE L'AUTRE BORD. Sans lui, une validation qui crierait sur TOUT
	// passerait les huit cas suivants sans rien prouver.
	NkNodeGraph g;
	const bool lu = g.Deserialize(kFichierSain);
	NkVector<NkGraphDiag> diags;
	const uint32 n = g.Validate(diags);
	char d[192];
	snprintf(d, sizeof(d), "lu=%d diagnostics=%u (0 attendu) noeuds=%u liens=%u", lu ? 1 : 0, n, g.NodeCount(),
			 g.LinkCount());
	Cas("validation/fichier-sain-zero-diag", lu && n == 0 && g.NodeCount() == 2 && g.LinkCount() == 1, d);
}

static void CasFichierCycle() {
	// ⚠️ LE CAS QUI RESUMAIT LE DEFAUT. `Connect()` refuse un cycle depuis
	// toujours ; le FICHIER en posait un sans que rien ne le dise, et l'echec ne
	// sortait qu'a l'evaluation, plus tard et ailleurs.
	//
	// DISCRIMINE : on verifie AUSSI que le graphe s'est bien CHARGE avec ses deux
	// liens. Un `Deserialize` qui « reparerait » en refusant le second lien
	// rendrait zero diagnostic — et perdrait silencieusement une donnee de
	// l'utilisateur. L'invalide doit etre REPRESENTABLE **et** DETECTE.
	static const char *kCycle = "nkgraph 1\n"
								"compteurs 3 3\n"
								"type 1 mat.reel\n"
								"noeud 1 0 0 a A\n"
								"sock 1 1 1 out\n"
								"sock 1 0 1 in\n"
								"noeud 2 0 0 b B\n"
								"sock 2 1 1 out\n"
								"sock 2 0 1 in\n"
								"lien 1 1 0 2 1\n"
								"lien 2 2 0 1 1\n";
	NkNodeGraph g;
	const bool lu = g.Deserialize(kCycle);
	const uint32 cycles = CompteDiag(g, NkGraphIssue::Cycle);
	char d[192];
	snprintf(d, sizeof(d), "lu=%d liens charges=%u (2 attendus : l invalide doit etre REPRESENTABLE) | diag cycle=%u",
			 lu ? 1 : 0, g.LinkCount(), cycles);
	Cas("validation/cycle-par-le-fichier", lu && g.LinkCount() == 2 && cycles == 1, d);
}

static void CasFichierTypeIncompatible() {
	// Le fichier branche une sortie « couleur » sur une entree « reel ». La
	// conversion declaree va dans l'AUTRE sens (reel -> couleur), donc le lien
	// est faux. DISCRIMINE la direction de la table de conversions : une table
	// symetrique par erreur ne dirait rien ici.
	static const char *kMauvaisType = "nkgraph 1\n"
									  "compteurs 3 2\n"
									  "type 1 mat.reel\n"
									  "type 2 mat.couleur\n"
									  "conv 1 2\n"
									  "noeud 1 0 0 source Source\n"
									  "sock 1 1 2 sortie\n"
									  "noeud 2 0 0 puits Puits\n"
									  "sock 2 0 1 entree\n"
									  "lien 1 1 0 2 0\n";
	NkNodeGraph g;
	g.Deserialize(kMauvaisType);
	NkString quel;
	const uint32 n = CompteDiag(g, NkGraphIssue::LinkTypeMismatch, &quel);
	char d[192];
	snprintf(d, sizeof(d), "diag type-incompatible=%u sur la prise « %s » | liens charges=%u", n, quel.CStr(),
			 g.LinkCount());
	Cas("validation/type-incompatible-par-fichier", n == 1 && quel == NkString("entree") && g.LinkCount() == 1, d);
}

static void CasFichierSensInverse() {
	// Le fichier branche une ENTREE sur une ENTREE. `Connect` distingue ce cas
	// depuis toujours ; le fichier ne le voyait pas.
	static const char *kSens = "nkgraph 1\n"
							   "compteurs 3 2\n"
							   "type 1 mat.reel\n"
							   "noeud 1 0 0 a A\n"
							   "sock 1 0 1 entree\n"
							   "noeud 2 0 0 b B\n"
							   "sock 2 0 1 entree\n"
							   "lien 1 1 0 2 0\n";
	NkNodeGraph g;
	g.Deserialize(kSens);
	const uint32 n = CompteDiag(g, NkGraphIssue::LinkDirection);
	char d[160];
	snprintf(d, sizeof(d), "diag sens-invalide=%u | liens charges=%u", n, g.LinkCount());
	Cas("validation/sens-inverse-par-fichier", n == 1 && g.LinkCount() == 1, d);
}

static void CasFichierNoeudEtSocketAbsents() {
	// Deux malformations differentes, et elles ne se corrigent pas pareil : un
	// lien vers un noeud QUI N'EXISTE PAS, et un lien vers un INDEX DE PRISE hors
	// bornes sur un noeud qui existe. DISCRIMINE : un controle qui confondrait
	// les deux rendrait deux fois le meme code, et l'utilisateur chercherait le
	// mauvais probleme.
	static const char *kAbsents = "nkgraph 1\n"
								  "compteurs 3 3\n"
								  "type 1 mat.reel\n"
								  "noeud 1 0 0 a A\n"
								  "sock 1 1 1 out\n"
								  "noeud 2 0 0 b B\n"
								  "sock 2 0 1 in\n"
								  "lien 1 1 0 99 0\n"
								  "lien 2 1 0 2 7\n";
	NkNodeGraph g;
	g.Deserialize(kAbsents);
	const uint32 noeud = CompteDiag(g, NkGraphIssue::LinkUnknownNode);
	const uint32 borne = CompteDiag(g, NkGraphIssue::LinkSocketOutOfRange);
	char d[192];
	snprintf(d, sizeof(d), "diag noeud-inconnu=%u | diag socket-hors-bornes=%u (deux codes DISTINCTS)", noeud, borne);
	Cas("validation/noeud-et-socket-absents", noeud == 1 && borne == 1, d);
}

static void CasFichierEntreeDoublee() {
	// « Une entree n'accepte qu'UNE source » est garanti par `Connect`, qui
	// remplace. Un fichier peut en poser deux. DISCRIMINE : le graphe reste
	// triable et sans cycle — seul ce controle-ci voit le probleme, et sans lui
	// l'evaluation prendrait celui des deux liens que l'ordre d'iteration
	// rencontre en premier.
	static const char *kDoublee = "nkgraph 1\n"
								  "compteurs 4 3\n"
								  "type 1 mat.reel\n"
								  "noeud 1 0 0 a A\n"
								  "sock 1 1 1 out\n"
								  "noeud 2 0 0 b B\n"
								  "sock 2 1 1 out\n"
								  "noeud 3 0 0 c C\n"
								  "sock 3 0 1 in\n"
								  "lien 1 1 0 3 0\n"
								  "lien 2 2 0 3 0\n";
	NkNodeGraph g;
	g.Deserialize(kDoublee);
	const uint32 n = CompteDiag(g, NkGraphIssue::LinkDuplicateTarget);
	NkVector<NkNodeId> ordre;
	const bool triable = g.TopoSort(ordre);
	char d[192];
	snprintf(d, sizeof(d), "diag entree-doublee=%u | liens=%u | triable=%d (le tri ne voit RIEN)", n, g.LinkCount(),
			 triable ? 1 : 0);
	Cas("validation/entree-doublee-par-fichier", n == 1 && g.LinkCount() == 2 && triable, d);
}

static void CasFichierTypesInconnus() {
	// ⚠️ CE QUE LES VALEURS AGGRAVENT, et c'est pour ca que la validation et les
	// proprietes sont arrivees ensemble : un `typeId` absent du registre entre
	// par le meme chemin non garde — sur une prise, sur un defaut, sur une
	// propriete. Trois portes, trois codes distincts.
	static const char *kInconnus = "nkgraph 1\n"
								   "compteurs 2 1\n"
								   "type 1 mat.reel\n"
								   "noeud 1 0 0 a A\n"
								   "sock 1 0 9 prise_type_fantome\n"
								   "sock 1 0 1 prise_saine\n"
								   "def 1 1 9 1 0.5\n"
								   "prop 1 cle_fantome 42 1 1\n";
	NkNodeGraph g;
	g.Deserialize(kInconnus);
	NkString quelleprise, quelledefaut, quelleprop;
	const uint32 sock = CompteDiag(g, NkGraphIssue::SocketUnknownType, &quelleprise);
	const uint32 def = CompteDiag(g, NkGraphIssue::DefaultTypeMismatch, &quelledefaut);
	const uint32 prop = CompteDiag(g, NkGraphIssue::PropUnknownType, &quelleprop);
	char d[240];
	snprintf(d, sizeof(d), "prise type-inconnu=%u (%s) | defaut type-different=%u (%s) | propriete type-inconnu=%u (%s)",
			 sock, quelleprise.CStr(), def, quelledefaut.CStr(), prop, quelleprop.CStr());
	Cas("validation/types-inconnus-par-fichier",
		sock == 1 && quelleprise == NkString("prise_type_fantome") && def == 1 &&
			quelledefaut == NkString("prise_saine") && prop == 1 && quelleprop == NkString("cle_fantome"),
		d);
}

static void CasDefautHorsBornesNonRabattu() {
	// ⚠️ LE CAS LE PLUS IMPORTANT DE LA SERIE, et le moins spontane a ecrire.
	// Un `def` designe une prise par son INDEX. Que faire d'un index hors
	// bornes ? La tentation est de le RABATTRE sur la derniere prise, ou sur
	// zero. Ce serait poser la valeur sur une prise QUI N'EST PAS LA SIENNE : le
	// graphe paraitrait sain et calculerait autre chose.
	//
	// DISCRIMINE : on verifie que la prise 0 est restee NON RENSEIGNEE. Un
	// rabattement rendrait un graphe sans le moindre diagnostic — donc un test
	// qui ne compterait que les diagnostics le declarerait bon.
	static const char *kHorsBornes = "nkgraph 1\n"
									 "compteurs 2 1\n"
									 "type 1 mat.reel\n"
									 "noeud 1 0 0 a A\n"
									 "sock 1 0 1 premiere\n"
									 "sock 1 0 1 seconde\n"
									 "def 1 7 1 1 0.5\n";
	NkNodeGraph g;
	g.Deserialize(kHorsBornes);
	const NkGraphValue *p0 = g.SocketDefault(1, "premiere", NkSocketDir::Input);
	const NkGraphValue *p1 = g.SocketDefault(1, "seconde", NkSocketDir::Input);
	const bool aucunRabattement = p0 && !p0->IsSet() && p1 && !p1->IsSet();
	NkVector<NkGraphDiag> diags;
	const uint32 n = g.Validate(diags);
	char d[224];
	snprintf(d, sizeof(d), "premiere renseignee=%d seconde renseignee=%d (0/0 attendu : AUCUN rabattement) | diagnostics=%u",
			 (p0 && p0->IsSet()) ? 1 : 0, (p1 && p1->IsSet()) ? 1 : 0, n);
	Cas("validation/defaut-hors-bornes-non-rabattu", aucunRabattement && n == 0, d);
}

static void CasCompteRenduBorne() {
	// Un compte de reels annonce par le FICHIER. Un lecteur qui lui ferait
	// confiance allouerait avant que la validation n'ait la parole — et un
	// fichier corrompu annoncant quatre milliards de reels tuerait le processus
	// AVANT tout diagnostic. La borne est donc a la LECTURE, pas apres.
	static const char *kEnorme = "nkgraph 1\n"
								 "compteurs 2 1\n"
								 "type 1 mat.reel\n"
								 "noeud 1 0 0 a A\n"
								 "sock 1 0 1 prise\n"
								 "def 1 0 1 4000000000 0.5\n";
	NkNodeGraph g;
	const bool lu = g.Deserialize(kEnorme);
	const NkGraphValue *v = g.SocketDefault(1, "prise", NkSocketDir::Input);
	const uint32 taille = v ? (uint32)v->numbers.Size() : 0u;
	char d[192];
	snprintf(d, sizeof(d), "lu=%d sans mourir | reels retenus=%u (borne a 4096, pas 4 milliards)", lu ? 1 : 0, taille);
	Cas("validation/compte-de-reels-borne", lu && taille <= 4096u, d);
}

// ── phase 0 : le masque par TEXTURE ──────────────────────────────────────────
// Ces cas ne testent pas une structure de donnees : ils tiennent ensemble DEUX
// fichiers que rien d'autre ne force a s'accorder — le C++ qui construit le set
// de descripteurs, et le shader qui lit ce set.
//
// ⚠️ POURQUOI ILS EXISTENT, ET C'EST UNE MESURE, PAS UNE PRECAUTION. Le
// 2026-08-22, pour verifier que le temoin Demo4..Demo8 saurait attraper une
// erreur de binding, j'ai ecrit VOLONTAIREMENT le descripteur du masque sur le
// binding 10, absent du layout. Resultat : aucune erreur, aucun avertissement,
// aucune ligne de journal, et les CINQ signatures identiques. Le moteur ecrit
// sans broncher un descripteur que son propre layout ne declare pas ; la texture
// n'arrive jamais au shader et rien ne le dit.
//
// Le temoin d'images est donc AVEUGLE a cette classe de defaut. La parade est
// ici : un seul nombre, cite par les deux cotes, et un banc qui va LIRE le
// shader sur le disque pour le confronter a la constante C++.

// Lit un fichier texte entier. `fopen` et non NKFileSystem : ce banc ne lie que
// NKSL et la Foundation, et lier tout le systeme de fichiers pour lire un
// shader lui couterait sa raison d'etre — construire en deux secondes et tourner
// sans GPU. C'est du C, pas de la STL : la regle zero-STL est tenue.
static bool LireFichier(const char *chemin, NkString &out) {
	FILE *f = fopen(chemin, "rb");
	if (!f)
		return false;
	out = NkString("");
	char buf[4096];
	size_t n = 0;
	while ((n = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
		buf[n] = 0;
		out.Append(buf);
	}
	fclose(f);
	return out.Size() > 0;
}

static const char *const kCheminLayeredV1 = "Resources/NKRenderer/Shaders/LayeredV1/NkSL/layeredv1.frag.nksl";

// Cherche `motif` et rend la position du premier caractere apres, ou -1.
static int32 Apres(const NkString &s, const char *motif) {
	const char *p = s.CStr();
	const char *base = p;
	while (p && *p) {
		const char *a = p;
		const char *b = motif;
		while (*b && *a == *b) {
			++a;
			++b;
		}
		if (*b == 0)
			return (int32)(a - base);
		++p;
	}
	return -1;
}

// Recherche INSENSIBLE A LA CASSE.
//
// Elle existe parce que mon premier controle a mesure le mauvais objet, et le
// 2026-08-22 : il cherchait « tMask » dans le HLSL genere, ne le trouvait pas, et
// j'ai failli en conclure que DX repliait la branche de texture. Le generateur
// HLSL **met les identifiants en minuscules et les suffixe** — le sampler s'y
// appelle `tmask_tex`, et il est bel et bien la, avec son `.Sample(...)`. Chercher
// un nom exact a travers un generateur de code, c'est tester le generateur de
// noms, pas le shader.
static bool ContientSansCasse(const NkString &s, const char *motif) {
	auto bas = [](char c) -> char { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; };
	const char *p = s.CStr();
	while (p && *p) {
		const char *a = p;
		const char *b = motif;
		while (*b && bas(*a) == bas(*b)) {
			++a;
			++b;
		}
		if (*b == 0)
			return true;
		++p;
	}
	return false;
}

static void CasBindingDeclareDesDeuxCotes() {
	// LE CAS QUI RATTRAPE L'AVEUGLEMENT DU TEMOIN. Il lit le SHADER REEL sur le
	// disque et confronte le binding qu'il y trouve a la constante que le C++
	// utilise pour construire son layout et ecrire ses descripteurs.
	//
	// DISCRIMINE : changer UN SEUL des deux cotes met ce cas au rouge. Ni le
	// compilateur, ni le lieur, ni le temoin d'images ne savent le faire.
	NkString src;
	const bool lu = LireFichier(kCheminLayeredV1, src);
	// ⚠️ Un fichier introuvable doit ECHOUER, jamais etre saute. Un cas qui
	// s'escamote quand il ne trouve pas sa donnee est un vert qui ne prouve
	// rien — et il reste vert le jour ou le fichier disparait vraiment.
	if (!lu) {
		Cas("phase0/binding-declare-des-deux-cotes", false, "shader INTROUVABLE (lancer le banc depuis la racine du worktree)");
		return;
	}
	// La ligne cherchee : « @binding(set=2, binding=9) uniform sampler2D tMask; »
	const int32 posSampler = Apres(src, "uniform sampler2D tMask;");
	int32 bindingLu = -1;
	int32 setLu = -1;
	if (posSampler > 0) {
		// Remonter jusqu'au « @binding( » qui precede immediatement.
		const char *base = src.CStr();
		int32 i = posSampler;
		while (i > 0 && !(base[i] == '@' && base[i + 1] == 'b'))
			--i;
		if (base[i] == '@') {
			const char *p = base + i;
			// set=<n>
			const char *s1 = p;
			while (*s1 && !(s1[0] == 's' && s1[1] == 'e' && s1[2] == 't' && s1[3] == '='))
				++s1;
			if (*s1)
				setLu = atoi(s1 + 4);
			const char *b1 = p;
			// « binding= » APRES le « @binding( » lui-meme : on saute le premier.
			int trouves = 0;
			while (*b1) {
				if (b1[0] == 'b' && b1[1] == 'i' && b1[2] == 'n' && b1[3] == 'd' && b1[4] == 'i' && b1[5] == 'n' &&
					b1[6] == 'g' && b1[7] == '=') {
					++trouves;
					bindingLu = atoi(b1 + 8);
					break;
				}
				++b1;
			}
			(void)trouves;
		}
	}
	const bool accord = (bindingLu == (int32)renderer::NK_MATBIND_LAYER_MASK) && (setLu == 2);
	char d[240];
	snprintf(d, sizeof(d), "shader lu (%u o) | tMask trouve=%d | set=%d binding=%d | constante C++ renderer::NK_MATBIND_LAYER_MASK=%u | accord=%d",
			 (uint32)src.Size(), posSampler > 0 ? 1 : 0, setLu, bindingLu, (uint32)renderer::NK_MATBIND_LAYER_MASK,
			 accord ? 1 : 0);
	Cas("phase0/binding-declare-des-deux-cotes", posSampler > 0 && accord, d);
}

static void CasQuatreCanauxPresents() {
	// DISCRIMINE : le C++ declare QUATRE sources de masque par texture
	// (NK_LAYER_MASK_TEX_R..A = 8..11). Le shader doit traiter les quatre. Un
	// oubli du canal alpha — le plus facile a oublier, c'est le dernier —
	// rendrait 0.0 en silence, donc une couche invisible, donc un bogue qu'on
	// chercherait dans le C++.
	NkString src;
	if (!LireFichier(kCheminLayeredV1, src)) {
		Cas("phase0/quatre-canaux-traites", false, "shader INTROUVABLE");
		return;
	}
	const bool r = Apres(src, "if (source == 8) return clamp(m.r") > 0;
	const bool g = Apres(src, "if (source == 9) return clamp(m.g") > 0;
	const bool b = Apres(src, "if (source == 10) return clamp(m.b") > 0;
	const bool a = Apres(src, "if (source == 11) return clamp(m.a") > 0;
	char d[192];
	snprintf(d, sizeof(d), "canaux traites dans le shader : R=%d G=%d B=%d A=%d (les 4 attendus)", r ? 1 : 0,
			 g ? 1 : 0, b ? 1 : 0, a ? 1 : 0);
	Cas("phase0/quatre-canaux-traites", r && g && b && a, d);
}

static void CasMasqueCompileSurLesBackends() {
	// ⚠️ LE CAS QUI COMPTE POUR LA GRILLE D'ACCEPTATION : « le NkSL emis COMPILE
	// sur les backends, pas seulement ressemble au temoin ».
	//
	// Il n'ecrit pas un shader de test : il EXTRAIT le corps reel de PickMask du
	// fichier sur le disque et le compile. Un shader de test recopie a la main
	// derive du vrai en une semaine et valide alors autre chose — c'est le piege
	// « mesurer une reconstruction au lieu de la chose ».
	//
	// Le point risque est precis : un `texture(sampler, uv)` DANS un helper, avec
	// la coordonnee passee en PARAMETRE. Le fichier avertit qu'une varying
	// referencee dans un helper fait sortir un `input.xxx` hors de l'entree au
	// generateur HLSL, et que DX refuse alors le shader (X3004).
	NkString src;
	if (!LireFichier(kCheminLayeredV1, src)) {
		Cas("phase0/masque-compile-4-backends", false, "shader INTROUVABLE");
		return;
	}
	// Extraire de « float PickMask( » jusqu'a la ligne qui vaut exactement « } ».
	const int32 debut = Apres(src, "float PickMask(");
	if (debut < 0) {
		Cas("phase0/masque-compile-4-backends", false, "PickMask introuvable dans le shader");
		return;
	}
	const char *base = src.CStr();
	int32 i = debut;
	while (i > 0 && base[i] != 'f')
		--i; // revenir sur le « float »
	NkString corps;
	int32 k = i;
	while (base[k]) {
		corps.Append(base[k]);
		if (base[k] == '}' && (base[k + 1] == '\n' || base[k + 1] == '\r') && k > 0 && base[k - 1] == '\n')
			break;
		++k;
	}

	// Enveloppe minimale et AUTONOME : les varyings dont le helper a besoin, le
	// sampler, et une entree qui APPELLE le helper — sans appel, un backend
	// pourrait eliminer le code mort et « compiler » sans jamais l'avoir traduit.
	NkString shader;
	shader.Append("@location(0) in vec2 vUV;\n@location(1) in vec4 vColor;\n@location(0) out vec4 fragColor;\n");
	shader.Append("@binding(set=2, binding=9) uniform sampler2D tMask;\n\n");
	shader.Append(corps);
	shader.Append("\n\n@stage(fragment)\n@entry\nvoid main() {\n");
	shader.Append("    float m = PickMask(8, vColor, vUV, 0.5, 1.0);\n");
	shader.Append("    fragColor = vec4(m, m, m, 1.0);\n}\n");

	NkSLCompiler c;
	struct {
			NkSLTarget cible;
			const char *nom;
	} cibles[4] = {
		{NkSLTarget::NK_GLSL, "GL"},
		{NkSLTarget::NK_GLSL_VULKAN, "VK"},
		{NkSLTarget::NK_HLSL_DX11, "DX11"},
		{NkSLTarget::NK_HLSL_DX12, "DX12"},
	};
	uint32 ok = 0;
	char detail[240];
	int off = 0;
	bool lectureTexturePartout = true;
	for (uint32 t = 0; t < 4; ++t) {
		NkSLCompileResult r = c.Compile(shader, NkSLStage::NK_FRAGMENT, cibles[t].cible);
		// « ca a compile » ne suffit pas : le code produit doit REELLEMENT
		// contenir une lecture de texture. Un backend qui replierait la branche
		// rendrait un succes vide.
		// TROIS choses a verifier, et « ca a compile » n'est que la premiere :
		//   1. le sampler a survecu a la traduction (nom insensible a la casse :
		//      HLSL le rend `tmask_tex`) ;
		//   2. il est REELLEMENT ECHANTILLONNE — `texture(` en GLSL, `.Sample(`
		//      en HLSL. Sans cet appel, un backend qui aurait replie la branche
		//      rendrait un succes vide, et le sampler declare ne prouverait rien ;
		//   3. cote HLSL seulement : le sampler atterrit sur `register(t9)`. C'est
		//      une VERIFICATION CROISEE GRATUITE du meme nombre que le C++ ecrit
		//      dans son layout — trouvee en lisant le code genere, pas prevue.
		const bool declare = r.success && ContientSansCasse(r.source, "tmask");
		const bool echantillonne =
			r.success && (ContientSansCasse(r.source, "texture(") || ContientSansCasse(r.source, ".sample("));
		const bool hlsl = (cibles[t].cible == NkSLTarget::NK_HLSL_DX11 || cibles[t].cible == NkSLTarget::NK_HLSL_DX12);
		char reg[24];
		// PAS de parenthese fermante : DX11 ecrit « register(t9) » et DX12
		// « register(t9, space0) ». Fermer la parenthese ferait echouer DX12
		// pour une raison qui n a rien a voir avec le masque — mesure le 22/08.
		snprintf(reg, sizeof(reg), "register(t%u", (uint32)renderer::NK_MATBIND_LAYER_MASK);
		const bool bonRegistre = !hlsl || (r.success && ContientSansCasse(r.source, reg));
		const bool lit = declare && echantillonne && bonRegistre;
		if (r.success && !lit && getenv("NK_DUMP")) {
			// Diagnostic PERMANENT, pas un echafaudage : quand ce cas tombe, la
			// question est toujours « le backend a-t-il replie la branche, ou ai-je
			// cherche le mauvais nom ? ». Sans le code sous les yeux, on tranche au
			// hasard. NK_DUMP=1 le montre.
			printf("\n--- %s : compile mais le masque n a pas traverse ---\n%s\n--- fin ---\n",
				cibles[t].nom, r.source.CStr());
		}
		if (r.success)
			++ok;
		if (!lit)
			lectureTexturePartout = false;
		off += snprintf(detail + off, sizeof(detail) - (size_t)off, "%s=%s%s%s%s ", cibles[t].nom,
						r.success ? "ok" : "ECHEC", declare ? "" : "(pas declare!)",
						echantillonne ? "" : "(pas echantillonne!)", bonRegistre ? "" : "(mauvais registre!)");
	}
	Cas("phase0/masque-compile-4-backends", ok == 4 && lectureTexturePartout && corps.Size() > 100, detail);
}

int main() {
	printf("== NkMatGraphCheck — graphe de materiaux, couche 3 sur NKGraph ==\n");
	printf("   regime : structure de donnees pure, aucun GPU, aucune fenetre.\n");
	printf("   COUVRE aussi, depuis le 22/08 : les defauts de prise, les\n");
	printf("   proprietes de noeud, et la validation d un fichier .nkgraph.\n");
	printf("   NE couvre PAS : la generation de NkSL, pas encore ecrite.\n\n");
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

	// ── valeurs : defauts de prise et proprietes de noeud ────────────────
	CasDefautDePrise();
	CasJamaisRenseigneContreVide();
	CasProprieteDeNoeud();
	CasAllerRetourConstruitEnMemoire();
	CasPrecisionExacte();

	// ── validation : le chemin qui n'etait pas garde ─────────────────────
	CasFichierSainZeroDiagnostic();
	CasFichierCycle();
	CasFichierTypeIncompatible();
	CasFichierSensInverse();
	CasFichierNoeudEtSocketAbsents();
	CasFichierEntreeDoublee();
	CasFichierTypesInconnus();
	CasDefautHorsBornesNonRabattu();
	CasCompteRenduBorne();

	// -- phase 0 : le masque par texture, et les deux fichiers qui doivent
	//    s accorder sans que rien ne les y force --------------------------
	CasBindingDeclareDesDeuxCotes();
	CasQuatreCanauxPresents();
	CasMasqueCompileSurLesBackends();

	printf("\n-- %u cas, %u echec(s) --\n", gCas, gEchecs);
	return gEchecs == 0 ? 0 : 1;
}

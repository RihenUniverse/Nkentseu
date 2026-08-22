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
#include "NKRenderer/Materials/Graph/NkMatGraphCompile.h"
// Les numeros de binding du set materiau. En-tete SANS DEPENDANCE, fait pour
// etre lisible par un banc qui ne lie ni NKRenderer ni NKRHI.
#include "NKRenderer/Materials/NkMaterialBindings.h"
// Le VRAI compilateur NkSL : c est lui qui dit si le shader traverse les
// quatre backends, et il n a besoin d aucun device (cf. NkSLCheck).
#include "NKSL/Compiler/NkSLCompiler.h"

// ── POURQUOI CE BANC N'IMPRIME PLUS AVEC printf ─────────────────────────────
// Rodolf, 2026-08-22 (et c'est la deuxieme fois) : « ne pas utiliser directement
// printf, le systeme definit des loggers, pourquoi ne pas les utiliser ? »
//
// ⚠️ ET PASSER A `Infof` N'AURAIT RIEN CORRIGE. `Infof` prend une chaine de
// format et des arguments variadiques NON TYPES — la meme mecanique que printf.
// Le defaut mesure la meme nuit par l'agent NK3DModeler — un printf reclamant
// sept `%u` pour six arguments, le septieme lisant la pile et affichant
// « nonmanif=18 » sur un cube parfaitement manifold — se reproduit a l'identique
// avec `Infof`. Rien ne plante, rien n'avertit, ET LE NOMBRE EST CREDIBLE.
//
// Seule la forme POSITIONNELLE change la nature du probleme : `NkFormat("{0}")`
// capture les arguments PAR LEUR TYPE, jamais reinterpretes. S'il en manque un,
// on obtient un TROU VISIBLE, pas un entier plausible. Pour un banc dont toute
// la valeur tient dans les nombres qu'il imprime, c'est la seule propriete qui
// compte : un trou se voit ; un « 18 » plausible se recopie dans un rapport,
// puis dans une ROADMAP, puis dans une decision.
//
//   `-Werror=format` (pose dans le .jenga) DETECTE le desalignement.
//   La forme `{0}` le rend IRREPRESENTABLE. On garde les deux : le drapeau est
//   un filet pour ce qui resterait, la forme est la solution.
//
// Le motif de journal est « %v » : le message SEUL, sans horodatage ni fichier.
// La sortie d'un banc EST son resultat ; une decoration la rendrait illisible et
// instable d'une execution a l'autre.
//
// ⚠️ Et les chaines IMPRIMEES sont en ASCII : le puits console ne transporte pas
// l'UTF-8 (mesure — « — » sortait « - », les guillemets sortaient « ? »). Mieux
// vaut ecrire ce qui sera lu que laisser des caracteres se perdre en chemin. Les
// commentaires, eux, gardent leur typographie : ils ne sont jamais imprimes.
#include "NKContainers/String/NkFormat.h"
#include "NKLogger/NkLog.h"
#include <stdlib.h>
#include <string.h> // strlen : du C, pas de la STL

using namespace nkentseu;
using namespace nkentseu::graph;
using namespace nkentseu::renderer::matgraph;

static uint32 gCas = 0;
static uint32 gEchecs = 0;

static void Cas(const char *nom, bool ok, const NkString &detail) {
	++gCas;
	if (!ok)
		++gEchecs;
	logger.Info("{0:<34} {1} | {2}", NkString(nom), NkString(ok ? "OK  " : "ECHEC"), detail);
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
	NkString d;
	d = NkFormat("reel={0} vect={1} coul={2} shader={3} | distincts={4} retrouves-par-nom={5}", t.real, t.vector, t.color, t.shader, distincts ? 1 : 0, retrouves ? 1 : 0);
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
	NkString d;
	d = NkFormat("reel>couleur={0} couleur>reel={1} | couleur>shader={2} shader>couleur={3}", reelVersCouleur ? "ok" : "REFUSE", couleurVersReel ? "ACCEPTE" : "refuse", couleurVersShader ? "ACCEPTE" : "refuse", shaderVersCouleur ? "ACCEPTE" : "refuse");
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
	NkString d;
	d = NkFormat("noeud={0} prises={1} base_color={2}(type-ok={3}) bsdf-sortie={4} bsdf-entree={5}(-1 attendu)", n, nd ? (uint32)nd->sockets.Size() : 0u, baseColor, bonType ? 1 : 0, bsdfOut, bsdfIn);
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
	NkString d;
	d = NkFormat("rendu={0} (0 attendu) noeuds-dans-le-graphe={1} (0 attendu)", n, g.NodeCount());
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
	NkString d;
	d = NkFormat("rendu={0} (0 attendu) noeuds={1} (0 attendu)", n, g.NodeCount());
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
	NkString d;
	d = NkFormat("lien={0} ordre={1} (bsdf avant sortie) validation={2} sortie-trouvee={3}", NkLinkErrorName(e), bonOrdre ? "bsdf>sortie" : "MAUVAIS", NkMatGraphErrorName(v), trouve == out ? 1 : 0);
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
	NkString d;
	d = NkFormat("temoin-positif(emission>surface)={0} | entree>entree={1} | liens={2} (1 attendu)", NkLinkErrorName(typeFaux), NkLinkErrorName(sensFaux), liens);
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
	NkString d;
	d = NkFormat("1 sortie={0} | 2 sorties={1} (coeur triable={2}, il ne voit rien) | apres retrait={3}", NkMatGraphErrorName(avant), NkMatGraphErrorName(apres), coeurContent ? 1 : 0, NkMatGraphErrorName(apresRetrait));
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
	NkString d;
	d = NkFormat("validation={0} | index du socket surface={1} (0 : la sentinelle a zero serait un BUG)", NkMatGraphErrorName(v), idx);
	Cas("graphe/sortie-non-reliee", v == NkMatGraphError::OutputUnlinked && idx == 0, d);
}

static void CasAucuneSortie() {
	// Le temoin de l'autre bord : sans lui, une validation qui refuserait TOUT
	// passerait les cas precedents sans rien prouver.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkMatGraphError v = NkMatValidate(g);
	NkString d;
	d = NkFormat("validation={0}", NkMatGraphErrorName(v));
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
	NkString d;
	d = NkFormat("liens={0}/{1}/{2} | positions diff={3} emis={4} mix={5} sortie={6} | validation={7}", NkLinkErrorName(e1), NkLinkErrorName(e2), NkLinkErrorName(e3), pDiff, pEmis, pMix, pOut, NkMatGraphErrorName(v));
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
	NkString d;
	d = NkFormat("lu={0} texte-identique={1} octets={2} validation={3} | semantique-survit={4} (types {5}/{6})", lu ? 1 : 0, identique ? 1 : 0, (uint32)a.Size(), NkMatGraphErrorName(v), semantique ? 1 : 0, r2, c2);
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
	NkString d;
	d = NkFormat("absente={0} | vierge non-renseignee={1} | pose={2} relu-exact={3} | pose sur absente refusee={4}", absente ? "POINTEUR (nullptr attendu)" : "nullptr", viergeNonRenseignee ? 1 : 0, pose ? 1 : 0, bonne ? 1 : 0, refuse ? 1 : 0);
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

	NkString d;
	d = NkFormat("proprietes posees={0} | aucune ligne def={1} aucune ligne prop={2} | absente apres relecture={3} | temoin vide-mais-posee : ligne ecrite={4} survit={5}", posees, aucunDef ? 1 : 0, aucunProp ? 1 : 0, absenteApres ? 1 : 0, uneSeuleLigne ? 1 : 0, survit ? 1 : 0);
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
	NkString d;
	d = NkFormat("1 pose={0} | 2 poses meme cle={1} (1 attendu) remplacee={2} | inconnue nullptr={3} | retiree={4} retrait-inconnu-refuse={5} voisine-intacte={6}", apres1, apres2, remplacee ? 1 : 0, inconnueNulle ? 1 : 0, retiree ? 1 : 0, retraitInconnu ? 1 : 0, resteIntacte ? 1 : 0);
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
	NkString d;
	d = NkFormat("lu={0} texte-identique={1} octets={2} | valeurs egales au bit={3} | texte a espaces preserve={4}", lu ? 1 : 0, (a == b) ? 1 : 0, (uint32)a.Size(), valeursEgales ? 1 : 0, texteAEspaces ? 1 : 0);
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
	NkString d;
	d = NkFormat("1/3 ecrit puis relu | egal au bit={0} ({1:.9g} vs {2:.9g})", exact ? 1 : 0, (double)tiers, (v && v->numbers.Size() == 1) ? (double)v->numbers[0] : 0.0);
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
	NkString d;
	d = NkFormat("lu={0} diagnostics={1} (0 attendu) noeuds={2} liens={3}", lu ? 1 : 0, n, g.NodeCount(), g.LinkCount());
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
	NkString d;
	d = NkFormat("lu={0} liens charges={1} (2 attendus : l invalide doit etre REPRESENTABLE) | diag cycle={2}", lu ? 1 : 0, g.LinkCount(), cycles);
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
	NkString d;
	d = NkFormat("diag type-incompatible={0} sur la prise '{1}' | liens charges={2}", n, quel.CStr(), g.LinkCount());
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
	NkString d;
	d = NkFormat("diag sens-invalide={0} | liens charges={1}", n, g.LinkCount());
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
	NkString d;
	d = NkFormat("diag noeud-inconnu={0} | diag socket-hors-bornes={1} (deux codes DISTINCTS)", noeud, borne);
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
	NkString d;
	d = NkFormat("diag entree-doublee={0} | liens={1} | triable={2} (le tri ne voit RIEN)", n, g.LinkCount(), triable ? 1 : 0);
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
	NkString d;
	d = NkFormat("prise type-inconnu={0} ({1}) | defaut type-different={2} ({3}) | propriete type-inconnu={4} ({5})", sock, quelleprise.CStr(), def, quelledefaut.CStr(), prop, quelleprop.CStr());
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
	NkString d;
	d = NkFormat("premiere renseignee={0} seconde renseignee={1} (0/0 attendu : AUCUN rabattement) | diagnostics={2}", (p0 && p0->IsSet()) ? 1 : 0, (p1 && p1->IsSet()) ? 1 : 0, n);
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
	NkString d;
	d = NkFormat("lu={0} sans mourir | reels retenus={1} (borne a 4096, pas 4 milliards)", lu ? 1 : 0, taille);
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
	NkString d;
	d = NkFormat("shader lu ({0} o) | tMask trouve={1} | set={2} binding={3} | constante C++ renderer::NK_MATBIND_LAYER_MASK={4} | accord={5}", (uint32)src.Size(), posSampler > 0 ? 1 : 0, setLu, bindingLu, (uint32)renderer::NK_MATBIND_LAYER_MASK, accord ? 1 : 0);
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
	NkString d;
	d = NkFormat("canaux traites dans le shader : R={0} G={1} B={2} A={3} (les 4 attendus)", r ? 1 : 0, g ? 1 : 0, b ? 1 : 0, a ? 1 : 0);
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
	NkString detail;
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
			// PAS de parenthese fermante : DX11 ecrit « register(t9) » et DX12
		// « register(t9, space0) ». Fermer la parenthese ferait echouer DX12
		// pour une raison qui n a rien a voir avec le masque — mesure le 22/08.
		const NkString reg = NkFormat("register(t{0}", (uint32)renderer::NK_MATBIND_LAYER_MASK);
		const bool bonRegistre = !hlsl || (r.success && ContientSansCasse(r.source, reg.CStr()));
		const bool lit = declare && echantillonne && bonRegistre;
		if (r.success && !lit && getenv("NK_DUMP")) {
			// Diagnostic PERMANENT, pas un echafaudage : quand ce cas tombe, la
			// question est toujours « le backend a-t-il replie la branche, ou ai-je
			// cherche le mauvais nom ? ». Sans le code sous les yeux, on tranche au
			// hasard. NK_DUMP=1 le montre.
			logger.Info("\n--- {0} : compile mais le masque n a pas traverse ---\n{1}\n--- fin ---", cibles[t].nom, r.source.CStr());
		}
		if (r.success)
			++ok;
		if (!lit)
			lectureTexturePartout = false;
		detail.Append(NkFormat("{0}={1}{2}{3}{4} ", NkString(cibles[t].nom),
							   NkString(r.success ? "ok" : "ECHEC"), NkString(declare ? "" : "(pas declare!)"),
							   NkString(echantillonne ? "" : "(pas echantillonne!)"),
							   NkString(bonRegistre ? "" : "(mauvais registre!)")));
	}
	Cas("phase0/masque-compile-4-backends", ok == 4 && lectureTexturePartout && corps.Size() > 100, detail);
}

// ── le compilateur : graphe -> NkSL -> quatre backends ───────────────────────

// Compile un shader NkSL sur les quatre backends et rend combien passent. Le
// vrai NkSLCompiler, sans device : c'est ce que NkSLCheck prouve deja faisable.
static uint32 CompileSurLesBackends(const NkString &nksl, NkString &detail, NkString *premiereErreur) {
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
	detail = NkString("");
	for (uint32 t = 0; t < 4; ++t) {
		NkSLCompileResult r = c.Compile(nksl, NkSLStage::NK_FRAGMENT, cibles[t].cible);
		if (r.success)
			++ok;
		else if (premiereErreur && premiereErreur->Size() == 0 && r.errors.Size() > 0)
			*premiereErreur = r.errors[0].message;
		detail.Append(NkFormat("{0}={1} ", NkString(cibles[t].nom), NkString(r.success ? "ok" : "ECHEC")));
	}
	return ok;
}

// Le graphe minimal REEL : un Principled avec ses defauts, vers la sortie.
static NkNodeId MonteUnPrincipled(NkNodeGraph &g, const NkMatTypes &t, NkNodeId *outSortie) {
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	const float32 rouge[3] = {0.8f, 0.15f, 0.1f};
	g.SetSocketDefault(bsdf, "base_color", NkSocketDir::Input, NkValueVec(t.color, rouge, 3));
	g.SetSocketDefault(bsdf, "metallic", NkSocketDir::Input, NkValueReal(t.real, 0.0f));
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 0.35f));
	if (outSortie)
		*outSortie = out;
	return bsdf;
}

static void CasCompilePrincipled() {
	// ⚠️ LE CAS CENTRAL DE LA GRILLE D'ACCEPTATION : « le NkSL emis COMPILE sur
	// les backends, pas seulement ressemble au temoin ».
	//
	// DISCRIMINE : on ne compare pas a un texte de reference. On donne le NkSL au
	// VRAI compilateur, quatre fois. Un emetteur qui produirait du texte
	// plausible mais invalide passerait n'importe quelle comparaison de chaines
	// et echouerait ici.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	MonteUnPrincipled(g, t, nullptr);
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	NkString be;
	NkString err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	// Et la valeur du defaut doit se retrouver DANS le code emis : un compilateur
	// qui oublierait de lire les defauts produirait un shader qui compile
	// parfaitement et rendrait du noir.
	const bool porteLeRouge = r.ok && ContientSansCasse(r.source, "0.800000012");
	NkString d;
	d = NkFormat("emis={0} ({1} o) | {2}| defaut de base_color present={3} {4}", r.ok ? 1 : 0, (uint32)r.source.Size(), be, porteLeRouge ? 1 : 0, r.ok ? "" : (r.error.Size() ? r.error.CStr() : ""));
	Cas("compile/principled-4-backends", r.ok && ok == 4 && porteLeRouge, d);
	if (ok != 4 && err.Size() > 0)
		logger.Info("      premiere erreur du backend : {0}", err.CStr());
}

static void CasCompileMixShader() {
	// LE NOEUD PAR LEQUEL TOUT A COMMENCE — « mixer les shader BSDF ». Deux BSDF
	// melanges vers une sortie, et le resultat doit traverser les quatre backends.
	//
	// DISCRIMINE : on verifie qu'il y a QUATRE `mix(` dans le code emis, un par
	// composante. Un melangeur qui n'en emettrait qu'un — la couleur, la plus
	// visible — donnerait un shader qui compile et perdrait la rugosite et le
	// metallique du second BSDF. C'est le genre de defaut qu'on ne voit pas sur
	// une sphere mate.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_SHADER);
	const NkNodeId diff = NkMatAddNode(g, NK_MN_DIFFUSE);
	const NkNodeId emis = NkMatAddNode(g, NK_MN_EMISSION);
	g.Connect(diff, "bsdf", mix, "shader1");
	g.Connect(emis, "emission", mix, "shader2");
	g.Connect(mix, "shader", out, "surface");
	const float32 bleu[3] = {0.1f, 0.3f, 0.9f};
	g.SetSocketDefault(diff, "color", NkSocketDir::Input, NkValueVec(t.color, bleu, 3));
	g.SetSocketDefault(diff, "roughness", NkSocketDir::Input, NkValueReal(t.real, 0.6f));
	const float32 chaud[3] = {1.0f, 0.6f, 0.2f};
	g.SetSocketDefault(emis, "color", NkSocketDir::Input, NkValueVec(t.color, chaud, 3));
	g.SetSocketDefault(emis, "strength", NkSocketDir::Input, NkValueReal(t.real, 2.0f));
	g.SetSocketDefault(mix, "fac", NkSocketDir::Input, NkValueReal(t.real, 0.5f));

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	// COMPTER LES « mix( » DU FICHIER ENTIER NE MARCHE PAS, et je l'ai appris en
	// le mesurant : le puits en emet DEUX pour son propre compte (`specExp` et
	// `specColor`). Ma premiere version attendait 5 et en a trouve 6 — la
	// prediction etait fausse, pas le code. Un controle qui vise « tous les mix
	// du shader » mesure le modele d'eclairage en meme temps que le melangeur,
	// et cassera au prochain reglage de l'ombrage sans que rien n'ait bouge.
	//
	// On compte donc les occurrences de la LOCALE DU FACTEUR de CE noeud :
	// 1 declaration + 4 usages, un par composante. Un melangeur qui n'emettrait
	// que la couleur — la plus visible — en donnerait 2, et perdrait en silence
	// la rugosite et le metallique du second BSDF.
	const NkString nomFac = NkFormat("n{0}_fac", (uint32)mix);
	uint32 nbFac = 0;
	if (r.ok) {
		const char *p = r.source.CStr();
		const size_t L = (size_t)nomFac.Size();
		while (*p) {
			size_t k = 0;
			while (k < L && p[k] == nomFac.CStr()[k])
				++k;
			if (k == L)
				++nbFac;
			++p;
		}
	}
	NkString be;
	NkString err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	// Le puits emet lui aussi un `mix` (specColor) : on en attend donc 4 + 1.
	NkString d;
	d = NkFormat("emis={0} ({1} o) | {2}| {3} cite {4} fois (1 declaration + 4 composantes = 5) {5}", r.ok ? 1 : 0, (uint32)r.source.Size(), be, nomFac, nbFac, r.ok ? "" : r.error.CStr());
	Cas("compile/melange-deux-bsdf-4-backends", r.ok && ok == 4 && nbFac == 5, d);
	// NK_DUMP=1 imprime le NkSL engendre. Ce n est pas un echafaudage oublie :
	// quand un cas de compilation tombe, la question est toujours « qu a-t-il
	// donc ecrit ? », et un banc qui ne sait pas le montrer oblige a rajouter
	// un printf puis a le retirer, a chaque fois.
	if (getenv("NK_DUMP") && r.ok)
		logger.Info("{0}", r.source.CStr());
	if (ok != 4 && err.Size() > 0)
		logger.Info("      premiere erreur du backend : {0}", err.CStr());
}

static void CasCompileOrdreRespecte() {
	// DISCRIMINE : une locale doit etre DECLAREE avant d'etre lue. C'est la seule
	// chose que l'ordre topologique garantit, et c'est exactement ce qu'un
	// emetteur qui parcourrait le graphe dans l'ordre d'insertion casserait.
	//
	// Les noeuds sont crees dans l'ordre INVERSE de leur dependance : la sortie
	// d'abord, les sources ensuite. Un emetteur naif ecrirait donc le puits en
	// premier, lirait `n2_albedo` avant sa declaration, et AUCUN backend ne
	// l'accepterait. C'est le compilateur NkSL qui tranche, pas une inspection
	// de texte.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out = NK_NODE_INVALID;
	const NkNodeId bsdf = MonteUnPrincipled(g, t, &out);
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	// La declaration du bsdf doit apparaitre AVANT sa lecture par le puits.
	const NkString nomAlbedo = NkFormat("n{0}_albedo", (uint32)bsdf);
	const int32 premiere = r.ok ? Apres(r.source, nomAlbedo.CStr()) : -1;
	const int32 lecture = r.ok ? Apres(r.source, "surfAlbedo = ") : -1;
	NkString be;
	NkString err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	NkString d;
	d = NkFormat("declaration de {0} a {1}, lecture du puits a {2} (declaration AVANT attendue) | {3}", nomAlbedo, premiere, lecture, be);
	Cas("compile/ordre-topologique-respecte", r.ok && ok == 4 && premiere > 0 && lecture > premiere, d);
}

static void CasCompileRefuseAvantDeGenerer() {
	// ⚠️ CE QUI SE PASSE QUAND LE GRAPHE EST MAUVAIS, et c'est la moitie du
	// travail d'un compilateur. Trois graphes fautifs, trois messages DISTINCTS,
	// et AUCUN shader emis. Un compilateur qui generait quand meme laisserait le
	// backend accuser une ligne de shader au lieu du noeud coupable.
	//
	// DISCRIMINE : les trois messages doivent DIFFERER. Un compilateur qui
	// rendrait « graphe invalide » pour tout passerait un test qui ne compterait
	// que les echecs.
	// CE CAS A SURVECU A SA PROPRE MUTATION, et la faute etait dans le cas. Sa
	// premiere version notait le message « seulement si !r.ok », puis ajoutait un
	// garde defensif : « si un shader a quand meme ete emis, message = SHADER
	// PARTIEL EMIS ». Sous la mutation « un noeud inconnu est SAUTE au lieu
	// d'etre refuse », la compilation reussissait, le garde defensif remplissait
	// le troisieme message, les trois restaient distincts, et le cas passait au
	// vert. **Le garde cense renforcer le cas est ce qui l'a rendu aveugle.**
	// On exige donc l'ECHEC lui-meme, pas la seule presence d'un message.
	NkString m1, m2, m3;
	bool e1 = false, e2 = false, e3 = false;
	bool rienEmis3 = false;
	{ // aucune sortie
		NkNodeGraph g;
		NkMatRegisterTypes(g);
		NkMatAddNode(g, NK_MN_PRINCIPLED);
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		e1 = !r.ok;
		m1 = r.error;
	}
	{ // sortie non reliee
		NkNodeGraph g;
		NkMatRegisterTypes(g);
		NkMatAddNode(g, NK_MN_OUTPUT);
		NkMatAddNode(g, NK_MN_PRINCIPLED);
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		e2 = !r.ok;
		m2 = r.error;
	}
	{ // un noeud dont le compilateur ne sait rien faire : il existe dans le
	  // coeur, il porte des prises valides, et pourtant il n'a pas d'emetteur.
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
		const NkNodeId inconnu = g.AddNode("mat.noeud_futur", "Noeud pas encore compilable");
		g.AddSocket(inconnu, "bsdf", t.shader, NkSocketDir::Output);
		g.Connect(inconnu, "bsdf", out, "surface");
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		e3 = !r.ok;
		m3 = r.error;
		// Et RIEN ne doit avoir ete emis : un shader partiel serait pire qu'aucun.
		rienEmis3 = (r.source.Size() == 0);
	}
	const bool troisDistincts = e1 && e2 && e3 && rienEmis3 && m1.Size() > 0 && m2.Size() > 0 && m3.Size() > 0 &&
								!(m1 == m2) && !(m2 == m3) && !(m1 == m3);
	NkString d;
	d = NkFormat("[{0}] [{1}] [{2}] | trois ECHECS={3}{4}{5} rien emis={6} distincts={7}", m1.CStr(), m2.CStr(), m3.CStr(), e1 ? 1 : 0, e2 ? 1 : 0, e3 ? 1 : 0, rienEmis3 ? 1 : 0, troisDistincts ? 1 : 0);
	Cas("compile/refuse-avant-de-generer", troisDistincts, d);
}

static void CasCompileEntreeNiCableeNiRenseignee() {
	// LE CHOIX QU'IL FAUT DIRE. Une entree ni cablee ni renseignee : que met le
	// compilateur ? Le blanc ferait passer un materiau non fini pour un materiau
	// clair. On rend le NOIR, qui se VOIT — meme logique que le repli d'un
	// shader manquant.
	//
	// DISCRIMINE : on verifie que ca compile ET que le code contient bien le
	// neutre. Un compilateur qui laisserait l'expression VIDE produirait
	// « vec3 n1_albedo = ; », ce qu'aucun backend n'accepte — donc le seul fait
	// de compiler prouve deja qu'une decision a ete prise. Le reste verifie
	// LAQUELLE.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED); // AUCUN defaut pose
	g.Connect(bsdf, "bsdf", out, "surface");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	NkString be;
	NkString err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	const bool neutreNoir = r.ok && ContientSansCasse(r.source, "vec3(0.0)");
	NkString d;
	d = NkFormat("emis={0} | {1}| neutre noir present={2} (le blanc flatterait un materiau non fini)", r.ok ? 1 : 0, be, neutreNoir ? 1 : 0);
	Cas("compile/entree-vide-donne-le-neutre", r.ok && ok == 4 && neutreNoir, d);
}

static void CasCompileGrapheVenuDunFichier() {
	// LE CHEMIN COMPLET, celui qui compte en production : on construit, on
	// SERIALISE, on RELIT, et on compile le graphe RELU. Les deux shaders
	// doivent etre identiques au caractere pres.
	//
	// DISCRIMINE : c'est le seul cas qui prouve que les defauts de prise
	// traversent le fichier POUR DE VRAI jusqu'au shader. Un aller-retour qui
	// perdrait les defauts rendrait un shader different — tout noir, et qui
	// compile parfaitement.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	MonteUnPrincipled(g, t, nullptr);
	NkMatCompileResult direct = NkMatCompileToNkSL(g);

	NkString fichier;
	g.Serialize(fichier);
	NkNodeGraph g2;
	const bool relu = g2.Deserialize(fichier.CStr());
	NkMatCompileResult apres = NkMatCompileToNkSL(g2);

	const bool memeShader = direct.ok && apres.ok && (direct.source == apres.source);
	NkString be;
	NkString err;
	const uint32 ok = apres.ok ? CompileSurLesBackends(apres.source, be, &err) : 0u;
	NkString d;
	d = NkFormat("relu={0} | shader identique apres aller-retour={1} ({2} vs {3} o) | {4}", relu ? 1 : 0, memeShader ? 1 : 0, (uint32)direct.source.Size(), (uint32)apres.source.Size(), be);
	Cas("compile/graphe-venu-d-un-fichier", relu && memeShader && ok == 4, d);
}

// ── la bibliotheque interrogee PAR TYPE DE PRISE ─────────────────────────────
// La demande de Rodolf du 2026-08-22 : dans Blender, chaque parametre porte un
// point, et le menu qui s'ouvre est FILTRE PAR LE TYPE DE LA PRISE. Le menu de
// `Base Color` et celui de `Roughness` n'ont pas le meme contenu.

// Cherche un prototype par sa cle dans un resultat de requete.
static bool DansLeMenu(const NkMatNodeProto **menu, uint32 n, const char *cle) {
	for (uint32 i = 0; i < n; ++i) {
		const char *a = menu[i]->key;
		const char *b = cle;
		while (*a && *a == *b) {
			++a;
			++b;
		}
		if (!*a && !*b)
			return true;
	}
	return false;
}

static void CasMenuPriseShader() {
	// DISCRIMINE par ce qui NE DOIT PAS y etre. Une requete qui rendrait tous
	// les prototypes passerait un test qui se contenterait de compter « au moins
	// un ». `Material Output` n'a AUCUNE sortie : rien ne peut en sortir, il ne
	// peut donc jamais etre propose. Et `Value`/`RGB` produisent un reel et une
	// couleur, pas un shader.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkMatNodeProto *menu[16];
	const uint32 n = NkMatNoeudsPourPrise(g, t.shader, menu, 16);
	const bool bons = DansLeMenu(menu, n, NK_MN_PRINCIPLED) && DansLeMenu(menu, n, NK_MN_DIFFUSE) &&
					  DansLeMenu(menu, n, NK_MN_EMISSION) && DansLeMenu(menu, n, NK_MN_MIX_SHADER);
	const bool absents = !DansLeMenu(menu, n, NK_MN_OUTPUT) && !DansLeMenu(menu, n, NK_MN_VALUE) &&
						 !DansLeMenu(menu, n, NK_MN_RGB);
	Cas("biblio/menu-prise-shader", n == 4 && bons && absents,
		NkFormat("{0} propositions (4 attendues) | les 4 BSDF presents={1} | sortie/valeur/rgb absents={2}", n,
				 bons ? 1 : 0, absents ? 1 : 0));
}

static void CasMenuPriseCouleurEtReelle() {
	// LE CAS QUI TRANCHE, et il tient dans une asymetrie. Les conversions sont
	// DIRIGEES : reel -> couleur est declaree, couleur -> reel ne l'est pas
	// (luminance ? moyenne ? canal rouge ? trois reponses plausibles, donc
	// aucune par defaut). Le menu doit donc etre ASYMETRIQUE :
	//     prise COULEUR : `RGB` **et** `Value` (le reel se diffuse en gris)
	//     prise REELLE  : `Value` seul -- `RGB` doit en etre ABSENT
	// Une table de conversions symetrique par erreur mettrait `RGB` dans les
	// deux, et aucun test a sens unique ne le verrait.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const NkMatNodeProto *mc[16], *mr[16];
	const uint32 nc = NkMatNoeudsPourPriseDe(g, NK_MN_PRINCIPLED, "base_color", mc, 16);
	const uint32 nr = NkMatNoeudsPourPriseDe(g, NK_MN_PRINCIPLED, "roughness", mr, 16);
	const bool couleurOk = DansLeMenu(mc, nc, NK_MN_RGB) && DansLeMenu(mc, nc, NK_MN_VALUE);
	const bool reelOk = DansLeMenu(mr, nr, NK_MN_VALUE) && !DansLeMenu(mr, nr, NK_MN_RGB);
	Cas("biblio/menu-asymetrique-couleur-reel", nc == 2 && nr == 1 && couleurOk && reelOk,
		NkFormat("base_color : {0} propositions (RGB+Value attendus, ok={1}) | roughness : {2} (Value SEUL, RGB "
				 "doit etre absent, ok={3})",
				 nc, couleurOk ? 1 : 0, nr, reelOk ? 1 : 0));
}

static void CasMenuPriseInconnue() {
	// Une prise qui n'existe pas rend un menu VIDE, jamais le menu complet. Une
	// requete qui retomberait sur « tout » ferait proposer n'importe quoi sur
	// une prise mal orthographiee.
	NkNodeGraph g;
	NkMatRegisterTypes(g);
	const uint32 n = NkMatNoeudsPourPriseDe(g, NK_MN_PRINCIPLED, "prise_qui_nexiste_pas");
	const uint32 m = NkMatNoeudsPourPriseDe(g, "mat.noeud_inconnu", "base_color");
	Cas("biblio/menu-prise-ou-noeud-inconnu", n == 0 && m == 0,
		NkFormat("prise inconnue -> {0} | noeud inconnu -> {1} (0 et 0 attendus)", n, m));
}

static void CasPriseConstanteSeulement() {
	// ⚠️ CE QUE CE CAS COUVRE, ET CE QU'IL NE COUVRE PAS.
	// Certains parametres alimentent l'ETAT DU PIPELINE (mode de melange, mode
	// d'ombre) et ne peuvent pas varier par pixel a moindre cout : leur prise
	// doit pouvoir se declarer « constante seulement », et l'interface ne doit
	// alors PAS afficher de point de connexion — un menu vide laisserait croire
	// a une panne.
	//
	// AUCUN des sept prototypes actuels n'est dans ce cas, et je ne vais pas en
	// inventer un pour faire vert. On teste donc le MECANISME sur une
	// declaration construite ici : le drapeau est lu, et il decide. Le jour ou
	// un vrai parametre d'etat arrive, il n'y aura qu'a poser le booleen.
	NkMatSocketDecl libre = {"exemple", NK_MT_REAL, NkSocketDir::Input, false};
	NkMatSocketDecl figee = {"exemple", NK_MT_REAL, NkSocketDir::Input, true};
	const bool ok = NkMatPriseAccepteUnLien(libre) && !NkMatPriseAccepteUnLien(figee);
	Cas("biblio/prise-constante-seulement", ok,
		NkFormat("prise libre accepte un lien={0} | prise figee refuse={1} | usage reel dans la bibliotheque : AUCUN "
				 "a ce jour, et c'est dit",
				 NkMatPriseAccepteUnLien(libre) ? 1 : 0, NkMatPriseAccepteUnLien(figee) ? 0 : 1));
}

static void CasCompileRGBVersBaseColor() {
	// LE PRINCIPE DE RODOLF, COMPILE : « un champ de couleur peut recevoir une
	// texture ou un procedural ». Ici la source est un noeud `RGB`, dont la
	// valeur vit dans une PROPRIETE DE NOEUD — les proprietes livrees dans le
	// coeur cette nuit traversent donc jusqu'au shader pour la premiere fois.
	//
	// DISCRIMINE : la valeur de la propriete doit se retrouver dans le code
	// emis. Un compilateur qui declarerait la locale sans lire la propriete
	// produirait un shader qui compile parfaitement et rendrait du noir.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId rgb = NkMatAddNode(g, NK_MN_RGB);
	g.Connect(bsdf, "bsdf", out, "surface");
	const NkLinkError e = g.Connect(rgb, "color", bsdf, "base_color");
	const float32 turquoise[3] = {0.04f, 0.33f, 0.37f};
	g.SetProp(rgb, NK_MPROP_COLOR, NkValueVec(t.color, turquoise, 3));

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	NkString be;
	NkString err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	const bool porteLaValeur = r.ok && ContientSansCasse(r.source, "0.330000013");
	// Et le Principled doit LIRE la locale du RGB, pas un litteral : c'est la
	// preuve que le lien a ete suivi et non que la valeur a ete recopiee.
	const NkString nomVal = NkFormat("n{0}_val", (uint32)rgb);
	const bool lien = r.ok && Apres(r.source, nomVal.CStr()) > 0;
	Cas("compile/rgb-vers-base-color", e == NkLinkError::Ok && r.ok && ok == 4 && porteLaValeur && lien,
		NkFormat("lien={0} | {1}| valeur de la propriete presente={2} | le BSDF lit {3}={4}",
				 NkString(NkLinkErrorName(e)), be, porteLaValeur ? 1 : 0, nomVal, lien ? 1 : 0));
}

static void CasCompileValeurVersRoughness() {
	// Le pendant reel : un noeud `Value` alimente `roughness`. Il exerce la
	// SECONDE porte des valeurs -- une propriete SCALAIRE -- et le fait que le
	// compilateur choisisse `float` et non `vec3` pour ce type de sortie.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId val = NkMatAddNode(g, NK_MN_VALUE);
	g.Connect(bsdf, "bsdf", out, "surface");
	const NkLinkError e = g.Connect(val, "value", bsdf, "roughness");
	g.SetProp(val, NK_MPROP_VALUE, NkValueReal(t.real, 0.125f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	NkString be;
	NkString err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	const NkString decl = NkFormat("float n{0}_val = 0.125", (uint32)val);
	const bool bonType = r.ok && Apres(r.source, decl.CStr()) > 0;
	Cas("compile/valeur-vers-roughness", e == NkLinkError::Ok && r.ok && ok == 4 && bonType,
		NkFormat("lien={0} | {1}| declaration '{2}' presente={3}", NkString(NkLinkErrorName(e)), be, decl,
				 bonType ? 1 : 0));
}

int main() {
	// ⚠️ `Pattern()` est GLOBAL ET PERSISTANT : il modifie l'instance de journal
	// du PROCESSUS, pas l'appel. On le pose donc UNE FOIS ici, et pas a chaque
	// ligne — sinon chaque composant du moteur qui journalise ensuite herite
	// silencieusement du motif du banc. Consequence assumee : les lignes du
	// moteur perdent aussi leur horodatage dans ce processus ; elles restent
	// reconnaissables a leur crochet ouvrant, et se filtrent par « ^\[ ».
	logger.Pattern("%v");
	logger.Info("== NkMatGraphCheck - graphe de materiaux, couche 3 sur NKGraph ==");
	logger.Info("   regime : structure de donnees pure, aucun GPU, aucune fenetre.");
	logger.Info("   COUVRE aussi, depuis le 22/08 : les defauts de prise, les");
	logger.Info("   proprietes, la validation d un fichier, et le compilateur vers NkSL.");
	logger.Info("   NE couvre PAS : le RENDU (aucun GPU ici) -- seulement le fait que\n");
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

	// -- le compilateur : graphe -> NkSL -> quatre backends -------------
	CasCompilePrincipled();
	CasCompileMixShader();
	CasCompileOrdreRespecte();
	CasCompileRefuseAvantDeGenerer();
	CasCompileEntreeNiCableeNiRenseignee();
	CasCompileGrapheVenuDunFichier();

	// -- la bibliotheque interrogee par TYPE DE PRISE (demande de Rodolf) --
	CasMenuPriseShader();
	CasMenuPriseCouleurEtReelle();
	CasMenuPriseInconnue();
	CasPriseConstanteSeulement();
	CasCompileRGBVersBaseColor();
	CasCompileValeurVersRoughness();

	logger.Info("\n-- {0} cas, {1} echec(s) --", gCas, gEchecs);
	return gEchecs == 0 ? 0 : 1;
}

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
#include <stdio.h> // fwrite : ecrire la source SANS passer par le formateur

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
	// Cinquieme verdict : le seul rendu par un vrai compilateur (glslang, via
	// NK_SPIRV). Lu au MOT MAGIQUE, jamais a `success` -- voir la note longue
	// dans CompileSurLesBackends : quand glslang refuse, success reste a 1.
	{
		NkSLCompileResult sp = c.Compile(shader, NkSLStage::NK_FRAGMENT, NkSLTarget::NK_SPIRV);
		bool vrai = false;
		if (sp.bytecode.Size() >= 4) {
			const uint8 *o = sp.bytecode.Data();
			vrai = (o[0] == 0x03 && o[1] == 0x02 && o[2] == 0x23 && o[3] == 0x07);
		}
		if (vrai)
			++ok;
		detail.Append(NkFormat("GLSLANG={0} ", NkString(vrai ? "ok" : "ECHEC")));
	}
	Cas("phase0/masque-gen-4-backends-plus-glslang", ok == 5 && lectureTexturePartout && corps.Size() > 100, detail);
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
		// ⚠️ « ok » ici ne veut PAS dire « compile ». Mesure du 22/08 :
		// NkSLCompiler::Compile ne consulte AUCUN compilateur natif pour ces
		// quatre cibles -- il appelle le GENERATEUR NkSL et rend son verdict.
		// L'analyse semantique existe mais est DELIBEREMENT non bloquante
		// (NkSLCompiler.cpp:290) : fonction jamais declaree, variable inconnue,
		// mismatch de type remontent en AVERTISSEMENTS. Un shader appelant
		// « NkFonctionQuiNExistePas » rend GL=ok VK=ok DX11=ok DX12=ok.
		// Ces quatre colonnes attestent la GENERATION, jamais la COMPILATION.
		detail.Append(NkFormat("{0}={1} ", NkString(cibles[t].nom), NkString(r.success ? "gen" : "ECHEC")));
	}

	// ── Le CINQUIEME verdict, et le seul rendu par un vrai compilateur ────────
	// La cible NK_SPIRV genere le GLSL Vulkan puis le passe a glslang EMBARQUE
	// (NkGLSLToSPIRV). glslang, lui, REFUSE une fonction non declaree.
	//
	// 🔴 MAIS ON NE PEUT PAS LIRE SON VERDICT DANS `success`. Quand glslang
	// refuse, NkSLCompiler.cpp:325 rattrape l'echec, REND LE TEXTE GLSL a la
	// place du bytecode et LAISSE success=1. Le seul bit qui distingue les deux
	// cas est le MOT MAGIQUE : du vrai SPIR-V commence par 0x07230203 ; le texte
	// rendu par defaut commence par « #ver ». Mesure du 22/08, les deux sens :
	//   shader invalide -> success=1, octets=186, magie = 35 118 101 114 (#ver)
	//   shader valide   -> success=1, octets=560, magie = 3 2 35 7 (SPIR-V)
	//
	// NE REMPLACE PAS CE TEST PAR `r.success` EN LE CROYANT EQUIVALENT.
	// C'est exactement l'erreur qui a fait passer pour « compile sur 4 backends »
	// une chaine qui n'en compilait aucun.
	{
		NkSLCompileResult sp = c.Compile(nksl, NkSLStage::NK_FRAGMENT, NkSLTarget::NK_SPIRV);
		bool vrai = false;
		if (sp.bytecode.Size() >= 4) {
			const uint8 *o = sp.bytecode.Data();
			vrai = (o[0] == 0x03 && o[1] == 0x02 && o[2] == 0x23 && o[3] == 0x07);
		}
		if (vrai)
			++ok;
		detail.Append(NkFormat("GLSLANG={0} ", NkString(vrai ? "ok" : "ECHEC")));
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
	Cas("compile/principled-4-backends", r.ok && ok == 5 && porteLeRouge, d);
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
	d = NkFormat("emis={0} ({1} o) | {2}| {3} cite {4} fois (1 declaration + {5} composantes) {6}", r.ok ? 1 : 0, (uint32)r.source.Size(), be, nomFac, nbFac, NkMatComposanteCount(), r.ok ? NkString("") : r.error);
	Cas("compile/melange-deux-bsdf-4-backends", r.ok && ok == 5 && nbFac == NkMatComposanteCount() + 1u, d);
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
	Cas("compile/ordre-topologique-respecte", r.ok && ok == 5 && premiere > 0 && lecture > premiere, d);
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
	Cas("compile/entree-vide-donne-le-neutre", r.ok && ok == 5 && neutreNoir, d);
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
	Cas("compile/graphe-venu-d-un-fichier", relu && memeShader && ok == 5, d);
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
	// Dimensionne sur le NOMBRE DE PROTOTYPES, pas sur un nombre choisi :
	// le menu grandit a chaque noeud ajoute, et un tampon fixe finit
	// toujours par etre depasse. Il l'a ete a 17.
	const NkMatNodeProto *menu[64];
	const uint32 n = NkMatNoeudsPourPrise(g, t.shader, menu, 64);
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
	const NkMatNodeProto *mc[64], *mr[64];
	const uint32 nc = NkMatNoeudsPourPriseDe(g, NK_MN_PRINCIPLED, "base_color", mc, 64);
	const uint32 nr = NkMatNoeudsPourPriseDe(g, NK_MN_PRINCIPLED, "roughness", mr, 64);
	// Ce qui produit une COULEUR ou peut s'y convertir : RGB, Mix Color, et
	// Value (un reel se diffuse en gris). Math aussi, pour la meme raison.
	const bool couleurOk = DansLeMenu(mc, nc, NK_MN_RGB) && DansLeMenu(mc, nc, NK_MN_VALUE) &&
						   DansLeMenu(mc, nc, NK_MN_MIX_COLOR) && DansLeMenu(mc, nc, NK_MN_MATH) &&
						   DansLeMenu(mc, nc, NK_MN_COLOR_RAMP) &&
						   DansLeMenu(mc, nc, NK_MN_IMAGE_TEXTURE) &&
						   DansLeMenu(mc, nc, NK_MN_NORMAL_MAP) && DansLeMenu(mc, nc, NK_MN_BUMP) &&
						   DansLeMenu(mc, nc, NK_MN_NOISE) && DansLeMenu(mc, nc, NK_MN_CHECKER) &&
						   DansLeMenu(mc, nc, NK_MN_VORONOI) && DansLeMenu(mc, nc, NK_MN_BRICK);
	// Ce qui produit un REEL : Value et Math. Ni RGB ni Mix Color, parce que
	// `couleur -> reel` n'est PAS declaree — et c'est la tout le cas.
	const bool reelOk = DansLeMenu(mr, nr, NK_MN_VALUE) && DansLeMenu(mr, nr, NK_MN_MATH) &&
						!DansLeMenu(mr, nr, NK_MN_RGB) && !DansLeMenu(mr, nr, NK_MN_MIX_COLOR) &&
						!DansLeMenu(mr, nr, NK_MN_COLOR_RAMP) &&
						// `Image Texture` a une sortie `alpha` REELLE : il est donc
						// legitimement dans les DEUX menus, par deux prises differentes.
						DansLeMenu(mr, nr, NK_MN_IMAGE_TEXTURE) &&
						// Separate XYZ sort TROIS reels : il est dans le menu reel,
						// et pas dans le menu couleur (reel->couleur est declaree,
						// donc il y est aussi -- ce qui est correct).
						DansLeMenu(mr, nr, NK_MN_SEPARATE_XYZ) &&
						// Les proceduraux ont une sortie `fac` REELLE : ils sont dans
						// les deux menus, par deux prises differentes.
						DansLeMenu(mr, nr, NK_MN_NOISE) && DansLeMenu(mr, nr, NK_MN_GRADIENT) &&
						DansLeMenu(mr, nr, NK_MN_WAVE) && DansLeMenu(mr, nr, NK_MN_VORONOI);
	Cas("biblio/menu-asymetrique-couleur-reel", nc == 17 && nr == 10 && couleurOk && reelOk,
		NkFormat("base_color : {0} propositions (RGB+MixColor+ColorRamp+ImageTex+Value+Math+coord+mappage, ok={1}) | roughness : {2} (Value+Math "
				 "SEULS, RGB et MixColor doivent etre absents, ok={3})",
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
	// La locale porte le nom de la PRISE source : `RGB` sort sur « color ».
	const NkString nomVal = NkFormat("n{0}_color", (uint32)rgb);
	const bool lien = r.ok && Apres(r.source, nomVal.CStr()) > 0;
	Cas("compile/rgb-vers-base-color", e == NkLinkError::Ok && r.ok && ok == 5 && porteLaValeur && lien,
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
	// `Value` sort sur « value ».
	const NkString decl = NkFormat("float n{0}_value = 0.125", (uint32)val);
	const bool bonType = r.ok && Apres(r.source, decl.CStr()) > 0;
	Cas("compile/valeur-vers-roughness", e == NkLinkError::Ok && r.ok && ok == 5 && bonType,
		NkFormat("lien={0} | {1}| declaration '{2}' presente={3}", NkString(NkLinkErrorName(e)), be, decl,
				 bonType ? 1 : 0));
}


// ── Math et Mix Color : une propriete qui porte une DECISION ─────────────────

static void CasOperationInconnueRefusee() {
	// ⚠️ LE CAS LE PLUS IMPORTANT DE LA SERIE. Une operation que le compilateur
	// ne connait pas doit FAIRE ECHOUER la compilation, jamais retomber sur la
	// premiere de la liste. Un repli sur « ajouter » produirait un materiau qui
	// compile, qui rend, et qui calcule AUTRE CHOSE que ce que le fichier dit —
	// un fichier ecrit par une version future, ou une simple faute de frappe,
	// passerait inapercu jusqu'au resultat.
	//
	// DISCRIMINE aussi le cas VOISIN et legitime : une propriete ABSENTE (le
	// noeud vient d'etre pose, l'auteur n'a pas choisi) doit, elle, compiler.
	// Un controle qui refuserait les deux serait aussi faux qu'un qui accepte
	// les deux.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId math = NkMatAddNode(g, NK_MN_MATH);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(math, "value", bsdf, "roughness");

	// (a) propriete absente -> doit compiler
	NkMatCompileResult sansProp = NkMatCompileToNkSL(g);
	// (b) operation inconnue -> doit ECHOUER, et nommer la faute
	g.SetProp(math, NK_MPROP_OPERATION, NkValueText(t.real, "racine_carree_hyperbolique"));
	NkMatCompileResult inconnue = NkMatCompileToNkSL(g);
	// (c) operation valide -> doit compiler de nouveau
	g.SetProp(math, NK_MPROP_OPERATION, NkValueText(t.real, "multiplier"));
	NkMatCompileResult valide = NkMatCompileToNkSL(g);

	const bool nomme = !inconnue.ok && Apres(inconnue.error, "racine_carree_hyperbolique") > 0;
	const bool rienEmis = inconnue.source.Size() == 0;
	Cas("compile/operation-inconnue-refusee",
		sansProp.ok && !inconnue.ok && nomme && rienEmis && valide.ok,
		NkFormat("propriete absente compile={0} | inconnue refusee={1} en la nommant={2} rien emis={3} | valide "
				 "recompile={4} | message : {5}",
				 sansProp.ok ? 1 : 0, inconnue.ok ? 0 : 1, nomme ? 1 : 0, rienEmis ? 1 : 0, valide.ok ? 1 : 0,
				 inconnue.error));
}

static void CasToutesLesOperationsCompilent() {
	// DISCRIMINE : on ne teste pas UNE operation, on les teste TOUTES, sur les
	// quatre backends. Une branche oubliee dans l'emetteur — le `else` final qui
	// avale un cas non prevu — ne se verrait pas autrement. Et le compte vient de
	// la table, pas d'un nombre ecrit ici : ajouter une operation sans l'emettre
	// mettra ce cas au rouge tout seul.
	uint32 okMath = 0, okMix = 0;
	NkString premierEchec;
	for (uint32 pass = 0; pass < 2; ++pass) {
		const bool couleur = (pass == 1);
		for (uint32 i = 0; i < NkMatOperationCount(couleur); ++i) {
			NkNodeGraph g;
			const NkMatTypes t = NkMatRegisterTypes(g);
			const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
			const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
			g.Connect(bsdf, "bsdf", out, "surface");
			const NkNodeId n = NkMatAddNode(g, couleur ? NK_MN_MIX_COLOR : NK_MN_MATH);
			g.Connect(n, couleur ? "color" : "value", bsdf, couleur ? "base_color" : "roughness");
			g.SetProp(n, NK_MPROP_OPERATION, NkValueText(t.real, NkMatOperationAt(couleur, i)->cle));
			NkMatCompileResult r = NkMatCompileToNkSL(g);
			NkString be, err;
			const uint32 nb = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
			if (r.ok && nb == 5) {
				if (couleur)
					++okMix;
				else
					++okMath;
			} else if (premierEchec.Size() == 0) {
				premierEchec = NkString(NkMatOperationAt(couleur, i)->cle);
				premierEchec.Append(" : ");
				premierEchec.Append(r.ok ? be : r.error);
			}
		}
	}
	const uint32 attMath = NkMatOperationCount(false), attMix = NkMatOperationCount(true);
	Cas("compile/toutes-les-operations-4-backends", okMath == attMath && okMix == attMix,
		NkFormat("Math {0}/{1} | Mix Color {2}/{3} (sur les 4 backends chacune) {4}", okMath, attMath, okMix,
				 attMix, premierEchec));
}

static void CasDivisionParZeroGardee() {
	// DISCRIMINE : une division nue produirait un NaN, qui contamine tout l'aval
	// et se voit comme un pixel noir OU blanc selon le backend — un defaut qui
	// change d'aspect d'une machine a l'autre, donc le pire a diagnostiquer.
	// On verifie que le code emis porte la garde, ET qu'il compile partout.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId math = NkMatAddNode(g, NK_MN_MATH);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(math, "value", bsdf, "roughness");
	g.SetProp(math, NK_MPROP_OPERATION, NkValueText(t.real, "diviser"));
	g.SetSocketDefault(math, "a", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	g.SetSocketDefault(math, "b", NkSocketDir::Input, NkValueReal(t.real, 0.f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	const bool garde = r.ok && ContientSansCasse(r.source, "== 0.0 ? 0.0");
	Cas("compile/division-par-zero-gardee", r.ok && ok == 5 && garde,
		NkFormat("emis={0} | {1}| garde presente dans le code emis={2}", r.ok ? 1 : 0, be, garde ? 1 : 0));
}


// ── ColorRamp : la premiere propriete a charge utile VARIABLE ────────────────

// Monte un graphe `ColorRamp -> base_color` et rend le resultat de compilation.
static NkMatCompileResult CompileRampe(NkNodeGraph &g, const NkMatTypes &t, const float32 *arrets, uint32 nbReels,
									   const char *interp) {
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId ramp = NkMatAddNode(g, NK_MN_COLOR_RAMP);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(ramp, "color", bsdf, "base_color");
	if (arrets)
		g.SetProp(ramp, NK_MPROP_STOPS, NkValueVec(t.ramp, arrets, nbReels));
	if (interp)
		g.SetProp(ramp, NK_MPROP_INTERP, NkValueText(t.ramp, interp));
	return NkMatCompileToNkSL(g);
}

static void CasRampeChargeVariable() {
	// LE CAS QUI EPROUVE LE SAC DE PROPRIETES. Jusqu'ici toutes les charges
	// utiles etaient de taille FIXE : un reel, trois, quatre. Une rampe en porte
	// 4xN, et N vient du fichier.
	//
	// DISCRIMINE : on compte les mix que la rampe emet. Une rampe a N arrets en
	// produit exactement N-1. Un emetteur qui n'en produirait qu'un — le premier
	// et le dernier arret, en ignorant ceux du milieu — donnerait un shader qui
	// compile et une rampe FAUSSE. Le compte est verifie pour 2, 3 et 5 arrets :
	// une seule taille ne prouverait rien d'une charge variable.
	const float32 a2[8] = {0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
	const float32 a3[12] = {0.f, 1.f, 0.f, 0.f, 0.5f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f};
	const float32 a5[20] = {0.f, 1.f, 0.f, 0.f, 0.25f, 1.f, 1.f, 0.f, 0.5f, 0.f,
							1.f, 0.f, 0.75f, 0.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f};
	struct Jeu {
			const float32 *st;
			uint32 n;
			uint32 arrets;
	};
	const Jeu jeux[3] = {{a2, 8, 2}, {a3, 12, 3}, {a5, 20, 5}};
	uint32 bons = 0;
	NkString detail;
	for (uint32 j = 0; j < 3; ++j) {
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		NkMatCompileResult r = CompileRampe(g, t, jeux[j].st, jeux[j].n, "lineaire");
		uint32 nbMix = 0;
		if (r.ok) {
			const char *q = r.source.CStr();
			while (*q) {
				if (q[0] == 'm' && q[1] == 'i' && q[2] == 'x' && q[3] == '(')
					++nbMix;
				++q;
			}
		}
		// Le puits emet DEUX mix pour son propre compte (specExp, specColor) :
		// c'est la lecon d'hier, ou compter tous les mix du shader mesurait le
		// modele d'eclairage en meme temps que le noeud.
		const uint32 attendus = jeux[j].arrets - 1u + 2u;
		NkString be, err;
		const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
		if (r.ok && ok == 5 && nbMix == attendus)
			++bons;
		detail.Append(NkFormat("{0} arrets->{1} mix (attendu {2}) ", jeux[j].arrets, nbMix, attendus));
	}
	Cas("colorramp/charge-utile-variable", bons == 3, detail);
}

static void CasRampeRefusNommes() {
	// QUATRE malformations, QUATRE messages distincts, et AUCUN shader emis.
	// DISCRIMINE : un compilateur qui rendrait le meme message pour tout
	// passerait un test qui ne compterait que les echecs. Et le cinquieme cas —
	// la propriete ABSENTE — doit au contraire COMPILER : c'est le voisin
	// legitime (le noeud vient d'etre pose), et le confondre avec une erreur
	// serait aussi faux que l'inverse.
	const float32 malForme[3] = {0.f, 1.f, 0.f};
	const float32 desordre[8] = {1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f};
	const float32 egales[8] = {0.5f, 1.f, 0.f, 0.f, 0.5f, 0.f, 0.f, 1.f};
	float32 trop[(NK_RAMP_ARRETS_MAX + 1) * 4];
	for (uint32 i = 0; i <= NK_RAMP_ARRETS_MAX; ++i) {
		trop[i * 4 + 0] = (float32)i * 0.001f;
		trop[i * 4 + 1] = 1.f;
		trop[i * 4 + 2] = 0.f;
		trop[i * 4 + 3] = 0.f;
	}
	NkString m[4];
	bool emisVide = true;
	const float32 *jeux[4] = {malForme, desordre, egales, trop};
	const uint32 tailles[4] = {3, 8, 8, (NK_RAMP_ARRETS_MAX + 1) * 4};
	for (uint32 j = 0; j < 4; ++j) {
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		NkMatCompileResult r = CompileRampe(g, t, jeux[j], tailles[j], "lineaire");
		m[j] = r.ok ? NkString("A COMPILE (ne devait pas)") : r.error;
		if (r.source.Size() != 0)
			emisVide = false;
	}
	NkNodeGraph g2;
	const NkMatTypes t2 = NkMatRegisterTypes(g2);
	NkMatCompileResult absente = CompileRampe(g2, t2, nullptr, 0, nullptr);
	// ⚠️ LE REFUS DOIT DIRE LE COMPTE. Sans le nombre, l'auteur doit deviner
	// combien d'arrets retirer.
	const bool ditLeCompte = Apres(m[3], "33 demandes, plafond 32") > 0;
	const bool distincts = !(m[0] == m[1]) && !(m[1] == m[2]) && !(m[2] == m[3]) && !(m[0] == m[3]);
	Cas("colorramp/refus-nommes-et-distincts", distincts && emisVide && absente.ok && ditLeCompte,
		NkFormat("[{0}] [{1}] [{2}] [{3}] | distincts={4} rien emis={5} absente compile={6} plafond dit le "
				 "compte={7}",
				 m[0], m[1], m[2], m[3], distincts ? 1 : 0, emisVide ? 1 : 0, absente.ok ? 1 : 0,
				 ditLeCompte ? 1 : 0));
}

static void CasRampeInterpolation() {
	// Les deux interpolations emettent des codes DIFFERENTS — step pour la
	// constante, une pente pre-calculee pour la lineaire — et les deux compilent.
	// Une interpolation inconnue est refusee en la nommant, meme discipline que
	// les operations.
	//
	// ⚠️ Et un point qui merite d'etre verifie : la rampe lineaire ne contient
	// AUCUNE division dans le shader. Le denominateur est calcule a la
	// compilation, et NkMatLisRampe a deja garanti qu'il est non nul en refusant
	// les positions egales. Une division laissee dans le shader serait une garde
	// a maintenir pour rien.
	const float32 a[8] = {0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
	NkNodeGraph g1;
	const NkMatTypes t1 = NkMatRegisterTypes(g1);
	NkMatCompileResult lin = CompileRampe(g1, t1, a, 8, "lineaire");
	NkNodeGraph g2;
	const NkMatTypes t2 = NkMatRegisterTypes(g2);
	NkMatCompileResult cst = CompileRampe(g2, t2, a, 8, "constante");
	NkNodeGraph g3;
	const NkMatTypes t3 = NkMatRegisterTypes(g3);
	NkMatCompileResult inc = CompileRampe(g3, t3, a, 8, "spline_de_bezier_cubique");

	NkString be1, be2, e1, e2;
	const uint32 ok1 = lin.ok ? CompileSurLesBackends(lin.source, be1, &e1) : 0u;
	const uint32 ok2 = cst.ok ? CompileSurLesBackends(cst.source, be2, &e2) : 0u;
	const bool pasDeStepEnLineaire = lin.ok && !ContientSansCasse(lin.source, "step(");
	const bool stepEnConstante = cst.ok && ContientSansCasse(cst.source, "step(");
	const bool nomme = !inc.ok && Apres(inc.error, "spline_de_bezier_cubique") > 0;
	Cas("colorramp/interpolations-et-refus",
		lin.ok && cst.ok && ok1 == 5 && ok2 == 5 && pasDeStepEnLineaire && stepEnConstante && nomme,
		NkFormat("lineaire {0}| constante {1}| step absent en lineaire={2} present en constante={3} | inconnue "
				 "refusee en la nommant={4}",
				 be1, be2, pasDeStepEnLineaire ? 1 : 0, stepEnConstante ? 1 : 0, nomme ? 1 : 0));
}

static void CasRampeAllerRetourFichier() {
	// ⚠️ LA CHARGE VARIABLE DOIT SURVIVRE AU FICHIER, et c'est le vrai test du
	// sac de proprietes : jusqu'ici l'aller-retour ne portait que des charges de
	// taille fixe. On compare les textes ET on recompile le graphe relu : les
	// deux shaders doivent etre identiques au caractere pres. Une charge tronquee
	// donnerait une rampe plus courte, qui compile parfaitement.
	const float32 a5[20] = {0.f, 1.f, 0.f, 0.f, 0.25f, 1.f, 1.f, 0.f, 0.5f, 0.f,
							1.f, 0.f, 0.75f, 0.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f};
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	NkMatCompileResult direct = CompileRampe(g, t, a5, 20, "lineaire");
	NkString fichier;
	g.Serialize(fichier);
	NkNodeGraph g2;
	const bool relu = g2.Deserialize(fichier.CStr());
	NkMatCompileResult apres = NkMatCompileToNkSL(g2);
	const bool meme = direct.ok && apres.ok && (direct.source == apres.source);
	const NkGraphValue *v = nullptr;
	for (uint32 i = 0; i < g2.RawNodeCount(); ++i) {
		const NkNode *nd = g2.RawNodeAt(i);
		if (nd && nd->alive && nd->type == NkString(NK_MN_COLOR_RAMP))
			v = g2.FindProp(nd->id, NK_MPROP_STOPS);
	}
	const bool taille = v && v->numbers.Size() == 20;
	Cas("colorramp/charge-variable-survit-au-fichier", relu && meme && taille,
		NkFormat("relu={0} shader identique={1} | reels retrouves={2} (20 attendus) | fichier de {3} o",
				 relu ? 1 : 0, meme ? 1 : 0, v ? (uint32)v->numbers.Size() : 0u, (uint32)fichier.Size()));
}


// ── Image Texture : la premiere RESSOURCE consommee par un graphe ────────────

// Releve tous les bindings du set 2 declares dans un shader emis. On COMPTE des
// NOMBRES, on ne cherche pas des noms : le generateur HLSL minuscule et suffixe
// les identifiants, et chercher « nkGraphTex0 » testerait le generateur de noms
// plutot que le shader. Lecon payee le 22/08 sur `tMask` -> `tmask_tex`.
static uint32 RelieveBindingsSet2(const NkString &nksl, uint32 *out, uint32 maxOut) {
	const char *p = nksl.CStr();
	const char *motif = "@binding(set=2, binding=";
	uint32 n = 0;
	while (*p) {
		const char *a = p;
		const char *b = motif;
		while (*b && *a == *b) {
			++a;
			++b;
		}
		if (*b == 0) {
			uint32 v = 0;
			while (*a >= '0' && *a <= '9') {
				v = v * 10u + (uint32)(*a - '0');
				++a;
			}
			if (out && n < maxOut)
				out[n] = v;
			++n;
			p = a;
			continue;
		}
		++p;
	}
	return n;
}

static bool SlotConnu(uint32 binding) {
	for (uint32 i = 0; i < renderer::NK_MATBIND_GRAPH_SLOT_COUNT; ++i)
		if (renderer::NK_MATBIND_GRAPH_SLOTS[i] == binding)
			return true;
	return false;
}

// Monte un graphe avec `nb` noeuds Image Texture, tous branches vers la sortie
// par un empilement de Mix Color (il faut bien que chacun serve a quelque chose,
// sinon rien ne garantit qu'il soit emis).
static NkMatCompileResult CompileNTextures(uint32 nb) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	NkNodeId precedent = NK_NODE_INVALID;
	for (uint32 i = 0; i < nb; ++i) {
		const NkNodeId tex = NkMatAddNode(g, NK_MN_IMAGE_TEXTURE);
		g.SetProp(tex, NK_MPROP_IMAGE, NkValueText(t.ramp, "image.png"));
		if (precedent == NK_NODE_INVALID) {
			precedent = tex;
		} else {
			const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_COLOR);
			g.Connect(precedent, "color", mix, "color1");
			g.Connect(tex, "color", mix, "color2");
			precedent = mix;
		}
	}
	if (precedent != NK_NODE_INVALID)
		g.Connect(precedent, precedent == NK_NODE_INVALID ? "color" : "color", bsdf, "base_color");
	return NkMatCompileToNkSL(g);
}

static void CasTexturePlafondRefusNomme() {
	// LE CAS QUI TRANCHE : `plafond` textures compile, `plafond + 1` est refuse
	// EN NOMMANT LE COMPTE. Sans le nombre, l'auteur doit deviner ce qu'il retire.
	//
	// DISCRIMINE aussi le bord exact : un plafond mal ecrit (`>=` au lieu de `>`)
	// refuserait le cas parfaitement legitime a `plafond` pile.
	const uint32 plafond = renderer::NK_MATBIND_GRAPH_SLOT_COUNT;
	NkMatCompileResult pile = CompileNTextures(plafond);
	NkMatCompileResult trop = CompileNTextures(plafond + 1);
	NkString be, err;
	const uint32 ok = pile.ok ? CompileSurLesBackends(pile.source, be, &err) : 0u;
	const NkString attendu = NkFormat("ce graphe demande {0} textures, le plafond est {1}", plafond + 1, plafond);
	const bool ditLeCompte = !trop.ok && Apres(trop.error, attendu.CStr()) > 0;
	Cas("imgtex/plafond-refus-nomme", pile.ok && ok == 5 && !trop.ok && ditLeCompte && trop.source.Size() == 0,
		NkFormat("{0} textures : compile={1} {2}| {3} textures : refuse={4} en disant le compte={5} | message : {6}",
				 plafond, pile.ok ? 1 : 0, be, plafond + 1, trop.ok ? 0 : 1, ditLeCompte ? 1 : 0, trop.error));
}

static void CasTextureBindingsDesDeuxCotes() {
	// ⚠️ LE CONTROLE QUI PROTEGE LA CONDITION NON NEGOCIABLE, et le plus
	// important des trois. Le compilateur et le layout doivent lire LE MEME
	// endroit ; si le plafond vivait a deux places, un compilateur qui autorise 8
	// quand le layout en declare 6 ecrirait sur deux bindings inexistants — sans
	// erreur, sans journal, sans difference d'image.
	//
	// On confronte donc le SHADER EMIS a la table C++ : chaque binding declare
	// doit etre dans `NK_MATBIND_GRAPH_SLOTS`, et leur nombre doit valoir le
	// nombre de noeuds texture. C'est la comparaison du code a une VERITE
	// EXTERNE, comme en phase 0.
	//
	// ⚠️ Et on COMPTE DES NOMBRES : chercher le nom du sampler testerait le
	// generateur de noms, pas le shader.
	const uint32 plafond = renderer::NK_MATBIND_GRAPH_SLOT_COUNT;
	uint32 bindings[16] = {};
	NkMatCompileResult r = CompileNTextures(plafond);
	const uint32 n = r.ok ? RelieveBindingsSet2(r.source, bindings, 16) : 0u;
	bool tousConnus = (n == plafond);
	bool tousDistincts = true;
	for (uint32 i = 0; i < n; ++i) {
		if (!SlotConnu(bindings[i]))
			tousConnus = false;
		for (uint32 k = i + 1; k < n; ++k)
			if (bindings[i] == bindings[k])
				tousDistincts = false;
	}
	NkString liste;
	for (uint32 i = 0; i < n; ++i)
		liste.Append(NkFormat("{0} ", bindings[i]));
	Cas("imgtex/bindings-declares-des-deux-cotes", r.ok && tousConnus && tousDistincts,
		NkFormat("{0} bindings emis [{1}] | tous dans NK_MATBIND_GRAPH_SLOTS={2} | tous distincts={3} | plafond C++={4}",
				 n, liste, tousConnus ? 1 : 0, tousDistincts ? 1 : 0, plafond));
}

static void CasTextureAucunBindingNeuf() {
	// LE COROLLAIRE, et il merite son propre cas : le graphe ne doit JAMAIS
	// declarer un binding hors de la table — c'est-a-dire ne jamais rien ajouter
	// au layout partage. Un emetteur qui numeroterait ses samplers 10, 11, 12
	// (des nombres libres a l'oeil) passerait le cas precedent si celui-ci ne
	// verifiait que le compte.
	//
	// DISCRIMINE par le MAXIMUM : on compare au plus grand slot de la table, pas
	// a une constante recopiee ici.
	uint32 maxTable = 0;
	for (uint32 i = 0; i < renderer::NK_MATBIND_GRAPH_SLOT_COUNT; ++i)
		if (renderer::NK_MATBIND_GRAPH_SLOTS[i] > maxTable)
			maxTable = renderer::NK_MATBIND_GRAPH_SLOTS[i];
	uint32 bindings[16] = {};
	NkMatCompileResult r = CompileNTextures(renderer::NK_MATBIND_GRAPH_SLOT_COUNT);
	const uint32 n = r.ok ? RelieveBindingsSet2(r.source, bindings, 16) : 0u;
	uint32 maxEmis = 0;
	for (uint32 i = 0; i < n; ++i)
		if (bindings[i] > maxEmis)
			maxEmis = bindings[i];
	Cas("imgtex/aucun-binding-hors-table", r.ok && n > 0 && maxEmis <= maxTable,
		NkFormat("plus grand binding emis={0} | plus grand de la table={1} | aucun binding neuf={2}", maxEmis,
				 maxTable, (maxEmis <= maxTable) ? 1 : 0));
}

static void CasTextureDeuxSorties() {
	// `Image Texture` est le PREMIER noeud a deux sorties. C'est lui qui a impose
	// de nommer les locales d'apres la prise et non par un « val » unique.
	//
	// DISCRIMINE : `color` alimente base_color, `alpha` alimente roughness. Les
	// deux locales doivent EXISTER et etre DIFFERENTES. Un emetteur qui garderait
	// un nom unique par noeud ferait lire la meme valeur aux deux entrees — un
	// shader qui compile et un materiau faux.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId tex = NkMatAddNode(g, NK_MN_IMAGE_TEXTURE);
	g.Connect(bsdf, "bsdf", out, "surface");
	const NkLinkError e1 = g.Connect(tex, "color", bsdf, "base_color");
	const NkLinkError e2 = g.Connect(tex, "alpha", bsdf, "roughness");
	g.SetProp(tex, NK_MPROP_IMAGE, NkValueText(t.ramp, "peau.png"));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkString nc = NkFormat("n{0}_color", (uint32)tex);
	const NkString na = NkFormat("n{0}_alpha", (uint32)tex);
	const bool deux = r.ok && Apres(r.source, nc.CStr()) > 0 && Apres(r.source, na.CStr()) > 0;
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("imgtex/deux-sorties-distinctes",
		e1 == NkLinkError::Ok && e2 == NkLinkError::Ok && r.ok && ok == 5 && deux,
		NkFormat("liens {0}/{1} | {2}| locales '{3}' et '{4}' presentes={5}", NkString(NkLinkErrorName(e1)),
				 NkString(NkLinkErrorName(e2)), be, nc, na, deux ? 1 : 0));
}

static void CasTextureCoordonneeEtMappage() {
	// Sans entree `vector`, la texture lit l'UV du maillage — c'est le
	// comportement DEFINI du noeud chez Blender, pas un bouche-trou.
	// Avec un `Mapping` branche, elle doit lire la coordonnee TRANSFORMEE.
	//
	// DISCRIMINE : le shader du second cas doit citer la locale du Mapping ; le
	// premier doit citer `vUV`. Un emetteur qui ignorerait l'entree lirait `vUV`
	// dans les deux, et l'image ne bougerait jamais quoi qu'on branche.
	NkNodeGraph g1;
	const NkMatTypes t1 = NkMatRegisterTypes(g1);
	{
		const NkNodeId out = NkMatAddNode(g1, NK_MN_OUTPUT);
		const NkNodeId bsdf = NkMatAddNode(g1, NK_MN_PRINCIPLED);
		const NkNodeId tex = NkMatAddNode(g1, NK_MN_IMAGE_TEXTURE);
		g1.Connect(bsdf, "bsdf", out, "surface");
		g1.Connect(tex, "color", bsdf, "base_color");
		(void)t1;
	}
	NkMatCompileResult sansMap = NkMatCompileToNkSL(g1);

	NkNodeGraph g2;
	const NkMatTypes t2 = NkMatRegisterTypes(g2);
	const NkNodeId out2 = NkMatAddNode(g2, NK_MN_OUTPUT);
	const NkNodeId bsdf2 = NkMatAddNode(g2, NK_MN_PRINCIPLED);
	const NkNodeId tex2 = NkMatAddNode(g2, NK_MN_IMAGE_TEXTURE);
	const NkNodeId map = NkMatAddNode(g2, NK_MN_MAPPING);
	const NkNodeId coord = NkMatAddNode(g2, NK_MN_TEX_COORD);
	g2.Connect(bsdf2, "bsdf", out2, "surface");
	g2.Connect(tex2, "color", bsdf2, "base_color");
	g2.Connect(map, "vector_out", tex2, "vector");
	g2.Connect(coord, "uv", map, "vector");
	const float32 ech[3] = {4.f, 4.f, 1.f};
	g2.SetSocketDefault(map, "scale", NkSocketDir::Input, NkValueVec(t2.vector, ech, 3));
	NkMatCompileResult avecMap = NkMatCompileToNkSL(g2);

	const NkString nomMap = NkFormat("n{0}_vector_out", (uint32)map);
	const bool litUV = sansMap.ok && Apres(sansMap.source, ", vUV)") > 0;
	// ⚠️ CE CAS A SURVECU A SA MUTATION, et la faute etait dans le cas. Sa
	// premiere version cherchait le NOM de la locale du Mapping n'importe ou dans
	// le shader. Sous la mutation « l'entree vector est ignoree », le noeud
	// Mapping emettait toujours SA DECLARATION -- donc le nom etait present, et le
	// cas passait au vert alors que la texture lisait l'UV brut.
	//
	// **Chercher un nom n'est pas chercher un USAGE.** On verifie donc DEUX
	// choses : que l'appel de texture CITE la locale du mappage, et que le shader
	// mappe ne lit PLUS l'UV brut dans son appel de texture.
	const NkString appelMappe = NkFormat(", ({0}).xy)", nomMap);
	const bool litMap = avecMap.ok && Apres(avecMap.source, appelMappe.CStr()) > 0;
	const bool nePlusLireUV = avecMap.ok && Apres(avecMap.source, ", vUV)") < 0;
	// Le neutre MULTIPLICATIF : un Mapping sans echelle renseignee ne doit rien
	// changer. Le neutre general du compilateur est le NOIR — ici ce serait
	// annuler l image. Le cas verifie donc que l echelle posee (4) est bien la.
	// Le litteral porte son point decimal : PutLit le force, et ce cas a d abord
	// echoue parce que j attendais « vec3(4, 4, 1) ». L attendu etait faux, pas le code.
	const bool echelle = avecMap.ok && ContientSansCasse(avecMap.source, "vec3(4.0, 4.0, 1.0)");
	NkString be, err;
	const uint32 ok = avecMap.ok ? CompileSurLesBackends(avecMap.source, be, &err) : 0u;
	Cas("imgtex/uv-par-defaut-et-mappage", litUV && litMap && nePlusLireUV && ok == 5 && echelle,
		NkFormat("sans mappage lit vUV={0} | l'appel de texture CITE {1}={2} | ne lit plus l'UV brut={3} | echelle 4 "
				 "presente={4} | {5}",
				 litUV ? 1 : 0, nomMap, litMap ? 1 : 0, nePlusLireUV ? 1 : 0, echelle ? 1 : 0, be));
}


// ── Normal Map, Bump, Separate XYZ : le relief ───────────────────────────────

// Monte `Image Texture -> Normal Map -> Principled.normal`, avec la convention
// demandee posee sur la texture.
static NkMatCompileResult CompileCarteNormales(const char *convention) {
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId nm = NkMatAddNode(g, NK_MN_NORMAL_MAP);
	const NkNodeId tex = NkMatAddNode(g, NK_MN_IMAGE_TEXTURE);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(nm, "normal", bsdf, "normal");
	g.Connect(tex, "color", nm, "color");
	g.SetProp(tex, NK_MPROP_IMAGE, NkValueText(t.ramp, "relief.png"));
	if (convention)
		g.SetProp(tex, NK_MPROP_NORMAL_CONV, NkValueText(t.ramp, convention));
	return NkMatCompileToNkSL(g);
}

static void CasConventionNeChangePasLeShader() {
	// ⚠️ LE CAS QUI DECIDE, et il teste une ABSENCE d'effet.
	//
	// La convention d'une carte de normales est une donnee de PROVENANCE DU
	// FICHIER : une carte DirectX se convertit A L'IMPORT, jamais dans le
	// shader. La tentation naturelle est de retourner Y par pixel — ca marche,
	// et ca coute a chaque fragment tout en rendant l'etat de la texture
	// invisible dans la donnee.
	//
	// DISCRIMINE : les deux shaders doivent etre identiques AU CARACTERE PRES.
	// Un retournement dans le shader ajouterait un signe quelque part, et aucune
	// comparaison de comportement ne le verrait — seule l'egalite des textes le
	// denonce.
	NkMatCompileResult gl = CompileCarteNormales("opengl");
	NkMatCompileResult dx = CompileCarteNormales("directx");
	NkMatCompileResult sans = CompileCarteNormales(nullptr);
	const bool identiques = gl.ok && dx.ok && sans.ok && (gl.source == dx.source) && (gl.source == sans.source);
	Cas("normalmap/convention-ne-change-pas-le-shader", identiques,
		NkFormat("opengl {0} o | directx {1} o | sans convention {2} o | les trois identiques au caractere pres={3}",
				 (uint32)gl.source.Size(), (uint32)dx.source.Size(), (uint32)sans.source.Size(),
				 identiques ? 1 : 0));
}

static void CasConventionInconnueRefusee() {
	// Meme discipline que les operations : un mot inconnu est REFUSE en le
	// nommant. Un repli sur OpenGL inverserait le relief de la moitie des
	// fichiers, et l'image resterait plausible — le pire des cas.
	NkMatCompileResult r = CompileCarteNormales("mikktspace_inverse");
	const bool nomme = !r.ok && Apres(r.error, "mikktspace_inverse") > 0;
	Cas("normalmap/convention-inconnue-refusee", !r.ok && nomme && r.source.Size() == 0,
		NkFormat("refuse={0} en la nommant={1} rien emis={2} | message : {3}", r.ok ? 0 : 1, nomme ? 1 : 0,
				 r.source.Size() == 0 ? 1 : 0, r.error));
}

static void CasBaseTangenteSeulementSiUtile() {
	// Une base tangente coute DEUX paires de derivees par pixel. Un shader qui
	// la calculerait sans s'en servir paierait pour rien.
	//
	// DISCRIMINE dans les deux sens : absente quand aucun noeud ne la reclame,
	// PRESENTE des qu'un `Normal Map` apparait. Un controle a sens unique
	// laisserait passer « on ne l'emet jamais », ce qui casserait le relief.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	(void)t;
	NkMatCompileResult sans = NkMatCompileToNkSL(g);
	NkMatCompileResult avec = CompileCarteNormales("opengl");
	const bool absente = sans.ok && Apres(sans.source, "nkT") < 0;
	const bool presente = avec.ok && Apres(avec.source, "nkT") > 0 && ContientSansCasse(avec.source, "dFdx(vUV)");
	NkString be, err;
	const uint32 ok = avec.ok ? CompileSurLesBackends(avec.source, be, &err) : 0u;
	Cas("normalmap/base-tangente-seulement-si-utile", absente && presente && ok == 5,
		NkFormat("sans Normal Map : base tangente absente={0} | avec : presente={1} | {2}", absente ? 1 : 0,
				 presente ? 1 : 0, be));
}

static void CasNormaleVoyageJusquAuPuits() {
	// ⚠️ LE DEFAUT QUE CE CAS EXISTE POUR ATTRAPER, et il a vraiment existe :
	// avant le 22/08 le puits recalculait `normalize(vNormal)` et jetait EN
	// SILENCE tout travail de relief en amont. Le shader compilait, l'image etait
	// plausible, et le noeud `Normal Map` ne servait a rien.
	//
	// DISCRIMINE : le puits doit lire la locale de la CHAINE, pas la normale
	// geometrique. On verifie que `surfNormal` cite la locale du Normal Map.
	// On monte le graphe ici pour disposer des IDENTIFIANTS : la chaine a
	// verifier compte DEUX maillons, et mon premier attendu n'en voyait qu'un.
	// La normale ne saute pas du Normal Map au puits — elle passe PAR le
	// Principled, qui la porte comme composante de son shader.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId nm = NkMatAddNode(g, NK_MN_NORMAL_MAP);
	const NkNodeId tex = NkMatAddNode(g, NK_MN_IMAGE_TEXTURE);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(nm, "normal", bsdf, "normal");
	g.Connect(tex, "color", nm, "color");
	g.SetProp(tex, NK_MPROP_IMAGE, NkValueText(t.ramp, "relief.png"));
	NkMatCompileResult r = NkMatCompileToNkSL(g);

	// Maillon 1 : le Principled prend la normale du Normal Map.
	const NkString m1 = NkFormat("n{0}_normal = n{1}_normal", (uint32)bsdf, (uint32)nm);
	// Maillon 2 : le puits prend celle du Principled.
	const NkString m2 = NkFormat("surfNormal = n{0}_normal", (uint32)bsdf);
	const bool maillon1 = r.ok && Apres(r.source, m1.CStr()) > 0;
	const bool maillon2 = r.ok && Apres(r.source, m2.CStr()) > 0;
	// Et l'eclairage s'en sert VRAIMENT : c'est ce qui manquait avant le 22/08.
	const bool puitsUtilise = r.ok && Apres(r.source, "normalize(surfNormal)") > 0;
	// ⚠️ Assertion d'ABSENCE, en plus : le puits ne doit PLUS recalculer la
	// normale geometrique pour eclairer. Sans elle, un puits qui ferait les deux
	// passerait les trois presences ci-dessus.
	const bool neRecalculePlus = r.ok && Apres(r.source, "N3 = normalize(vNormal)") < 0;
	Cas("normal/voyage-jusqu-au-puits", r.ok && maillon1 && maillon2 && puitsUtilise && neRecalculePlus,
		NkFormat("maillon '{0}'={1} | maillon '{2}'={3} | eclaire avec={4} | ne recalcule plus la geometrique={5}",
				 m1, maillon1 ? 1 : 0, m2, maillon2 ? 1 : 0, puitsUtilise ? 1 : 0, neRecalculePlus ? 1 : 0));
}

static void CasBumpDeriveesEtGarde() {
	// `Bump` derive une normale d'un champ de hauteur par derivees d'ecran. Le
	// scalaire variable vient d'un `Separate XYZ` sur la coordonnee de texture —
	// sans lui, aucun scalaire VARIABLE n'existe dans un graphe et le relief ne
	// pourrait etre mesure que sur son absence d'effet.
	//
	// DISCRIMINE : le determinant DOIT etre garde. Une division nue produirait un
	// NaN sur un triangle degenere ou vu par la tranche — et un NaN contamine
	// l'aval en changeant d'aspect d'un backend a l'autre, donc il fait accuser
	// la machine.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId bump = NkMatAddNode(g, NK_MN_BUMP);
	const NkNodeId sep = NkMatAddNode(g, NK_MN_SEPARATE_XYZ);
	const NkNodeId coord = NkMatAddNode(g, NK_MN_TEX_COORD);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(bump, "normal", bsdf, "normal");
	g.Connect(sep, "x", bump, "height");
	g.Connect(coord, "uv", sep, "vector");
	g.SetSocketDefault(bump, "strength", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const bool derive = r.ok && ContientSansCasse(r.source, "dFdx(n3_h)");
	const bool garde = r.ok && ContientSansCasse(r.source, "< 1e-12 ? 1e-12");
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("bump/derivees-et-garde-du-determinant", r.ok && ok == 5 && derive && garde,
		NkFormat("{0}| derivee de la hauteur presente={1} | determinant garde={2}", be, derive ? 1 : 0,
				 garde ? 1 : 0));
}

static void CasSepareTroisSorties() {
	// `Separate XYZ` est le second noeud a sorties multiples, et le premier a en
	// avoir TROIS. DISCRIMINE : les trois locales doivent exister ET etre
	// distinctes. Un emetteur qui reutiliserait un nom unique ferait lire la
	// meme composante aux trois entrees — un shader qui compile et un materiau
	// faux.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId sep = NkMatAddNode(g, NK_MN_SEPARATE_XYZ);
	const NkNodeId coord = NkMatAddNode(g, NK_MN_TEX_COORD);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(coord, "uv", sep, "vector");
	g.Connect(sep, "x", bsdf, "metallic");
	g.Connect(sep, "y", bsdf, "roughness");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkString nx = NkFormat("n{0}_x", (uint32)sep);
	const NkString ny = NkFormat("n{0}_y", (uint32)sep);
	const NkString nz = NkFormat("n{0}_z", (uint32)sep);
	const bool trois = r.ok && Apres(r.source, nx.CStr()) > 0 && Apres(r.source, ny.CStr()) > 0 &&
					   Apres(r.source, nz.CStr()) > 0;
	// Et le BSDF doit lire DEUX composantes DIFFERENTES.
	const NkString lecture = NkFormat("float surfMetallic");
	const bool distinctes = r.ok && Apres(r.source, NkFormat("n{0}_metallic = n{1}_x", (uint32)bsdf,
															 (uint32)sep).CStr()) > 0 &&
							Apres(r.source, NkFormat("n{0}_roughness = n{1}_y", (uint32)bsdf, (uint32)sep).CStr()) > 0;
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	(void)t;
	(void)lecture;
	Cas("separate/trois-sorties-distinctes", r.ok && ok == 5 && trois && distinctes,
		NkFormat("{0}| trois locales presentes={1} | le BSDF lit x et y separement={2}", be, trois ? 1 : 0,
				 distinctes ? 1 : 0));
}


// ── Les parametres EXPOSES : l'API publique du moteur ────────────────────────

// Pose une exposition sur une prise : la cle est `expose.<prise>`, la charge
// utile est le nom PUBLIC, et deux reels optionnels sont les bornes.
static void Expose(NkNodeGraph &g, const NkMatTypes &t, NkNodeId n, const char *prise, const char *nomPublic,
				   bool bornes = false, float32 mini = 0.f, float32 maxi = 1.f) {
	NkString cle(NK_MPROP_EXPOSE_PREFIX);
	cle.Append(prise);
	NkGraphValue v = NkValueText(t.real, nomPublic);
	if (bornes) {
		v.numbers.PushBack(mini);
		v.numbers.PushBack(maxi);
	}
	g.SetProp(n, cle.CStr(), v);
}

// Un Principled minimal vers la sortie, pour poser des expositions dessus.
static NkNodeId MontePrincipledExposable(NkNodeGraph &g, const NkMatTypes &t) {
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 0.35f));
	return bsdf;
}

static void CasExposeLitLeBloc() {
	// DISCRIMINE : une prise exposee doit lire le BLOC UNIFORME, jamais un
	// litteral. C'est tout l'objet de l'exposition — une valeur repliee dans le
	// code ne peut plus changer a l'execution. On verifie donc la PRESENCE de la
	// lecture ET l'ABSENCE du litteral qui aurait ete emis sans exposition.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId bsdf = MontePrincipledExposable(g, t);
	Expose(g, t, bsdf, "roughness", "usure");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	// SONDE DEMANDEE PAR LE COORDINATEUR : ecrire la source EMISE dans un
	// fichier et la regarder. Volontairement par fwrite et NON par le
	// journal : une source de shader est pleine d ACCOLADES LITTERALES, et
	// c est exactement ce que le formateur du depot detruit. Mesurer avec
	// l outil qui a cause la panne serait la meme faute une fois de plus.
	if (getenv("NK_DUMP_EXPOSE") && r.ok) {
		FILE *f = fopen("mesures_matgraph/source_expose.nksl", "wb");
		if (f) {
			fwrite(r.source.CStr(), 1, (size_t)r.source.Size(), f);
			fclose(f);
		}
	}
	const bool lit = r.ok && Apres(r.source, "nkParams.usure") > 0;
	const bool plusDeLitteral = r.ok && Apres(r.source, "roughness = 0.35") < 0;
	const bool bloc = r.ok && Apres(r.source, "uniform NkGraphParams") > 0;
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("variable/expose-lit-le-bloc", r.ok && ok == 5 && lit && plusDeLitteral && bloc && r.params.Size() == 1,
		NkFormat("{0}| bloc declare={1} lit nkParams.usure={2} | le litteral 0.35 a disparu={3} | {4} parametre(s)",
				 be, bloc ? 1 : 0, lit ? 1 : 0, plusDeLitteral ? 1 : 0, (uint32)r.params.Size()));
}

static void CasDecalagesStd140() {
	// ⚠️ CE CAS A ETE VERT PENDANT TROIS JOURS EN NE PROUVANT RIEN. Lis la
	// suite avant de lui faire confiance.
	//
	// Il verifie que reel, vec3, reel donnent 0/16/28 et non 0/4/16. C est
	// juste, et c est ce que `std140` prescrit. Mais il compare la table de
	// decalages du compilateur A ELLE-MEME : il ne traverse jamais HLSL, et
	// HLSL ne range pas un cbuffer comme `std140`. Un `float3` n a pas le
	// droit de CHEVAUCHER une frontiere de 16 octets, mais il a le droit d en
	// PARTAGER une : apres un `float`, HLSL le place a 4 quand `std140` le
	// place a 16. Pendant que ce cas etait vert, le moteur ecrivait a 16 et le
	// shader lisait a 4, et le pixel etait noir sans un mot.
	//
	// 🔴 UN CONTROLE QUI VERIFIE UNE CONVENTION NE VERIFIE PAS UN ACCORD.
	// Le seul controle qui pouvait attraper cela LIT LA VALEUR DEPUIS LA CARTE :
	// c est `rendu/parametre-expose-pilote-le-pixel`, dans NkMatGraphDemo, et
	// il compare deux variantes dont une seule aurait suffi a rassurer.
	//
	// Ce cas GARDE quand meme deux choses, et c est pour cela qu il reste :
	//   1. les decalages publies, contre une disposition sequentielle naive ;
	//   2. la PRESENCE du remplissage dans la source emise -- le mecanisme qui
	//      force les deux conventions a coincider. Attention : verifier qu il
	//      est present n est PAS verifier qu il fonctionne. C est le banc de
	//      rendu qui le prouve ; celui-ci empeche seulement qu on le retire par
	//      megarde en trouvant le bloc trop gros.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId bsdf = MontePrincipledExposable(g, t);
	Expose(g, t, bsdf, "metallic", "metal");    // reel  : 4 o
	Expose(g, t, bsdf, "base_color", "teinte"); // vec3  : aligne 16
	Expose(g, t, bsdf, "roughness", "usure");   // reel
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatParamExpose *a = r.ok ? r.TrouveParam("metal") : nullptr;
	const NkMatParamExpose *b = r.ok ? r.TrouveParam("teinte") : nullptr;
	const NkMatParamExpose *c = r.ok ? r.TrouveParam("usure") : nullptr;
	const bool bons = a && b && c && a->decalage == 0u && b->decalage == 16u && c->decalage == 28u &&
			  r.paramsTaille == 32u;
	// Trois reels de remplissage doivent separer `metal` de `teinte` : sans eux
	// HLSL rangerait `teinte` a 4. On exige le COMPTE, pas la simple presence du
	// mot -- un seul remplissage laisserait la moitie du desaccord en place.
	const uint32 pads = b ? b->remplissageAvant : 0u;
	const bool remplissageEmis = r.ok && Apres(r.source, "_nkPad0") > 0 && Apres(r.source, "_nkPad1") > 0 &&
			   Apres(r.source, "_nkPad2") > 0 && pads == 3u;
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("variable/decalages-et-remplissage-emis", r.ok && ok == 5 && bons && remplissageEmis,
		NkFormat("metal@{0} teinte@{1} usure@{2} | bloc={3} o | attendus 0/16/28 et 32 (PAS 0/4/16) | "
			 "remplissage avant teinte={4} (3 attendu) et emis dans la source={5} | {6}",
			 a ? a->decalage : 999u, b ? b->decalage : 999u, c ? c->decalage : 999u, r.paramsTaille, pads,
			 remplissageEmis ? 1 : 0, be));
}

static void CasPriseConnecteeEtExposeeRefusee() {
	// 🔴 LE CAS LE PLUS INSIDIEUX DE LA SERIE, et celui que ma proposition avait
	// oublie. Si une prise recoit un LIEN **et** porte une exposition, le lien
	// REMPLACE la valeur exposee : `SetFloat("usure", 0.9f)` ne fait RIEN. Le
	// materiau compile, il rend, le parametre est mort — aucune erreur, aucun
	// journal.
	//
	// ⚠️ Chez Blender le probleme ne se pose pas parce que brancher un lien FAIT
	// DISPARAITRE le widget : l'interface rend l'etat impossible. Nous n'avons
	// pas d'interface — c'est donc la validation qui doit le rendre impossible.
	//
	// DISCRIMINE : le TEMOIN est la meme exposition SANS le lien, qui doit
	// compiler. Un controle qui refuserait toute exposition passerait le premier
	// sans rien prouver.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId bsdf = MontePrincipledExposable(g, t);
	Expose(g, t, bsdf, "roughness", "usure");
	NkMatCompileResult temoin = NkMatCompileToNkSL(g);

	const NkNodeId val = NkMatAddNode(g, NK_MN_VALUE);
	const NkLinkError e = g.Connect(val, "value", bsdf, "roughness");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const bool nomme = !r.ok && Apres(r.error, "roughness") > 0 && Apres(r.error, "CONNECTEE") > 0;
	Cas("variable/prise-connectee-et-exposee-refusee",
		temoin.ok && e == NkLinkError::Ok && !r.ok && nomme && r.source.Size() == 0,
		NkFormat("temoin sans lien compile={0} | avec lien refuse={1} en nommant la prise={2} rien emis={3} | "
				 "message : {4}",
				 temoin.ok ? 1 : 0, r.ok ? 0 : 1, nomme ? 1 : 0, r.source.Size() == 0 ? 1 : 0, r.error));
}

static void CasExposeRefusNommes() {
	// QUATRE refus, QUATRE messages distincts. Ils ne se reparent pas pareil :
	// un doublon se renomme, un nom invalide se corrige, une prise inconnue
	// vient d'un renommage, et une prise shader n'est simplement pas une valeur.
	NkString m[4];
	{ // doublon de nom public
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		const NkNodeId bsdf = MontePrincipledExposable(g, t);
		Expose(g, t, bsdf, "metallic", "reglage");
		Expose(g, t, bsdf, "roughness", "reglage");
		m[0] = NkMatCompileToNkSL(g).error;
	}
	{ // nom public invalide
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		const NkNodeId bsdf = MontePrincipledExposable(g, t);
		Expose(g, t, bsdf, "roughness", "2 usures");
		m[1] = NkMatCompileToNkSL(g).error;
	}
	{ // prise inconnue -- ce que laisse un renommage de prise
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		const NkNodeId bsdf = MontePrincipledExposable(g, t);
		Expose(g, t, bsdf, "rugosite", "usure");
		m[2] = NkMatCompileToNkSL(g).error;
	}
	{ // une prise SHADER n'est pas une valeur uniforme
		// ⚠️ CE CAS A D'ABORD MESURE AUTRE CHOSE. Ma premiere version exposait
		// `Material Output.surface`, qui est CONNECTEE : c'est donc le controle
		// du lien qui repondait, et le refus « type shader » n'etait jamais
		// atteint. Les quatre messages etaient bien distincts — simplement pas
		// les quatre que je croyais tester.
		//
		// On expose donc une prise shader LIBRE : `Mix Shader.shader1`, sur un
		// graphe par ailleurs valide.
		NkNodeGraph g;
		const NkMatTypes t = NkMatRegisterTypes(g);
		const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
		const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_SHADER);
		const NkNodeId emis = NkMatAddNode(g, NK_MN_EMISSION);
		g.Connect(mix, "shader", out, "surface");
		g.Connect(emis, "emission", mix, "shader2"); // shader1 reste LIBRE
		Expose(g, t, mix, "shader1", "melange");
		m[3] = NkMatCompileToNkSL(g).error;
	}
	const bool tous = m[0].Size() && m[1].Size() && m[2].Size() && m[3].Size();
	const bool distincts = tous && !(m[0] == m[1]) && !(m[1] == m[2]) && !(m[2] == m[3]) && !(m[0] == m[3]);
	Cas("variable/refus-nommes-et-distincts", distincts,
		NkFormat("[{0}] [{1}] [{2}] [{3}] | quatre messages distincts={4}", m[0], m[1], m[2], m[3],
				 distincts ? 1 : 0));
}

static void CasDefautEstCeluiDeLaPrise() {
	// ⚠️ RENFORCEMENT (c) : le defaut d'un parametre expose EST le `defaultValue`
	// de sa prise, jamais une seconde valeur rangee a cote. Deux sources pour une
	// meme chose divergent, et c'est alors l'editeur qui montre l'une pendant que
	// le moteur envoie l'autre.
	//
	// DISCRIMINE : on CHANGE le defaut de la prise apres avoir pose l'exposition,
	// et la disposition doit suivre. Une implantation qui aurait recopie la
	// valeur au moment de l'exposition rendrait l'ancienne.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId bsdf = MontePrincipledExposable(g, t); // roughness = 0,35
	Expose(g, t, bsdf, "roughness", "usure", true, 0.f, 1.f);
	g.SetSocketDefault(bsdf, "roughness", NkSocketDir::Input, NkValueReal(t.real, 0.72f));
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatParamExpose *p = r.ok ? r.TrouveParam("usure") : nullptr;
	const bool suit = p && p->defaut.IsSet() && p->defaut.numbers.Size() == 1 && p->defaut.numbers[0] == 0.72f;
	const bool bornes = p && p->bornes && p->borneMin == 0.f && p->borneMax == 1.f;
	Cas("variable/le-defaut-est-celui-de-la-prise", r.ok && suit && bornes,
		NkFormat("defaut retenu={0} (0,72 attendu apres modification de la prise) | bornes lues={1} [{2} ; {3}]",
				 (p && p->defaut.numbers.Size() == 1) ? (double)p->defaut.numbers[0] : -1.0, bornes ? 1 : 0,
				 p ? (double)p->borneMin : -1.0, p ? (double)p->borneMax : -1.0));
}

static void CasJetonSuitLaDisposition() {
	// ⚠️ RENFORCEMENT (b) : recompiler un graphe EDITE peut reordonner le bloc.
	// Du code de jeu ayant retenu un DECALAGE ecrirait alors dans le mauvais
	// parametre — sans erreur, avec une valeur credible. Le jeton existe pour
	// qu'un cache de decalages puisse se jeter.
	//
	// DISCRIMINE dans les DEUX sens : deux compilations du meme graphe donnent le
	// MEME jeton (sinon tout cache serait inutile), et une disposition differente
	// en donne un AUTRE (sinon le jeton ne protegerait rien).
	NkNodeGraph g1;
	const NkMatTypes t1 = NkMatRegisterTypes(g1);
	const NkNodeId b1 = MontePrincipledExposable(g1, t1);
	Expose(g1, t1, b1, "metallic", "metal");
	Expose(g1, t1, b1, "roughness", "usure");
	NkMatCompileResult a1 = NkMatCompileToNkSL(g1);
	NkMatCompileResult a2 = NkMatCompileToNkSL(g1);

	NkNodeGraph g2;
	const NkMatTypes t2 = NkMatRegisterTypes(g2);
	const NkNodeId b2 = MontePrincipledExposable(g2, t2);
	Expose(g2, t2, b2, "roughness", "usure"); // ordre INVERSE
	Expose(g2, t2, b2, "metallic", "metal");
	NkMatCompileResult b = NkMatCompileToNkSL(g2);

	const bool stable = a1.ok && a2.ok && a1.jeton == a2.jeton && a1.jeton != 0;
	const bool change = b.ok && a1.jeton != b.jeton;
	// Et la disposition a REELLEMENT change : sinon le jeton changerait pour rien.
	const NkMatParamExpose *pa = a1.ok ? a1.TrouveParam("usure") : nullptr;
	const NkMatParamExpose *pb = b.ok ? b.TrouveParam("usure") : nullptr;
	const bool disposition = pa && pb && pa->decalage != pb->decalage;
	Cas("variable/jeton-suit-la-disposition", stable && change && disposition,
		NkFormat("meme graphe deux fois : jeton stable={0} | ordre inverse : jeton different={1} | et le decalage "
				 "d'usure a bouge {2}->{3} : {4}",
				 stable ? 1 : 0, change ? 1 : 0, pa ? pa->decalage : 999u, pb ? pb->decalage : 999u,
				 disposition ? 1 : 0));
}

static void CasRechercheParNom() {
	// L'API du moteur est par NOM. Un nom inconnu rend nullptr, jamais le
	// premier parametre — sinon `SetFloat("faute_de_frappe", …)` piloterait
	// silencieusement autre chose.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId bsdf = MontePrincipledExposable(g, t);
	Expose(g, t, bsdf, "metallic", "metal");
	Expose(g, t, bsdf, "roughness", "usure");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const bool trouve = r.ok && r.TrouveParam("usure") && r.TrouveParam("metal");
	const bool inconnu = r.ok && r.TrouveParam("usur") == nullptr && r.TrouveParam("usuree") == nullptr &&
						 r.TrouveParam("") == nullptr;
	Cas("variable/recherche-par-nom", r.ok && trouve && inconnu,
		NkFormat("les deux noms trouves={0} | prefixe, suffixe et vide rendent nullptr={1}", trouve ? 1 : 0,
				 inconnu ? 1 : 0));
}

static void CasSansExpositionAucunBloc() {
	// Le temoin de l'autre bord : un graphe sans exposition ne doit declarer
	// AUCUN bloc. Sans ce cas, une implantation qui emettrait toujours le bloc
	// passerait tous les autres — et ferait payer un tampon uniforme a chaque
	// materiau constant.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	MontePrincipledExposable(g, t);
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const bool aucunBloc = r.ok && Apres(r.source, "NkGraphParams") < 0;
	Cas("variable/sans-exposition-aucun-bloc", r.ok && aucunBloc && r.params.Empty() && r.paramsTaille == 0u,
		NkFormat("bloc absent={0} | {1} parametre(s) | taille={2} o", aucunBloc ? 1 : 0, (uint32)r.params.Size(),
				 r.paramsTaille));
}


static void CasBlocEtTexturesNeSeMarchentPasDessus() {
	// ⚠️ « PRESENT DANS LA TABLE » NE SUFFIT PAS : un binding peut etre declare
	// ET DEJA OCCUPE. Le controle precedent verifiait que chaque binding de
	// TEXTURE appartient a la table ; il ne disait rien du bloc uniforme, qui vit
	// dans le meme set et donc dans le meme espace de numeros.
	//
	// Ce cas monte le graphe le plus charge possible — le plafond de textures ET
	// des parametres exposes — et exige que TOUS les bindings du set 2 soient
	// deux a deux distincts. Un jour ou quelqu'un ajoutera le slot des parametres
	// a la table des textures, ce cas tombera ; sans lui, le shader declarerait
	// deux ressources au meme endroit et l'une ecraserait l'autre en silence.
	NkNodeGraph g;
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	g.Connect(bsdf, "bsdf", out, "surface");
	// Le plafond de textures, chainees par des Mix Color pour que chacune serve.
	NkNodeId precedent = NK_NODE_INVALID;
	for (uint32 i = 0; i < renderer::NK_MATBIND_GRAPH_SLOT_COUNT; ++i) {
		const NkNodeId tex = NkMatAddNode(g, NK_MN_IMAGE_TEXTURE);
		g.SetProp(tex, NK_MPROP_IMAGE, NkValueText(t.ramp, "img.png"));
		if (precedent == NK_NODE_INVALID) {
			precedent = tex;
		} else {
			const NkNodeId mix = NkMatAddNode(g, NK_MN_MIX_COLOR);
			g.Connect(precedent, "color", mix, "color1");
			g.Connect(tex, "color", mix, "color2");
			precedent = mix;
		}
	}
	g.Connect(precedent, "color", bsdf, "base_color");
	// ET des parametres exposes, donc un bloc uniforme dans le MEME set.
	Expose(g, t, bsdf, "roughness", "usure");
	Expose(g, t, bsdf, "metallic", "metal");

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	uint32 bindings[32] = {};
	const uint32 n = r.ok ? RelieveBindingsSet2(r.source, bindings, 32) : 0u;
	// textures + le bloc = plafond + 1
	const bool compte = (n == renderer::NK_MATBIND_GRAPH_SLOT_COUNT + 1u);
	bool distincts = true;
	for (uint32 i = 0; i < n; ++i)
		for (uint32 k = i + 1; k < n; ++k)
			if (bindings[i] == bindings[k])
				distincts = false;
	// Et le binding du bloc ne doit PAS figurer dans la table des textures.
	bool blocHorsDesTextures = true;
	for (uint32 i = 0; i < renderer::NK_MATBIND_GRAPH_SLOT_COUNT; ++i)
		if (renderer::NK_MATBIND_GRAPH_SLOTS[i] == renderer::NK_MATBIND_GRAPH_PARAMS)
			blocHorsDesTextures = false;
	NkString liste;
	for (uint32 i = 0; i < n; ++i)
		liste.Append(NkFormat("{0} ", bindings[i]));
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("imgtex/bloc-et-textures-ne-se-marchent-pas-dessus",
		r.ok && ok == 5 && compte && distincts && blocHorsDesTextures,
		NkFormat("{0} bindings emis [{1}] (attendu {2}) | deux a deux distincts={3} | le slot du bloc ({4}) est "
				 "hors de la table des textures={5} | {6}",
				 n, liste, renderer::NK_MATBIND_GRAPH_SLOT_COUNT + 1u, distincts ? 1 : 0,
				 (uint32)renderer::NK_MATBIND_GRAPH_PARAMS, blocHorsDesTextures ? 1 : 0, be));
}


static void CasIncludeNeSeResoutPas() {
	// SONDE : l'#include du dialecte NkSL se resout-il quand le shader est
	// compile DEPUIS UNE CHAINE et non depuis un fichier ? La reponse decide si
	// les noeuds proceduraux REUTILISENT NkNoise.glsli ou doivent l'inliner.
	NkString src;
	src.Append("#include \"Include/NkNoise.glsli\"\n\n");
	src.Append("@location(0) in vec2 vUV;\n@location(0) out vec4 fragColor;\n\n");
	src.Append("@stage(fragment)\n@entry\nvoid main() {\n");
	src.Append("    float n = NkFBM2D(vUV * 4.0, 3);\n    fragColor = vec4(n, n, n, 1.0);\n}\n");
	NkString be, err;
	const uint32 ok = CompileSurLesBackends(src, be, &err);
	// ⚠️ CE CAS ATTESTE UNE LIMITE, PAS UNE CAPACITE — et c'est voulu.
	//
	// Le `#include` du dialecte NkSL NE SE RESOUT PAS quand le shader est compile
	// depuis une CHAINE : le compilateur rend « #include not found ». Un shader
	// engendre n'existe pas sur disque, il doit donc etre AUTONOME — c'est ce qui
	// oblige a recopier les briques de NkNoise.glsli dans le compilateur.
	//
	// On garde ce cas parce qu'une limite non testee se perd : si un jour le
	// resolveur apprend a travailler depuis une chaine, CE CAS PASSERA AU ROUGE,
	// et ce sera le bon moment pour supprimer la recopie.
	Cas("nksl/include-ne-se-resout-pas-depuis-une-chaine", ok == 0,
		NkFormat("{0}| aucun backend ne resout l include (0 attendu) | message : {1}", be, err));
}


// ── Rang 2 : le procedural ───────────────────────────────────────────────────

// Monte `<noeud procedural>.<prise> -> Principled.<cible>` et compile.
static NkMatCompileResult CompileProcedural(const char *cleNoeud, const char *prise, const char *cible,
											const char *typeProp, NkNodeId *outNode = nullptr) {
	static NkNodeGraph g;
	g.Clear();
	const NkMatTypes t = NkMatRegisterTypes(g);
	const NkNodeId out = NkMatAddNode(g, NK_MN_OUTPUT);
	const NkNodeId bsdf = NkMatAddNode(g, NK_MN_PRINCIPLED);
	const NkNodeId n = NkMatAddNode(g, cleNoeud);
	g.Connect(bsdf, "bsdf", out, "surface");
	g.Connect(n, prise, bsdf, cible);
	if (typeProp)
		g.SetProp(n, NK_MPROP_TYPE, NkValueText(t.ramp, typeProp));
	if (outNode)
		*outNode = n;
	return NkMatCompileToNkSL(g);
}

static void CasProceduralCompile() {
	// Les trois noeuds proceduraux, sur les quatre backends. DISCRIMINE aussi une
	// propriete du systeme qui compte : ils ne consomment AUCUN slot de texture.
	// Un procedural qui en prendrait un reduirait le plafond sans le dire.
	struct Jeu {
			const char *cle;
			const char *prise;
			const char *cible;
	};
	const Jeu jeux[6] = {{NK_MN_NOISE, "fac", "roughness"},
						 {NK_MN_GRADIENT, "fac", "roughness"},
						 {NK_MN_CHECKER, "color", "base_color"},
						 {NK_MN_VORONOI, "distance", "roughness"},
						 {NK_MN_WAVE, "fac", "roughness"},
						 {NK_MN_BRICK, "color", "base_color"}};
	uint32 bons = 0;
	uint32 slotsUtilises = 0;
	NkString detail;
	for (uint32 i = 0; i < 6; ++i) {
		NkMatCompileResult r = CompileProcedural(jeux[i].cle, jeux[i].prise, jeux[i].cible, nullptr);
		NkString be, err;
		const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
		uint32 bind[16] = {};
		slotsUtilises += r.ok ? RelieveBindingsSet2(r.source, bind, 16) : 0u;
		if (r.ok && ok == 5)
			++bons;
		detail.Append(NkFormat("{0}={1} ", NkString(jeux[i].cle), r.ok ? be : r.error));
	}
	Cas("procedural/six-noeuds-4-backends", bons == 6 && slotsUtilises == 0,
		NkFormat("{0}| slots de texture consommes={1} (0 attendu : le procedural est gratuit en ressources)",
				 detail, slotsUtilises));
}

static void CasBriquesRecopieesVerbatim() {
	// ⚠️ LE CAS QUI GARDE LA DUPLICATION. Le shader engendre ne peut pas inclure
	// `NkNoise.glsli` (le resolveur ne travaille pas depuis une chaine, cf. le cas
	// `nksl/include-...`), donc le compilateur en RECOPIE les fonctions. Une
	// recopie sans garde diverge — c'est le motif que ce depot a paye quatre fois
	// en une nuit.
	//
	// DISCRIMINE : on lit le `.glsli` SUR LE DISQUE et on exige que chaque
	// fonction emise s'y retrouve **mot pour mot**. Le jour ou quelqu'un corrige
	// une formule dans le fichier, ce cas passe au rouge tant que le compilateur
	// n'a pas suivi. C'est la meme parade que pour les bindings : comparer le code
	// a une VERITE EXTERNE, faute de pouvoir partager.
	NkString glsli;
	if (!LireFichier("Resources/NKRenderer/Shaders/Include/NkNoise.glsli", glsli)) {
		Cas("procedural/briques-recopiees-verbatim", false, NkString("NkNoise.glsli INTROUVABLE"));
		return;
	}
	NkNodeId n = NK_NODE_INVALID;
	NkMatCompileResult r = CompileProcedural(NK_MN_NOISE, "fac", "roughness", nullptr, &n);
	// Les signatures ET un morceau de corps de chacune : une signature seule
	// passerait alors que le corps aurait diverge.
	const char *morceaux[6] = {"float NkHash2(vec2 p) {",
							   "return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);",
							   "float NkValueNoise2D(vec2 p) {",
							   "vec2 u = f * f * (3.0 - 2.0 * f);",
							   "float NkFBM2D(vec2 p, int octaves) {",
							   "value += NkValueNoise2D(p) * amp;"};
	uint32 dansLeShader = 0, dansLeFichier = 0;
	for (uint32 i = 0; i < 6; ++i) {
		if (r.ok && Apres(r.source, morceaux[i]) > 0)
			++dansLeShader;
		if (Apres(glsli, morceaux[i]) > 0)
			++dansLeFichier;
	}
	Cas("procedural/briques-recopiees-verbatim", r.ok && dansLeShader == 6 && dansLeFichier == 6,
		NkFormat("{0}/6 morceaux dans le shader emis | {1}/6 dans NkNoise.glsli sur le disque | identiques={2}",
				 dansLeShader, dansLeFichier, (dansLeShader == 6 && dansLeFichier == 6) ? 1 : 0));
}

static void CasBriquesSeulementSiUtiles() {
	// Un shader qui porterait les fonctions de bruit sans s'en servir alourdirait
	// chaque materiau pour rien. DISCRIMINE dans les DEUX sens : absentes sans
	// noeud de bruit, presentes des qu'il y en a un — un controle a sens unique
	// laisserait passer « on ne les emet jamais ».
	NkMatCompileResult sans = CompileProcedural(NK_MN_CHECKER, "color", "base_color", nullptr);
	NkMatCompileResult avec = CompileProcedural(NK_MN_NOISE, "fac", "roughness", nullptr);
	const bool absentes = sans.ok && Apres(sans.source, "NkFBM2D") < 0;
	const bool presentes = avec.ok && Apres(avec.source, "NkFBM2D") > 0;
	Cas("procedural/briques-seulement-si-utiles", absentes && presentes,
		NkFormat("sans noeud de bruit : absentes={0} | avec : presentes={1}", absentes ? 1 : 0,
				 presentes ? 1 : 0));
}

static void CasDegradeTypesEtRefus() {
	// Les quatre types de degrade emettent des codes DIFFERENTS et compilent tous.
	// Un type inconnu est refuse en le nommant — meme discipline que partout.
	//
	// DISCRIMINE : on verifie que les quatre sources emises sont deux a deux
	// DIFFERENTES. Un emetteur qui ignorerait le type les rendrait identiques, et
	// un simple « ca compile » ne le verrait pas.
	NkString sources[4];
	uint32 ok4 = 0;
	for (uint32 i = 0; i < NkMatTypeDegradeCount(); ++i) {
		NkMatCompileResult r = CompileProcedural(NK_MN_GRADIENT, "fac", "roughness",
												 NkMatTypeDegradeAt(i)->cle);
		NkString be, err;
		if (r.ok && CompileSurLesBackends(r.source, be, &err) == 5)
			++ok4;
		sources[i] = r.source;
	}
	bool tousDifferents = true;
	for (uint32 i = 0; i < 4; ++i)
		for (uint32 k = i + 1; k < 4; ++k)
			if (sources[i] == sources[k])
				tousDifferents = false;
	NkMatCompileResult inc = CompileProcedural(NK_MN_GRADIENT, "fac", "roughness", "spirale_logarithmique");
	const bool nomme = !inc.ok && Apres(inc.error, "spirale_logarithmique") > 0;
	Cas("procedural/degrade-types-et-refus",
		ok4 == NkMatTypeDegradeCount() && tousDifferents && nomme && inc.source.Size() == 0,
		NkFormat("{0}/{1} types compilent sur 4 backends | les 4 sources sont differentes={2} | type inconnu "
				 "refuse en le nommant={3}",
				 ok4, NkMatTypeDegradeCount(), tousDifferents ? 1 : 0, nomme ? 1 : 0));
}

static void CasOctavesBornees() {
	// ⚠️ Le nombre d'octaves vient d'une VALEUR DU GRAPHE, donc potentiellement
	// d'un parametre expose : une boucle dont le compte est libre peut ne pas se
	// derouler, et certains backends refusent alors le shader. Le borner coute
	// deux appels ; ne pas le borner coute un shader qui compile ici et pas
	// ailleurs.
	//
	// DISCRIMINE : la borne doit etre DANS LE CODE EMIS, pas appliquee a la
	// valeur au moment de la compilation — sinon un `detail` branche sur un autre
	// noeud echapperait a la borne.
	NkNodeId n = NK_NODE_INVALID;
	NkMatCompileResult r = CompileProcedural(NK_MN_NOISE, "fac", "roughness", nullptr, &n);
	const bool borne = r.ok && ContientSansCasse(r.source, "clamp(") &&
					   ContientSansCasse(r.source, ", 1.0, 8.0)");
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("procedural/octaves-bornees-dans-le-shader", r.ok && ok == 5 && borne,
		NkFormat("{0}| borne [1,8] presente dans le code emis={1}", be, borne ? 1 : 0));
}


static void CasOndeTypesEtRefus() {
	// Meme discipline que le degrade : deux types, deux codes DIFFERENTS, et un
	// type inconnu refuse en le nommant.
	NkString a, b;
	uint32 ok = 0;
	for (uint32 i = 0; i < NkMatTypeOndeCount(); ++i) {
		NkMatCompileResult r = CompileProcedural(NK_MN_WAVE, "fac", "roughness", NkMatTypeOndeAt(i)->cle);
		NkString be, err;
		if (r.ok && CompileSurLesBackends(r.source, be, &err) == 5)
			++ok;
		if (i == 0)
			a = r.source;
		else
			b = r.source;
	}
	NkMatCompileResult inc = CompileProcedural(NK_MN_WAVE, "fac", "roughness", "dents_de_scie");
	const bool nomme = !inc.ok && Apres(inc.error, "dents_de_scie") > 0;
	Cas("procedural/onde-types-et-refus", ok == NkMatTypeOndeCount() && !(a == b) && nomme,
		NkFormat("{0}/{1} types compilent | bandes et anneaux different={2} | inconnu refuse en le nommant={3}", ok,
				 NkMatTypeOndeCount(), (a == b) ? 0 : 1, nomme ? 1 : 0));
}

static void CasVoronoiBorneFixe() {
	// ⚠️ Le voisinage de Voronoi est DEROULE sur 3x3, une borne connue a la
	// compilation. Un rayon variable ferait une boucle que certains backends
	// refusent de derouler — donc un shader qui compile ici et pas ailleurs, le
	// defaut le plus penible a diagnostiquer parce qu'il accuse la machine.
	//
	// DISCRIMINE : les bornes litterales doivent etre DANS le code emis, et la
	// fonction ne doit apparaitre QUE si un Voronoi est present.
	NkMatCompileResult avec = CompileProcedural(NK_MN_VORONOI, "distance", "roughness", nullptr);
	NkMatCompileResult sans = CompileProcedural(NK_MN_CHECKER, "color", "base_color", nullptr);
	const bool bornes = avec.ok && ContientSansCasse(avec.source, "for (int j = -1; j <= 1; ++j)") &&
						ContientSansCasse(avec.source, "for (int i = -1; i <= 1; ++i)");
	const bool absenteSansVoronoi = sans.ok && Apres(sans.source, "NkVoronoiF1") < 0;
	const bool presenteAvec = avec.ok && Apres(avec.source, "NkVoronoiF1") > 0;
	// ⚠️ ET SA DEPENDANCE. `NkVoronoiF1` appelle `NkHash22`, qui vient du bloc des
	// briques de bruit. Une mutation retirant Voronoi de la liste des noeuds qui
	// reclament ces briques a SURVECU a la premiere version de ce cas : la
	// fonction etait bien la, son hachage non, et rien ne le disait.
	//
	// Verifier qu'une fonction est emise ne verifie pas que ce qu'elle APPELLE
	// l'est. C'est la meme famille que « chercher un nom n'est pas chercher un
	// usage », prise par l'autre bout.
	const bool dependanceDeclaree = avec.ok && Apres(avec.source, "vec2 NkHash22(vec2 p)") > 0;
	NkString be, err;
	const uint32 ok = avec.ok ? CompileSurLesBackends(avec.source, be, &err) : 0u;
	Cas("procedural/voronoi-voisinage-borne",
		ok == 5 && bornes && absenteSansVoronoi && presenteAvec && dependanceDeclaree,
		NkFormat("{0}| voisinage 3x3 litteral={1} | fonction absente sans Voronoi={2} presente avec={3} | sa "
				 "dependance NkHash22 declaree={4}",
				 be, bornes ? 1 : 0, absenteSansVoronoi ? 1 : 0, presenteAvec ? 1 : 0, dependanceDeclaree ? 1 : 0));
}

static void CasBriquesJointsDecales() {
	// DISCRIMINE : sans le decalage d'une rangee sur deux, on obtient un
	// QUADRILLAGE — un mur parfaitement plausible, et faux. On verifie que le
	// decalage est calcule (`mod(rangee, 2.0) * 0.5`) et qu'il entre bien dans la
	// coordonnee de brique.
	NkNodeId n = NK_NODE_INVALID;
	NkMatCompileResult r = CompileProcedural(NK_MN_BRICK, "color", "base_color", nullptr, &n);
	const NkString dec = NkFormat("n{0}_dec = mod(n{0}_rangee, 2.0) * 0.5", (uint32)n);
	const NkString usage = NkFormat("fract(n{0}_q.x * 0.5 + n{0}_dec)", (uint32)n);
	const bool calcule = r.ok && Apres(r.source, dec.CStr()) > 0;
	// ⚠️ Presence ET usage : un decalage calcule mais jamais employe donnerait
	// exactement le quadrillage qu'on veut eviter, et le premier controle seul
	// passerait. C'est la lecon du Mapping, appliquee ici d'avance.
	const bool employe = r.ok && Apres(r.source, usage.CStr()) > 0;
	NkString be, err;
	const uint32 ok = r.ok ? CompileSurLesBackends(r.source, be, &err) : 0u;
	Cas("procedural/briques-joints-decales", ok == 5 && calcule && employe,
		NkFormat("{0}| decalage calcule={1} | ET employe dans la coordonnee={2}", be, calcule ? 1 : 0,
				 employe ? 1 : 0));
}

// ═══════════════════════════════════════════════════════════════════════════
//  (a) LES SORTIES NOMMEES, PAR MATERIAU, COTE PROCESSEUR
// ═══════════════════════════════════════════════════════════════════════════

// Pose un noeud de sortie nommee complet. Le nom et l'etage sont des MOTS, et
// l'etage n'a PAS de defaut : il faut le dire a chaque fois, ici comme partout.
static NkNodeId PoseSortie(NkNodeGraph &g, const NkMatTypes &t, const char *nom, const char *etage) {
	const NkNodeId s = NkMatAddNode(g, NK_MN_OUTPUT_VALUE);
	if (nom)
		g.SetProp(s, NK_MPROP_SORTIE_NOM, NkValueText(t.real, nom));
	if (etage)
		g.SetProp(s, NK_MPROP_SORTIE_ETAGE, NkValueText(t.real, etage));
	return s;
}

static bool Proche(float32 a, float32 b) {
	const float32 d = a - b;
	return (d < 0.f ? -d : d) < 0.0005f;
}

static void CasSortieValeurCalculee() {
	// ⚠️ L'ATTENDU EST CALCULABLE A LA MAIN, et c'est tout l'interet :
	// 0.25 * 4 = 1.0. Un banc qui se contenterait de « une sortie existe »
	// passerait avec un evaluateur qui rend zero.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	MonteUnPrincipled(g, t, &out);

	const NkNodeId v = NkMatAddNode(g, NK_MN_VALUE);
	g.SetProp(v, NK_MPROP_VALUE, NkValueReal(t.real, 0.25f));
	const NkNodeId m = NkMatAddNode(g, NK_MN_MATH);
	g.SetProp(m, NK_MPROP_OPERATION, NkValueText(t.real, "multiplier"));
	g.Connect(v, "value", m, "a");
	g.SetSocketDefault(m, "b", NkSocketDir::Input, NkValueReal(t.real, 4.0f));
	const NkNodeId s = PoseSortie(g, t, "usure", "par_materiau");
	g.Connect(m, "value", s, "value");

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatSortieMateriau *so = r.ok ? r.TrouveSortie("usure") : nullptr;
	const bool bonne = so && so->composantes == 1 && Proche(so->valeur[0], 1.0f);
	// Une sortie d'etage (a) ne doit produire AUCUNE ligne de shader : c'est
	// ce qui la rend quasi gratuite. Si le nom public apparaissait dans le
	// NkSL, l'argument de cout serait faux sans que rien ne le dise.
	const bool riendansleshader = r.ok && !ContientSansCasse(r.source, "usure");
	// Aucune dependance : la chaine est entierement constante, donc la valeur
	// ne sera JAMAIS a reevaluer.
	const bool constante = so && so->dependDe.Size() == 0;
	Cas("sortie/par-materiau-valeur-calculee", bonne && riendansleshader && constante,
		NkFormat("ok={0} | valeur={1} (attendu 1.0 : 0.25 * 4) | composantes={2} | absente du shader={3} | "
				 "dependances={4} (0 attendu)",
				 r.ok ? 1 : 0, so ? so->valeur[0] : -1.f, so ? so->composantes : 0u,
				 riendansleshader ? 1 : 0, so ? (uint32)so->dependDe.Size() : 999u));
}

static void CasSortieCouleurEtRampe() {
	// Deux choses d'un coup, et la seconde est la plus utile : la rampe est la
	// seule formule non triviale que l'evaluateur reproduit. Rampe par defaut
	// noir -> blanc, lue en 0.5 => gris 0.5 exactement.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	MonteUnPrincipled(g, t, &out);

	const NkNodeId ramp = NkMatAddNode(g, NK_MN_COLOR_RAMP);
	g.SetSocketDefault(ramp, "fac", NkSocketDir::Input, NkValueReal(t.real, 0.5f));
	const NkNodeId s = PoseSortie(g, t, "teinte", "par_materiau");
	g.Connect(ramp, "color", s, "color");

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatSortieMateriau *so = r.ok ? r.TrouveSortie("teinte") : nullptr;
	const bool trois = so && so->composantes == 3;
	const bool gris = so && Proche(so->valeur[0], 0.5f) && Proche(so->valeur[1], 0.5f) &&
					  Proche(so->valeur[2], 0.5f);
	Cas("sortie/couleur-rampe-evaluee", trois && gris,
		NkFormat("composantes={0} (3 attendu) | valeur=({1}, {2}, {3}) attendu (0.5, 0.5, 0.5) : rampe noir "
				 "vers blanc lue en 0.5 | erreur={4}",
				 so ? so->composantes : 0u, so ? so->valeur[0] : -1.f, so ? so->valeur[1] : -1.f,
				 so ? so->valeur[2] : -1.f, r.error));
}

static void CasSortieRefusParPixel() {
	// 🔴 LE CAS QUI JUSTIFIE TOUTE LA PASSE.
	//
	// Le noeud par pixel est a DEUX noeuds d'ecart de la sortie : c'est la
	// situation reelle. Une verification qui ne regarderait que le voisin
	// immediat serait verte ici et laisserait passer le graphe.
	//
	// Et le refus doit NOMMER le coupable : « depend du pixel » tout court
	// obligerait l'auteur a fouiller son graphe entier.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	MonteUnPrincipled(g, t, &out);

	const NkNodeId bruit = NkMatAddNode(g, NK_MN_NOISE);
	const NkNodeId m1 = NkMatAddNode(g, NK_MN_MATH);
	const NkNodeId m2 = NkMatAddNode(g, NK_MN_MATH);
	g.Connect(bruit, "fac", m1, "a");
	g.Connect(m1, "value", m2, "a");
	const NkNodeId s = PoseSortie(g, t, "densite", "par_materiau");
	g.Connect(m2, "value", s, "value");

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const bool refuse = !r.ok;
	// ⚠️ NE PAS SE CONTENTER DE « mat.bruit APPARAIT DANS LE MESSAGE ».
	// Mutation du 22/08 : en supprimant la CONTAGION, le graphe est quand meme
	// refuse — mais par l EVALUATEUR, qui ne sait pas evaluer un bruit et
	// nomme lui aussi « mat.bruit ». Le cas restait vert en ne mesurant plus
	// rien. On exige donc le MOTIF du refus, pas seulement le nom du coupable :
	// c est la quatrieme fois dans ce chantier que chercher un nom se revele
	// n etre pas chercher un usage.
	const bool bonMotif = Apres(r.error, "depend du pixel") > 0;
	const bool nommeLeCoupable = bonMotif && Apres(r.error, "mat.bruit") > 0;
	const bool nommeLaSortie = Apres(r.error, "densite") > 0;
	// Un refus doit laisser le resultat VIDE. Rendre a la fois une erreur et
	// une source utilisable inviterait un appelant a ignorer l'erreur.
	const bool vide = !r.ok && r.source.Size() == 0 && r.sorties.Size() == 0;
	Cas("sortie/refus-par-pixel-a-deux-noeuds-en-nommant",
		refuse && nommeLeCoupable && nommeLaSortie && vide,
		NkFormat("refuse={0} | nomme le noeud coupable={1} | nomme la sortie={2} | resultat vide={3} | "
				 "message={4}",
				 refuse ? 1 : 0, nommeLeCoupable ? 1 : 0, nommeLaSortie ? 1 : 0, vide ? 1 : 0, r.error));
}

static void CasSortieMappageNEstPasParPixel() {
	// ⚠️ LE CAS QUI DISCRIMINE UNE CLASSIFICATION FAITE A VUE.
	//
	// `Mapping` a tout l'air d'un noeud de texture. Il ne fabrique pourtant
	// aucune coordonnee : il transforme celle qu'on lui donne. Le MEME noeud
	// est donc acceptable ou refuse selon son amont — et c'est la seule facon
	// de prouver que la contagion se propage vraiment au lieu d'etre une
	// liste de noms.
	NkNodeGraph g1;
	NkMatTypes t1 = NkMatRegisterTypes(g1);
	NkNodeId o1;
	MonteUnPrincipled(g1, t1, &o1);
	const NkNodeId mp1 = NkMatAddNode(g1, NK_MN_MAPPING);
	const float32 trois[3] = {3.f, 0.f, 0.f};
	g1.SetSocketDefault(mp1, "vector", NkSocketDir::Input, NkValueVec(t1.vector, trois, 3));
	const NkNodeId sx1 = NkMatAddNode(g1, NK_MN_SEPARATE_XYZ);
	g1.Connect(mp1, "vector_out", sx1, "vector");
	const NkNodeId s1 = PoseSortie(g1, t1, "largeur", "par_materiau");
	g1.Connect(sx1, "x", s1, "value");
	NkMatCompileResult r1 = NkMatCompileToNkSL(g1);
	const NkMatSortieMateriau *so1 = r1.ok ? r1.TrouveSortie("largeur") : nullptr;
	// L'echelle par defaut vaut 1 (neutre MULTIPLICATIF), la position 0 :
	// 3 * 1 + 0 = 3. Un evaluateur qui prendrait 0 comme echelle par defaut
	// rendrait 0, ce qui reste parfaitement plausible pour une largeur.
	const bool accepte = so1 && Proche(so1->valeur[0], 3.f);

	NkNodeGraph g2;
	NkMatTypes t2 = NkMatRegisterTypes(g2);
	NkNodeId o2;
	MonteUnPrincipled(g2, t2, &o2);
	const NkNodeId tc = NkMatAddNode(g2, NK_MN_TEX_COORD);
	const NkNodeId mp2 = NkMatAddNode(g2, NK_MN_MAPPING);
	g2.Connect(tc, "uv", mp2, "vector");
	const NkNodeId sx2 = NkMatAddNode(g2, NK_MN_SEPARATE_XYZ);
	g2.Connect(mp2, "vector_out", sx2, "vector");
	const NkNodeId s2 = PoseSortie(g2, t2, "largeur", "par_materiau");
	g2.Connect(sx2, "x", s2, "value");
	NkMatCompileResult r2 = NkMatCompileToNkSL(g2);
	// Meme garde que dans le cas precedent, et pour la meme mutation : sans
	// contagion, l evaluateur refuse en nommant « mat.coord_texture » lui aussi.
	const bool refuse = !r2.ok && Apres(r2.error, "depend du pixel") > 0 &&
						Apres(r2.error, "mat.coord_texture") > 0;

	Cas("sortie/mappage-par-contagion-seulement", accepte && refuse,
		NkFormat("mappage sur une constante : accepte={0} valeur={1} (attendu 3.0 = 3 * 1 + 0) | le MEME "
				 "mappage nourri par une coordonnee : refuse en nommant la source={2} | message={3}",
				 accepte ? 1 : 0, so1 ? so1->valeur[0] : -1.f, refuse ? 1 : 0, r2.error));
}

static void CasSortieExposeNeContaminePas() {
	// ⚠️ UN PARAMETRE EXPOSE EST UN UNIFORME, PAS UNE VALEUR PAR PIXEL.
	//
	// Il vaut la meme chose pour tous les pixels du materiau. Le confondre
	// avec une source par pixel interdirait la moitie des usages utiles de
	// l'etage (a) — precisement ceux ou le code de jeu veut relire l'effet
	// d'un reglage qu'il vient d'ecrire.
	//
	// En revanche la sortie DEPEND de lui : sans cette liste, « reevalue au
	// changement de parametre » serait inapplicable et la valeur se figerait
	// sur celle du jour de la compilation, en restant plausible.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	const NkNodeId bsdf = MonteUnPrincipled(g, t, &out);
	(void)bsdf;

	const NkNodeId m = NkMatAddNode(g, NK_MN_MATH);
	g.SetProp(m, NK_MPROP_OPERATION, NkValueText(t.real, "ajouter"));
	g.SetSocketDefault(m, "a", NkSocketDir::Input, NkValueReal(t.real, 0.2f));
	g.SetSocketDefault(m, "b", NkSocketDir::Input, NkValueReal(t.real, 0.5f));
	NkString cle(NK_MPROP_EXPOSE_PREFIX);
	cle.Append("a");
	g.SetProp(m, cle.CStr(), NkValueText(t.real, "usure_globale"));
	const NkNodeId s = PoseSortie(g, t, "somme", "par_materiau");
	g.Connect(m, "value", s, "value");

	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatSortieMateriau *so = r.ok ? r.TrouveSortie("somme") : nullptr;
	const bool accepte = so != nullptr;
	// 0.2 + 0.5 = 0.7 : le defaut de la prise exposee EST sa valeur de depart.
	const bool valeur = so && Proche(so->valeur[0], 0.7f);
	const bool depend = so && so->dependDe.Size() == 1 && so->dependDe[0] == NkString("usure_globale");
	Cas("sortie/expose-ne-contamine-pas-mais-cree-une-dependance", accepte && valeur && depend,
		NkFormat("accepte={0} | valeur={1} (attendu 0.7 = defaut expose 0.2 + 0.5) | dependances={2} | "
				 "premiere={3} (attendu usure_globale) | erreur={4}",
				 accepte ? 1 : 0, so ? so->valeur[0] : -1.f, so ? (uint32)so->dependDe.Size() : 999u,
				 (so && so->dependDe.Size() > 0) ? so->dependDe[0] : NkString("-"), r.error));
}

static void CasSortieEtagesRefuses() {
	// Les deux etages non construits refusent EN SE NOMMANT, et l'absence
	// d'etage refuse aussi : il n'y a pas de defaut. Se replier sur (a) quand
	// la propriete manque transformerait silencieusement une sortie voulue par
	// pixel en constante calculee une seule fois.
	struct Jeu {
			const char *etage;
			const char *attendu;
	};
	const Jeu jeux[4] = {{nullptr, "etage non renseigne"},
						 {"par_pixel_cible", "par_pixel_cible"},
						 {"par_pixel_processeur", "par_pixel_processeur"},
						 {"par_pixel_magique", "par_pixel_magique"}};
	uint32 bons = 0;
	NkString detail;
	for (uint32 i = 0; i < 4; ++i) {
		NkNodeGraph g;
		NkMatTypes t = NkMatRegisterTypes(g);
		NkNodeId out;
		MonteUnPrincipled(g, t, &out);
		const NkNodeId v = NkMatAddNode(g, NK_MN_VALUE);
		g.SetProp(v, NK_MPROP_VALUE, NkValueReal(t.real, 1.f));
		const NkNodeId s = PoseSortie(g, t, "essai", jeux[i].etage);
		g.Connect(v, "value", s, "value");
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		const bool ok = !r.ok && Apres(r.error, jeux[i].attendu) > 0;
		if (ok)
			++bons;
		detail.Append(NkFormat("[{0} -> refus nomme={1}] ", NkString(jeux[i].etage ? jeux[i].etage : "(absent)"),
							   ok ? 1 : 0));
	}
	// Et (b2) doit dire POURQUOI, pas seulement « indisponible » : la
	// relecture synchrone se manifeste par « le jeu rame », jamais par « la
	// relecture est lente », et c'est ce qui coute des semaines a imputer.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	MonteUnPrincipled(g, t, &out);
	const NkNodeId v = NkMatAddNode(g, NK_MN_VALUE);
	const NkNodeId s = PoseSortie(g, t, "essai", "par_pixel_processeur");
	g.Connect(v, "value", s, "value");
	NkMatCompileResult rb2 = NkMatCompileToNkSL(g);
	const bool ditPourquoi = Apres(rb2.error, "SYNCHRONE") > 0 || Apres(rb2.error, "synchrone") > 0;

	Cas("sortie/etages-non-construits-refusent-en-se-nommant", bons == 4 && ditPourquoi,
		NkFormat("{0}| (b2) dit pourquoi={1}", detail, ditPourquoi ? 1 : 0));
}

static void CasSortieSourcesEtNoms() {
	// Zero source, deux sources, nom invalide, nom en double : quatre etats
	// qu'aucune interface n'empechera puisqu'il n'y en a pas encore.
	uint32 bons = 0;
	NkString detail;

	{ // aucune source
		NkNodeGraph g;
		NkMatTypes t = NkMatRegisterTypes(g);
		NkNodeId out;
		MonteUnPrincipled(g, t, &out);
		PoseSortie(g, t, "vide", "par_materiau");
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		const bool ok = !r.ok && Apres(r.error, "aucune source") > 0;
		bons += ok ? 1 : 0;
		detail.Append(NkFormat("[aucune source refusee={0}] ", ok ? 1 : 0));
	}
	{ // deux sources
		NkNodeGraph g;
		NkMatTypes t = NkMatRegisterTypes(g);
		NkNodeId out;
		MonteUnPrincipled(g, t, &out);
		const NkNodeId v = NkMatAddNode(g, NK_MN_VALUE);
		const NkNodeId c = NkMatAddNode(g, NK_MN_RGB);
		const NkNodeId s = PoseSortie(g, t, "double_source", "par_materiau");
		g.Connect(v, "value", s, "value");
		g.Connect(c, "color", s, "color");
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		const bool ok = !r.ok && Apres(r.error, "DEUX sources") > 0;
		bons += ok ? 1 : 0;
		detail.Append(NkFormat("[deux sources refusees={0}] ", ok ? 1 : 0));
	}
	{ // nom invalide
		NkNodeGraph g;
		NkMatTypes t = NkMatRegisterTypes(g);
		NkNodeId out;
		MonteUnPrincipled(g, t, &out);
		const NkNodeId v = NkMatAddNode(g, NK_MN_VALUE);
		const NkNodeId s = PoseSortie(g, t, "2 mots", "par_materiau");
		g.Connect(v, "value", s, "value");
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		const bool ok = !r.ok && Apres(r.error, "nom absent ou invalide") > 0;
		bons += ok ? 1 : 0;
		detail.Append(NkFormat("[nom invalide refuse={0}] ", ok ? 1 : 0));
	}
	{ // deux sorties du meme nom
		NkNodeGraph g;
		NkMatTypes t = NkMatRegisterTypes(g);
		NkNodeId out;
		MonteUnPrincipled(g, t, &out);
		const NkNodeId v1 = NkMatAddNode(g, NK_MN_VALUE);
		const NkNodeId v2 = NkMatAddNode(g, NK_MN_VALUE);
		const NkNodeId s1 = PoseSortie(g, t, "meme", "par_materiau");
		const NkNodeId s2 = PoseSortie(g, t, "meme", "par_materiau");
		g.Connect(v1, "value", s1, "value");
		g.Connect(v2, "value", s2, "value");
		NkMatCompileResult r = NkMatCompileToNkSL(g);
		const bool ok = !r.ok && Apres(r.error, "deux sorties portent le nom") > 0;
		bons += ok ? 1 : 0;
		detail.Append(NkFormat("[doublon refuse={0}] ", ok ? 1 : 0));
	}
	Cas("sortie/sources-et-noms-quatre-refus", bons == 4, detail);
}

static void CasSortiePlusieursEtGraphesExistants() {
	// Deux controles que rien d'autre ne couvre :
	//   - plusieurs sorties coexistent et se retrouvent CHACUNE par son nom ;
	//   - un graphe SANS aucune sortie nommee — c'est-a-dire tous les graphes
	//     ecrits jusqu'ici — rend une liste VIDE et compile comme avant. Une
	//     passe neuve qui casserait l'existant se verrait ici, et nulle part
	//     ailleurs dans ce banc.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	MonteUnPrincipled(g, t, &out);
	const NkNodeId a = NkMatAddNode(g, NK_MN_VALUE);
	g.SetProp(a, NK_MPROP_VALUE, NkValueReal(t.real, 0.125f));
	const NkNodeId b = NkMatAddNode(g, NK_MN_RGB);
	const float32 rouge[3] = {0.9f, 0.1f, 0.2f};
	g.SetProp(b, NK_MPROP_COLOR, NkValueVec(t.color, rouge, 3));
	const NkNodeId s1 = PoseSortie(g, t, "opacite", "par_materiau");
	const NkNodeId s2 = PoseSortie(g, t, "teinte_dominante", "par_materiau");
	g.Connect(a, "value", s1, "value");
	g.Connect(b, "color", s2, "color");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatSortieMateriau *o1 = r.ok ? r.TrouveSortie("opacite") : nullptr;
	const NkMatSortieMateriau *o2 = r.ok ? r.TrouveSortie("teinte_dominante") : nullptr;
	const bool deux = r.sorties.Size() == 2 && o1 && o2;
	const bool valeurs = o1 && o2 && Proche(o1->valeur[0], 0.125f) && Proche(o2->valeur[0], 0.9f) &&
						 Proche(o2->valeur[2], 0.2f);
	// Un nom absent doit rendre nullptr, pas la premiere sortie venue.
	const bool absentEstNul = r.TrouveSortie("nexiste_pas") == nullptr;

	NkNodeGraph g0;
	NkMatTypes t0 = NkMatRegisterTypes(g0);
	NkNodeId out0;
	MonteUnPrincipled(g0, t0, &out0);
	NkMatCompileResult r0 = NkMatCompileToNkSL(g0);
	const bool existantIntact = r0.ok && r0.sorties.Size() == 0 && r0.source.Size() > 100;

	Cas("sortie/plusieurs-et-graphes-sans-sortie", deux && valeurs && absentEstNul && existantIntact,
		NkFormat("deux sorties retrouvees par nom={0} | valeurs justes={1} | nom absent rend nul={2} | "
				 "graphe sans sortie nommee intact={3} (liste={4}, shader={5} octets)",
				 deux ? 1 : 0, valeurs ? 1 : 0, absentEstNul ? 1 : 0, existantIntact ? 1 : 0,
				 (uint32)r0.sorties.Size(), (uint32)r0.source.Size()));
}


static void CasSortieDivisionParZero() {
	// ⚠️ LE SEUL ENDROIT DU CHANTIER OU DEUX IMPLEMENTATIONS CALCULENT LA MEME
	// CHOSE — le shader et l evaluateur processeur — ET OU LEUR DESACCORD SERAIT
	// INVISIBLE. Personne ne compare la valeur rendue au code de jeu avec ce que
	// le pixel affiche : les deux resteraient plausibles chacune de son cote.
	//
	// Le shader garde la division et rend 0. Si l evaluateur divisait nu, il
	// rendrait un NaN — et un NaN se compare faux a TOUT, y compris a lui-meme,
	// donc le code de jeu prendrait des decisions inversees sans qu aucune
	// erreur ne soit levee.
	//
	// Mutation du 22/08 : ce cas a ete ecrit APRES avoir constate qu en retirant
	// la garde AUCUN des huit autres cas ne tombait.
	NkNodeGraph g;
	NkMatTypes t = NkMatRegisterTypes(g);
	NkNodeId out;
	MonteUnPrincipled(g, t, &out);
	const NkNodeId m = NkMatAddNode(g, NK_MN_MATH);
	g.SetProp(m, NK_MPROP_OPERATION, NkValueText(t.real, "diviser"));
	g.SetSocketDefault(m, "a", NkSocketDir::Input, NkValueReal(t.real, 1.f));
	g.SetSocketDefault(m, "b", NkSocketDir::Input, NkValueReal(t.real, 0.f));
	const NkNodeId s = PoseSortie(g, t, "quotient", "par_materiau");
	g.Connect(m, "value", s, "value");
	NkMatCompileResult r = NkMatCompileToNkSL(g);
	const NkMatSortieMateriau *so = r.ok ? r.TrouveSortie("quotient") : nullptr;
	const bool zero = so && so->valeur[0] == 0.f;
	// Un NaN se detecte par sa seule propriete stable : il differe de lui-meme.
	// Tester « == 0 » seul ne suffirait pas a le NOMMER dans le detail.
	const bool nan = so && !(so->valeur[0] == so->valeur[0]);
	// Et le shader, lui, garde bien la division : les deux doivent s accorder.
	const bool gardeDansLeShader = r.ok && ContientSansCasse(r.source, "== 0.0");
	Cas("sortie/division-par-zero-accorde-avec-le-shader", zero && !nan && gardeDansLeShader,
		NkFormat("valeur={0} (attendu 0, comme la garde du shader) | est un NaN={1} | le shader garde aussi "
				 "la division={2}",
				 so ? so->valeur[0] : -1.f, nan ? 1 : 0, gardeDansLeShader ? 1 : 0));
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

	// -- Math et Mix Color : une propriete qui porte une DECISION --------
	CasOperationInconnueRefusee();
	CasToutesLesOperationsCompilent();
	CasDivisionParZeroGardee();

	// -- ColorRamp : la premiere charge utile VARIABLE --------------------
	CasRampeChargeVariable();
	CasRampeRefusNommes();
	CasRampeInterpolation();
	CasRampeAllerRetourFichier();

	// -- Image Texture : la premiere RESSOURCE consommee par un graphe ----
	CasTexturePlafondRefusNomme();
	CasTextureBindingsDesDeuxCotes();
	CasTextureAucunBindingNeuf();
	CasTextureDeuxSorties();
	CasTextureCoordonneeEtMappage();

	// -- le relief : Normal Map, Bump, Separate XYZ ----------------------
	CasConventionNeChangePasLeShader();
	CasConventionInconnueRefusee();
	CasBaseTangenteSeulementSiUtile();
	CasNormaleVoyageJusquAuPuits();
	CasBumpDeriveesEtGarde();
	CasSepareTroisSorties();

	// -- les parametres EXPOSES : l'API publique du moteur ---------------
	CasExposeLitLeBloc();
	CasDecalagesStd140();
	CasPriseConnecteeEtExposeeRefusee();
	CasExposeRefusNommes();
	CasDefautEstCeluiDeLaPrise();
	CasJetonSuitLaDisposition();
	CasRechercheParNom();
	CasSansExpositionAucunBloc();
	CasBlocEtTexturesNeSeMarchentPasDessus();
	CasIncludeNeSeResoutPas();

	// -- rang 2 : le procedural ------------------------------------------
	CasSortieValeurCalculee();
	CasSortieCouleurEtRampe();
	CasSortieRefusParPixel();
	CasSortieMappageNEstPasParPixel();
	CasSortieExposeNeContaminePas();
	CasSortieEtagesRefuses();
	CasSortieSourcesEtNoms();
	CasSortiePlusieursEtGraphesExistants();
	CasSortieDivisionParZero();
	CasProceduralCompile();
	CasBriquesRecopieesVerbatim();
	CasBriquesSeulementSiUtiles();
	CasDegradeTypesEtRefus();
	CasOctavesBornees();
	CasOndeTypesEtRefus();
	CasVoronoiBorneFixe();
	CasBriquesJointsDecales();

	logger.Info("\n-- {0} cas, {1} echec(s) --", gCas, gEchecs);
	return gEchecs == 0 ? 0 : 1;
}

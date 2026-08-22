#pragma once
// -----------------------------------------------------------------------------
// @File    NkMatGraphCompile.h
// @Brief   Le compilateur : un graphe de materiau -> du NkSL.
// @Author  Rihen
// @License Proprietary - All Rights Reserved (see LICENSE)
//
// POURQUOI IL EMET DU NkSL ET PAS DU GLSL
//   `Kernel/Runtime/NKSL` porte Frontend, Compiler, CodeGen, Reflection,
//   ShaderConvert et VM, et tourne sur cinq backends sur six. Un graphe qui emet
//   du NkSL herite donc GRATUITEMENT d'OpenGL, Vulkan, DX11 et DX12. Emettre du
//   GLSL a la main creerait un second generateur de shaders a maintenir en
//   parallele, et il divergerait. **Le graphe est un frontend de plus pour NkSL.**
//
// ⚠️ CE QU'UN RASTERISEUR PEUT ET NE PEUT PAS FAIRE — la question de Rodolf
//   « mixer les shader BSDF » n'a pas le meme sens selon le moteur :
//     Cycles (tracage de chemins) melange des CLOSURES — il tire une lobe au
//       hasard par rayon, ponderee par le facteur ;
//     EEVEE (rasteriseur)        APPROXIME — il melange les parametres avant
//       ombrage, ou les resultats apres.
//   NKRenderer est un rasteriseur. `Mix Shader` melange donc ici les PARAMETRES,
//   composante par composante. Ce n'est pas un renoncement : c'est ce que font
//   EEVEE, Unreal et Unity. La FORME du graphe est celle de Blender ; seule son
//   execution differe, et l'execution n'est pas l'affaire du graphe.
//
// ⚠️ TROIS CONTRAINTES DU DIALECTE NkSL, relevees dans `layeredv1.frag.nksl` et
//   qui cadrent tout ce fichier. Aucune n'est devinable :
//     1. PAS de variable locale de type struct utilisateur. Un « shader » ne peut
//        donc pas etre une struct : il est emis en QUATRE LOCALES SCALAIRES ou
//        vectorielles (albedo, metallic, roughness, emission). C'est exactement
//        le motif de l'accumulateur de LayeredV1.
//     2. PAS de retour de struct depuis un helper.
//     3. JAMAIS de varying referencee dans un helper — elle se passe en
//        parametre, sinon le generateur HLSL sort un `input.xxx` hors de l'entree
//        et DX refuse le shader (X3004). Ce compilateur n'emet aucun helper : il
//        met tout dans l'entree, ce qui rend la contrainte 3 sans objet.
//
// EN-TETE PUR, sans cible de build — comme NkNodeGraph et NkMatGraphTypes.h. Le
//   banc l'exerce sans lier NKRenderer.
//
// ZERO-STL : NkString / NkVector, aucun `std::`.
// -----------------------------------------------------------------------------

#include "NKRenderer/Materials/Graph/NkMatGraphTypes.h"

#include <stdio.h> // snprintf : formatage des litteraux, pas de flux

namespace nkentseu {
	namespace renderer {
		namespace matgraph {

			struct NkMatCompileResult {
					bool ok = false;
					NkString source; ///< le NkSL emis
					NkString error;	 ///< renseigne SEULEMENT si ok == false
			};

			namespace detail {

				// Litteral flottant pour un shader. `%.9g` traverse le texte sans
				// perdre un bit — mais il peut rendre « 1 », et « float x = 1; »
				// n'est pas « float x = 1.0; » dans tous les dialectes. On force
				// donc le point quand il manque.
				//
				// ⚠️ PRECAUTION NON PROUVEE, et je le dis plutot que de laisser
				// croire le contraire. Le 2026-08-22, la mutation qui RETIRE ce
				// forcage a SURVECU : les quatre backends exerces (GL, Vulkan,
				// DX11, DX12) acceptent « 2 » la ou l'on attend « 2.0 », et aucun
				// cas du banc n'est passe au rouge. Le forcage reste parce qu'il ne
				// coute rien et couvre les dialectes qu'on n'exerce PAS encore
				// (Metal, le backend logiciel) — mais il n'est aujourd'hui appuye
				// par aucune mesure, et il ne faut pas s'y fier comme a une regle
				// verifiee.
				inline void PutLit(NkString &out, float32 v) {
					char b[40];
					snprintf(b, sizeof(b), "%.9g", (double)v);
					bool aPoint = false;
					for (const char *p = b; *p; ++p)
						if (*p == '.' || *p == 'e' || *p == 'E' || *p == 'n' || *p == 'i')
							aPoint = true; // nan/inf compris : ne pas leur coller « .0 »
					out.Append(b);
					if (!aPoint)
						out.Append(".0");
				}

				// Comparaison de cles d'operation. Une fonction plutot qu'un
				// `strcmp` inline : elle est appelee une douzaine de fois par
				// noeud et une inversion d'argument y passerait inapercue.
				inline bool OpEst(const char *a, const char *b) {
					while (*a && *a == *b) {
						++a;
						++b;
					}
					return *a == 0 && *b == 0;
				}

				inline void PutU(NkString &out, uint32 v) {
					char b[16];
					snprintf(b, sizeof(b), "%u", v);
					out.Append(b);
				}

				// Nom de locale d'un noeud. On prend l'IDENTIFIANT, jamais le
				// libelle : un libelle est traduisible, peut contenir des espaces
				// et change sans prevenir. Un identifiant est stable par contrat.
				inline void PutNom(NkString &out, NkNodeId id, const char *suffixe) {
					out.Append("n");
					PutU(out, id);
					out.Append("_");
					out.Append(suffixe);
				}

				// Les quatre composantes d'un « shader » approxime a la EEVEE.
				// L'ordre est fixe et cite partout : le changer ici et pas la-bas
				// donnerait un shader qui compile et rend faux.
				static const char *const kComp[4] = {"albedo", "metallic", "roughness", "emission"};
				// Les types NkSL correspondants, dans le meme ordre.
				static const char *const kCompType[4] = {"vec3", "float", "float", "vec3"};

				// Litteral d'une valeur, adapte au TYPE ATTENDU par la prise. Une
				// couleur a 4 reels alimentant un vec3 perd son alpha — c'est
				// voulu et dit, pas un accident de conversion.
				inline void PutValeur(NkString &out, const NkGraphValue &v, const char *typeAttendu) {
					const uint32 n = (uint32)v.numbers.Size();
					const bool scalaire = typeAttendu[0] == 'f'; // "float"
					if (scalaire) {
						PutLit(out, n > 0 ? v.numbers[0] : 0.f);
						return;
					}
					out.Append("vec3(");
					for (uint32 i = 0; i < 3; ++i) {
						if (i)
							out.Append(", ");
						// Un seul reel se DIFFUSE sur les trois composantes : c'est
						// la conversion reel -> couleur declaree au registre, et
						// elle doit se comporter ici comme elle se declare la-bas.
						const float32 f = (n == 1) ? v.numbers[0] : (i < n ? v.numbers[i] : 0.f);
						PutLit(out, f);
					}
					out.Append(")");
				}

			} // namespace detail

			// ── LE COMPILATEUR ───────────────────────────────────────────────
			// Parcours topologique, une locale par sortie, les entrees resolues
			// soit vers la locale qui les alimente, soit vers le DEFAUT de la
			// prise. C'est precisement pour cette derniere branche que les
			// defauts de prise ont ete ajoutes au coeur : sans eux, un
			// `Principled` non cable n'a aucune couleur a emettre.
			inline NkMatCompileResult NkMatCompileToNkSL(const NkNodeGraph &g) {
				NkMatCompileResult r;

				// 1. La validation de domaine d'abord. On ne compile pas un graphe
				// dont on sait qu'il n'a pas de sortie : le message doit venir
				// d'ici, pas d'une erreur de shader trois etages plus bas.
				NkNodeId sortie = NK_NODE_INVALID;
				const NkMatGraphError ve = NkMatValidate(g, &sortie);
				if (ve != NkMatGraphError::Ok) {
					r.error = NkString("graphe invalide : ");
					r.error.Append(NkMatGraphErrorName(ve));
					return r;
				}
				// 2. Puis la validation STRUCTURELLE du coeur. Un graphe charge
				// depuis un fichier peut porter un lien de type incompatible que
				// l'API n'aurait jamais laisse poser.
				NkVector<graph::NkGraphDiag> diags;
				if (g.Validate(diags) != 0) {
					r.error = NkString("graphe malforme : ");
					r.error.Append(graph::NkGraphIssueName(diags[0].issue));
					return r;
				}

				NkVector<NkNodeId> ordre;
				if (!g.TopoSort(ordre)) {
					r.error = NkString("cycle");
					return r;
				}

				// ── PROLOGUE ────────────────────────────────────────────────
				// Les varyings et l'UBO camera sont ceux du moteur, recopies a
				// l'identique de `layeredv1.frag.nksl` : un materiau genere doit
				// entrer dans le meme pipeline que les archetypes ecrits a la
				// main, sinon il faudrait un second chemin de rendu.
				NkString &s = r.source;
				s.Append("// ENGENDRE par NkMatCompileToNkSL -- ne pas editer a la main.\n");
				s.Append("// Le graphe est la source ; ce fichier en est la trace lisible.\n");
				s.Append("//\n");
				s.Append("// Un « shader » est approxime a la EEVEE : quatre locales scalaires ou\n");
				s.Append("// vectorielles (albedo, metallic, roughness, emission), et non une closure.\n");
				s.Append("// NkSL n'accepte pas de variable locale de type struct utilisateur -- c'est\n");
				s.Append("// pour cette raison, pas par gout, que les composantes sont separees.\n\n");
				s.Append("@location(0) in vec3 vWorldPos;\n");
				s.Append("@location(1) in vec3 vNormal;\n");
				s.Append("@location(2) in vec2 vUV;\n");
				s.Append("@location(3) in vec4 vColor;\n\n");
				s.Append("@location(0) out vec4 fragColor;\n\n");
				s.Append("@binding(set=0, binding=0)\n");
				s.Append("uniform CameraUBO {\n");
				s.Append("    mat4  view;\n    mat4  proj;\n    mat4  viewProj;\n    mat4  invViewProj;\n");
				s.Append("    vec4  camPos;\n    vec4  camDir;\n    vec2  viewport;\n");
				s.Append("    float time;\n    float deltaTime;\n    float iblStrength;\n} uCam;\n\n");
				s.Append("@stage(fragment)\n@entry\nvoid main() {\n");

				// Resout une entree : soit la locale du producteur, soit le defaut
				// de la prise, soit le neutre documente.
				auto ecrisEntree = [&](const NkNode &n, const char *prise, const char *typeAttendu,
									   const char *composante) {
					const int32 idx = n.FindSocket(prise, NkSocketDir::Input);
					if (idx >= 0) {
						const graph::NkLink *l = g.IncomingOf(n.id, idx);
						if (l) {
							// Une entree « shader » lit LA composante demandee du
							// producteur ; une entree scalaire ou vectorielle lit sa
							// locale unique.
							if (composante)
								detail::PutNom(s, l->fromNode, composante);
							else
								detail::PutNom(s, l->fromNode, "val");
							return;
						}
						const NkGraphValue &d = n.sockets[(uint32)idx].defaultValue;
						if (d.IsSet()) {
							detail::PutValeur(s, d, typeAttendu);
							return;
						}
					}
					// ⚠️ NEUTRE DOCUMENTE, et il n'est pas anodin. Une entree ni
					// cablee ni renseignee ne doit pas produire une valeur
					// flatteuse : le blanc ferait passer un materiau non fini pour
					// un materiau clair. On rend donc le NOIR (ou zero), qui se
					// VOIT, exactement comme le repli d'un shader manquant.
					if (typeAttendu[0] == 'f')
						s.Append("0.0");
					else
						s.Append("vec3(0.0)");
				};

				// ── CORPS : un bloc par noeud, dans l'ordre topologique ─────
				for (uint32 i = 0; i < (uint32)ordre.Size(); ++i) {
					const NkNode *n = g.Find(ordre[i]);
					if (!n)
						continue;
					const NkString &t = n->type;

					s.Append("\n    // ");
					s.Append(n->type);
					s.Append(" #");
					detail::PutU(s, n->id);
					if (n->label.Size() > 0) {
						s.Append(" -- ");
						s.Append(n->label);
					}
					s.Append("\n");

					auto declare = [&](uint32 c, const char *expr) {
						s.Append("    ");
						s.Append(detail::kCompType[c]);
						s.Append(" ");
						detail::PutNom(s, n->id, detail::kComp[c]);
						s.Append(" = ");
						s.Append(expr);
						s.Append(";\n");
					};
					auto declareDebut = [&](uint32 c) {
						s.Append("    ");
						s.Append(detail::kCompType[c]);
						s.Append(" ");
						detail::PutNom(s, n->id, detail::kComp[c]);
						s.Append(" = ");
					};

					if (t == NkString(NK_MN_PRINCIPLED)) {
						declareDebut(0);
						ecrisEntree(*n, "base_color", "vec3", nullptr);
						s.Append(";\n");
						declareDebut(1);
						ecrisEntree(*n, "metallic", "float", nullptr);
						s.Append(";\n");
						declareDebut(2);
						ecrisEntree(*n, "roughness", "float", nullptr);
						s.Append(";\n");
						declareDebut(3);
						ecrisEntree(*n, "emission", "vec3", nullptr);
						s.Append(";\n");
					} else if (t == NkString(NK_MN_DIFFUSE)) {
						declareDebut(0);
						ecrisEntree(*n, "color", "vec3", nullptr);
						s.Append(";\n");
						// Un diffus n'est pas metallique : c'est la definition du
						// noeud, pas un defaut manquant.
						declare(1, "0.0");
						declareDebut(2);
						ecrisEntree(*n, "roughness", "float", nullptr);
						s.Append(";\n");
						declare(3, "vec3(0.0)");
					} else if (t == NkString(NK_MN_EMISSION)) {
						// Une emission n'a pas d'albedo diffus : toute son energie
						// part dans le canal emissif, multiplie par sa force.
						declare(0, "vec3(0.0)");
						declare(1, "0.0");
						declare(2, "1.0");
						declareDebut(3);
						ecrisEntree(*n, "color", "vec3", nullptr);
						s.Append(" * ");
						ecrisEntree(*n, "strength", "float", nullptr);
						s.Append(";\n");
					} else if (t == NkString(NK_MN_MIX_SHADER)) {
						// LE NOEUD QUE RODOLF A DEMANDE. Melange COMPOSANTE PAR
						// COMPOSANTE, borne a [0,1] : un facteur hors bornes
						// extrapolerait, ce que Blender ne fait pas non plus.
						s.Append("    float ");
						detail::PutNom(s, n->id, "fac");
						s.Append(" = clamp(");
						ecrisEntree(*n, "fac", "float", nullptr);
						s.Append(", 0.0, 1.0);\n");
						for (uint32 c = 0; c < 4; ++c) {
							declareDebut(c);
							s.Append("mix(");
							ecrisEntree(*n, "shader1", detail::kCompType[c], detail::kComp[c]);
							s.Append(", ");
							ecrisEntree(*n, "shader2", detail::kCompType[c], detail::kComp[c]);
							s.Append(", ");
							detail::PutNom(s, n->id, "fac");
							s.Append(");\n");
						}
					} else if (t == NkString(NK_MN_VALUE) || t == NkString(NK_MN_RGB)) {
						// LES DEUX NOEUDS SOURCES, et les PREMIERS a lire une
						// PROPRIETE DE NOEUD jusque dans le shader. Leur valeur
						// n'est pas une prise : il n'y a rien a y brancher, c'est
						// le depart d'une chaine. Elle vit donc dans `props`.
						const bool estCouleur = (t == NkString(NK_MN_RGB));
						const NkGraphValue *pv =
							g.FindProp(n->id, estCouleur ? NK_MPROP_COLOR : NK_MPROP_VALUE);
						s.Append("    ");
						s.Append(estCouleur ? "vec3 " : "float ");
						detail::PutNom(s, n->id, "val");
						s.Append(" = ");
						if (pv && pv->IsSet())
							detail::PutValeur(s, *pv, estCouleur ? "vec3" : "float");
						else
							s.Append(estCouleur ? "vec3(0.0)" : "0.0");
						s.Append(";\n");
					} else if (t == NkString(NK_MN_MATH) || t == NkString(NK_MN_MIX_COLOR)) {
						// ── LES DEUX NOEUDS DONT LE CALCUL EST CHOISI PAR UNE
						//    PROPRIETE ────────────────────────────────────────
						// Jusqu'ici une propriete portait une VALEUR ; celles-ci
						// portent une DECISION. C'est le premier endroit du
						// compilateur ou une propriete pilote le CODE EMIS.
						const bool couleur = (t == NkString(NK_MN_MIX_COLOR));
						const NkGraphValue *pop = g.FindProp(n->id, NK_MPROP_OPERATION);

						// ⚠️ UNE OPERATION INCONNUE FAIT ECHOUER LA COMPILATION.
						// La tentation est de retomber sur la premiere de la
						// liste — « ajouter » — parce que ca « marche ». Ce serait
						// le pire des choix : le materiau compilerait, rendrait,
						// et calculerait AUTRE CHOSE que ce que le fichier dit.
						// Un fichier ecrit par une version future, ou une faute de
						// frappe, passerait inapercu jusqu'au resultat.
						//
						// Une propriete ABSENTE, elle, est un cas different et
						// legitime : le noeud vient d'etre pose et l'auteur n'a
						// pas encore choisi. On prend alors le premier, et c'est
						// ecrit ici pour que ce ne soit pas confondu avec le cas
						// precedent.
						int32 iop = 0;
						if (pop && pop->IsSet()) {
							iop = NkMatTrouveOperation(couleur, pop->text.CStr());
							if (iop < 0) {
								r.error = NkString("operation inconnue sur ");
								r.error.Append(n->type);
								r.error.Append(" : ");
								r.error.Append(pop->text);
								r.source = NkString("");
								return r;
							}
						}
						// ⚠️ LE POINTEUR EST VERIFIE, PAS SUPPOSE. Une mutation qui
						// retirait le refus ci-dessus laissait `iop` a -1, et ce
						// `->cle` dereferencait un pointeur NUL : le banc mourait
						// au lieu d'echouer. Un code qui ne peut se tromper QUE
						// par un plantage n'est pas robuste, il est chanceux.
						const NkMatOperation *opd =
							(iop >= 0) ? NkMatOperationAt(couleur, (uint32)iop) : nullptr;
						if (!opd) {
							r.error = NkString("operation hors table sur ");
							r.error.Append(n->type);
							r.source = NkString("");
							return r;
						}
						const char *op = opd->cle;

						s.Append("    ");
						s.Append(couleur ? "vec3 " : "float ");
						detail::PutNom(s, n->id, "val");
						s.Append(" = ");

						if (!couleur) {
							// Math : deux reels, une operation.
							const NkString a = NkString("a"), b = NkString("b");
							auto ecrisA = [&]() { ecrisEntree(*n, "a", "float", nullptr); };
							auto ecrisB = [&]() { ecrisEntree(*n, "b", "float", nullptr); };
							if (detail::OpEst(op, "ajouter")) {
								ecrisA();
								s.Append(" + ");
								ecrisB();
							} else if (detail::OpEst(op, "soustraire")) {
								ecrisA();
								s.Append(" - ");
								ecrisB();
							} else if (detail::OpEst(op, "multiplier")) {
								ecrisA();
								s.Append(" * ");
								ecrisB();
							} else if (detail::OpEst(op, "diviser")) {
								// ⚠️ DIVISION PAR ZERO : on rend 0, comme Blender.
								// Laisser passer une division nue produirait un
								// NaN qui contamine tout le reste du graphe, et
								// qui se voit comme un pixel noir ou blanc selon
								// le backend — donc un defaut qui change d'aspect
								// d'une machine a l'autre.
								s.Append("((");
								ecrisB();
								s.Append(") == 0.0 ? 0.0 : (");
								ecrisA();
								s.Append(") / (");
								ecrisB();
								s.Append("))");
							} else if (detail::OpEst(op, "minimum")) {
								s.Append("min(");
								ecrisA();
								s.Append(", ");
								ecrisB();
								s.Append(")");
							} else if (detail::OpEst(op, "maximum")) {
								s.Append("max(");
								ecrisA();
								s.Append(", ");
								ecrisB();
								s.Append(")");
							} else { // puissance
								// `pow` d'une base negative n'est pas defini : on
								// borne, comme Blender, plutot que de laisser le
								// resultat dependre du backend.
								s.Append("pow(max(");
								ecrisA();
								s.Append(", 0.0), ");
								ecrisB();
								s.Append(")");
							}
						} else {
							// Mix Color : le facteur melange color1 avec le
							// RESULTAT de l'operation, exactement comme Blender —
							// et non les deux couleurs directement.
							s.Append("mix(");
							ecrisEntree(*n, "color1", "vec3", nullptr);
							s.Append(", ");
							if (detail::OpEst(op, "melanger")) {
								ecrisEntree(*n, "color2", "vec3", nullptr);
							} else if (detail::OpEst(op, "multiplier")) {
								ecrisEntree(*n, "color1", "vec3", nullptr);
								s.Append(" * ");
								ecrisEntree(*n, "color2", "vec3", nullptr);
							} else if (detail::OpEst(op, "ajouter")) {
								ecrisEntree(*n, "color1", "vec3", nullptr);
								s.Append(" + ");
								ecrisEntree(*n, "color2", "vec3", nullptr);
							} else if (detail::OpEst(op, "soustraire")) {
								ecrisEntree(*n, "color1", "vec3", nullptr);
								s.Append(" - ");
								ecrisEntree(*n, "color2", "vec3", nullptr);
							} else if (detail::OpEst(op, "eclaircir")) {
								s.Append("max(");
								ecrisEntree(*n, "color1", "vec3", nullptr);
								s.Append(", ");
								ecrisEntree(*n, "color2", "vec3", nullptr);
								s.Append(")");
							} else { // assombrir
								s.Append("min(");
								ecrisEntree(*n, "color1", "vec3", nullptr);
								s.Append(", ");
								ecrisEntree(*n, "color2", "vec3", nullptr);
								s.Append(")");
							}
							s.Append(", clamp(");
							ecrisEntree(*n, "fac", "float", nullptr);
							s.Append(", 0.0, 1.0))");
						}
						s.Append(";\n");
					} else if (t == NkString(NK_MN_OUTPUT)) {
						// ── LE PUITS : ombrage puis ecriture ────────────────
						// Le modele d'eclairage est celui de LayeredV1, a
						// l'identique : un materiau engendre doit s'eclairer comme
						// les archetypes ecrits a la main, sinon deux materiaux
						// cotes a cote dans la meme scene ne se ressembleraient pas.
						s.Append("    vec3 surfAlbedo = ");
						ecrisEntree(*n, "surface", "vec3", "albedo");
						s.Append(";\n    float surfMetallic = ");
						ecrisEntree(*n, "surface", "float", "metallic");
						s.Append(";\n    float surfRoughness = ");
						ecrisEntree(*n, "surface", "float", "roughness");
						s.Append(";\n    vec3 surfEmission = ");
						ecrisEntree(*n, "surface", "vec3", "emission");
						s.Append(";\n\n");
						s.Append("    vec3 N3 = normalize(vNormal);\n");
						s.Append("    vec3 L  = normalize(vec3(-0.3, 1.0, 0.4));\n");
						s.Append("    vec3 V  = normalize(uCam.camPos.xyz - vWorldPos);\n");
						s.Append("    vec3 H  = normalize(L + V);\n");
						s.Append("    float NdotL = max(dot(N3, L), 0.0);\n");
						s.Append("    float NdotH = max(dot(N3, H), 0.0);\n");
						s.Append("    float specExp = mix(128.0, 2.0, surfRoughness);\n");
						s.Append("    float spec = pow(NdotH, specExp);\n");
						s.Append("    vec3 specColor = mix(vec3(1.0), surfAlbedo, surfMetallic);\n");
						s.Append("    vec3 diffuse = surfAlbedo * (NdotL + 0.18);\n");
						s.Append("    vec3 specular = specColor * spec * (surfMetallic * 0.7 + 0.3);\n");
						s.Append("    fragColor = vec4(diffuse + specular + surfEmission, 1.0);\n");
					} else {
						// ⚠️ ON REFUSE, on n'ignore pas. Un noeud inconnu qu'on
						// sauterait laisserait son consommateur lire une locale
						// jamais declaree — le shader ne compilerait pas, et
						// l'erreur accuserait le generateur au lieu du graphe.
						r.error = NkString("noeud non compilable : ");
						r.error.Append(n->type);
						r.source = NkString("");
						return r;
					}
				}

				s.Append("}\n");
				r.ok = true;
				return r;
			}

		} // namespace matgraph
	} // namespace renderer
} // namespace nkentseu

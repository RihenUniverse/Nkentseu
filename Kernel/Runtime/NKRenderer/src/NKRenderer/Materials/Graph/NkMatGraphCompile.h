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
// ⚠️ LE PLAFOND DE TEXTURES ET LES SLOTS VIENNENT D'ICI, ET DE NULLE PART
// AILLEURS. Un compilateur qui autoriserait 8 slots quand le layout en
// declare 6 ecrirait sur deux bindings inexistants — sans erreur, sans
// journal, sans difference d'image. C'est la panne mesuree le 22/08.
#include "NKRenderer/Materials/NkMaterialBindings.h"

#include <stdio.h> // snprintf : formatage des litteraux, pas de flux

namespace nkentseu {
	namespace renderer {
		namespace matgraph {

			// Un parametre expose, tel que le MOTEUR doit le voir.
			//
			// ⚠️ IL PORTE SON DECALAGE, PAS SON RANG. En `std140` un `vec3`
			// s'aligne sur 16 octets et en occupe 12 : trois reels a la suite
			// n'occupent PAS trois emplacements contigus. Un moteur qui deduirait
			// les positions de l'ordre de declaration ecrirait a cote des le
			// premier `vec3` — sans erreur, avec une valeur credible.
			struct NkMatParamExpose {
					NkString nom;	 ///< le nom PUBLIC, celui qu'emploie le code du jeu
					NkTypeId type = NK_TYPE_INVALID;
					uint32 decalage = 0; ///< en octets, depuis le debut du bloc
					uint32 taille = 0;	 ///< en octets
					// ⚠️ LE DEFAUT EST CELUI DE LA PRISE, recopie ici pour que le
					// moteur n'ait pas a retraverser le graphe. Il n'existe PAS de
					// seconde valeur : deux sources pour une meme chose divergent,
					// et c'est alors l'editeur qui montre l'une pendant que le
					// moteur envoie l'autre.
					NkGraphValue defaut;
					bool bornes = false;
					float32 borneMin = 0.f, borneMax = 0.f;
					// D'ou il vient, pour qu'un message d'erreur puisse le designer.
					NkNodeId noeud = NK_NODE_INVALID;
					NkString prise;
			};

			struct NkMatCompileResult {
					bool ok = false;
					NkString source; ///< le NkSL emis
					NkString error;	 ///< renseigne SEULEMENT si ok == false

					// ── L'API PUBLIQUE ───────────────────────────────────────
					// La disposition du bloc uniforme, decrite ICI et NULLE PART
					// AILLEURS. A deux endroits, le moteur ecrirait a un decalage
					// et le shader lirait a un autre : ni erreur, ni journal, et
					// une valeur credible.
					NkVector<NkMatParamExpose> params;
					uint32 paramsTaille = 0; ///< taille totale du bloc, en octets

					// ⚠️ JETON DE COMPILATION. Recompiler un graphe EDITE peut
					// reordonner le bloc ; du code de jeu ayant retenu un decalage
					// ecrirait alors dans le mauvais parametre — sans erreur, avec
					// une valeur credible. **L'API du moteur est par NOM**, et tout
					// cache de decalage doit porter ce jeton et se jeter quand il
					// change.
					uint64 jeton = 0;

					const NkMatParamExpose *TrouveParam(const char *nom) const;
			};

			// ⚠️ L'API DU MOTEUR EST PAR NOM. Un decalage retenu par du code de jeu
			// devient faux des que le graphe est reedite et recompile ; le nom, lui,
			// ne ment pas. Tout cache de decalage doit porter le `jeton` et se
			// jeter quand il change.
			inline const NkMatParamExpose *NkMatCompileResult::TrouveParam(const char *nom) const {
				if (!nom)
					return nullptr;
				for (uint32 i = 0; i < (uint32)params.Size(); ++i) {
					const char *a = params[i].nom.CStr();
					const char *b = nom;
					while (*a && *a == *b) {
						++a;
						++b;
					}
					if (!*a && !*b)
						return &params[i];
				}
				return nullptr;
			}

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
				// ⚠️ CINQ composantes depuis le 2026-08-22 : la NORMALE a rejoint le
				// lot. Sans elle, `Normal Map` et `Bump` n'avaient nulle part ou
				// aller — le puits recalculait `normalize(vNormal)` et jetait tout
				// travail de relief. Un noeud dont la sortie n'est lue par
				// personne est pire qu'un noeud absent : il donne l'illusion que
				// la capacite existe.
				static const uint32 kCompCount = 5;
				static const char *const kComp[5] = {"albedo", "metallic", "roughness", "emission", "normal"};
				static const char *const kCompType[5] = {"vec3", "float", "float", "vec3", "vec3"};

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

			// Combien de composantes porte un « shader » approxime. Public,
			// parce que le banc doit lire LA MEME verite que le compilateur :
			// un nombre recopie dans le banc cesserait de suivre le jour ou une
			// composante s ajoute, et le cas passerait au vert sans rien prouver.
			inline uint32 NkMatComposanteCount() {
				return detail::kCompCount;
			}


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

				// Les identifiants de type, retrouves depuis le registre du graphe.
				// On ne les REENREGISTRE pas : un `RegisterType` ici masquerait un
				// graphe dont les types n'ont jamais ete declares.
				NkMatTypes t;
				t.real = g.FindType(NK_MT_REAL);
				t.vector = g.FindType(NK_MT_VECTOR);
				t.color = g.FindType(NK_MT_COLOR);
				t.shader = g.FindType(NK_MT_SHADER);
				t.ramp = g.FindType(NK_MT_RAMP);

				NkVector<NkNodeId> ordre;
				if (!g.TopoSort(ordre)) {
					r.error = NkString("cycle");
					return r;
				}

				// ── PRE-PASSE : LES PARAMETRES EXPOSES ──────────────────────
				// C'est ici que le chantier devient une API publique : ce qui sort
				// de cette passe est ce que le code d'un jeu manipulera.
				for (uint32 i = 0; i < (uint32)ordre.Size(); ++i) {
					const NkNode *n = g.Find(ordre[i]);
					if (!n)
						continue;
					for (uint32 k = 0; k < (uint32)n->props.Size(); ++k) {
						const NkGraphProp &pr = n->props[k];
						// La cle est `expose.<prise>` : on isole le suffixe.
						const char *cle = pr.name.CStr();
						const char *pref = NK_MPROP_EXPOSE_PREFIX;
						const char *c = cle;
						while (*pref && *c == *pref) {
							++c;
							++pref;
						}
						if (*pref != 0)
							continue; // pas une propriete d'exposition
						const char *nomPrise = c;

						auto refuse = [&](const char *quoi) {
							r.error = NkString(quoi);
							r.error.Append(" : ");
							r.error.Append(n->type);
							r.error.Append(".");
							r.error.Append(nomPrise);
							r.source = NkString("");
						};

						const int32 idx = n->FindSocket(nomPrise, NkSocketDir::Input);
						if (idx < 0) {
							// Une exposition sur une prise inexistante est morte :
							// elle ne pilotera jamais rien, et personne ne le
							// remarquera. C'est typiquement ce que laisse un
							// renommage de prise.
							refuse("expose sur une prise inconnue");
							return r;
						}
						const NkSocket &sk = n->sockets[(uint32)idx];

						// 🔴 UNE PRISE CONNECTEE **ET** EXPOSEE EST REFUSEE.
						// C'est le cas le plus insidieux de tous : le lien
						// REMPLACE la valeur exposee, donc `SetFloat("usure", …)`
						// ne fait RIEN. Le materiau compile, il rend, et le
						// parametre est mort — aucune erreur, aucun journal.
						//
						// ⚠️ Chez Blender le probleme ne se pose pas parce que
						// brancher un lien FAIT DISPARAITRE le widget : l'interface
						// rend l'etat impossible. Nous n'avons pas d'interface —
						// c'est donc la validation qui doit le rendre impossible.
						if (g.IncomingOf(n->id, idx)) {
							refuse("prise CONNECTEE et exposee (le lien remplacerait la valeur exposee, "
								   "qui ne piloterait plus rien)");
							return r;
						}

						if (!pr.value.IsSet() || !NkMatNomPublicValide(pr.value.text.CStr())) {
							// Le nom devient un membre de bloc uniforme ET une cle
							// pour le code du jeu : un nom a espaces produirait un
							// shader invalide, et l'erreur accuserait le
							// generateur au lieu du nom.
							refuse("nom public absent ou invalide (lettres, chiffres et souligne, ne commencant "
								   "pas par un chiffre)");
							return r;
						}
						if (sk.type == t.shader) {
							refuse("une prise de type shader ne peut pas etre exposee (ce n'est pas une valeur "
								   "uniforme)");
							return r;
						}

						NkMatParamExpose p;
						p.nom = pr.value.text;
						p.type = sk.type;
						p.defaut = sk.defaultValue;
						p.noeud = n->id;
						p.prise = NkString(nomPrise);
						if ((uint32)pr.value.numbers.Size() >= NK_EXPOSE_BORNES_REELS) {
							p.bornes = true;
							p.borneMin = pr.value.numbers[0];
							p.borneMax = pr.value.numbers[1];
						}

						// Unicite du nom PUBLIC. Deux parametres homonymes
						// rendraient `SetFloat` dependant de l'ordre de
						// declaration — donc l'un des deux inaccessible.
						for (uint32 q = 0; q < (uint32)r.params.Size(); ++q)
							if (r.params[q].nom == p.nom) {
								r.error = NkString("deux parametres exposes portent le meme nom public : ");
								r.error.Append(p.nom);
								r.source = NkString("");
								return r;
							}
						r.params.PushBack(p);
					}
				}

				// Les decalages `std140`, calcules dans l'ordre de declaration.
				{
					uint32 curseur = 0;
					for (uint32 i = 0; i < (uint32)r.params.Size(); ++i) {
						NkMatParamExpose &p = r.params[i];
						const uint32 a = NkMatStd140Align(p.type, t);
						p.decalage = NkMatAlignUp(curseur, a);
						p.taille = NkMatStd140Size(p.type, t);
						curseur = p.decalage + p.taille;
					}
					// Un bloc uniforme se termine sur un multiple de 16.
					r.paramsTaille = NkMatAlignUp(curseur, 16u);
				}

				// ── PRE-PASSE : LES TEXTURES ────────────────────────────────
				// Il faut les compter AVANT d'ecrire le prologue, puisque c'est la
				// que leurs samplers se declarent. On leur attribue un slot dans
				// l'ordre topologique : deterministe, donc reproductible d'une
				// compilation a l'autre — un ordre qui varierait ferait changer le
				// shader emis sans qu'aucune donnee n'ait bouge.
				// Une base tangente coute deux paires de derivees : on ne l'emet que
				// si un noeud la reclame. Un shader qui la calculerait sans
				// l'utiliser paierait a chaque pixel pour rien.
				bool besoinTBN = false;
				for (uint32 i = 0; i < (uint32)ordre.Size(); ++i) {
					const NkNode *n = g.Find(ordre[i]);
					if (n && n->type == NkString(NK_MN_NORMAL_MAP))
						besoinTBN = true;
				}

				NkVector<NkNodeId> texNodes;
				for (uint32 i = 0; i < (uint32)ordre.Size(); ++i) {
					const NkNode *n = g.Find(ordre[i]);
					if (n && n->type == NkString(NK_MN_IMAGE_TEXTURE))
						texNodes.PushBack(n->id);
				}
				if ((uint32)texNodes.Size() > NK_MATBIND_GRAPH_SLOT_COUNT) {
					// ⚠️ LE REFUS DIT COMBIEN. « Trop de textures » seul obligerait
					// l'auteur a deviner ce qu'il doit retirer de son graphe.
					r.error = NkString("ce graphe demande ");
					detail::PutU(r.error, (uint32)texNodes.Size());
					r.error.Append(" textures, le plafond est ");
					detail::PutU(r.error, NK_MATBIND_GRAPH_SLOT_COUNT);
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
				// Le bloc des parametres exposes. Les membres sont declares dans
				// l'ordre des decalages calcules — c'est ce qui fait que `std140`
				// place chacun la ou la disposition l'annonce.
				if (!r.params.Empty()) {
					s.Append("@binding(set=2, binding=");
					detail::PutU(s, NK_MATBIND_GRAPH_PARAMS);
					s.Append(")\nuniform ");
					s.Append(NK_MATBIND_GRAPH_PARAMS_BLOCK);
					s.Append(" {\n");
					for (uint32 i = 0; i < (uint32)r.params.Size(); ++i) {
						s.Append("    ");
						s.Append(r.params[i].type == t.real ? "float " : "vec3  ");
						s.Append(r.params[i].nom);
						s.Append(";\n");
					}
					s.Append("} ");
					s.Append(NK_MATBIND_GRAPH_PARAMS_VAR);
					s.Append(";\n\n");
				}

				// Les samplers du graphe. AUCUN BINDING NEUF : on reutilise les
				// emplacements que le layout materiau declare deja, dans l'ordre.
				// Un materiau engendre n'a que faire de `tAlbedo` ou de `tHeight` —
				// ces slots sont morts pour lui, et le depot reutilise deja le meme
				// binding pour des usages differents selon l'archetype.
				for (uint32 i = 0; i < (uint32)texNodes.Size(); ++i) {
					s.Append("@binding(set=2, binding=");
					detail::PutU(s, NK_MATBIND_GRAPH_SLOTS[i]);
					s.Append(") uniform sampler2D ");
					s.Append(NK_MATBIND_GRAPH_SAMPLER_PREFIX);
					detail::PutU(s, i);
					s.Append(";\n");
				}
				if (!texNodes.Empty())
					s.Append("\n");

				s.Append("@stage(fragment)\n@entry\nvoid main() {\n");
				// La normale geometrique sert de defaut a toute prise `normal` non
				// cablee, et de base au relief. On la nomme une fois.
				s.Append("    vec3 nkGeomN = normalize(vNormal);\n");
				if (besoinTBN) {
					// ⚠️ BASE TANGENTE PAR DERIVEES D'ECRAN (cadre cotangent de
					// Schuler). Ce n'est pas un pis-aller : c'est deja ce que fait
					// `pbr.frag.nksl`, et pour une raison ecrite la-bas — les
					// tangentes de sommet peuvent etre nulles ou desalignees des
					// UV, et le relief part alors dans une direction ARBITRAIRE
					// par face. Ici T et B suivent exactement le sens des UV du
					// pixel, miroirs et rotations compris.
					//
					// Le vertex engendre ne fournit d'ailleurs AUCUNE tangente :
					// s'en passer n'est pas un choix, c'est la seule voie honnete.
					s.Append("    vec3 nkDpx = dFdx(vWorldPos);\n");
					s.Append("    vec3 nkDpy = dFdy(vWorldPos);\n");
					s.Append("    vec2 nkDux = dFdx(vUV);\n");
					s.Append("    vec2 nkDuy = dFdy(vUV);\n");
					s.Append("    vec3 nkPerpY = cross(nkDpy, nkGeomN);\n");
					s.Append("    vec3 nkPerpX = cross(nkGeomN, nkDpx);\n");
					s.Append("    vec3 nkT = nkPerpY * nkDux.x + nkPerpX * nkDuy.x;\n");
					s.Append("    vec3 nkB = nkPerpY * nkDux.y + nkPerpX * nkDuy.y;\n");
					// Normalisation commune : garde le rapport T/B, donc l'anisotropie
					// reelle des UV, la ou deux normalisations separees l'effacent.
					s.Append("    float nkInvMax = 1.0 / sqrt(max(max(dot(nkT, nkT), dot(nkB, nkB)), 1e-20));\n");
					s.Append("    nkT *= nkInvMax;\n    nkB *= nkInvMax;\n");
				}

				// Resout une entree : soit la locale du producteur, soit le defaut
				// de la prise, soit le neutre documente.
				auto ecrisEntree = [&](const NkNode &n, const char *prise, const char *typeAttendu,
									   const char *composante) {
					const int32 idx = n.FindSocket(prise, NkSocketDir::Input);
					if (idx >= 0) {
						const graph::NkLink *l = g.IncomingOf(n.id, idx);
						if (l) {
							// Une entree « shader » lit LA composante demandee du
							// producteur. Une entree ordinaire lit la locale nommee
							// d'apres LA PRISE SOURCE.
							//
							// ⚠️ POURQUOI PAS UN « val » UNIQUE, comme avant :
							// `Image Texture` a DEUX sorties — `color` et `alpha`.
							// Un nom unique par noeud ne peut pas les distinguer, et
							// l'entree qui lirait « la » valeur en prendrait une au
							// hasard. Le nom de la prise est deja la cle stable du
							// modele ; la locale en herite.
							if (composante) {
								detail::PutNom(s, l->fromNode, composante);
							} else {
								const NkNode *src = g.Find(l->fromNode);
								const char *nomPrise = "val";
								if (src && l->fromSocket >= 0 &&
									(uint32)l->fromSocket < (uint32)src->sockets.Size())
									nomPrise = src->sockets[(uint32)l->fromSocket].name.CStr();
								detail::PutNom(s, l->fromNode, nomPrise);
							}
							return;
						}
						// ⚠️ UNE PRISE EXPOSEE LIT LE BLOC UNIFORME, jamais un
						// litteral. C'est tout l'objet de l'exposition : la valeur
						// doit pouvoir changer a l'execution, donc elle ne peut pas
						// etre repliee dans le code. Le defaut, lui, part dans la
						// disposition — c'est la meme valeur, a un autre endroit.
						for (uint32 q = 0; q < (uint32)r.params.Size(); ++q)
							if (r.params[q].noeud == n.id && r.params[q].prise == NkString(prise)) {
								s.Append(NK_MATBIND_GRAPH_PARAMS_VAR);
								s.Append(".");
								s.Append(r.params[q].nom);
								return;
							}
						const NkGraphValue &d = n.sockets[(uint32)idx].defaultValue;
						if (d.IsSet()) {
							detail::PutValeur(s, d, typeAttendu);
							return;
						}
					}
					// ⚠️ CAS PARTICULIER DE LA PRISE `normal` : son neutre n'est pas
					// le noir mais la NORMALE GEOMETRIQUE. Un vecteur nul serait
					// une normale nulle, donc un eclairage indefini — et le noir
					// n'a aucun sens pour une direction. Ce n'est pas un repli
					// plausible : c'est le comportement DEFINI de la prise, celui
					// de Blender.
					{
						const char *a = prise;
						const char *b = "normal";
						while (*a && *a == *b) {
							++a;
							++b;
						}
						if (*a == 0 && *b == 0) {
							s.Append("nkGeomN");
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
						declareDebut(4);
						ecrisEntree(*n, "normal", "vec3", nullptr);
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
						declareDebut(4);
						ecrisEntree(*n, "normal", "vec3", nullptr);
						s.Append(";\n");
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
						// Une emission n'a pas de relief : elle prend la normale
						// geometrique, et le dire vaut mieux que de l'omettre.
						declare(4, "nkGeomN");
					} else if (t == NkString(NK_MN_MIX_SHADER)) {
						// LE NOEUD QUE RODOLF A DEMANDE. Melange COMPOSANTE PAR
						// COMPOSANTE, borne a [0,1] : un facteur hors bornes
						// extrapolerait, ce que Blender ne fait pas non plus.
						s.Append("    float ");
						detail::PutNom(s, n->id, "fac");
						s.Append(" = clamp(");
						ecrisEntree(*n, "fac", "float", nullptr);
						s.Append(", 0.0, 1.0);\n");
						for (uint32 c = 0; c < detail::kCompCount; ++c) {
							declareDebut(c);
							s.Append("mix(");
							ecrisEntree(*n, "shader1", detail::kCompType[c], detail::kComp[c]);
							s.Append(", ");
							ecrisEntree(*n, "shader2", detail::kCompType[c], detail::kComp[c]);
							s.Append(", ");
							detail::PutNom(s, n->id, "fac");
							s.Append(");\n");
						}
						// ⚠️ Melanger deux directions ne rend pas une direction :
						// la somme ponderee de deux vecteurs unitaires ne l'est
						// plus. On renormalise, sinon l'eclairage s'assombrit la
						// ou les deux normales divergent — un defaut graduel, donc
						// qu'on attribue a autre chose.
						s.Append("    ");
						detail::PutNom(s, n->id, "normal");
						s.Append(" = normalize(");
						detail::PutNom(s, n->id, "normal");
						s.Append(");\n");
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
						// `RGB` sort sur « color », `Value` sur « value » : la locale
						// porte le nom de la prise, pas un « val » generique.
						detail::PutNom(s, n->id, estCouleur ? "color" : "value");
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
						// ⚠️ LE POINTEUR EST VERIFIE, PAS SUPPOSE.
						//
						//     UN CODE QUI NE PEUT SE TROMPER QUE PAR UN PLANTAGE
						//     N'EST PAS ROBUSTE, IL EST CHANCEUX.
						//
						// Trouve par une mutation, le 2026-08-22 : en retirant le
						// refus ci-dessus, `iop` restait a -1 et ce `->cle`
						// dereferencait un pointeur NUL. Le banc MOURAIT au lieu
						// d'echouer — donc la mutation etait bien tuee, mais par
						// un segfault et non par le cas prevu. La difference
						// compte : un plantage ne dit pas CE QUI est faux.
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
						detail::PutNom(s, n->id, couleur ? "color" : "value");
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
								//
								// NE RETIRE PAS CETTE GARDE EN LA PRENANT POUR DE
								// LA TIMIDITE. Une division nue produit un NaN ;
								// le NaN CONTAMINE tout l'aval du graphe, et il se
								// voit comme un pixel noir ICI et blanc LA selon
								// le backend. Un defaut qui change d'aspect d'une
								// machine a l'autre FAIT ACCUSER LA MACHINE — on
								// cherche alors le pilote, la carte, le systeme,
								// et jamais la ligne de shader qui divise par
								// zero. C'est le cout reel de cette garde
								// absente : des heures passees au mauvais etage.
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
					} else if (t == NkString(NK_MN_COLOR_RAMP)) {
						// ── LA PREMIERE PROPRIETE A CHARGE UTILE VARIABLE ─────
						// N arrets, quatre reels chacun. Le decoupage passe par
						// `NkMatLisRampe`, partage avec le banc : un decoupage
						// duplique finirait par diverger, et c'est le compilateur
						// qui aurait raison sans que personne le sache.
						const NkGraphValue *ps = g.FindProp(n->id, NK_MPROP_STOPS);
						uint32 arrets = 0;
						const NkMatRampeErreur re = NkMatLisRampe(ps, &arrets);

						// La rampe par defaut de Blender : noir -> blanc. C'est le
						// cas LEGITIME d'une propriete absente — le noeud vient
						// d'etre pose. Toute AUTRE erreur est un refus.
						static const float32 kDefaut[8] = {0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f};
						const float32 *st = kDefaut;
						if (re == NkMatRampeErreur::Absente) {
							arrets = 2;
						} else if (re != NkMatRampeErreur::Ok) {
							r.error = NkString("rampe invalide sur ");
							r.error.Append(n->type);
							r.error.Append(" : ");
							r.error.Append(NkMatRampeErreurNom(re));
							if (re == NkMatRampeErreur::TropDArrets) {
								// ⚠️ LE REFUS DIT COMBIEN. « trop d'arrets » seul
								// oblige l'auteur a deviner ce qu'il doit retirer.
								r.error.Append(" (");
								detail::PutU(r.error, (uint32)ps->numbers.Size() / NK_RAMP_REELS_PAR_ARRET);
								r.error.Append(" demandes, plafond ");
								detail::PutU(r.error, NK_RAMP_ARRETS_MAX);
								r.error.Append(")");
							}
							r.source = NkString("");
							return r;
						} else {
							st = ps->numbers.Data();
						}

						// Le mode d'interpolation, meme discipline que les
						// operations : un mot, et un mot inconnu est refuse.
						const NkGraphValue *pi = g.FindProp(n->id, NK_MPROP_INTERP);
						int32 ii = 0;
						if (pi && pi->IsSet()) {
							ii = NkMatTrouveInterp(pi->text.CStr());
							if (ii < 0) {
								r.error = NkString("interpolation inconnue sur ");
								r.error.Append(n->type);
								r.error.Append(" : ");
								r.error.Append(pi->text);
								r.source = NkString("");
								return r;
							}
						}
						const NkMatOperation *id = NkMatInterpAt((uint32)ii);
						if (!id) {
							// Meme raison qu'ailleurs dans ce fichier : un code qui
							// ne peut se tromper que par un plantage n'est pas
							// robuste, il est chanceux.
							r.error = NkString("interpolation hors table");
							r.source = NkString("");
							return r;
						}
						const bool constante = detail::OpEst(id->cle, "constante");

						// Le facteur, borne : une rampe est definie sur [0,1], et
						// extrapoler au-dela donnerait des couleurs que l'auteur
						// n'a jamais choisies.
						s.Append("    float ");
						detail::PutNom(s, n->id, "f");
						s.Append(" = clamp(");
						ecrisEntree(*n, "fac", "float", nullptr);
						s.Append(", 0.0, 1.0);\n");

						// On part du PREMIER arret, puis on empile les suivants.
						// Deroule a la compilation : pas de boucle, pas d'index
						// dynamique — le shader reste lisible et aucun backend
						// n'a besoin d'indexation dynamique de tableau.
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "color");
						s.Append(" = vec3(");
						for (uint32 c = 0; c < 3; ++c) {
							if (c)
								s.Append(", ");
							detail::PutLit(s, st[1 + c]);
						}
						s.Append(");\n");

						for (uint32 k = 1; k < arrets; ++k) {
							const float32 p0 = st[(k - 1) * NK_RAMP_REELS_PAR_ARRET];
							const float32 p1 = st[k * NK_RAMP_REELS_PAR_ARRET];
							s.Append("    ");
							detail::PutNom(s, n->id, "color");
							s.Append(" = mix(");
							detail::PutNom(s, n->id, "color");
							s.Append(", vec3(");
							for (uint32 c = 0; c < 3; ++c) {
								if (c)
									s.Append(", ");
								detail::PutLit(s, st[k * NK_RAMP_REELS_PAR_ARRET + 1 + c]);
							}
							s.Append("), ");
							if (constante) {
								// Palier : la couleur bascule d'un coup a la
								// position de l'arret.
								s.Append("step(");
								detail::PutLit(s, p1);
								s.Append(", ");
								detail::PutNom(s, n->id, "f");
								s.Append(")");
							} else {
								// Lineaire. Le denominateur est calcule ICI, a la
								// compilation, et `NkMatLisRampe` a deja garanti
								// qu'il est strictement positif — les positions
								// egales sont refusees en amont. Il n'y a donc
								// aucune division a garder dans le shader.
								s.Append("clamp((");
								detail::PutNom(s, n->id, "f");
								s.Append(" - ");
								detail::PutLit(s, p0);
								s.Append(") * ");
								detail::PutLit(s, 1.f / (p1 - p0));
								s.Append(", 0.0, 1.0)");
							}
							s.Append(");\n");
						}
					} else if (t == NkString(NK_MN_TEX_COORD)) {
						// L'UV du maillage, sans calcul. `vUV` est une varying, et
						// on est dans l'ENTREE : la contrainte du dialecte (jamais
						// de varying dans un helper) ne s'applique pas ici.
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "uv");
						s.Append(" = vec3(vUV, 0.0);\n");
					} else if (t == NkString(NK_MN_MAPPING)) {
						// vecteur * echelle + position. L'echelle vaut 1 et la
						// position 0 par defaut : un Mapping fraichement pose ne
						// doit RIEN changer, sinon l'auteur voit son image bouger
						// en branchant un noeud cense etre neutre.
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "vector_out");
						s.Append(" = (");
						ecrisEntree(*n, "vector", "vec3", nullptr);
						s.Append(") * (");
						{
							const int32 iv = n->FindSocket("scale", NkSocketDir::Input);
							const bool cable = iv >= 0 && g.IncomingOf(n->id, iv);
							const NkGraphValue *d =
								iv >= 0 ? &n->sockets[(uint32)iv].defaultValue : nullptr;
							if (cable || (d && d->IsSet()))
								ecrisEntree(*n, "scale", "vec3", nullptr);
							else
								s.Append("vec3(1.0)"); // neutre MULTIPLICATIF
						}
						s.Append(") + (");
						ecrisEntree(*n, "location", "vec3", nullptr);
						s.Append(");\n");
					} else if (t == NkString(NK_MN_IMAGE_TEXTURE)) {
						// La convention de normale, si elle est declaree, est
						// VALIDEE ici et n'influence RIEN dans le code emis : la
						// conversion se fait a l'import. Un mot inconnu est refuse
						// — un repli sur OpenGL inverserait le relief de la moitie
						// des fichiers, et l'image resterait plausible.
						{
							const NkGraphValue *pc = g.FindProp(n->id, NK_MPROP_NORMAL_CONV);
							if (pc && pc->IsSet() && NkMatTrouveConvNormale(pc->text.CStr()) < 0) {
								r.error = NkString("convention de normale inconnue sur ");
								r.error.Append(n->type);
								r.error.Append(" : ");
								r.error.Append(pc->text);
								r.source = NkString("");
								return r;
							}
						}
						// ── LE PREMIER NOEUD QUI CONSOMME UNE RESSOURCE ───────
						uint32 slot = 0;
						for (uint32 i = 0; i < (uint32)texNodes.Size(); ++i)
							if (texNodes[i] == n->id)
								slot = i;
						// La coordonnee : celle qu'on lui donne, ou l'UV du
						// maillage. Blender fait exactement ce repli, et il n'est
						// pas « plausible » — c'est le comportement DEFINI du
						// noeud, pas un bouche-trou.
						const int32 iv = n->FindSocket("vector", NkSocketDir::Input);
						const bool cable = iv >= 0 && g.IncomingOf(n->id, iv);
						s.Append("    vec4 ");
						detail::PutNom(s, n->id, "texel");
						s.Append(" = texture(");
						s.Append(NK_MATBIND_GRAPH_SAMPLER_PREFIX);
						detail::PutU(s, slot);
						s.Append(", ");
						if (cable) {
							s.Append("(");
							ecrisEntree(*n, "vector", "vec3", nullptr);
							s.Append(").xy");
						} else {
							s.Append("vUV");
						}
						s.Append(");\n");
						// DEUX sorties, deux locales nommees d'apres leurs prises.
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "color");
						s.Append(" = ");
						detail::PutNom(s, n->id, "texel");
						s.Append(".rgb;\n    float ");
						detail::PutNom(s, n->id, "alpha");
						s.Append(" = ");
						detail::PutNom(s, n->id, "texel");
						s.Append(".a;\n");
					} else if (t == NkString(NK_MN_SEPARATE_XYZ)) {
						// Trois sorties reelles. Le seul moyen, aujourd'hui, de
						// tirer un scalaire VARIABLE d'un graphe.
						static const char *const kAxes[3] = {"x", "y", "z"};
						for (uint32 a = 0; a < 3; ++a) {
							s.Append("    float ");
							detail::PutNom(s, n->id, kAxes[a]);
							s.Append(" = (");
							ecrisEntree(*n, "vector", "vec3", nullptr);
							s.Append(").");
							s.Append(kAxes[a]);
							s.Append(";\n");
						}
					} else if (t == NkString(NK_MN_NORMAL_MAP)) {
						// ⚠️ AUCUNE CONVERSION DE CONVENTION ICI, ET C'EST LE
						// POINT. Une carte DirectX se convertit A L'IMPORT. Le
						// shader ne sait pas d'ou vient la texture et n'a pas a le
						// savoir : le faire par pixel couterait a chaque fragment
						// et rendrait l'etat de la donnee invisible. La convention
						// est validee (plus haut) et RECOPIEE nulle part.
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "nts");
						s.Append(" = (");
						ecrisEntree(*n, "color", "vec3", nullptr);
						s.Append(") * 2.0 - 1.0;\n");
						// `strength` melange entre la normale geometrique et la
						// normale decodee, comme Blender — pas une multiplication
						// de la normale, qui la denormaliserait.
						s.Append("    ");
						detail::PutNom(s, n->id, "nts");
						s.Append(" = normalize(mix(vec3(0.0, 0.0, 1.0), ");
						detail::PutNom(s, n->id, "nts");
						s.Append(", ");
						{
							const int32 iv = n->FindSocket("strength", NkSocketDir::Input);
							const bool cable = iv >= 0 && g.IncomingOf(n->id, iv);
							const NkGraphValue *d = iv >= 0 ? &n->sockets[(uint32)iv].defaultValue : nullptr;
							if (cable || (d && d->IsSet()))
								ecrisEntree(*n, "strength", "float", nullptr);
							else
								s.Append("1.0"); // neutre : la carte s'applique en entier
						}
						s.Append("));\n");
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "normal");
						s.Append(" = normalize(nkT * ");
						detail::PutNom(s, n->id, "nts");
						s.Append(".x + nkB * ");
						detail::PutNom(s, n->id, "nts");
						s.Append(".y + nkGeomN * ");
						detail::PutNom(s, n->id, "nts");
						s.Append(".z);\n");
					} else if (t == NkString(NK_MN_BUMP)) {
						// Relief par derivees d'ecran du champ de hauteur. La
						// hauteur est un SCALAIRE quelconque du graphe ; sa pente
						// a l'ecran donne l'inclinaison de la surface.
						s.Append("    float ");
						detail::PutNom(s, n->id, "h");
						s.Append(" = ");
						ecrisEntree(*n, "height", "float", nullptr);
						s.Append(";\n");
						s.Append("    vec3 nkBdpx = dFdx(vWorldPos);\n");
						s.Append("    vec3 nkBdpy = dFdy(vWorldPos);\n");
						s.Append("    float ");
						detail::PutNom(s, n->id, "dhx");
						s.Append(" = dFdx(");
						detail::PutNom(s, n->id, "h");
						s.Append(");\n    float ");
						detail::PutNom(s, n->id, "dhy");
						s.Append(" = dFdy(");
						detail::PutNom(s, n->id, "h");
						s.Append(");\n");
						s.Append("    vec3 nkBr1 = cross(nkBdpy, nkGeomN);\n");
						s.Append("    vec3 nkBr2 = cross(nkGeomN, nkBdpx);\n");
						// ⚠️ Le determinant peut etre nul sur un triangle degenere
						// ou vu par la tranche. Une division nue produirait un NaN
						// qui contamine l'aval et change d'aspect d'un backend a
						// l'autre — meme raison que la division du noeud Math.
						s.Append("    float nkBdet = dot(nkBdpx, nkBr1);\n");
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "grad");
						s.Append(" = (nkBr1 * ");
						detail::PutNom(s, n->id, "dhx");
						s.Append(" + nkBr2 * ");
						detail::PutNom(s, n->id, "dhy");
						s.Append(") / (abs(nkBdet) < 1e-12 ? 1e-12 : abs(nkBdet));\n");
						s.Append("    vec3 ");
						detail::PutNom(s, n->id, "normal");
						s.Append(" = normalize(");
						ecrisEntree(*n, "normal", "vec3", nullptr);
						s.Append(" - (");
						{
							const int32 iv = n->FindSocket("strength", NkSocketDir::Input);
							const bool cable = iv >= 0 && g.IncomingOf(n->id, iv);
							const NkGraphValue *d = iv >= 0 ? &n->sockets[(uint32)iv].defaultValue : nullptr;
							if (cable || (d && d->IsSet()))
								ecrisEntree(*n, "strength", "float", nullptr);
							else
								s.Append("1.0");
						}
						s.Append(") * (");
						{
							const int32 iv = n->FindSocket("distance", NkSocketDir::Input);
							const bool cable = iv >= 0 && g.IncomingOf(n->id, iv);
							const NkGraphValue *d = iv >= 0 ? &n->sockets[(uint32)iv].defaultValue : nullptr;
							if (cable || (d && d->IsSet()))
								ecrisEntree(*n, "distance", "float", nullptr);
							else
								s.Append("1.0");
						}
						s.Append(") * ");
						detail::PutNom(s, n->id, "grad");
						s.Append(");\n");
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
						s.Append(";\n    vec3 surfNormal = ");
						ecrisEntree(*n, "surface", "vec3", "normal");
						s.Append(";\n\n");
						// La normale vient du GRAPHE. Avant le 22/08 le puits
						// recalculait `normalize(vNormal)` ici, ce qui jetait en
						// silence tout travail de relief en amont.
						s.Append("    vec3 N3 = normalize(surfNormal);\n");
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

				// ⚠️ LE JETON. Il est calcule sur la SOURCE EMISE, donc il change
				// des que la disposition change — et aussi quand seul le code
				// change, ce qui est le bon sens de l'erreur : un cache jete pour
				// rien coute une ecriture, un cache garde a tort ecrit dans le
				// mauvais parametre. FNV-1a, parce qu'on veut un identifiant
				// stable et bon marche, pas une garantie cryptographique.
				{
					uint64 h = 1469598103934665603ull;
					const char *p = r.source.CStr();
					while (p && *p) {
						h ^= (uint64)(unsigned char)*p++;
						h *= 1099511628211ull;
					}
					r.jeton = h;
				}
				r.ok = true;
				return r;
			}

		} // namespace matgraph
	} // namespace renderer
} // namespace nkentseu

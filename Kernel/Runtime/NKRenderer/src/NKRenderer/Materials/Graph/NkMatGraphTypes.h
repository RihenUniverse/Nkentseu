#pragma once
// -----------------------------------------------------------------------------
// @File    NkMatGraphTypes.h
// @Brief   COUCHE 3 pour les materiaux : types de sockets et bibliotheque de
//          prototypes de noeuds, poses SUR le coeur agnostique NKGraph.
// @Author  Rihen
// @License Proprietary - All Rights Reserved (see LICENSE)
//
// OU CE FICHIER SE SITUE, ET POURQUOI CA COMPTE
//   Decision d'architecture du 2026-07-09 (cf. Kernel/Runtime/NKGraph/ROADMAP.md) :
//   UN SEUL systeme de graphe pour tout l'ecosysteme, en trois couches.
//     couche 1 — le coeur, Kernel/Runtime/NKGraph : modele pur, AUCUN type metier ;
//     couche 2 — le canevas d'edition, Engine/NKEditorKit, partage ;
//     couche 3 — CE FICHIER : la semantique MATERIAU, et rien d'autre.
//   Le mot « couleur », le mot « shader » et le mot « Principled » n'ont le droit
//   d'exister qu'ICI. Les mettre dans le coeur tuerait l'architecture — c'est le
//   garde-fou n°1 du module, ecrit dans l'en-tete de NkNodeGraph.h.
//
// EN-TETE PUR, SANS CIBLE DE BUILD — meme raison que NkNodeGraph et
//   NkShortcutTable : le banc console peut l'exercer sans lier NKRenderer, donc
//   sans GPU, sans fenetre et sans les trente secondes de liaison qui vont avec.
//   La cible deviendra legitime le jour ou un consommateur reel (le systeme de
//   materiaux, l'editeur) la liera.
//
// CE QUE CE FICHIER NE FAIT PAS ENCORE, ET POURQUOI
//   Il ne porte AUCUNE valeur de parametre (Base Color = blanc, Roughness = 0,5,
//   les arrets d'un ColorRamp, l'operation d'un noeud Math). Ce n'est pas un
//   oubli : `NkNode` du coeur n'a aujourd'hui aucun endroit ou les ranger et le
//   format `.nkgraph` aucune directive pour les ecrire. La question est posee en
//   Q14 dans echanges/nkrenderer.questions.md et se decide au niveau du coeur,
//   pas ici. Tant qu'elle n'est pas tranchee, les prototypes ci-dessous
//   declarent la FORME d'un noeud — ses prises, leurs noms, leurs types — ce qui
//   est deja ce dont le compilateur a besoin pour parcourir un graphe.
//
// ZERO-STL : NkVector / NkString, aucun `std::`.
// -----------------------------------------------------------------------------

#include "NKGraph/NkNodeGraph.h"

namespace nkentseu {
	namespace renderer {
		namespace matgraph {

			using graph::NkGraphProp;
			using graph::NkGraphValue;
			using graph::NkNode;
			using graph::NkNodeGraph;
			using graph::NkNodeId;
			using graph::NkSocket;
			using graph::NkSocketDir;
			using graph::NkTypeId;
			using graph::NK_NODE_INVALID;
			using graph::NK_TYPE_INVALID;

			// ── LES TYPES DE PRISES D'UN GRAPHE DE MATERIAU ──────────────────
			// Ce sont des NOMS que le coeur enregistre et compare sans jamais
			// savoir ce qu'ils designent. Le decoupage suit celui de Blender, qui
			// est le modele demande — et il n'est pas arbitraire :
			//
			//   reel / vecteur / couleur  se convertissent entre eux dans un SENS
			//                             precis (un reel alimente un vecteur : il
			//                             se diffuse sur les trois composantes).
			//   shader                    ne se convertit avec RIEN. C'est ce qui
			//                             empeche de brancher une couleur la ou le
			//                             moteur attend une surface ombree — et
			//                             c'est la moitie des erreurs qu'un auteur
			//                             commet dans l'editeur de Blender.
			static const char *const NK_MT_REAL = "mat.reel";
			static const char *const NK_MT_VECTOR = "mat.vecteur";
			static const char *const NK_MT_COLOR = "mat.couleur";
			static const char *const NK_MT_SHADER = "mat.shader";

			struct NkMatTypes {
					NkTypeId real = graph::NK_TYPE_INVALID;
					NkTypeId vector = graph::NK_TYPE_INVALID;
					NkTypeId color = graph::NK_TYPE_INVALID;
					NkTypeId shader = graph::NK_TYPE_INVALID;

					bool Valid() const {
						return real && vector && color && shader;
					}
			};

			// Enregistre les quatre types et les conversions AUTORISEES.
			//
			// ⚠️ LES CONVERSIONS SONT DIRIGEES, ET C'EST TOUT L'INTERET. Le coeur
			// ne devine jamais : il applique ce qu'on declare. On declare donc
			// exactement ce que fait Blender, et rien de plus :
			//   reel -> vecteur   : la valeur se diffuse en (v, v, v)
			//   reel -> couleur   : niveau de gris (v, v, v)
			//   vecteur <-> couleur : les deux sens, ce sont trois flottants dans
			//                        les deux cas et Blender les echange librement
			// Ce qu'on NE declare PAS, volontairement :
			//   couleur -> reel   : il faudrait choisir entre la luminance, la
			//                       moyenne et le canal rouge. Trois reponses
			//                       plausibles, donc aucune par defaut : l'auteur
			//                       pose un noeud qui DIT laquelle il veut.
			//   shader -> quoi que ce soit, et l'inverse.
			inline NkMatTypes NkMatRegisterTypes(NkNodeGraph &g) {
				NkMatTypes t;
				t.real = g.RegisterType(NK_MT_REAL);
				t.vector = g.RegisterType(NK_MT_VECTOR);
				t.color = g.RegisterType(NK_MT_COLOR);
				t.shader = g.RegisterType(NK_MT_SHADER);
				g.AllowConversion(t.real, t.vector);
				g.AllowConversion(t.real, t.color);
				g.AllowConversion(t.vector, t.color);
				g.AllowConversion(t.color, t.vector);
				return t;
			}

			// ── LES OPERATIONS RECONNUES ─────────────────────────────────────
			// UN SEUL endroit. Le compilateur les lit, le banc les enumere, et
			// l'interface les proposera. Trois listes finiraient par diverger, et
			// c'est le compilateur qui aurait raison sans que personne le sache.
			//
			// ⚠️ Ce sont des MOTS, jamais des numeros. Un numero d'enumeration se
			// decale des qu'on insere une valeur au milieu, et les graphes deja
			// enregistres se mettent alors a calculer autre chose EN SILENCE.
			struct NkMatOperation {
					const char *cle;	 ///< ce qui s'ecrit dans le fichier
					const char *libelle; ///< ce qui s'affiche, traduisible
			};

			namespace detail {
				static const NkMatOperation kOpsMath[] = {
					{"ajouter", "Add"},		  {"soustraire", "Subtract"}, {"multiplier", "Multiply"},
					{"diviser", "Divide"},	  {"minimum", "Minimum"},	  {"maximum", "Maximum"},
					{"puissance", "Power"},
				};
				static const uint32 kOpsMathCount = 7;

				static const NkMatOperation kOpsMix[] = {
					{"melanger", "Mix"},		{"multiplier", "Multiply"}, {"ajouter", "Add"},
					{"soustraire", "Subtract"}, {"eclaircir", "Lighten"},	{"assombrir", "Darken"},
				};
				static const uint32 kOpsMixCount = 6;
			} // namespace detail

			inline uint32 NkMatOperationCount(bool pourMelangeCouleur) {
				return pourMelangeCouleur ? detail::kOpsMixCount : detail::kOpsMathCount;
			}

			inline const NkMatOperation *NkMatOperationAt(bool pourMelangeCouleur, uint32 i) {
				const uint32 n = NkMatOperationCount(pourMelangeCouleur);
				if (i >= n)
					return nullptr;
				return pourMelangeCouleur ? &detail::kOpsMix[i] : &detail::kOpsMath[i];
			}

			// Rend -1 si la cle est inconnue. ⚠️ L'appelant doit REFUSER dans ce
			// cas, jamais retomber sur la premiere operation : une operation
			// inconnue traitee comme « ajouter » produit un materiau qui compile,
			// qui rend, et qui calcule autre chose que ce que le fichier disait.
			inline int32 NkMatTrouveOperation(bool pourMelangeCouleur, const char *cle) {
				if (!cle)
					return -1;
				const uint32 n = NkMatOperationCount(pourMelangeCouleur);
				for (uint32 i = 0; i < n; ++i) {
					const char *a = NkMatOperationAt(pourMelangeCouleur, i)->cle;
					const char *b = cle;
					while (*a && *a == *b) {
						++a;
						++b;
					}
					if (!*a && !*b)
						return (int32)i;
				}
				return -1;
			}

			// ── PROTOTYPES DE NOEUDS ─────────────────────────────────────────
			// Un prototype decrit la FORME d'un noeud : sa cle de type, son
			// libelle, et ses prises NOMMEES. Les noms sont des CLES stables :
			// renommer le libelle affiche ne doit jamais casser un graphe
			// enregistre, et c'est pour ca que le lien porte un nom et pas un
			// numero de colonne.
			struct NkMatSocketDecl {
					const char *name;
					const char *type; ///< un des NK_MT_*
					NkSocketDir dir;
					// ⚠️ CERTAINES PRISES NE PEUVENT PAS RECEVOIR D'EXPRESSION.
					// Un parametre qui alimente l'ETAT DU PIPELINE — mode de
					// melange, mode d'ombre — ne peut pas varier par pixel a
					// moindre cout. Blender a la meme limite : certaines entrees
					// refusent un lien.
					//
					// L'interface doit alors NE PAS AFFICHER le point de
					// connexion, plutot qu'ouvrir un menu vide : un menu vide
					// laisse croire a une panne, une prise sans point dit « ce
					// parametre est une constante », ce qui est la verite.
					//
					// C'est declare ICI, en couche 3, et pas dans le coeur : le
					// coeur ne sait pas ce qu'est un etat de pipeline. La
					// consequence est que `Connect` ne le REFUSE pas — c'est la
					// requete de bibliotheque qui rend une liste vide, et une
					// passe de validation qui pourra le signaler. Conforme au
					// principe : la validation est une passe, jamais une
					// precondition de structure.
					bool constanteSeulement = false;
			};

			struct NkMatNodeProto {
					const char *key;   ///< ex. « mat.principled »
					const char *label; ///< libelle par defaut, traduisible
					const NkMatSocketDecl *sockets;
					uint32 socketCount;
			};

			// Cles des noeuds de cette premiere tranche. On n'en declare que ce
			// qu'on sait deja compiler ou refuser : une bibliotheque de 150
			// noeuds dont trois marchent ne rend service a personne.
			static const char *const NK_MN_PRINCIPLED = "mat.principled";
			static const char *const NK_MN_DIFFUSE = "mat.diffuse";
			static const char *const NK_MN_EMISSION = "mat.emission";
			static const char *const NK_MN_MIX_SHADER = "mat.melange_shader";
			static const char *const NK_MN_OUTPUT = "mat.sortie";
			// Les deux noeuds SOURCES de Blender : `Value` et `RGB`. Ils ne
			// prennent aucune entree et produisent une valeur constante, portee
			// par une PROPRIETE DE NOEUD. Ce sont eux qui donnent une reponse
			// non vide a « que puis-je brancher sur une prise de type couleur ? »
			// — et ce sont les premiers noeuds du depot a exercer `NkNode::props`
			// jusque dans le shader.
			static const char *const NK_MN_VALUE = "mat.valeur";
			static const char *const NK_MN_RGB = "mat.rgb";
			// Cles des proprietes qu'ils portent. Ce sont des CLES stables : le
			// compilateur les lit par ce nom, l'interface les ecrit par ce nom.
			static const char *const NK_MPROP_VALUE = "valeur";
			static const char *const NK_MPROP_COLOR = "couleur";

			// `Math` et `Mix Color` : les deux premiers noeuds dont le CALCUL lui
			// meme est choisi par une propriete. Jusqu'ici une propriete portait
			// une valeur ; celles-ci portent une DECISION.
			static const char *const NK_MN_MATH = "mat.math";
			static const char *const NK_MN_MIX_COLOR = "mat.melange_couleur";
			// La cle de l'operation, commune aux deux : le consommateur lit un
			// mot, jamais un numero. Un numero d'enumeration se decale des qu'on
			// insere une valeur au milieu, et les graphes enregistres se mettent
			// alors a calculer autre chose SANS que rien ne le dise.
			static const char *const NK_MPROP_OPERATION = "operation";

			namespace detail {

				// Principled REDUIT a ce qui a un sens dans un rasteriseur, et
				// nomme comme chez Blender. Le Principled complet en a une
				// vingtaine ; les entrees absentes ici (subsurface, sheen,
				// clearcoat, transmission) ne sont pas oubliees, elles attendent
				// d'avoir une reponse cote rendu — une prise qui existe et que
				// personne n'honore est pire qu'une prise absente.
				static const NkMatSocketDecl kPrincipled[] = {
					{"base_color", NK_MT_COLOR, NkSocketDir::Input},
					{"metallic", NK_MT_REAL, NkSocketDir::Input},
					{"roughness", NK_MT_REAL, NkSocketDir::Input},
					{"emission", NK_MT_COLOR, NkSocketDir::Input},
					{"normal", NK_MT_VECTOR, NkSocketDir::Input},
					{"bsdf", NK_MT_SHADER, NkSocketDir::Output},
				};

				static const NkMatSocketDecl kDiffuse[] = {
					{"color", NK_MT_COLOR, NkSocketDir::Input},
					{"roughness", NK_MT_REAL, NkSocketDir::Input},
					{"normal", NK_MT_VECTOR, NkSocketDir::Input},
					{"bsdf", NK_MT_SHADER, NkSocketDir::Output},
				};

				static const NkMatSocketDecl kEmission[] = {
					{"color", NK_MT_COLOR, NkSocketDir::Input},
					{"strength", NK_MT_REAL, NkSocketDir::Input},
					{"emission", NK_MT_SHADER, NkSocketDir::Output},
				};

				// Mix Shader : DEUX entrees shader et un facteur. Sur un
				// rasteriseur on melange les resultats d'ombrage, pas des
				// closures a la Cycles — c'est ce que fait EEVEE, et Unreal, et
				// Unity. La FORME du noeud est la meme ; seule son execution
				// differe, et l'execution n'est pas l'affaire du graphe.
				static const NkMatSocketDecl kMixShader[] = {
					{"fac", NK_MT_REAL, NkSocketDir::Input},
					{"shader1", NK_MT_SHADER, NkSocketDir::Input},
					{"shader2", NK_MT_SHADER, NkSocketDir::Input},
					{"shader", NK_MT_SHADER, NkSocketDir::Output},
				};

				// Material Output : le puits. AUCUNE sortie — c'est ce qui le rend
				// reconnaissable sans un `if` sur son nom au moment du parcours.
				static const NkMatSocketDecl kOutput[] = {
					{"surface", NK_MT_SHADER, NkSocketDir::Input},
				};

				// `Value` : un reel constant. `RGB` : une couleur constante. Leur
				// valeur vit dans une propriete de noeud, pas dans une prise : il
				// n'y a rien a y brancher, c'est le point de depart d'une chaine.
				static const NkMatSocketDecl kValue[] = {
					{"value", NK_MT_REAL, NkSocketDir::Output, false},
				};
				static const NkMatSocketDecl kRGB[] = {
					{"color", NK_MT_COLOR, NkSocketDir::Output, false},
				};

				static const NkMatSocketDecl kMath[] = {
					{"a", NK_MT_REAL, NkSocketDir::Input, false},
					{"b", NK_MT_REAL, NkSocketDir::Input, false},
					{"value", NK_MT_REAL, NkSocketDir::Output, false},
				};

				// Mix Color : le `fac` d'abord, comme chez Blender, puis les deux
				// couleurs. L'ordre des prises EST leur index, et les liens s'y
				// referent : le changer casserait les graphes enregistres.
				static const NkMatSocketDecl kMixColor[] = {
					{"fac", NK_MT_REAL, NkSocketDir::Input, false},
					{"color1", NK_MT_COLOR, NkSocketDir::Input, false},
					{"color2", NK_MT_COLOR, NkSocketDir::Input, false},
					{"color", NK_MT_COLOR, NkSocketDir::Output, false},
				};

				static const NkMatNodeProto kProtos[] = {
					{NK_MN_PRINCIPLED, "Principled BSDF", kPrincipled, 6},
					{NK_MN_DIFFUSE, "Diffuse BSDF", kDiffuse, 4},
					{NK_MN_EMISSION, "Emission", kEmission, 3},
					{NK_MN_MIX_SHADER, "Mix Shader", kMixShader, 4},
					{NK_MN_OUTPUT, "Material Output", kOutput, 1},
					{NK_MN_VALUE, "Value", kValue, 1},
					{NK_MN_RGB, "RGB", kRGB, 1},
					{NK_MN_MATH, "Math", kMath, 3},
					{NK_MN_MIX_COLOR, "Mix Color", kMixColor, 4},
				};
				static const uint32 kProtoCount = 9;

			} // namespace detail

			inline uint32 NkMatProtoCount() {
				return detail::kProtoCount;
			}

			inline const NkMatNodeProto *NkMatProtoAt(uint32 i) {
				return i < detail::kProtoCount ? &detail::kProtos[i] : nullptr;
			}

			inline const NkMatNodeProto *NkMatFindProto(const char *key) {
				if (!key)
					return nullptr;
				for (uint32 i = 0; i < detail::kProtoCount; ++i) {
					const char *a = detail::kProtos[i].key;
					const char *b = key;
					while (*a && *a == *b) {
						++a;
						++b;
					}
					if (*a == 0 && *b == 0)
						return &detail::kProtos[i];
				}
				return nullptr;
			}

			// Instancie un prototype DANS le graphe : cree le noeud puis ajoute
			// ses prises, dans l'ordre declare.
			//
			// Rend NK_NODE_INVALID si la cle est inconnue ou si les types n'ont
			// pas ete enregistres. On ne cree PAS un noeud a moitie forme : un
			// noeud sans ses prises se relierait mal et l'erreur se verrait trois
			// manipulations plus loin, loin de sa cause.
			inline NkNodeId NkMatAddNode(NkNodeGraph &g, const char *protoKey) {
				const NkMatNodeProto *p = NkMatFindProto(protoKey);
				if (!p)
					return graph::NK_NODE_INVALID;
				// Les types doivent DEJA etre enregistres : les chercher, jamais
				// les creer ici. Un RegisterType() implicite masquerait un appel
				// oublie a NkMatRegisterTypes et priverait le graphe de ses
				// conversions — il refuserait alors des liens parfaitement bons.
				for (uint32 i = 0; i < p->socketCount; ++i)
					if (g.FindType(p->sockets[i].type) == graph::NK_TYPE_INVALID)
						return graph::NK_NODE_INVALID;

				const NkNodeId n = g.AddNode(p->key, p->label);
				if (n == graph::NK_NODE_INVALID)
					return n;
				for (uint32 i = 0; i < p->socketCount; ++i) {
					const NkMatSocketDecl &s = p->sockets[i];
					g.AddSocket(n, s.name, g.FindType(s.type), s.dir);
				}
				return n;
			}

			// Une prise accepte-t-elle un lien ? Isole en une fonction pour que la
			// requete ET le banc jugent par le MEME chemin : un drapeau lu a deux
			// endroits differents finit par etre lu differemment.
			inline bool NkMatPriseAccepteUnLien(const NkMatSocketDecl &sd) {
				return !sd.constanteSeulement;
			}

			// ── « QUE PUIS-JE BRANCHER ICI ? » ───────────────────────────────
			// La demande de Rodolf (2026-08-22) : dans Blender, chaque parametre
			// porte un point, et cliquer ce point ouvre un menu de sources
			// FILTRE PAR LE TYPE DE LA PRISE — le menu de `Base Color` et celui
			// de `Roughness` n'ont pas le meme contenu.
			//
			// ⚠️ LA REPONSE SE CALCULE, ELLE NE S'ECRIT PAS. Une liste tenue a la
			// main par famille se perimerait au premier noeud ajoute, et
			// divergerait de ce que le graphe accepte VRAIMENT — on aurait alors
			// un menu qui propose ce que `Connect` refuse. Ici la question est
			// posee au registre : un prototype est proposable si l'une de ses
			// SORTIES est acceptee par la prise, conversions dirigees comprises.
			//
			// Consequence directe, et c'est le principe que Rodolf a formule :
			// **un parametre n'est pas une valeur, c'est une EXPRESSION d'un type
			// donne.** Une constante en est une forme, un echantillonnage une
			// autre. Ce qui les rend interchangeables est le TYPE.
			//
			// Rend le nombre de prototypes proposables ; remplit `out` jusqu'a
			// `maxOut`. `out` peut etre nul pour ne compter que.
			inline uint32 NkMatNoeudsPourPrise(const NkNodeGraph &g, NkTypeId typePrise,
											   const NkMatNodeProto **out = nullptr, uint32 maxOut = 0) {
				if (typePrise == NK_TYPE_INVALID)
					return 0;
				uint32 n = 0;
				for (uint32 i = 0; i < NkMatProtoCount(); ++i) {
					const NkMatNodeProto *p = NkMatProtoAt(i);
					bool proposable = false;
					for (uint32 k = 0; k < p->socketCount; ++k) {
						const NkMatSocketDecl &sd = p->sockets[k];
						if (sd.dir != NkSocketDir::Output)
							continue;
						// `Accepts` porte les conversions DIRIGEES : un reel
						// alimente une couleur, l'inverse est refuse. C'est ce qui
						// fait que le menu d'une prise couleur contient `Value`,
						// et que celui d'une prise reelle ne contient pas `RGB`.
						if (g.Accepts(typePrise, g.FindType(sd.type))) {
							proposable = true;
							break;
						}
					}
					// Un noeud SANS AUCUNE SORTIE — le Material Output — n'est
					// jamais proposable. Rien ne peut sortir de lui : c'est un
					// puits. Il tombe naturellement, sans cas particulier.
					if (!proposable)
						continue;
					if (out && n < maxOut)
						out[n] = p;
					++n;
				}
				return n;
			}

			// La meme question, posee comme l'interface la pose : « sur CETTE
			// prise de CE noeud ». Rend 0 si la prise est declaree constante.
			inline uint32 NkMatNoeudsPourPriseDe(const NkNodeGraph &g, const char *protoKey, const char *prise,
												 const NkMatNodeProto **out = nullptr, uint32 maxOut = 0) {
				const NkMatNodeProto *p = NkMatFindProto(protoKey);
				if (!p || !prise)
					return 0;
				for (uint32 k = 0; k < p->socketCount; ++k) {
					const NkMatSocketDecl &sd = p->sockets[k];
					if (sd.dir != NkSocketDir::Input)
						continue;
					const char *a = sd.name;
					const char *b = prise;
					while (*a && *a == *b) {
						++a;
						++b;
					}
					if (*a || *b)
						continue;
					if (!NkMatPriseAccepteUnLien(sd))
						return 0; // pas de point de connexion sur cette prise
					return NkMatNoeudsPourPrise(g, g.FindType(sd.type), out, maxOut);
				}
				return 0;
			}

			// ── VALIDATION PROPRE AU DOMAINE ─────────────────────────────────
			// Elle est une PASSE, jamais une precondition : un graphe en cours
			// d'edition passe son temps invalide — zero sortie pendant qu'on la
			// pose, deux sorties pendant qu'on remplace la premiere. Le refuser a
			// la construction rendrait le graphe chargeable mais pas editable.
			enum class NkMatGraphError : uint8 {
				Ok = 0,
				NoOutput,	   ///< aucun Material Output
				MultipleOutput, ///< plusieurs Material Output
				OutputUnlinked, ///< la sortie existe mais rien n'y entre
				Cycle,
			};

			inline const char *NkMatGraphErrorName(NkMatGraphError e) {
				switch (e) {
					case NkMatGraphError::Ok:
						return "ok";
					case NkMatGraphError::NoOutput:
						return "aucune-sortie";
					case NkMatGraphError::MultipleOutput:
						return "plusieurs-sorties";
					case NkMatGraphError::OutputUnlinked:
						return "sortie-non-reliee";
					case NkMatGraphError::Cycle:
						return "cycle";
				}
				return "?";
			}

			// `outOutput` recoit le noeud de sortie quand il y en a exactement un.
			inline NkMatGraphError NkMatValidate(const NkNodeGraph &g, NkNodeId *outOutput = nullptr) {
				NkNodeId found = graph::NK_NODE_INVALID;
				uint32 count = 0;
				// Parcours BRUT : il faut voir toute la table. Un noeud supprime
				// reste dans le tableau avec alive=false, et le compter ferait
				// dire « plusieurs sorties » a un graphe qui n'en a qu'une.
				for (uint32 i = 0; i < g.RawNodeCount(); ++i) {
					const graph::NkNode *n = g.RawNodeAt(i);
					if (!n || !n->alive)
						continue;
					const char *a = n->type.CStr();
					const char *b = NK_MN_OUTPUT;
					while (*a && *a == *b) {
						++a;
						++b;
					}
					if (*a == 0 && *b == 0) {
						++count;
						if (count == 1)
							found = n->id;
					}
				}
				if (outOutput)
					*outOutput = found;
				if (count == 0)
					return NkMatGraphError::NoOutput;
				if (count > 1)
					return NkMatGraphError::MultipleOutput;
				if (g.HasCycle())
					return NkMatGraphError::Cycle;

				const graph::NkNode *out = g.Find(found);
				const int32 s = out ? out->FindSocket("surface", NkSocketDir::Input) : -1;
				if (s < 0 || !g.IncomingOf(found, s))
					return NkMatGraphError::OutputUnlinked;
				return NkMatGraphError::Ok;
			}

		} // namespace matgraph
	} // namespace renderer
} // namespace nkentseu

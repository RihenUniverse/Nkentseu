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

				static const NkMatNodeProto kProtos[] = {
					{NK_MN_PRINCIPLED, "Principled BSDF", kPrincipled, 6},
					{NK_MN_DIFFUSE, "Diffuse BSDF", kDiffuse, 4},
					{NK_MN_EMISSION, "Emission", kEmission, 3},
					{NK_MN_MIX_SHADER, "Mix Shader", kMixShader, 4},
					{NK_MN_OUTPUT, "Material Output", kOutput, 1},
				};
				static const uint32 kProtoCount = 5;

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

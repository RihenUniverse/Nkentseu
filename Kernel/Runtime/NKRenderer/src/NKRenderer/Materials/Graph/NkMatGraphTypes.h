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
			// ⚠️ UN TYPE DE VALEUR QUI N'EST LE TYPE D'AUCUNE PRISE, et c'est
			// legitime. La condition posee sur les proprietes est que leur
			// `NkTypeId` vienne DU REGISTRE — pas qu'il soit porte par une prise.
			// Une rampe ne se branche pas : elle se REGLE. Elle a pourtant besoin
			// d'un type, sans quoi le coeur ne pourrait ni la comparer ni la
			// valider, et on aurait rouvert la porte de derriere du garde-fou n°1.
			static const char *const NK_MT_RAMP = "mat.rampe";

			// Quatre reels par arret : c'est ecrit ici parce que le compilateur,
			// le banc et l'interface doivent decouper le tableau de la MEME
			// facon. Un decoupage suppose a trois endroits est un decoupage qui
			// divergera.
			static const uint32 NK_RAMP_REELS_PAR_ARRET = 4;
			// ⚠️ PLAFOND. Une charge variable sans borne est une invitation : un
			// fichier annoncant dix mille arrets deroulerait dix mille `mix` dans
			// le shader, qui ne compilerait plus — et l'erreur accuserait le
			// backend. On borne ici, et le refus dira le compte.
			static const uint32 NK_RAMP_ARRETS_MAX = 32;

			struct NkMatTypes {
					NkTypeId real = graph::NK_TYPE_INVALID;
					NkTypeId vector = graph::NK_TYPE_INVALID;
					NkTypeId color = graph::NK_TYPE_INVALID;
					NkTypeId shader = graph::NK_TYPE_INVALID;
					NkTypeId ramp = graph::NK_TYPE_INVALID;

					bool Valid() const {
						return real && vector && color && shader && ramp;
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
				t.ramp = g.RegisterType(NK_MT_RAMP);
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

				// Types de degrade. Ceux de Blender qui ont un sens sans
				// coordonnee d'objet : les autres demanderaient des varyings que
				// le vertex engendre ne fournit pas, et une prise qui rendrait
				// zero serait le repli plausible qu'on refuse partout.
				static const NkMatOperation kTypesDegrade[] = {
					{"lineaire", "Linear"},
					{"quadratique", "Quadratic"},
					{"radial", "Radial"},
					{"spherique", "Spherical"},
				};
				static const uint32 kTypesDegradeCount = 4;

				// Conventions de carte de normales. Des MOTS, une seule table, et
				// une convention inconnue est REFUSEE : un repli sur OpenGL
				// donnerait un relief inverse sur la moitie des fichiers, et
				// l'image resterait plausible.
				static const NkMatOperation kConvNormale[] = {
					{"opengl", "OpenGL (+Y vers le haut)"},
					{"directx", "DirectX (-Y vers le haut)"},
				};
				static const uint32 kConvNormaleCount = 2;

				// Interpolations de rampe. Meme discipline que les operations :
				// des MOTS, une seule table, et un mode inconnu est REFUSE.
				static const NkMatOperation kInterps[] = {
					{"lineaire", "Linear"},
					{"constante", "Constant"},
				};
				static const uint32 kInterpsCount = 2;

				static const NkMatOperation kOpsMix[] = {
					{"melanger", "Mix"},		{"multiplier", "Multiply"}, {"ajouter", "Add"},
					{"soustraire", "Subtract"}, {"eclaircir", "Lighten"},	{"assombrir", "Darken"},
				};
				static const uint32 kOpsMixCount = 6;
			} // namespace detail

			inline uint32 NkMatTypeDegradeCount() {
				return detail::kTypesDegradeCount;
			}

			inline const NkMatOperation *NkMatTypeDegradeAt(uint32 i) {
				return i < detail::kTypesDegradeCount ? &detail::kTypesDegrade[i] : nullptr;
			}

			inline int32 NkMatTrouveTypeDegrade(const char *cle) {
				if (!cle)
					return -1;
				for (uint32 i = 0; i < detail::kTypesDegradeCount; ++i) {
					const char *a = detail::kTypesDegrade[i].cle;
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

			inline uint32 NkMatConvNormaleCount() {
				return detail::kConvNormaleCount;
			}

			inline const NkMatOperation *NkMatConvNormaleAt(uint32 i) {
				return i < detail::kConvNormaleCount ? &detail::kConvNormale[i] : nullptr;
			}

			inline int32 NkMatTrouveConvNormale(const char *cle) {
				if (!cle)
					return -1;
				for (uint32 i = 0; i < detail::kConvNormaleCount; ++i) {
					const char *a = detail::kConvNormale[i].cle;
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

			inline uint32 NkMatInterpCount() {
				return detail::kInterpsCount;
			}

			inline const NkMatOperation *NkMatInterpAt(uint32 i) {
				return i < detail::kInterpsCount ? &detail::kInterps[i] : nullptr;
			}

			inline int32 NkMatTrouveInterp(const char *cle) {
				if (!cle)
					return -1;
				for (uint32 i = 0; i < detail::kInterpsCount; ++i) {
					const char *a = detail::kInterps[i].cle;
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

			// ── UNE RAMPE, LUE ET VALIDEE AU MEME ENDROIT ────────────────────
			// Le compilateur ET le banc passent par ici : un decoupage duplique
			// finirait par diverger, et c'est le compilateur qui aurait raison
			// sans que personne le sache.
			enum class NkMatRampeErreur : uint8 {
				Ok = 0,
				Absente,	  ///< pas de propriete : cas LEGITIME, rampe par defaut
				MalFormee,	  ///< le compte de reels n'est pas un multiple de 4
				Vide,		  ///< zero arret
				TropDArrets,  ///< au-dela du plafond
				NonTriee,	  ///< positions dans le DESORDRE : il faut les reordonner
				// ⚠️ DEUX ARRETS A LA MEME POSITION EST UN AUTRE DEFAUT, et il se
				// repare autrement : le desordre se corrige en reordonnant, une
				// egalite en DEPLACANT un arret. Un message commun obligerait
				// l'auteur a comprendre lui-meme lequel des deux il a sous les
				// yeux. Separe apres qu'un cas de banc l'a exige.
				PositionsEgales,
			};

			inline NkMatRampeErreur NkMatLisRampe(const NkGraphValue *v, uint32 *outArrets) {
				if (outArrets)
					*outArrets = 0;
				if (!v || !v->IsSet())
					return NkMatRampeErreur::Absente;
				const uint32 n = (uint32)v->numbers.Size();
				if (n % NK_RAMP_REELS_PAR_ARRET != 0)
					return NkMatRampeErreur::MalFormee;
				const uint32 arrets = n / NK_RAMP_REELS_PAR_ARRET;
				if (arrets == 0)
					return NkMatRampeErreur::Vide;
				if (arrets > NK_RAMP_ARRETS_MAX)
					return NkMatRampeErreur::TropDArrets;
				// ⚠️ POSITIONS CROISSANTES. Deux arrets a la meme position font
				// diviser par zero dans l'interpolation ; des positions dans le
				// desordre rendent une rampe qui a l'air de marcher et qui lit
				// les couleurs dans le mauvais ordre. On REFUSE plutot que de
				// trier en silence : trier changerait le fichier de l'auteur sans
				// le lui dire.
				for (uint32 i = 1; i < arrets; ++i) {
					const float32 avant = v->numbers[(i - 1) * NK_RAMP_REELS_PAR_ARRET];
					const float32 ici = v->numbers[i * NK_RAMP_REELS_PAR_ARRET];
					if (ici < avant)
						return NkMatRampeErreur::NonTriee;
					if (ici == avant)
						return NkMatRampeErreur::PositionsEgales;
				}
				if (outArrets)
					*outArrets = arrets;
				return NkMatRampeErreur::Ok;
			}

			inline const char *NkMatRampeErreurNom(NkMatRampeErreur e) {
				switch (e) {
					case NkMatRampeErreur::Ok:
						return "ok";
					case NkMatRampeErreur::Absente:
						return "absente";
					case NkMatRampeErreur::MalFormee:
						return "mal-formee";
					case NkMatRampeErreur::Vide:
						return "vide";
					case NkMatRampeErreur::TropDArrets:
						return "trop-d-arrets";
					case NkMatRampeErreur::NonTriee:
						return "positions-dans-le-desordre";
					case NkMatRampeErreur::PositionsEgales:
						return "deux-arrets-a-la-meme-position";
				}
				return "?";
			}

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

			// ColorRamp : la PREMIERE propriete a charge utile VARIABLE. Ses
			// arrets sont N groupes de quatre reels — position, r, v, b — dans un
			// seul `NkGraphValue`. Jusqu'ici toutes les charges etaient de taille
			// fixe (un reel, trois, quatre) ; celle-ci est ce qui eprouve
			// vraiment le sac de proprietes.
			static const char *const NK_MN_COLOR_RAMP = "mat.rampe_couleur";

			// `Image Texture` et ses deux compagnons de rang 1. `Texture
			// Coordinate` et `Mapping` existent pour que l'auteur puisse dire OU
			// echantillonner ; sans eux une texture ne saurait que lire l'UV brut.
			static const char *const NK_MN_IMAGE_TEXTURE = "mat.texture_image";
			static const char *const NK_MN_TEX_COORD = "mat.coord_texture";
			static const char *const NK_MN_MAPPING = "mat.mappage";
			// Le chemin de l'image : une charge utile VARIABLE elle aussi, mais
			// textuelle cette fois. C'est le second usage du sac de proprietes
			// apres les arrets de rampe, et il valide l'autre moitie de
			// `NkGraphValue` — le texte, la ou la rampe validait les reels.
			static const char *const NK_MPROP_IMAGE = "image";

			// ⚠️ LA CONVENTION D'UNE CARTE DE NORMALES EST UNE DONNEE DE
			// PROVENANCE DU FICHIER, PAS UN CALCUL DE SHADER.
			//
			// Convention interne : **OpenGL, +Y vers le haut** (« vert vers le
			// haut »). Une carte DirectX (-Y) se convertit A L'IMPORT, jamais
			// dans le shader. Trois raisons, tranchees le 2026-08-22 :
			//   - Blender emploie cette convention, et Rodolf construit sur les
			//     siennes : diverger produirait des reliefs INVERSES en important
			//     son propre travail, avec un symptome notoirement difficile a
			//     diagnostiquer (l'image reste plausible, elle est juste creuse
			//     la ou elle devrait etre bombee) ;
			//   - Vulkan et OpenGL sont les deux backends valides ;
			//   - convertir a l'import evite de payer par pixel ET rend l'etat de
			//     la texture visible dans la donnee.
			//
			// Le drapeau existe DES MAINTENANT, meme si rien ne le consomme
			// encore : l'ajouter apres coup obligerait a DEVINER la convention
			// des textures deja importees — et les deux hypotheses donnent une
			// image plausible, donc le doute serait indecidable.
			static const char *const NK_MPROP_NORMAL_CONV = "convention_normale";

			// ─────────────────────────────────────────────────────────────────
			// LES PARAMETRES EXPOSES — l'endroit ou ce chantier devient une API
			// publique du moteur.
			// ─────────────────────────────────────────────────────────────────
			// Une propriete `expose.<prise>` porte le NOM PUBLIC du parametre.
			// Presente = expose ; absente = constante.
			//
			// ⚠️ POURQUOI LE NOM EST LA DONNEE, ET PAS UN BOOLEEN A COTE : c'est
			// ce nom que le code du jeu emploiera — `SetFloat("usure", 0.7f)`. Un
			// booleen obligerait a inventer le nom ailleurs, donc a le maintenir
			// a deux endroits. Et quinze proprietes `expose.*` sont verbeuses
			// mais SE REGROUPENT le jour ou l'on voudra un objet dedie ; quinze
			// booleens ne se regroupent pas.
			//
			// ⚠️ ET « EXPOSE » N'EST PAS LE DEFAUT. Une constante se REPLIE dans
			// le code emis — `x * 0` disparait, les branches mortes s'effacent —
			// tandis qu'une variable exposee vit dans un bloc uniforme et
			// qu'AUCUNE optimisation n'est plus possible sur elle. Tout exposer
			// donnerait un materiau pilotable et lent. C'est donc un choix par
			// parametre, pose par l'AUTEUR sur SON noeud : `roughness` est
			// exposable dans un materiau et pas dans un autre.
			static const char *const NK_MPROP_EXPOSE_PREFIX = "expose.";

			// La charge utile de cette propriete : le nom public dans `text`, et
			// DEUX reels optionnels dans `numbers` — les bornes. Elles ne sont pas
			// de la coquetterie : elles disent a un editeur quel curseur afficher,
			// et elles empechent un script d'ecrire une valeur absurde.
			static const uint32 NK_EXPOSE_BORNES_REELS = 2;

			// ⚠️ LE DEFAUT D'UN PARAMETRE EXPOSE **EST** LE `defaultValue` DE SA
			// PRISE. Jamais une seconde valeur rangee a cote : deux sources pour
			// une meme chose divergent, et c'est alors l'editeur qui montre l'une
			// pendant que le moteur envoie l'autre.

			// Un nom public doit etre un IDENTIFIANT : le shader en fait un membre
			// de bloc uniforme, et le code du jeu une cle. Un nom a espaces ou
			// commencant par un chiffre produirait un shader invalide, et l'erreur
			// accuserait le generateur au lieu du nom.
			inline bool NkMatNomPublicValide(const char *n) {
				if (!n || !*n)
					return false;
				const char *p = n;
				if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_'))
					return false;
				for (; *p; ++p) {
					const bool ok = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
									(*p >= '0' && *p <= '9') || *p == '_';
					if (!ok)
						return false;
				}
				return true;
			}

			// ── DISPOSITION std140 ───────────────────────────────────────────
			// ⚠️ LA DISPOSITION PORTE DES DECALAGES, JAMAIS UN ORDRE. En `std140`
			// un `vec3` s'aligne sur 16 octets et en occupe 12 : trois reels a la
			// suite n'occupent PAS trois emplacements contigus. Un moteur qui
			// deduirait les positions de l'ordre de declaration ecrirait a cote
			// des le premier `vec3` — sans erreur, avec une valeur credible.
			inline uint32 NkMatStd140Align(NkTypeId t, const NkMatTypes &types) {
				if (t == types.real)
					return 4u;
				return 16u; // vecteur et couleur : alignes sur 16 en std140
			}

			inline uint32 NkMatStd140Size(NkTypeId t, const NkMatTypes &types) {
				if (t == types.real)
					return 4u;
				return 12u; // vec3 : occupe 12, mais le SUIVANT s'aligne sur 16
			}

			inline uint32 NkMatAlignUp(uint32 v, uint32 a) {
				return (v + a - 1u) / a * a;
			}

			// Les noeuds de relief. `Normal Map` decode une carte tangente ;
			// `Bump` derive une normale d'un champ de hauteur. Et `Separate XYZ`
			// vient avec eux parce que sans lui aucun scalaire VARIABLE n'est
			// disponible pour alimenter une hauteur — un banc ne pourrait alors
			// mesurer que l'absence d'effet.
			static const char *const NK_MN_NORMAL_MAP = "mat.carte_normales";
			static const char *const NK_MN_BUMP = "mat.relief";
			static const char *const NK_MN_SEPARATE_XYZ = "mat.separer_xyz";

			// ── RANG 2 : LE PROCEDURAL ───────────────────────────────────────
			// Des motifs calcules, sans aucune texture — donc sans consommer un
			// seul slot du plafond. C'est ce qui les rend precieux : un materiau
			// entierement procedural ne coute aucune ressource.
			static const char *const NK_MN_NOISE = "mat.bruit";
			static const char *const NK_MN_GRADIENT = "mat.degrade";
			static const char *const NK_MN_CHECKER = "mat.damier";
			// Le type de degrade, meme discipline que les operations : un MOT.
			static const char *const NK_MPROP_TYPE = "type";
			static const char *const NK_MPROP_STOPS = "arrets";
			static const char *const NK_MPROP_INTERP = "interpolation";
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

				// Image Texture : DEUX sorties. C'est le premier noeud du depot
				// dans ce cas, et c'est ce qui a impose de nommer les locales
				// engendrees d'apres le NOM DE LA PRISE et non par un « val »
				// unique — un noeud a deux sorties n'a pas « une » valeur.
				static const NkMatSocketDecl kImageTexture[] = {
					{"vector", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"color", NK_MT_COLOR, NkSocketDir::Output, false},
					{"alpha", NK_MT_REAL, NkSocketDir::Output, false},
				};

				// Texture Coordinate : les coordonnees disponibles sans calcul.
				// On n'expose que `uv` pour l'instant — `generated`, `object` et
				// `camera` demandent des varyings que le vertex engendre ne
				// fournit pas encore, et une prise qui rendrait zero serait
				// exactement le repli plausible qu'on refuse partout ailleurs.
				static const NkMatSocketDecl kTexCoord[] = {
					{"uv", NK_MT_VECTOR, NkSocketDir::Output, false},
				};

				// Mapping : deplace, tourne, met a l'echelle une coordonnee.
				// La rotation est volontairement absente de cette premiere
				// tranche : elle demande une convention d'axes qu'il vaut mieux
				// poser avec `Normal Map` et `Bump`, qui en dependent aussi.
				static const NkMatSocketDecl kMapping[] = {
					{"vector", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"location", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"scale", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"vector_out", NK_MT_VECTOR, NkSocketDir::Output, false},
				};

				// Normal Map : decode une carte tangente. `strength` melange entre
				// la normale geometrique et la normale decodee, comme Blender.
				static const NkMatSocketDecl kNormalMap[] = {
					{"color", NK_MT_COLOR, NkSocketDir::Input, false},
					{"strength", NK_MT_REAL, NkSocketDir::Input, false},
					{"normal", NK_MT_VECTOR, NkSocketDir::Output, false},
				};

				// Bump : derive une normale d'un CHAMP DE HAUTEUR par derivees
				// d'ecran. `distance` est l'amplitude du relief en unites monde.
				static const NkMatSocketDecl kBump[] = {
					{"height", NK_MT_REAL, NkSocketDir::Input, false},
					{"strength", NK_MT_REAL, NkSocketDir::Input, false},
					{"distance", NK_MT_REAL, NkSocketDir::Input, false},
					{"normal", NK_MT_VECTOR, NkSocketDir::Output, false},
				};

				// Separate XYZ : trois sorties reelles. Sans lui, aucun scalaire
				// VARIABLE n'existe dans un graphe, et `Bump` ne pourrait etre
				// mesure que sur son absence d'effet.
				static const NkMatSocketDecl kSeparateXYZ[] = {
					{"vector", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"x", NK_MT_REAL, NkSocketDir::Output, false},
					{"y", NK_MT_REAL, NkSocketDir::Output, false},
					{"z", NK_MT_REAL, NkSocketDir::Output, false},
				};

				// Noise : `detail` pilote le nombre d'octaves. DEUX sorties, comme
				// chez Blender — la valeur scalaire et sa version en gris.
				static const NkMatSocketDecl kNoise[] = {
					{"vector", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"scale", NK_MT_REAL, NkSocketDir::Input, false},
					{"detail", NK_MT_REAL, NkSocketDir::Input, false},
					{"fac", NK_MT_REAL, NkSocketDir::Output, false},
					{"color", NK_MT_COLOR, NkSocketDir::Output, false},
				};

				static const NkMatSocketDecl kGradient[] = {
					{"vector", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"fac", NK_MT_REAL, NkSocketDir::Output, false},
					{"color", NK_MT_COLOR, NkSocketDir::Output, false},
				};

				static const NkMatSocketDecl kChecker[] = {
					{"vector", NK_MT_VECTOR, NkSocketDir::Input, false},
					{"color1", NK_MT_COLOR, NkSocketDir::Input, false},
					{"color2", NK_MT_COLOR, NkSocketDir::Input, false},
					{"scale", NK_MT_REAL, NkSocketDir::Input, false},
					{"color", NK_MT_COLOR, NkSocketDir::Output, false},
					{"fac", NK_MT_REAL, NkSocketDir::Output, false},
				};

				static const NkMatSocketDecl kColorRamp[] = {
					{"fac", NK_MT_REAL, NkSocketDir::Input, false},
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
					{NK_MN_COLOR_RAMP, "ColorRamp", kColorRamp, 2},
					{NK_MN_IMAGE_TEXTURE, "Image Texture", kImageTexture, 3},
					{NK_MN_TEX_COORD, "Texture Coordinate", kTexCoord, 1},
					{NK_MN_MAPPING, "Mapping", kMapping, 4},
					{NK_MN_NORMAL_MAP, "Normal Map", kNormalMap, 3},
					{NK_MN_BUMP, "Bump", kBump, 4},
					{NK_MN_SEPARATE_XYZ, "Separate XYZ", kSeparateXYZ, 4},
					{NK_MN_NOISE, "Noise Texture", kNoise, 5},
					{NK_MN_GRADIENT, "Gradient Texture", kGradient, 3},
					{NK_MN_CHECKER, "Checker Texture", kChecker, 6},
				};
				static const uint32 kProtoCount = 19;

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

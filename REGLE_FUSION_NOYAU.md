<!-- AUTEUR : TEUGUIA TADJUIDJE Rodolf Séderis — Rihen -->

# Le noyau de onze fichiers, et comment le trancher

**Mesure du 2026-09-12.** Seize branches vivantes ont ete confrontees a `main` par
`git merge-tree` (calcul sans toucher a l'arbre). Elles ne conflictent pas chacune
sur des fichiers differents : **elles conflictent toutes sur LE MEME petit noyau.**

    11/16  .gitignore
     9/16  Kernel/Runtime/NKRenderer/src/NKRenderer/Mesh/NkGLTFLoader.h
     9/16  Engine/NKEditorKit/src/NKEditorKit/NkEditorShell.cpp
     8/16  config/modules.jenga
     8/16  Kernel/Runtime/NKWindow/tests/benchmark_smoke.cpp
     8/16  Kernel/Runtime/NKRenderer/src/NKRenderer/Core/NkRendererImpl.cpp
     8/16  Kernel/Runtime/NKImage/NKImage.jenga
     8/16  Engine/NKEditorKit/src/NKEditorKit/NkEditorContextMenu.h
     8/16  Applications/NKUIDesign/src/NKUIDesign/main.cpp
     7/16  Kernel/Foundation/NKMath/tests/test_smoke.cpp
     7/16  Applications/NkAssetIODemo/src/main.cpp

**Il n'y a donc pas seize negociations a mener : il y en a ONZE, une fois.** Ce
fichier est le resultat de cette fois-la. Sans lui, les quinze fusions suivantes
redecouvriraient chaque arbitrage a l'aveugle -- et l'une d'elles le trancherait
autrement.

> ⚠️ **Une fusion sans conflit n'est pas une fusion sure.** Le defaut le plus grave
> trouve ce jour-la n'etait dans AUCUN conflit : il est entre par une zone que git
> a fusionnee silencieusement. Voir « Le piege du 11/09 » plus bas.

---

## Les six que l'on tranche seul, et comment

### 1. `NkGLTFLoader.h` — la regle etait DEJA ECRITE dans le depot

**Ne pas arbitrer : appliquer.** Le commit `f02539193` (2026-08-22) porte un
paragraphe « LIMITE CONNUE, ECRITE FAUTE DE PARADE » qui dit exactement ceci :

> *une branche creee a partir d'une base ANTERIEURE au correctif du 21/08
> reintroduira le doublon. Le reservoir n'est pas dans les branches vivantes, il
> est dans l'HISTORIQUE — et l'historique ne se corrige pas.*

Neuf branches sur seize sont dans ce cas. **Quelqu'un a ecrit l'avertissement pour
celui qui viendrait apres, et il avait raison.**

**RESOLUTION** : garder le cote `main` en entier. Mesure du 12/09 : le cote branche
est **vide** a cet endroit (il n'ajoute rien), donc ce n'est meme pas un arbitrage.

**TEMOIN OBLIGATOIRE APRES RESOLUTION** — le doublon se COMPTE, il ne se lit pas :

    grep -nE "^\s*struct \w+|^\s+NkString name;" <le fichier>

Il doit rester **un seul `NkString name;` par struct**. Etat sain mesure le 12/09 :
`NkGLTFMaterial` (l.40) -> l.56, `NkGLTFAnimation` (l.90) -> l.91, `NkGLTFNode`
(l.98) -> l.164. **Trois structs, trois membres, aucun doublon.**

### 2. `.gitignore` — union des deux cotes, jamais un cote entier

Les deux cotes ajoutent des motifs **differents** et complementaires.

**RESOLUTION** : union. Mesure du 12/09 : 10 motifs cote `main` (sorties Android
`Applications/*/libs/`, sorties des bancs media) + 6 cote branche (sorties de sonde
`nkuidesign_*.txt`), **zero doublon entre les deux**. Aucun cote n'est pris en
entier.

### 3. `benchmark_smoke.cpp` et 4. `test_smoke.cpp` — l'arbre tranche, pas moi

Ces deux-la **ressemblent** a un arbitrage : les deux cotes reparent la meme ligne
differemment. **La mesure montre qu'ils ont RAISON tOUS LES DEUX**, et elle se
refait en une commande :

| fichier | cote `main` | cote branche | mesure dans l'arbre fusionne |
|---|---|---|---|
| `benchmark_smoke.cpp` | `NK_GFX_API_MAX` | `NK_GFX_API_NVN + 1` | **les deux existent** (`NkCGXDetect.h:260-261`) ; `*_RENDERER_API_MAX` n'existe **nulle part** (0 en-tete) |
| `test_smoke.cpp` | `NkRect2i` | `NkRectI` | **deux alias du MEME type** `NkRectT<int32>` (`NkRectangle.h:298` et `:304`) |

**RESOLUTION** : garder la forme de `main` (la ligne publiee, reparee le 05/09) **et
ecrire dans le fichier ce que la branche a appris**. Le savoir de la branche ne doit
pas mourir avec son cote du conflit.

### 5. `NkAssetIODemo/src/main.cpp` — additif, et les APPELS commandent

Deux zones, et les deux cotes sont **compatibles**.

⚠️ **La signature de la branche est OBLIGATOIRE, et ce n'est pas un choix** : les
appels fusionnes passent **deja trois arguments** (`TestFBX("...", 0, 0)` l.284 et
`("...", 3, 3)` l.289). Garder la signature de `main` ne compilerait pas.

**RESOLUTION** :
- zone 1 : signature a trois arguments (branche) **+** `CheminRessource(path)` (main) ;
- zone 2 : **les deux controles**, ils verifient des choses differentes --
  `== baseMats` (main : l'adaptateur n'invente rien) **et** `== expectedMaterials`
  plus le compte de textures **REELLEMENT DECODEES** (branche).

### 6. `config/modules.jenga` — le REGISTRE tranche, et le BANC juge

**RESOLUTION mesuree** : hors zone de conflit, le registre fusionne declare la cle
`"NKAnima"` **une fois** et ne declare **ni** `"NKAnimation"` **ni**
`"NKAnimPhysics"` (fusion des deux modules le 04/09). Garder le cote `main` ferait
pointer `Noge` vers **deux modules inexistants**.
-> **ligne de dependances : cote branche** (`NKAnima`) ; **bloc de documentation :
cote `main`** (la decision « NKEditorKit agnostique du backend » du 01/09).

**APRES CHAQUE resolution de ce fichier, LANCER LE BANC** :

    python Tools/verif_registre_modules.py

Les contrôles 1 (les alias resolvent) et 2 (les dependances designent un module
existant) **valident ou refutent la resolution en une seconde**.

⚠️ **Quand la fusion aboutira, `ATTENDU_REGISTRY` passera de 56 a 55** -- c'est
legitime (deux modules fondus en un), et le banc exige que ce soit **declare**, pas
subi. `ATTENDU_ALIASES` doit **rester 36**.

---

## ⚠️ LE PIEGE DU 11/09 — il n'etait dans AUCUN conflit

**C'est le paragraphe le plus important de ce fichier.**

En fusionnant `feat/noge-feu`, le banc a signale `_ALIASES : 31 entrees, 36
attendues`. Les cinq manquants : **`animation`, `audio`, `navigation`, `ui`, `xr`**.
La cause, presente telle quelle dans le fichier fusionne :

    "physics": "NKPhysics", "animphysics": "NKAnima",  # rentre dans NKAnima le 2026-09-04 "animation": "NKAnima", ... "xr": "NKXR",

**Un `#` de fin de ligne avale les cinq alias qui le suivent.** Ils sont
PHYSIQUEMENT PRESENTS dans le texte -- on les relit, on les croit vivants, et le
dictionnaire est ampute de 12 %.

**Et la bombe etait AMORCEE**, pas dormante : `Applications/Pong2/Pong.jenga` et
`Applications/Pong copy/Pong.jenga` appellent `useappdeps(..., "ui", "audio")`, donc
`_require_key`, qui **LEVE** sur un nom inconnu. Mesure du 12/09 avant correction :
les cinq noms levaient. Le chargement du workspace cassait -- **et Jenga aurait rendu
0 quand meme.**

    AUCUN essai, AUCUNE construction, AUCUN conflit ne l'a signale.
    Les alias survivants resolvaient tous. Toutes les valeurs restantes etaient justes.
    SEUL LE COMPTE l'a vu.

**REGLE, et elle vaut pour toute structure de donnees du depot :**

> **Un commentaire s'ecrit sur SA PROPRE LIGNE.** En fin de ligne, il n'annule pas
> une valeur -- il en avale un NOMBRE. Un controle sur les valeurs reste vert.

**REGLE DE FUSION** : apres toute fusion touchant `config/modules.jenga`, le banc
est OBLIGATOIRE. C'est le seul temoin qui voie cette faute-la.

---

## Les cinq que l'on NE tranche PAS — elles appartiennent a Rodolf

**Critere** : quand la resolution demande de **choisir entre deux intentions** (et
non de reunir deux ajouts), on s'arrete. *Un arbitrage rendu par defaut est un
arbitrage perdu.*

| fichier | cote `main` | cote branche | la question posee |
|---|---|---|---|
| `NkEditorShell.cpp` | *« PAS D'INCLUDE DE NkEditorCanvasRenderer.h ICI, ET C'EST LE POINT »* (01/09) : le kit est agnostique du backend, l'application injecte | reintroduit `NkEditorCanvasRenderer.h` + ajoute `NkThemeToGui.h` | le kit doit-il redevenir lie a NKCanvas ? |
| `NKUIDesign/src/main.cpp` | meme decision d'architecture, cote application | ajoute `NkEditorModal.h` et `NkThemeToGui.h` | idem, cote consommateur |
| `NkEditorContextMenu.h` | documente `shortcuts` (colonne de raccourcis) | documente `shortcuts` **+** `sepAfter` **+** `rangee`, declares « ADDITIFS ET EN DERNIER » | 9 zones : deux redactions d'une meme API elargie |
| `NkRendererImpl.cpp` | **garde G1** : `EndFrame()` appele avant `Present()` -> erreur journalisee (attrape l'inversion de frame) | chrono GPU : `EndTimestampQuery`, `GetTimestampResults`, `gpuTimeMs` | **la garde et le chrono doivent coexister -- dans quel ordre ?** (seule vraie question, voir ci-dessous) |

> ⚠️ **`NkRendererImpl.cpp` : une PARTIE de ce fichier n'est PAS un arbitrage, la
> compilation la tranche.** Mesure du 12/09 : garder le cote `main` **ne compile
> pas**.
>
>     NkRendererImpl.cpp:438:63: error: too few arguments to function call, expected 4, have 3
>
>     main            mVFX->Init(mDevice, mTextures.Get(), mMeshSystem.Get())
>     feat/noge-feu   mVFX->Init(mDevice, mTextures.Get(), mMeshSystem.Get(), mShaders.Get())
>
> Le `NkVFXSystem` a **quatre** parametres dans l'arbre fusionne -- il y est arrive
> par une zone que git a fusionnee **sans conflit**. **L'appel VFX doit donc venir de
> la branche**, ce n'est pas une question de gout. Il ne reste a arbitrer, dans ce
> fichier, que la coexistence de la garde G1 et du chrono GPU.
>
> **Et c'est la demonstration la plus forte de la regle de ④** : « poser le cote
> `main` par defaut » n'est pas seulement un arbitrage perdu -- ici, **ca ne
> construit meme pas**.
| `NKImage.jenga` | `excludefiles(["tests/TestEXR.cpp"])` (05/09) : l'outil a son propre `main()` | `testownmain()` (04/09) : « la suite fournit son main() » | **deux reparations ALTERNATIVES du meme defaut** -- appliquer les deux retire le fichier *et* supprime le `main()` genere : plus aucun `main()`, lien casse |

⚠️ **`NKImage.jenga` etait classe « mecanique » ; la mesure dit le contraire.** Les
deux cotes ne s'additionnent pas, ils se remplacent. C'est un arbitrage.

---

## ⚠️ `git rerere` : essaye, MESURE, et RETIRE. Voici pourquoi.

**N'active pas `rerere` pour rejouer ces resolutions.** Ca a ete tente le 12/09, et
mesure. Deux raisons, et la seconde est grave.

**1. Dans un arbre de travail LIE, la configuration n'est pas locale.** `git config
rerere.enabled true` lance depuis `Nkentseu-merge` s'ecrit dans la configuration
**COMMUNE** -- celle des **29 arbres**. On croit se regler soi-meme, on regle tout
le monde. (De meme, `.git` y est un FICHIER : `rr-cache` vit dans
`git rev-parse --git-common-dir`, donc il est **partage**, pas local.)

**2. `git rerere` enregistre TOUT ce qui est resolu, sans distinguer ce qu'on a
VRAIMENT tranche de ce qu'on a pose la pour compiler.** Le 12/09, les cinq fichiers
de la famille ④ avaient ete places sur le cote `main` **pour mesurer une
construction**, pas pour etre resolus. Un `git rerere` les a enregistres **comme des
resolutions** :

    Recorded resolution for 'Engine/NKEditorKit/src/NKEditorKit/NkEditorShell.cpp'
    Recorded resolution for 'Applications/NKUIDesign/src/NKUIDesign/main.cpp'

**Consequence si on ne le voit pas** : toute fusion ulterieure, dans **n'importe
lequel des 29 arbres**, aurait tranche ces cinq fichiers en faveur de `main`
**automatiquement et en silence** -- l'arbitrage par defaut, exactement celui que ce
document interdit, et propage a tout le depot. `git rerere forget <chemin>` a
REFUSE de les retirer (« no remembered resolution »), le cache ayant sa propre
indexation par empreinte de conflit.

**Ce qui a ete fait** : `rr-cache` purge en entier et les deux cles de configuration
retirees -- le depot est revenu a son etat d'avant (rerere y etait **desactive**).

> **Une memoire automatique qui ne sait pas distinguer une decision d'un
> echafaudage est pire que pas de memoire du tout.**

**La memoire, c'est CE FICHIER.** Il ne rejoue rien tout seul, et c'est sa qualite :
il oblige a relire ce qui a ete decide avant de le reappliquer.

## L'ordre de verification, apres chaque fusion

    1. git diff --name-only --diff-filter=U      les conflits restants, nommes
    2. python Tools/verif_registre_modules.py    OBLIGATOIRE si modules.jenga a bouge
    3. ./verif_chemins.sh                        la garde des chemins partages
    4. jenga rebuild --config Release            COMPLET, jamais incremental
       puis LIRE LE JOURNAL -- le code de sortie de Jenga rend 0 meme quand rien
       ne charge. Une passe incrementale derriere un etat douteux juge « a jour »
       des objets douteux : elle enterine, elle ne verifie pas.

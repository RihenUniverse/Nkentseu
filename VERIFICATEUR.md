# Le vérificateur de bancs

> *« Un banc s'ignore en ne le lançant pas — et on ne le lance pas le jour où on
> est pressé, c'est-à-dire exactement le jour où on casse quelque chose. »*

Une commande unique qui **construit** tous les bancs déclarés, les **exécute**,
rend un **verdict lisible**, et sort **non nul** si quoi que ce soit échoue.

```bash
./verif_bancs.sh                  # mode rapide (défaut), config Debug
./verif_bancs.sh --mode complet   # tous les bancs, y compris ceux qui veulent un GPU
./verif_bancs.sh --liste          # inventaire + garde seuls : ne construit rien
./verif_bancs.sh --banc NkSLCheck # un seul banc
./contre_epreuve_verif.sh         # prouve que le vérificateur sait dire NON
```

| fichier | rôle |
|---|---|
| `verif_bancs.sh` | l'outil |
| `config/bancs.list` | **la donnée** : qui est un banc, avec quels arguments, et comment on juge qu'il va bien |
| `contre_epreuve_verif.sh` | casse un banc volontairement et exige que l'outil le dise |
| `Build/Verif/*.log` | un journal de construction et un journal d'exécution par banc (gitignoré) |

---

## Ce que le vérificateur garantit, et ce qu'il ne garantit pas

**Il garantit** que chaque banc de `config/bancs.list` en classe `banc` :
1. **se construit** — et qu'un échec de construction est un **ÉCHEC**, jamais un
   succès silencieux ;
2. **s'exécute** — et que son verdict est celui déclaré pour lui, pas une
   supposition ;
3. **dit pourquoi** quand ça rate : nom du banc, construit ou non, code de
   sortie, et la première ligne significative de la sortie.

**Il ne garantit pas** que la liste des bancs est complète au sens du *sens* :
il garantit qu'aucun projet d'`Applications/` n'est **non classé**. Répondre
« ce n'est pas un banc » reste une réponse humaine, et 111 projets portent
aujourd'hui la note `non-examine` (voir la dette, plus bas).

---

## ⚠️ La limite la plus dangereuse : il ne mesure QU'UNE référence

**Un vérificateur qui ne mesure que `main` rapporte comme défauts des
corrections en attente de fusion.**

Ce n'est pas une inquiétude théorique, c'est ce qui s'est passé au premier
passage réel, le 22/08 :

| ce que le vérificateur a rapporté | ce que c'était vraiment |
|---|---|
| `NkEditableMeshDemo` : 4 `[FAIL]` de normales | des **attentes périmées du banc**, corrigées le 21 sur la branche de NK3DModeler et **jamais fusionnées**. Les intitulés rapportés sont mot pour mot ceux de `main` ; sur sa branche le même banc rend **37 OK / 0 FAIL**. |
| `gSkipCount` absent | **0 occurrence sur `main`, 5 sur sa branche.** |

⚠️ **L'outil n'a pas menti** : il a mesuré `main`, et `main` était en retard.
C'est la définition même de ce qu'il fait. Mais le coût de cette vérité
partielle n'est pas du bruit — **c'est qu'un agent rediagnostique un bug déjà
résolu.** Ça a failli arriver.

Et la mesure suivante a montré que c'est pire d'un cran : le soir du 22/08,
**la branche `feat/verificateur` elle-même était 4 commits DERRIÈRE `main`**
(`git rev-list --count HEAD..main` = 4). La première passe ne mesurait donc même
pas `main`, mais une photo de `main` prise le matin. *(Vérifié plutôt que
supposé : les 4 commits ne touchaient que des documents de design NKGraph —
aucune conséquence sur les bancs. Le principe, lui, tient.)*

**Ce que ça impose au vérificateur — et ce qui n'est pas encore fait :**

1. **Nommer sa référence.** Le rapport porte déjà `branche` et `HEAD`. Ce n'est
   pas assez : il doit dire **de combien de commits il est derrière `main`**, et
   le dire **en haut**, pas en note.
2. **Nommer les branches en avance sur les fichiers qu'il accuse.** Pour chaque
   banc en ÉCHEC, la question à poser AVANT de diagnostiquer est mécanique :
   *quelle branche non fusionnée touche les sources de ce banc ?*
   `git log --all --oneline main..<branche> -- <chemins du banc>` répond en une
   seconde.
3. **Écrire la phrase sur la ligne d'échec elle-même**, pas dans un document que
   personne ne relit : *« mesuré sur <réf> ; N branche(s) non fusionnée(s)
   touchent ces fichiers — vérifie avant de diagnostiquer »*.

⚠️ **Règle de lecture, en attendant que ce soit outillé** : un ÉCHEC de ce
vérificateur est **une mesure de la référence courante**, jamais un verdict sur
le code du dépôt. Avant de diagnostiquer quoi que ce soit, on demande à l'agent
qui tient le module s'il a la correction en attente.

---

## Les six règles, et le défaut mesuré qui a fait écrire chacune

### 1. Le code de sortie de la CONSTRUCTION est une donnée du contrôle

Un agent a compté une mutation comme « survivante » alors que **la compilation
avait échoué** et que **l'ancien binaire** avait tourné.

Ici : `jenga build --target <banc>` rend non zéro ⇒ verdict **ÉCHEC**, et
**l'exécutable n'est même pas cherché**. L'épreuve A de la contre-épreuve le
vérifie par une empreinte du journal d'exécution prise avant l'épreuve : si elle
bouge, c'est que l'ancien binaire a tourné, et la contre-épreuve échoue.

### 2. La liste des bancs est GARDÉE, pas seulement écrite

⚠️ **Le marquage « ceci est un banc » est une donnée, jamais une heuristique de
nom.** « Les projets finissant par `Test` » rate le premier banc nommé autrement,
en silence — c'est la cinquième fois que ce dépôt paie « chercher un nom n'est
pas chercher un usage ».

La garde est **totale et inversée** : on ne déclare pas les bancs, **on déclare
tout**.

- **Ensemble mesuré** : les `with project("...")` déclarés par les `.jenga`
  d'`Applications/` — la donnée du système de build.
- **Chaque projet doit avoir une ligne** dans `config/bancs.list`. Sans ligne :
  **ÉCHEC** (code 3). Une ligne sans projet dans cet arbre : **information**
  (branche en avance), jamais un échec.
- Le contrôle tourne aussi **automatiquement** via `gitcommit.sh`, pour tout
  commit touchant un `.jenga` ou `config/bancs.list` — en mode `--liste`
  seulement (~4 s, ne construit rien).

⚠️ **`Kind` n'est pas un marqueur de banc** : `jenga info` donne `NKUIDesign` en
`WindowedApp` alors que c'est un banc (son `.jenga` porte `consoleapp()` sous un
filtre Windows). `Kind` sert à énumérer, pas à classer.

### 3. Il a sa contre-épreuve

`./contre_epreuve_verif.sh` — **quatre épreuves**, chacune écrite avec ce qu'elle
réfuterait :

| épreuve | ce qu'on casse | ce qui est exigé |
|---|---|---|
| **A** | erreur de compilation injectée | ÉCHEC, `construit = NON`, code non nul, **et le journal d'exécution inchangé** — preuve que l'ancien binaire n'a pas tourné |
| **B** | `return 3` injecté | ÉCHEC, **code 3 rapporté tel quel**, ligne du banc citée |
| **C** | rien (restauration) | retour au **vert**, code 0, fichier identique à l'original |
| **D** | un projet `NkBancFantome` créé sans ligne de classement | il est **nommé**, la cause est dite, code **3** |

Il commence par un **contrôle négatif** : le banc doit être *déjà* au vert, sans
quoi A et B seraient vraies pour rien.

⚠️ **Le nom de l'épreuve D est choisi exprès.** `NkBancFantome` ne finit pas par
`Test`, ne contient ni `Check` ni `Bench`. **Aucune heuristique de nom ne le
verrait** — l'épreuve ne passe que parce que le marquage est une donnée.

#### Ce que la contre-épreuve a trouvé, dès son premier passage

**Elle a échoué — et c'était elle, pas l'outil.** L'épreuve C restaurait le
fichier par `cp -p`, qui **préserve la date**. La source redevenait juste avec une
date *antérieure* aux objets compilés pendant B ; le cache de Jenga compare les
dates, ne recompilait rien, et **l'ancien binaire — celui qui rend 3 — tournait**.

Le vérificateur ne mentait pas : le binaire rendait bien 3. C'est la
contre-épreuve qui mentait sur ce qu'elle avait restauré. **C'est exactement la
famille du défaut qui a motivé ce chantier**, venue se glisser dans l'outil censé
l'interdire, par une option de `cp`. Corrigé (`cat` + `touch`), et c'est de là
que vient l'option `--reconstruire`.

### 4. Aucun texte à accolades ne passe par un formateur à marqueurs

L'outil est un shell : tout sort par `printf '%s\n'` avec le texte en
**argument**, jamais en chaîne de format. C'est la transposition de la porte
`NkFormat` — et une des raisons de ne pas l'écrire en C++.

### 5. Il dit POURQUOI

Pour chaque échec : le nom, `construit oui/non`, le code de sortie, la durée, et
la **première ligne significative** — ligne du compilateur pour un échec de
construction, ligne de la sortie du banc pour un échec d'exécution — plus le
chemin du journal complet.

### 6. Coût : il doit être lançable

*S'il prend une heure, personne ne le lancera et on aura reconstruit le problème
qu'il devait résoudre.*

Voir la table de mesures plus bas. Deux modes :

| mode | ce qu'il garantit |
|---|---|
| `rapide` (défaut) | tous les bancs **qui ne demandent pas de périphérique GPU**. C'est le mode à lancer avant un commit ou une fusion. |
| `complet` | **tous** les bancs, GPU compris. Ce que `rapide` ne dit pas : rien sur `NkGpuProbe` — donc rien sur le chemin compute NKRHI réel. |

Et **trois niveaux de garantie sur la fraîcheur du binaire** :

| option | ce qu'elle garantit |
|---|---|
| `--sans-construire` | **rien** sur la fraîcheur. Pour rejouer une exécution, jamais pour conclure « ça passe ». |
| *(défaut)* | ce que le système de build affirme. Suffisant au quotidien. |
| `--reconstruire` | que le binaire vient des **sources actuelles**. C'est le mode qui tranche le cas mesuré plus haut, où un cache à base de dates disait « à jour » sur un binaire périmé. |

#### Coût mesuré (2026-08-22, `feat/verificateur`, mode `rapide`, 8 bancs)

| passe | durée |
|---|---|
| arbre **froid**, rien de construit | **15 min 35 s** |
| arbre **chaud**, rien de modifié | **12 min 58 s** |
| `--liste` seul (inventaire + garde) | **1 à 4 s** |

⚠️ **Le chaud ne gagne presque rien, et ce n'est pas normal.** Mesuré :

| séquence | durée | fichiers recompilés |
|---|---|---|
| `build --target NkSLCheck` deux fois de suite (**même** cible) | 4,6 s | **0** |
| puis `build --target NkSLComputeCheck` | 48,3 s | **219** |
| puis `build --target NkSLCheck` à nouveau | 49,5 s | **219** |

Aucune source n'a bougé entre ces trois commandes. **Le cache d'objets est
invalidé par le changement de cible**, pas par un changement de source. Le coût
du vérificateur ne vient donc pas du nombre de bancs mais de ce défaut :
8 bancs × ~50 s de recompilation inutile ≈ 7 min sur les 13. Avec un cache qui
tiendrait, la passe à chaud tomberait autour d'**une minute**.

C'est du ressort de **Jenga** (dépôt séparé), pas de cet outil. Mais c'est
mesuré, chiffré, et reproductible en trois commandes.

⚠️ Le mode par défaut est `rapide` **exprès** : un outil qu'on n'ose pas lancer
ne protège personne. Ce que `rapide` laisse de côté est écrit noir sur blanc
ci-dessus, et le rapport affiche toujours combien de bancs ont été lancés sur
combien de projets classés.

---

## Le format de `config/bancs.list`

```
<projet> | <classe> | <mode> | <arguments> | <verdict> | <note>
```

- **classe** : `banc` · `pas-banc` · `banc-arrete`
- **mode** : `rapide` · `complet`
- **verdict** : `code` · `code+absence:<motif>` · `code+presence:<motif>` ·
  `sans-verdict`

### `sans-verdict` — la valeur ajoutée après mesure, et le banc qu'elle a fait corriger

`NkSLCheck.exe` sortait **code 0** en 0,045 s, avec dans sa propre sortie :

```
----- PBR FS GLSL-OpenGL (success=0, 0 octets) -----
----- PBR FS GLSL-Vulkan (success=0, 0 octets) -----
```

Son `main` finissait par `return 0;` — inconditionnel. **Un banc qui échoue en
rendant « tout va bien » est la forme la plus dangereuse de la grille** : il
n'omet pas l'alarme, il l'éteint. C'était « un repli qui préserve `success` »
rejoué à l'échelle du contrôle.

**Corrigé le 22/08** : le banc compte ses échecs et rend 1 s'il y en a. Il est
passé de `sans-verdict` à `code` dans `config/bancs.list`.

⚠️ **Et la correction a immédiatement retourné l'accusation contre le banc.**
Une fois qu'il a su parler, il a dit *pourquoi* :

```
[FAIL] PBR FS GLSL-OpenGL : la compilation a échoué (success=0)
  ERREUR ligne 136: #include not found: Include/NkShadowAtlas.glsli
  ERREUR ligne 168: #include not found: Include/NkVoxelAO.glsli
```

Ce n'était **pas** un défaut de NkSL. Le banc lisait le `.nksl` et le passait à
`Compile()`, **qui ne connaît aucun dossier de recherche d'`#include`** — seul
`Preprocess(source, baseDir)` en prend un. Le banc mesurait donc **son propre
montage** et l'imputait au compilateur. Corrigé (`Preprocess` avec
`Resources/NKRenderer/Shaders`) : **10 contrôles, 0 en échec, code 0.**

C'est la deuxième fois en deux jours que la faute était dans l'instrument et non
dans le sujet (la première : `cp -p` dans la contre-épreuve). D'où la règle
tenue ici : **avant de rapporter un défaut, on vérifie que la mesure ne mesure
pas le montage.**

Un banc `sans-verdict` reste **lancé, journalisé, et compté INDÉTERMINÉ** —
**jamais vert**. Le rapport les liste à part, avec la phrase qui compte : *ces
bancs tournent, mais leur code de sortie ne juge rien.* Il n'en reste **aucun**
aujourd'hui.

---

## Les fixtures d'un banc doivent être versionnées

⚠️ **Un banc dont l'entrée n'est pas versionnée mesure la machine, pas le code.**
Il est vert sur une seule copie de travail et rouge dans tout arbre neuf, chez
tout agent, et dans toute CI.

Deux bancs étaient dans ce cas au premier passage. Arbitrage rendu le 22/08 :

| banc | fichiers | décision |
|---|---|---|
| `NkAssetIODemo` | `Resources/Models/tree.obj` (280 o), `Resources/Models/rock/rock.obj` (15 Ko) | **versés au dépôt** par `git add -f` (`.gitignore:45` ignore `*.obj`, `:704` ignore `/Resources/Models/`). *C'est la base même du lecteur OBJ : un lecteur dont aucun test ne peut tourner ailleurs que sur une machine n'est pas testé.* Leurs voisins `rock.mtl` / `rock.png` étaient déjà suivis. |
| `NkFBXParityDemo` | `CesiumMan/CesiumMan.fbx`, `XBot/` | **passé en `mode complet`** (gros fichiers, la parité FBX est un sujet à part). ⚠️ **Mesure du 22/08 : `CesiumMan.fbx` est absent du dépôt ET de la copie de travail de Rodolf** — seul le `.glb` existe. Ce banc n'est donc vert **nulle part** aujourd'hui, pas seulement « ailleurs ». `XBot`/`YBot` sont, eux, déjà sautés proprement par le banc. |

⚠️ Une fois `git add -f` passé, le fichier est **suivi** et les règles de
`.gitignore` ne s'y appliquent plus. On n'a donc **pas** touché à `.gitignore` :
`/Resources/Models/` y exclut le dossier entier, et git *ne redescend pas* dans
un dossier exclu — une ligne `!.../tree.obj` y serait **sans effet**, ce qui
donnerait un fichier de règles qui ment sur ce qu'il fait.

**Règle pour un banc neuf** : si son entrée n'est pas dans `git ls-files`, soit
on la verse, soit le banc la fabrique à l'exécution, soit il part en
`mode complet` avec la note qui dit ce qu'il exige. Pas de quatrième choix.

---

## Codes de sortie

| code | sens |
|---|---|
| 0 | tout va bien |
| 1 | au moins un banc en ÉCHEC (construction, exécution, ou verdict) |
| 2 | **échec d'instrument** : précondition manquante, montage cassé. **Rien n'est conclu sur le dépôt** — c'est le vérificateur qui n'est pas en état |
| 3 | **échec de classement** : un projet d'`Applications/` n'est pas classé |

La distinction 1 / 2 est le cœur de l'honnêteté de l'outil : *une mesure ne vaut
que ce que vaut son montage*.

---

## Préconditions vérifiées avant de juger quoi que ce soit

1. **`jenga` ≥ 2.4.0.** Avant, `build --project X` était **ignoré en silence**
   (le drapeau est `--target`) : le vérificateur croirait construire un banc et
   construirait tout le workspace. En dessous, il **refuse** de conclure.
   La copie embarquée sous `Build/_jenga-embed/` peut être plus ancienne
   (2.2.0 ici) ; elle n'est **jamais** utilisée, et l'écart est signalé.
2. **Sous-modules peuplés** — et seulement ceux que le workspace cite.
   ⚠️ Dans un worktree, `git submodule update --init` **ne fait rien et sort 0**.
   Il faut **`git submodule update --init --force`**. Le message d'erreur du
   vérificateur donne la commande.
3. **`config/bancs.list` présent.**
4. **Contrôle positif d'instrument** : l'inventaire doit retrouver deux témoins —
   un banc présent dans le workspace (`NkSLCheck`) et un banc **arrêté** dont
   l'`include` est commenté (`NkMatInventaireTest`). Si le second ressort « dans
   le workspace », c'est que le parseur ignore les commentaires, et **tout le
   reste est faux**. Un inventaire qui ne sait rien trouver ne prouve rien.

---

## Ajouter un banc

1. Écrire le banc comme une application console d'`Applications/`, et
   l'`include` dans `Nkentseu.jenga`.
2. Ajouter sa ligne dans `config/bancs.list` — sinon le prochain commit touchant
   un `.jenga` est **refusé**, avec le nom du projet non classé.
3. Choisir son `verdict`. Si le banc ne sait pas dire s'il va bien, écrire
   `sans-verdict` **plutôt que `code`** : mieux vaut un INDÉTERMINÉ visible
   qu'un vert faux.

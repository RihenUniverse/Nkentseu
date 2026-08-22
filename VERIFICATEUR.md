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
./verif_bancs.sh --capacites      # garde des capacités annoncées, seule (~10 s)
./contre_epreuve_verif.sh         # prouve que le vérificateur sait dire NON
./contre_epreuve_capacites.sh     # prouve que le détecteur de capacités sait dire NON
```

| fichier | rôle |
|---|---|
| `verif_bancs.sh` | l'outil |
| `config/bancs.list` | **la donnée** : qui est un banc, avec quels arguments, et comment on juge qu'il va bien |
| `contre_epreuve_verif.sh` | casse un banc volontairement et exige que l'outil le dise |
| `Build/Verif/*.log` | un journal de construction et un journal d'exécution par banc (gitignoré) |
| `verif_capacites.sh` | la **garde des capacités annoncées** : détecte les capacités creuses et exige qu'elles soient classées |
| `config/capacites.list` | **la donnée** : le classement des capacités détectées |
| `contre_epreuve_capacites.sh` | introduit `NkCapFantome` et exige le rouge |

---

## Ce que le vérificateur garantit, et ce qu'il ne garantit pas

**Il garantit** que chaque banc de `config/bancs.list` en classe `banc` :
1. **se construit** — et qu'un échec de construction est un **ÉCHEC**, jamais un
   succès silencieux ;
2. **s'exécute** — et que son verdict est celui déclaré pour lui, pas une
   supposition ;
3. **dit pourquoi** quand ça rate : nom du banc, construit ou non, code de
   sortie, la première ligne significative de la sortie, **et les branches non
   fusionnées qui touchent les fichiers accusés** — ou le fait qu'il n'y en a
   aucune, qui est l'information la plus utile des deux.

**Il ne garantit pas** que la liste des bancs est complète au sens du *sens* :
il garantit qu'aucun projet d'`Applications/` n'est **non classé**. Répondre
« ce n'est pas un banc » reste une réponse humaine, et 111 projets portent
aujourd'hui la note `non-examine` (voir la dette, plus bas).

**Il garantit aussi** qu'aucune **capacité annoncée et creuse** détectée par
`verif_capacites.sh` n'est **non classée** — mais il ne garantit **jamais**
qu'une capacité est implémentée : ce n'est pas le même énoncé, et le second
n'est pas mesurable par du texte. Voir *La garde des capacités annoncées*.

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

**Ce que ça impose au vérificateur — outillé le 22/08 au soir, les trois
points :**

1. **Il nomme sa référence, et son retard, EN HAUT du rapport.**
   ```
   branche : feat/verificateur   HEAD 0a58a608
   ⚠️ RETARD  : 4 commit(s) derriere main. Ce que tu vas lire mesure CETTE
                photo, pas main. Fusionne avant de conclure quoi que ce soit.
   ```
   Un vérificateur qui ne dit pas de quand date sa référence laisse lire ses
   rouges comme des faits sur le code.

2. **Il nomme les branches non fusionnées qui touchent les fichiers accusés,
   sur la ligne d'échec elle-même.** Les chemins accusés sont le dossier du banc
   plus tout fichier source dont le **nom apparaît dans la ligne de cause** :
   `NkFBXImporter::Import` donne le jeton `NkFBXImporter`, qui donne
   `.../NkFBXImporter.cpp` **si et seulement si git suit réellement un fichier de
   ce nom**. On ne devine pas un chemin ; on le résout.

   ```
   NkAssetIODemo :
     code de sortie 1 : [FAIL] NkFBXImporter::Import: 0 materiau
     journal complet : Build/Verif/NkAssetIODemo.run.log
     ⚠️ 1 branche(s) non fusionnee(s) touchent les fichiers accuses
        ET en different encore aujourd hui :
        refonte-interface-nk3dmodeler   (2 commit(s) inedit(s))
          Applications/NkAssetIODemo/src/main.cpp
   ```

   ⚠️ **Un nom qui désigne cent fichiers ne désigne rien.** Défaut mesuré dans
   cette fonction même, à sa première épreuve de bout en bout : la ligne de cause
   disait `Compilation Error: main.cpp`, le jeton `main` a résolu vers ~100
   fichiers, et l'outil a nommé **six branches** sur des `main.cpp` totalement
   étrangers au banc tombé. **Exactement ce que cette fonction existe pour
   empêcher — un outil qui accuse au lieu de situer — venu se loger dans la
   fonction qui porte l'exigence.** La règle qui tient : *un jeton ne désigne une
   unité de code que si tous ses fichiers vivent dans le même dossier.*
   `NkGLTFLoader` → le `.h` et le `.cpp` d'un même module : une unité. `main` →
   cent dossiers : rien. Après correction, le même cas rend **AUCUNE**, ce qui est
   la vérité.

   **Deux conditions, et pas une** : le fichier a bougé *sur la branche* depuis
   la base commune, **et** il diffère *encore* de ce qu'on mesure aujourd'hui.
   Sans la seconde, l'outil nommerait des branches dont la modification est déjà
   dans `HEAD` — il enverrait chercher une correction qui est sous ses yeux.

   Chaque branche porte son **nombre de commits inédits** (`git cherry`, par
   patch-id), et celles à 0 sont reléguées en fin de liste avec la mention
   « contenu déjà sur HEAD, regarde-la en dernier ».
   ⚠️ **Ce compte se trompe dans un sens et un seul** : une fusion **écrasée**
   (squash) donne un patch-id neuf, donc l'outil **surcompte** les inédits,
   jamais l'inverse. Mesuré : `refonte-interface-nk3dmodeler` est fusionnée dans
   `main` (`99b62b18`) et ressort à « 2 inédits ». C'est un **indice de lecture,
   pas un verdict** — l'outil situe, il ne tranche pas.

3. **Et l'ABSENCE est dite aussi fort que la présence :**
   ```
   branches non fusionnees touchant les fichiers accuses : AUCUNE
     (16 branche(s) examinee(s)) — le defaut est bien sur CETTE reference.
     Ne perds pas une heure a fusionner avant de chercher.
   ```
   C'est l'information **la plus utile des deux**. Un outil qui ne parlerait que
   quand il trouve laisserait le silence vouloir dire deux choses à la fois.

**Ce que ça a donné, mesuré.** Le 22/08 au soir, sur une photo de `main` vieille
de 31 commits : `NkAssetIODemo` **ÉCHEC**, `NkEditableMeshDemo` **ÉCHEC**. Après
fusion de `main` à jour, **la même passe, le même mode, les mêmes bancs** :

| banc | avant fusion | après fusion de `main` |
|---|---|---|
| `NkAssetIODemo` | ÉCHEC (1 FAIL) | **OK** |
| `NkEditableMeshDemo` | ÉCHEC (4 FAIL) | **OK** |
| les 6 autres | OK | OK |

**Les deux rouges n'étaient pas des défauts. C'était le retard de la référence.**
Aucun diagnostic n'avait à être fait ; il fallait fusionner et remesurer. C'est
la leçon de cette section, appliquée à la section elle-même.

⚠️ **Règle de lecture, maintenant que c'est outillé** : un ÉCHEC de ce
vérificateur reste **une mesure de la référence courante**, jamais un verdict
sur le code du dépôt. L'outil nomme désormais les branches à regarder — il ne
dispense pas de les regarder. **Fusionner d'abord, remesurer ensuite** ; ne
diagnostiquer qu'après.

---

## ⚠️ Une règle de normalisation peut défaire une correction sans toucher au code corrigé

**Aucun diff ne la montre. C'est ce qui la rend dangereuse.**

Le 22/08, verser au dépôt les deux fixtures OBJ de `NkAssetIODemo` était la
correction d'un défaut précis : *un banc dont l'entrée n'est pas versionnée
mesure la machine, pas le code*. `git add -f`, deux fichiers, défaut réglé.

Sauf que `.gitattributes` porte `* text=auto` et la config `core.autocrlf=true`.
Les deux `.obj` seraient donc **sortis de caisse en CRLF** chez le prochain qui
clone — alors qu'**aucun des deux lecteurs OBJ** (`NkOBJLoader.cpp`,
`NkOBJIO.cpp`) ne mentionne `\r` nulle part. J'aurais versé une fixture verte
chez moi et douteuse ailleurs : **exactement le défaut que verser les fixtures
devait supprimer, réintroduit par une règle de normalisation.**

### La forme générale, et elle dépasse largement les fixtures

> **Une règle de normalisation agit à la sortie de caisse, pas au commit. Elle
> peut donc défaire une correction sans qu'aucune ligne du code corrigé ne
> change — et donc sans qu'aucun `git diff`, aucune revue, aucun banc lu sur la
> machine de l'auteur ne la voie.**

Ce qui rend cette famille distincte de tout le reste de ce document :

| | un défaut ordinaire | un défaut de normalisation |
|---|---|---|
| se voit dans un diff | oui | **non** |
| se voit chez l'auteur | oui | **non — sa copie de travail est déjà juste** |
| se voit dans une revue | oui | **non — le fichier revu n'a pas changé** |
| se voit à la sortie de caisse suivante | — | **oui, et seulement là** |

Les trois premières colonnes sont **toutes** les façons dont on regarde
habituellement. C'est un défaut qui traverse le filet entier par construction.

### Ce que ça impose

1. **Toute donnée lue octet pour octet par du code doit porter sa règle
   explicite dans `.gitattributes`.** Une entrée de test doit être identique
   partout, sinon le banc mesure la machine.
   ```
   *.obj  text eol=lf
   *.list text eol=lf
   ```
   La seconde ligne est du même jour : `config/bancs.list` et
   `config/capacites.list` sont lus caractère par caractère par des scripts.

2. **On vérifie par la mesure, pas par la lecture du `.gitattributes`** — c'est
   le fichier lui-même qui peut mentir :
   ```
   git ls-files --eol config/capacites.list
   i/lf    w/lf    attr/text eol=lf
   ```
   `i/` c'est l'index, `w/` la copie de travail. Les deux doivent dire `lf`.

3. **Le même raisonnement vaut pour toute règle qui agit entre le dépôt et le
   disque** : filtres `clean`/`smudge`, `ident`, `export-subst`, LFS. Toutes
   transforment le fichier **après** que le diff a été calculé.

⚠️ **Et la raison pour laquelle ce paragraphe est ici et pas dans une note** :
ce défaut a été évité **par hasard**, en vérifiant une chose sans rapport. Rien
dans le processus ne l'aurait attrapé. C'est le seul de tous ceux consignés dans
ce document dont je ne peux pas dire « voici le contrôle qui l'aurait vu ».

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

## La garde des capacités annoncées

`./verif_capacites.sh` + `config/capacites.list` + `contre_epreuve_capacites.sh`.
Lancée automatiquement à la fin de chaque passe complète (`--sans-capacites`
pour s'en passer), ou seule par `./verif_bancs.sh --capacites` (~10 s).

### Le problème

Neuf capacités de NKRHI/NKRenderer étaient **annoncées et creuses** : un champ
qu'on règle et qui ne fait rien, une virtuelle qui rend un chiffre crédible au
lieu d'une erreur, une texture chargée que personne n'échantillonne. Aucune ne
casse une compilation. Aucune ne fait rougir un banc. **Elles ne se voient que
si quelqu'un les cherche — donc jamais.**

### Le dessin, et il est tout le sujet

> **Le détecteur n'a pas le droit de juger. Il n'a le droit que d'exiger un
> classement.**

C'est exactement le dessin de `config/bancs.list`, et pour une raison mesurée :
la première passe sort **126 candidats**. Un outil qui les crie est désactivé
avant vendredi — et on aurait reconstruit le problème qu'il devait résoudre.

- **Jour 1** : on classe les 126, et l'outil est **VERT**.
- **Jour 2+** : il devient **ROUGE** sur le 127e, c'est-à-dire exactement le
  jour où il sert.

Un détecteur qui prétend savoir si une capacité est implémentée a tort une fois
sur trois. Un détecteur qui dit « ce symbole n'est pas classé » a **toujours
raison** : c'est un fait sur le fichier de classement, pas un jugement sur le
code.

### Ce qu'il dit, et ce qu'il ne dira jamais

| | |
|---|---|
| il dit | « ce champ est écrit et jamais lu » — **exact, vérifiable** |
| il ne dit **pas** | « cette capacité n'est pas implémentée » — pas le même énoncé, et le second n'est pas mesurable par du texte |

Deux formes distinctes apparaissent dans la sortie, et les confondre serait
mentir sur ce qu'on sait :

- `ecrit-jamais-lu` — quelqu'un **règle** ce champ, et il ne se passe rien.
- `jamais-mentionne` — personne ne le nomme, ni en écriture ni en lecture.
  Énoncé **plus faible** : ça peut aussi vouloir dire que l'attribution n'a pas
  trouvé la voie d'accès.

### Les deux détecteurs livrés

- **D1** — virtuelle **creuse** (corps vide, ou un unique `return <littéral>;`),
  zéro surcharge et zéro appel dans le dépôt, **appariée** à un drapeau
  `mCaps.<x> = true` dont le nom partage un jeton porteur (≥ 5 caractères) avec
  le sien.
  ⚠️ **C'est l'appariement qui fait le filtre, pas le corps vide.** Sans lui,
  `SetDebugName` — trois surcharges, corps vides, zéro appel — serait rapporté à
  tort : un corps vide qui ne promet rien à personne est un **point d'extension**,
  pas un mensonge. Mesure : **4 candidats, 0 faux positif.**
- **D2** — champ scalaire à valeur par défaut dont **aucune occurrence du dépôt
  n'est une lecture**. Mesure : **122 candidats.**

**D3 n'est pas livré**, et la raison est mesurée : `./verif_capacites.sh
--pourquoi-pas-d3`. En résumé — côté shader tout est exact et qualifié par son
set (`layout(set=0, binding=0)`), mais **côté C++ l'index de set n'existe dans
aucune déclaration** : le lien « `NkFrameBinding` appartient au set 0 » ne vit
que dans un **commentaire** de `NkResources.h` et dans un nom de membre. Un
détecteur devrait donc **deviner** l'appariement — et un détecteur qui devine
est un générateur de faux positifs. Le fait mesuré est déjà un résultat :
*l'index de set des layouts standard n'est porté par aucune déclaration ; un
commentaire ne peut pas se tromper, il peut seulement être faux sans que rien
ne le dise.*

### ⚠️ La limite de D2 que j'ai trouvée en me contrôlant moi-même

En vérifiant à la main un résultat de mon propre outil, je l'ai cru faux : il
annonçait **4 écritures** de `timestampQueries` là où j'en comptais 3. **C'est ma
vérification à la main qui était fausse** — elle omettait `--include='*.hpp'`.

La quatrième écriture vit dans :

```
Kernel/Runtime/NKRHI/src/NKRHI/Opengl/NkOpenglDevice copy.hpp:211
    mCaps.timestampQueries  = true;
```

**Un fichier « copy » versionné, dans l'arbre source de NKRHI.** Mesure :
`git ls-files` en suit **27** de cette forme ; **un seul** porte des écritures
`mCaps.` — celui-là.

**Ce que ça impose de dire sur D2**, et je ne l'exclus pas par un filtre :

- D2 lit **tous** les fichiers C/C++ suivis. Il n'a **aucune heuristique de nom**,
  et « exclure les fichiers dont le nom contient *copy* » en serait une —
  exactement ce que ce dépôt interdit depuis quatre fois.
- **La direction d'erreur compte.** Une occurrence dans une copie morte peut
  **gonfler un compte d'écritures** (visible, sans conséquence sur le verdict), ou
  — beaucoup plus grave — **fournir la seule « lecture » d'un champ et faire
  MANQUER un défaut**. Cette seconde direction est **silencieuse**.
- Le vrai correctif n'est pas dans l'outil : **c'est que ces fichiers n'existent
  pas.** Un fichier qu'aucun build ne compile mais que tout `grep` lit est un
  faux témoin permanent, pour les outils comme pour les humains.

### La ligne de partage de `vision-assumee` (arbitrage de Rodolf)

| | classement permis |
|---|---|
| capacité **documentée**, sans drapeau, sans point d'entrée | `vision-assumee` ✅ |
| capacité dont un **drapeau annonce `true` à du code** | `vision-assumee` ❌ |

Un `true` n'est pas une déclaration d'intention : c'est une réponse à une
question que le code pose à l'exécution, et dont il va se servir pour choisir un
chemin. **Deux sorties seulement** : le drapeau rend `false`, ou la capacité
s'implémente. La classer « vision » et passer n'est pas un troisième choix — ce
serait rendre acceptable, **par un fichier de configuration**, exactement la
classe de mensonge que ce chantier traque. **Le détecteur refuse ce classement.
C'est la seule chose qu'il refuse.**

### `A-DATER` : ne pas inventer une échéance pour faire taire un outil

`dette-datee` exige une date. Un champ **vide** est un oubli → **ÉCHEC**. Le mot
**`A-DATER`** est une réponse *valide et différente* : il dit « cette échéance
appartient à Rodolf, je ne l'invente pas ». Le vérificateur **nomme ces lignes à
chaque passage**, sans rougir — une dette qu'on ne mentionne plus est une dette
oubliée, même règle que les bancs arrêtés.
Inventer une date pour faire taire l'outil serait produire un **chiffre crédible
et faux** : précisément ce qu'on traque.

### Elle ne refuse pas le commit, et c'est délibéré

Un contrôle qui bloque le commit se contourne par `--no-verify`, **en silence**,
par quelqu'un de pressé — et personne ne sait jamais qu'il a été contourné. Un
banc rouge, lui, reste rouge et **se voit**. On échange un blocage qu'on peut
faire taire contre un signal qu'on ne peut pas.

Elle n'est **pas non plus dans `--liste`**, et pour la même raison, mesurée :
elle coûte ~10 s, et `--liste` est ce que `gitcommit.sh` appelle à chaque commit
(1 à 4 s aujourd'hui). Y ajouter 10 s, c'est **fabriquer la raison de contourner
le hook**.

### Sa contre-épreuve : `NkCapFantome`

`./contre_epreuve_capacites.sh` introduit une capacité creuse — un drapeau mis à
`true` par du code, et une virtuelle vide — et **exige le rouge**. Le nom ne
contient ni `TODO`, ni `stub`, ni `unimplemented`, ni `WIP` : **aucune
heuristique de nom ne le verrait**. L'épreuve ne peut passer que si la détection
est **structurelle**.

| épreuve | ce qu'on introduit | exigé |
|---|---|---|
| E | la capacité creuse, non classée | ÉCHEC code 4, **les deux** symboles (D1 et D2) nommés |
| F | classée `vision-assumee` | ÉCHEC code 4, **CLASSEMENT INTERDIT**, les deux sorties rappelées |
| G | classée `dette-datee` + date | **VERT** — on revient au vert en classant, pas en désactivant |
| G bis | `dette-datee` sans date | reste **ROUGE** |
| H | tout est retiré | VERT, et **arbre identique** (`git status` muet) |

**Contrôle négatif** : la contre-épreuve **refuse de démarrer** si l'outil est
déjà rouge, ou si les fichiers qu'elle modifie ne sont pas propres. Introduire
une capacité creuse dans un dépôt déjà rouge rendrait E vraie **pour rien**, et
on lirait des `[vu]` sur un montage faux — c'est arrivé le 22/08 dans la
contre-épreuve des bancs.

**État mesuré : 12 contrôles, 0 raté.**

### Le mot `non-examine`

117 des 126 lignes le portent. Il est là pour qu'on ne prenne pas mon
remplissage pour un jugement : je les ai **classées** pour que la garde soit
totale dès le premier jour, je ne les ai pas **examinées** une par une. Le
vérificateur compte ces lignes à chaque passage. Chaque agent qui passe sur un
de ces symboles corrige sa ligne au passage — plutôt qu'une grande revue en une
fois, qui n'arrive jamais.

### Trois défauts de MON outil, trouvés par la mesure

1. **Les voies d'accès étaient cherchées dans les trois en-têtes examinés.** Or
   `NkDeviceCaps mCaps{}` est déclaré dans les **six en-têtes de backend**,
   jamais dans `NkIDevice.h`. La voie `mCaps` n'existait donc pas,
   `mCaps.maxDescriptorSets = ...` n'était attribué à personne, et le champ
   ressortait « jamais mentionné ». **L'outil transformait « écrit ailleurs » en
   « jamais touché »** — un instrument qui accuse le sujet à la place de son
   propre montage, pour la **troisième fois** de ce chantier.
2. **La reconnaissance d'une voie exigeait un `.` ou un `->` devant le membre.**
   `mCaps.timestampQueries` s'écrit **nu**. Même conséquence.
3. **L'attribution se faisait par fichier seulement.** `NkRendererImpl.cpp`
   nomme `NkIBLConfig` dans un **commentaire**, et sa lecture de
   `mCfg.shadow.enabled` était comptée comme une lecture de
   `NkIBLConfig::enabled` — qui n'en a aucune. ⚠️ **Un homonyme lu FAIT MANQUER
   un défaut** : l'outil se taisait sur un champ creux en croyant le voir lu.
   C'est la direction d'erreur la plus dangereuse des deux, parce qu'elle est
   **silencieuse**. Corrigé : **la voie d'accès prime sur le fichier.**

Les trois ont été attrapés par les **deux témoins d'instrument**
(`NkIDevice::GetTimestampPeriodNs` pour D1, `NkDeviceCaps::timestampQueries`
pour D2). Sans eux, « je n'ai rien trouvé » et « je ne lis plus le fichier »
auraient produit **la même sortie verte**.

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

`verif_capacites.sh`, lancé seul, a ses propres codes :

| code | sens |
|---|---|
| 0 | toute capacité détectée est classée, et aucun classement n'est interdit |
| 2 | **échec d'instrument** : témoin non retrouvé, corpus absent. Rien n'est conclu |
| 4 | **échec de classement de capacité**. **Ne bloque aucun commit** — rougit la passe |

⚠️ Le 4 remonte dans la passe complète sous forme de **1** (« au moins un
échec »), avec sa ligne propre sur le verdict. Il n'atteint jamais
`gitcommit.sh` : cette garde ne vit pas dans `--liste`.

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

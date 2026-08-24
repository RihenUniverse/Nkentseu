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
./preuve_copies_mortes.sh         # les fichiers « copy » versionnés, et la preuve qu'aucun build ne les compile
./epreuve_msaa_contract.sh        # prouve que NkMsaaContractCheck sait tomber (deux défauts réels)
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
| `preuve_copies_mortes.sh` | recalcule la preuve que les 27 fichiers « copy » ne sont compilés par rien |
| `epreuve_msaa_contract.sh` | injecte deux défauts réels dans `NkDeviceCaps` et exige que le banc MSAA les voie |

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
aujourd'hui la note `non-examine` — et 117 lignes de `config/capacites.list`
de même (voir *Dettes ouvertes*, en fin de document).

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

   ⚠️ **Et ça s'est produit, mesuré le 23/08** : après la fusion de `main`,
   `config/bancs.list` rendait `i/lf  w/crlf  attr/text eol=lf`. **L'index et
   l'attribut disaient `lf` ; le fichier sur le disque était en CRLF.** Un outil
   Windows l'avait réécrit après la sortie de caisse.

   Ce qui rend le cas instructif, c'est ce qu'il ne déclenche **pas** :
   `git diff` est **vide**, `git hash-object` rend **exactement** le blob de
   `HEAD` — git n'a jamais rien vu, parce que le filtre `eol` normalise avant de
   comparer. **Aucun diff, aucune revue, aucun commit n'aurait pu l'attraper.**
   Seul `git ls-files --eol` le dit, et c'est précisément pourquoi la
   vérification est cette commande-là et pas une lecture du `.gitattributes`.

   *(Sans conséquence ici — les `awk` des deux scripts font `sub(/\r$/, "", ligne)`.
   Mais la parade vit dans le **parseur**, pas dans le fichier : elle protège ces
   deux scripts-ci, et rien d'autre qui lirait ces `.list` un jour.)*

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

## ⚠️ Un outil qui situe peut accuser plus fort qu'un outil qui se tait

**Parce qu'on le croit.**

La fonction qui nomme les branches non fusionnées existe pour empêcher d'accuser
à tort. À sa **première épreuve de bout en bout**, elle a accusé six branches
étrangères.

Le banc `NKSmoothMeshTest` cassé volontairement, la ligne de cause était
`Compilation Error: main.cpp`. Le jeton `main` a résolu vers ~100 fichiers, et
l'outil a nommé `feat/materiaux-graphe` (pour `NkMatGraphDemo/src/main.cpp`),
`feat/noge-inventaire`, `sauvegarde-avant-nettoyage-carnets`… aucune n'avait le
moindre rapport avec le banc tombé.

> **Un nom qui désigne cent fichiers ne désigne rien. Un jeton ne désigne une
> unité de code que si tous ses fichiers vivent dans le même dossier.**

`NkGLTFLoader` → le `.h` et le `.cpp` d'un même module : une unité.
`main` → cent dossiers : rien. Après correction, le même cas rend **AUCUNE** —
la vérité.

### Ce que ça a coûté à trouver, et c'est le point

**La première épreuve de bout en bout. Pas une relecture.**

La fonction avait été validée en isolation, sur trois causes fabriquées à la
main (`NkFBXImporter::Import`, `NkGLTFLoader`, une cause vide). Les trois
passaient. Aucune ne ressemblait à ce que produit réellement un compilateur qui
tombe — et c'est *exactement* le cas où l'outil sert.

**Une validation sur des entrées qu'on a choisies soi-même valide surtout son
auteur.** Ce n'est pas une raison de ne pas la faire ; c'est une raison de ne
jamais s'y arrêter.

---

## ⚠️ Le cliquet de `A-DATER` — une dette qui ne se renouvelle pas

`dette-datee` exige une échéance. Deux cas se ressemblent et ne veulent pas dire
la même chose :

| champ date | sens | verdict |
|---|---|---|
| **vide** | un oubli | **ÉCHEC** |
| **`A-DATER`** | « cette échéance appartient à Rodolf, je ne l'invente pas » | nommé à chaque passage, **pas rouge** |

Inventer une date pour faire taire un outil, ce serait produire un **chiffre
crédible et faux** — précisément ce que ce chantier traque.

Mais `A-DATER` sans contrainte redeviendrait une porte ouverte : le stock se
renouvellerait exactement pendant qu'on le vide. D'où le **cliquet** :

> **Le compte de `A-DATER` peut descendre. Il ne peut pas monter.**

- Le stock actuel (24) est un **héritage**.
- Toute capacité **nouvelle** est datée **à la naissance**, sans exception.
- Le plafond est une **donnée** déclarée dans `config/capacites.list` :
  `# PLAFOND-A-DATER = 24`. Le relever est possible — et c'est un geste
  **délibéré qui apparaît dans un diff**. Le cliquet ne s'oppose pas à la
  décision, il s'oppose à la **dérive silencieuse**.
- **Son absence est un ÉCHEC D'INSTRUMENT**, pas un défaut : un cliquet absent
  et un cliquet satisfait produisent la même sortie verte.
- Quand le compte **descend**, l'outil le dit et demande qu'on resserre le
  plafond — **il ne le resserre pas tout seul** : un contrôle qui modifie sa
  propre donnée sans que personne ne le voie a cessé d'être un contrôle.

L'épreuve **I** de `contre_epreuve_capacites.sh` mesure les deux sens : une
dette `A-DATER` de plus → **CLIQUET ROMPU, code 4** ; le fichier sans directive
de plafond → **code 2**, jamais un vert.

### 📄 `CAPACITES_A_DATER.md` — de quoi trancher les 24 en dix minutes

> **Le cliquet empêche le stock de grossir. Il ne le vide pas.** Et il ne se
> vidait pas, pour une raison simple : *« les 24 dates »* ne dit à personne ce
> qu'il faut décider. **Ce n'était pas la faute de Rodolf, c'était la faute de
> la demande.**

`CAPACITES_A_DATER.md` donne, pour **chacune** des 24 : ce que la capacité
prétend faire **en une phrase lisible sans ouvrir le code**, le fichier et la
ligne, le **nombre d'appelants réels — comptés, pas estimés** — et la colonne
qui décide : **ce qui casse si on l'abandonne, ou « rien »**.

Il est **trié par cette quatrième colonne**, du plus facile au plus difficile,
et porte **une recommandation par ligne** (`implémenter` / `abandonner` /
`dater`) : la réponse attendue est *oui* ou *non*, jamais une date inventée
depuis rien.

⚠️ **Le document ne date rien et ne touche pas au plafond.** Il ne modifie pas
`config/capacites.list` — pour la raison écrite trois paragraphes plus haut :
*un contrôle qui modifie sa propre donnée sans que personne ne le voie a cessé
d'être un contrôle.*

**Ce que la mesure a trouvé, et qui n'était pas prévu** (extraits) :

- **`NkBlendAttachment::blendEnable` n'est PAS creux** : **6 lectures sur
  5 backends**. D2 ne résout pas la voie d'accès quand le backend écrit
  `auto &a = d.blend.attachments[i]` — le **type n'apparaît pas dans la ligne**.
  **Quatrième fois de ce chantier qu'un instrument accuse le sujet à la place de
  son propre montage.**
- **`NkDeviceCaps` n'est pas une structure morte** : `GetCaps()` compte
  **29 sites d'appel réels** (15 fichiers), dont **17 décident** d'un chemin —
  `computeShaders`, `maxComputeGroupSize*`, `maxComputeSharedMemory`,
  `indirectDispatch`. Passer six des drapeaux à `false` ferait donc mentir le
  rapport **dans l'autre sens** — ce n'est pas une sortie honnête, c'en est une
  deuxième.
  ⚠️ **Et ce 29 a d'abord été un 45.** 45 était le résultat brut d'un
  `grep GetCaps()` : il comptait `GetCapsule`, le `FnGetCaps` XInput de
  `NkWin32Gamepad.h` et des fichiers `* copy` morts. **Un compte brut présenté
  comme une mesure** — la faute même que ce chantier traque, rattrapée en
  recomptant avant publication, pas en relisant.
- **`NkRendererConfig::voxelAOEnabled`** : le commentaire du champ promet
  *« false = sous-système NON alloué (gratuit) »*. `NkRendererImpl.cpp:226`
  alloue **inconditionnellement**. **NkAnimaEditor demande explicitement cette
  économie et la paie.** Le contrat écrit est faux **aujourd'hui**, quoi qu'on
  décide du champ.
- **`NkSwapchainDesc` est déclarée deux fois** dans le même `namespace`, avec
  des défauts **différents** (`imageCount` 3 vs 2, `colorFormat`, `samples`).
  Ce serait une erreur de compilation — sauf que `NkISwapchain.h` n'est inclus
  par personne : son unique `#include` (`NkRHI.h:11`) est **en commentaire**.
  Le conflit ne se voit jamais.
- **Une limite de portée à dire tout haut** : `ENTETES_D2` ne couvre que trois
  en-têtes. `NkSLTargetCaps::geometryShaders` (`NkSLFeatures.h:34`) porte
  **6 écritures et 0 lecture** — le même défaut, dans un fichier que le
  détecteur **ne lit pas**. Son absence du rapport se lit « rien à signaler ».

⚠️ **Et la mesure a commencé par me corriger moi.** La branche était
**21 commits derrière `main`** au moment d'écrire ce document. Compter des
appelants sur cette photo aurait produit des chiffres **crédibles et faux** —
la leçon de la section *« il ne mesure QU'UNE référence »*, appliquée à son
propre auteur. J'ai fusionné, **puis** compté. *(Contre-vérifié : les 17 comptes
bruts sont identiques avant et après. Mais je ne pouvais pas le **savoir** avant
de fusionner, et c'est exactement le point.)*

---

## D3 — ce que le C++ lie contre ce que les shaders échantillonnent

**Livré.** Il était refusé le 22/08 pour une raison mesurée : côté C++, l'index
de set n'existait **dans aucune déclaration**. Ce n'était pas une fatalité,
c'était une ligne de code manquante.

### La correction, côté code

`NkResources.h` porte désormais `kStandardBindings[]` — `(set, binding, type,
étage, nom)` — et **`CreateStandardLayouts()` la parcourt**. L'appariement
`enum → set` est devenu une **donnée du code**, donc calculable.

La table n'est pas une seconde vérité à maintenir à côté du code : **c'est la
seule**. Si elle ment, les layouts mentent avec elle, et ça se voit.

**Preuve d'équivalence avant remplacement** : les deux versions produisent la
**même suite de 18 `(set, binding, type, étage)`, dans le même ordre** — comparé
mécaniquement, pas relu. Plus la compilation (24/24, `SUCCESS`).
⚠️ **Ce n'est pas une preuve d'exécution : aucun GPU n'a tourné.**

⚠️ **Un piège trouvé en compilant, pas en relisant** : deux types portent le nom
`NkShaderStage` — `nkentseu::NkShaderStage` (= `NkSLStage`, celui du RHI, qui
porte `NK_ALL_GRAPHICS`) et `nkentseu::renderer::NkShaderStage`
(`NkShaderBackend.h`). Non qualifié, **le second gagne**. Et l'erreur ne sort pas
dans le fichier fautif : `NkResources.cpp` compilait, `NkRender3D.cpp` non. Tout
est qualifié explicitement.

### Ce que D3 affirme, et rien de plus

| | |
|---|---|
| il dit | « le C++ lie ce `(set, binding)` et **aucun shader qualifié par un set** ne le déclare » |
| il ne dit **pas** | « le shader lit la mauvaise chose » — trancher demande de savoir ce que la **carte** lit, et rien ici ne le mesure |

⚠️ **Et il ne regarde pas le sens inverse comme un défaut.** Le corpus compte
**47 couples `(set, binding)` distincts** pour **18** bindings standard : la
majorité appartient à des familles (compute, 2D, post) qui n'utilisent pas les
layouts standard. Crier sur chacune fabriquerait 29 faux positifs le premier
jour.

⚠️ **Les déclarations SANS set sont comptées à part, et nommées.** OpenGL n'a pas
de descriptor sets : ses shaders écrivent `layout(binding=2)` tout court. Un D3
qui ne compterait que les déclarations qualifiées annoncerait « aucun shader ne
déclare ce slot » alors qu'une famille entière le déclare — **il accuserait**.
Défaut trouvé en contre-vérifiant ses deux premiers candidats ; le message porte
désormais les deux moitiés de ce qui est su.

### Le fait que D3 rapporte — et qu'il ne corrige pas

```
NK_BIND_IBL_IRRADIANCE (set=0,binding=5)  <->  tEnvIrradiance (set=0,binding=8)
   jeton commun: irradiance ; ici le slot du C++ porte: sampler2D tCookie3
```

Le C++ lie l'irradiance IBL au slot **(0,5)**, où les shaders déclarent
`tCookie3` — un `sampler2D`, une texture de cookie de lumière. Et
`tEnvIrradiance` (un `samplerCube`) vit en **(0,8)**.

⚠️ **Ce n'est pas un verdict.** Un commentaire qui ment sur un numéro de binding
peut être le commentaire qui a tort **ou** le code ; trancher demande de mesurer
ce que la carte lit. **L'outil situe, il ne tranche pas.**

Et il en sort un **second de la même forme**, que personne ne cherchait :
`NK_BIND_SHADOW_ATLAS` en (0,4) pendant que `tShadowAtlas` vit en (0,11).

L'information n'est rendue que lorsque le slot du C++ porte **autre chose** —
sinon le nom vit ailleurs *aussi*, et il n'y a rien à montrer. Sans ce filtre,
la section sortait 32 lignes dont la plupart ne montraient rien, et **une
information qu'on ne lit plus vaut un silence**.

---

## ⚠️ Autocontrôle : trois couches d'échappement, et deux mangent un `\n`

Entre le heredoc qui écrit un script, le shell qui le lit et `awk` qui compile
ses chaînes, un `\n` de format peut disparaître **en silence**. Payé **trois
fois** dans la même soirée : un `printf` awk coupé par un vrai saut de ligne,
l'awk qui refuse de compiler, et **une section entière du rapport restée vide**.

**Une section vide se lit « rien à signaler ».** C'est la forme exacte que ce
chantier traque : un outil cassé et un dépôt sain rendent la même sortie.

Deux parades, et aucune n'est une relecture :

1. `verif_capacites.sh` **se contrôle lui-même au démarrage** : une chaîne de
   format non fermée dans son propre source → **ÉCHEC D'INSTRUMENT, code 2**.
2. En awk, **`printf fmt, a, b >> f` se lit `printf fmt, a, (b >> f)`** : le
   dernier argument est pris pour une comparaison, et le fichier reste **vide**.
   Les parenthèses ne sont pas du style, elles sont la correction.

---

## Les fichiers « copy » versionnés — la preuve, pas la liste

`./preuve_copies_mortes.sh` (`--morts` pour les seuls chemins).

`git ls-files` suit **27** fichiers `… copy.cpp` / `… copy 2.h` / etc. dans
l'arbre source. Ils ne sont compilés par rien, mais **tout `grep` les lit** — les
outils comme les humains.

**Pourquoi ça compte ici** : `NkOpenglDevice copy.hpp` porte
`mCaps.timestampQueries = true`. Le détecteur de capacités l'a compté comme une
écriture réelle. Ce n'est pas le danger. Le danger est **symétrique** : une copie
morte peut fournir la **seule « lecture »** d'un champ creux et faire **manquer**
un défaut — en silence, et dans la mauvaise direction.

**Ils ne sont pas exclus par un filtre.** « Exclure les fichiers dont le nom
contient *copy* » serait une heuristique de nom, exactement ce que ce dépôt
interdit depuis quatre fois.

### Deux voies, et il faut les deux

Une première mesure ne cherchait **que** qui cite le basename (`#include` ou
source listée). Elle rendait « 25 non cités sur 27 ». **Elle était fausse comme
preuve** : **118** `.jenga` de ce dépôt déclarent leurs sources par **joker**
(`src/**.cpp`, `src/**/*.h`). Un fichier que personne ne nomme peut être compilé
quand même.

Et la version qui lisait les jokers s'est trompée dans l'autre sens : elle a
déclaré `NkOpenglDevice copy.hpp` **vivant** en s'appuyant sur
`src/NKRHI/Opengl/**` — qui est un **`excludefiles`**, l'exact contraire d'une
inclusion. *Un outil de preuve qui lit une exclusion comme une inclusion ne
prouve rien : il répond à une autre question que celle qu'on lui pose.*

**Résultat mesuré : 27 sur 27 prouvés morts** — aucune citation hors commentaire,
et aucun joker d'inclusion du `.jenga` le plus proche ne les couvre.

⚠️ **Le doute penche du côté sûr** : en cas d'ambiguïté sur un motif, le fichier
est déclaré **vivant**. Se tromper en disant « vivant » coûte un fichier qu'on ne
supprime pas ; se tromper en disant « mort » fait supprimer du code qui compile.
Les deux erreurs n'ont pas le même prix.

⚠️ **Ce script ne supprime rien**, et c'est délibéré : retirer 27 fichiers depuis
une branche pendant que cinq agents travaillent fabrique des conflits qu'on
résout mal. Il **prouve** ; la suppression se décide et se fait sur `main`.

---

## `NkMsaaContractCheck` — le nombre d'échantillons impossible

La neuvième des capacités creuses, et **la seule qui demandait une exécution** :
*« le nombre d'échantillons impossible qui rend 0 sans erreur »*. Aucun motif de
texte ne la voit ; tout détecteur statique qui prétendrait la voir serait un
générateur de faux positifs. **Ce n'est pas une règle, c'est un banc.**

### Le défaut, mesuré

`NkVulkanDevice::ToVkSamples()` est un `switch` sur `NkSampleCount` dont le
`default:` rend **`VK_SAMPLE_COUNT_1_BIT`**.

Or trois chemins y mènent :

| entrée | statut | ce que le backend rend |
|---|---|---|
| `NK_S32`, `NK_S64` | **déclarés dans l'enum**, honorés par aucun backend | **1 échantillon** |
| `static_cast<NkSampleCount>(3)` | légal — et `NkRendererConfig::msaaSamples` est un `uint32` nu | **1 échantillon** |
| `NK_S4` sur une carte sans MSAA 4× | non supporté | **1 échantillon** |

**Sans erreur, sans journal, sans retour.** L'appelant croit avoir du MSAA et
n'en a pas. C'est *« un repli qui préserve `success` n'est pas un repli, c'est un
mensonge »* dans sa forme la plus dangereuse : **le repli produit un résultat
plausible.**

### Ce que le banc mesure

`NkDeviceCaps::SupportsSamples()` rend la question **dicible** avant qu'elle
n'atteigne un backend, et `MaxSamples()` remplace un ternaire qui était recopié
à la main dans `GetContextInfo()`.

⚠️ **Deux mots qu'on confondait, et c'est toute l'affaire :**

| | |
|---|---|
| **EXPRIMABLE** | la valeur existe dans l'énum `NkSampleCount` |
| **HONORABLE** | ce périphérique sait réellement la rendre |

`NK_S32` est exprimable et ne sera jamais honorable.

### Des relations, pas des exemples

Le banc parcourt les **2⁴ = 16 combinaisons** de drapeaux MSAA et vérifie une
**relation** sur chacune — pas trois cas choisis à la main :

| cas | relation vérifiée |
|---|---|
| 1 | `NK_S1` est honorable sur les 16 combinaisons (sinon une carte sans MSAA ne rendrait plus rien) |
| 2 | 10 valeurs non-puissances-de-deux refusées sur les 16 — **dont 3 et 7, le cas du mandat** |
| 3 | `NK_S32` et `NK_S64` refusés sur les 16 |
| 4 | `SupportsSamples(2/4/8/16)` **==** le drapeau correspondant, 16 sur 16 |
| 5 | `SupportsSamples(MaxSamples())` vrai, **et rien au-dessus n'est honorable** |
| 6 | 3 et 7 refusés **même sur la carte la plus capable**, 4 accepté sur la même |

⚠️ Le cas 4 est ce qui fait qu'un cinquième drapeau MSAA ajouté sans `case`
correspondant **est dit**, au lieu de passer en silence. C'est une relation, pas
un compte figé — la leçon de `matgraph` appliquée à un banc neuf.

### Il sait tomber — et le premier essai a raté, ce qui a trouvé autre chose

Un banc vert ne prouve rien tant qu'il n'a pas montré qu'il sait rougir. Les
défauts sont **injectés dans le contrat lui-même**, pas dans le `main` du banc.

**Premier essai — le défaut A n'en était pas un.** J'avais retiré la garde
« puissance de deux » de `SupportsSamples`. **Aucun cas n'est tombé : 8 OK / 0
FAIL.**

Mon premier diagnostic — *« le banc a tourné sur l'ancien binaire »* — était une
**supposition, et elle était fausse**. La mesure :

| | fichiers recompilés | verdict du banc |
|---|---|---|
| `jenga build` #1 après le changement d'en-tête | **33** | 8 OK / 0 FAIL |
| `jenga build` #2, #3 (rien n'a bougé) | 0 | 8 OK / 0 FAIL |
| `jenga rebuild` (**table rase**) | tout | **8 OK / 0 FAIL** |

Jenga suit donc parfaitement les en-têtes. Le banc était vert pour la vraie
raison :

> **Le `default: return false` du `switch` refusait déjà 0, 3, 5, 6, 7, 100. La
> garde « puissance de deux » était entièrement REDONDANTE — du code que rien ne
> pouvait faire tomber, donc du code dont on ne pouvait pas savoir s'il était
> juste.**

⚠️ **Deux protections pour le même cas rendent le banc incapable de dire
laquelle tient.** C'est ce que l'épreuve a révélé, et c'est plus utile que
l'épreuve elle-même. La garde a été supprimée : une seule règle, exprimée une
seule fois.

**Second essai — deux défauts qui touchent des règles distinctes :**

| défaut injecté | mesuré |
|---|---|
| **A** — `case 4: return msaa4x;` devient `return true` (le drapeau est ignoré) | **code 1**, 6 OK / **2 FAIL** — cas 4 et 5 |
| **B** — `default: return false` devient `return true` | **code 1**, 3 OK / **5 FAIL** — cas 2, 3, 5, 6 |
| restauration | fichier **identique**, banc **code 0**, 8 OK / 0 FAIL |

Les deux défauts font tomber des **ensembles différents** de cas — c'est ce qui
prouve que les cas mesurent des règles distinctes, et pas la même trois fois.

⚠️ **La leçon de méthode, et elle vaut plus que le banc** : *un défaut injecté qui
n'en est pas un fait croire que le banc est aveugle.* Avant de conclure qu'un
contrôle ne voit rien, il faut prouver que **ce qu'on lui a montré était
visible**.

### La limite, dite plutôt que tue

Le banc mesure le **contrat** : CPU pur, aucun périphérique. Il tourne donc dans
un arbre neuf, chez tout agent, sur une machine sans carte — c'est pour ça qu'il
est en mode `rapide`.

Il ne mesure **pas** ce que le backend fait de la valeur demandée : cela demande
un GPU. **Il l'imprime sur sa dernière ligne.**

> **Un banc qui tait sa limite est pire qu'un banc absent : on croit la question
> réglée.**

📌 **Cette limite est levée depuis le 2026-08-23.** Le banc `NkMsaaDeviceCheck`
(mode `complet`) mesure la moitié GPU — et il a trouvé le défaut sur une vraie
carte. La phrase qui figurait ici, *« le banc rend le mensonge évitable, il ne le
supprime pas »*, était vraie tant que rien ne mesurait ce chemin ; elle ne l'est
plus, et l'effacer est ici la seule chose juste à faire. Voir la section
suivante.

⚠️ Ce qui **reste** vrai, et qu'il ne faut pas effacer avec elle : le contrat rend
le mensonge évitable, **le défaut lui-même est toujours dans les backends**. Un
appelant qui contourne `SupportsSamples()` sera encore ramené à 1 en silence.

---

## `NkMsaaDeviceCheck` — la moitié GPU, et **IGNORÉ comme troisième état**

Banc en mode `complet` (2026-08-23). Il ferme ce que `NkMsaaContractCheck`
laissait ouvert : pour chaque nombre d'échantillons, il confronte ce que le
**contrat** annonce à ce que la **carte** fait.

```
attendu = caps.SupportsSamples(s)
obtenu  = CreateTexture(desc avec samples = s).IsValid()
exigence : obtenu == attendu
```

### Les deux directions d'erreur n'ont pas le même sens, et sont nommées à part

| mesure | ce que ça veut dire |
|---|---|
| attendu **NON**, obtenu **OUI** | **le mensonge visé.** La carte a accepté une valeur que le contrat refuse : elle a dégradé en silence, et l'appelant croit avoir du MSAA. |
| attendu **OUI**, obtenu **NON** | le contrat est **trop optimiste** : il promet ce que la carte refuse. Moins dangereux, faux quand même. |

Les confondre sous un seul « FAIL » enverrait chercher au mauvais endroit.

### Ce qu'il a mesuré au premier passage — et c'est un vrai défaut

Périphérique DX11 headless, drapeaux `2x=oui 4x=oui 8x=oui 16x=non`,
`MaxSamples()=8`. **4 OK / 5 FAIL**, et les cinq sont dans la direction du
mensonge :

> demander **3, 7, 16, 32 ou 64** échantillons rend une texture **VALIDE**.

⚠️ Le cas **16** est le plus net : le contrat **et** la carte disent tous les deux
que 16x est impossible sur ce matériel, et `CreateTexture` rend quand même une
poignée valide.

**Prouvé à la source, pas déduit du comportement** —
`Kernel/Runtime/NKRHI/src/NKRHI/DirectX11/NkDirectX11Device.cpp:969-977` :

```cpp
if (td.SampleDesc.Count > 1) {
    UINT qualityLevels = 0;
    mDevice->CheckMultisampleQualityLevels(td.Format, td.SampleDesc.Count, &qualityLevels);
    if (qualityLevels == 0) {
        td.SampleDesc.Count = 1;   // <- et on cree la texture QUAND MEME
        td.SampleDesc.Quality = 0;
    }
```

C'est **exactement le `default:` de `ToVkSamples()`**, dans un autre backend, et
écrit explicitement plutôt que par omission. Même famille : *un repli qui
préserve `success` n'est pas un repli, c'est un mensonge.*

📌 **Ce banc n'a PAS été rendu vert.** Le défaut est dans le backend, et le
corriger change le comportement de tous les appelants — ce n'est pas une décision
d'outilleur. Le banc mesure, il rapporte, et il reste rouge tant que la question
n'est pas tranchée. **Un banc qu'on rend vert avant de corriger ce qu'il montre
n'a jamais servi à rien.**

⚠️ Et la limite de cette mesure, à ne pas surinterpréter : le banc retient le
**premier** périphérique disponible, et c'était DX11. `ToVkSamples()` lui-même
**n'est toujours pas mesuré** — il reste une lecture de source, pas une mesure.

### Le témoin, et pourquoi il passe avant toute accusation

> **Avant de conclure qu'un contrôle ne voit rien, prouver que ce qu'on lui a
> montré était visible.**

Le symétrique s'applique ici, et il mord plus fort : **avant de conclure que le
contrat ment, prouver que la question pouvait être posée.**

Si une texture 64×64 RGBA8 render-target à **un seul** échantillon ne peut pas
être créée, alors aucune ne le peut, et les huit autres valeurs rendraient toutes
« le contrat PROMET, la carte REFUSE » — **huit accusations pour zéro mesure**.
Le banc dirait « le contrat ment » sans avoir rien mesuré du contrat. C'est *une
supposition consignée comme mesure*, en huit exemplaires.

Donc le témoin à 1 échantillon tourne **en premier**, et son échec est un
**IGNORÉ**, jamais un ÉCHEC.

### ⚠️ IGNORÉ est un TROISIÈME état — c'est le cœur de ce banc

Un banc qui exige un GPU sera **sauté** sur une machine qui n'en a pas, ou dont
le pilote refuse.

> **Un banc ignoré qui rapporte vert est le mensonge que ce chantier traque : on
> croit la question mesurée alors que personne ne l'a posée.**

Et le compter ÉCHEC n'est pas mieux : une machine sans carte n'est le bug de
personne, et un banc qui accuse la moitié du parc est désactivé en une semaine.
C'est le défaut de `NkGpuProbe`, qui rend **1** quand aucun périphérique n'existe
— il confond « pas de carte » et « la carte est cassée ». **Un faux ÉCHEC, le
symétrique du faux vert.**

D'où trois états :

| verdict | code | sens |
|---|---|---|
| `OK` | 0 | la question a été posée, la réponse est bonne |
| `ÉCHEC` | 1 | la question a été posée, la réponse est mauvaise |
| `IGNORÉ` | 77 | **la question n'a pas été posée** |

### Comment le vérificateur l'apprend — une donnée, pas une constante devinée

Nouveau critère de verdict dans `config/bancs.list` :

```
NkMsaaDeviceCheck | banc | complet | | code+ignore:77 | ...
```

`code 0` → OK ; `code 77` → IGNORÉ ; **tout autre code** → ÉCHEC.

Le nombre est une **donnée de la ligne du banc**. S'il était une constante en dur
dans le script, le banc et le vérificateur auraient un **accord tacite** que rien
ne garde — et le jour où l'un des deux change, personne ne l'apprend.

⚠️ **`code+ignore:0` est refusé explicitement.** Ce serait déclarer que le code du
succès est aussi celui de « rien mesuré » : la distinction que le critère existe
pour porter s'effondrerait, silencieusement. *Une donnée qui détruit la
distinction qu'elle sert à porter n'est pas une donnée acceptable.*

### La raison est LUE, jamais fabriquée

`raison_ignore()` extrait la ligne `[IGNORE]` de la sortie du banc. « pas de
carte » et « la carte refuse la texture la plus banale » n'appellent pas la même
suite, et le vérificateur n'a pas à choisir pour le banc.

Si le banc n'a rien dit, le rapport écrit **qu'il n'a rien dit** — pas une raison
plausible à sa place.

### Ce que la passe complète doit dire, et qu'elle dit maintenant

Le compte des verts ne suffit pas. La passe imprime désormais :

- `IGNORE` dans la colonne verdict du tableau ;
- une section **`-- IGNORES : ces bancs N ONT RIEN MESURE --`**, un par un, avec
  sa raison et son journal ;
- le compte sur la ligne de verdict : `… au vert, N ignoré(s), … indéterminé(s)` ;
- et **sous** le verdict, l'avertissement qui empêche de mal lire le mot « OK » :

> ⚠️ *N banc(s) IGNORÉ(S) : leur question n'a pas été posée sur cette machine. Le
> verdict ci-dessus ne porte QUE sur les M banc(s) qui ont réellement tourné.*

Un IGNORÉ **ne change pas le code de sortie** — rien n'est cassé. Mais il rend
impossible de lire « OK » comme « tout est vérifié ».

### Son épreuve : `epreuve_ignore_gpu.sh`

Sur la machine où ce banc a été écrit, un GPU existe — **le chemin IGNORÉ n'avait
donc jamais tourné**. Un chemin d'erreur que rien n'a jamais emprunté est du code
dont on ne sait pas s'il est juste : c'est la leçon de la garde « puissance de
deux », redondante donc improuvable, supprimée le 22/08.

L'épreuve injecte **deux** défauts réels, sur des chemins **distincts** :

| défaut | injection | IGNORÉ attendu |
|---|---|---|
| **A** | `Ouvrir()` : le pilote refuse le périphérique | « aucun périphérique headless » |
| **B** | le témoin à 1 échantillon est refusé | « le témoin a été REFUSÉ » |

Ils doivent produire des **raisons différentes**. Si les deux donnaient la même
ligne, la raison ne serait pas lue dans la sortie du banc mais fabriquée.

Et l'épreuve exige les trois marques du mandat : le compte sur la ligne de
verdict, la section `IGNORES`, l'avertissement sous le verdict.

⚠️ **Le contrôle négatif y est armé AVANT le filet**, pas l'inverse — voir la
section suivante, qui existe parce que l'ordre inverse a détruit du travail dans
ce dépôt le 22/08.

**Résultat (2026-08-23) : 14 exigences sur 14, code 0.** Les deux défauts rendent
bien `77`, sont classés `IGNORE`, comptés, nommés, et donnent des raisons
distinctes. L'arbre revient à l'identique.

⚠️ **Et le premier passage a été rouge — sur MON assertion, pas sur la mesure.**
J'ai lu le code de sortie avec `grep '^  NkMsaaDeviceCheck ' | head -1`, qui
attrape la ligne de **progression** (`... IGNORE (1s)`, sans colonne code) au
lieu de la ligne du **tableau** (`oui  77  1s  IGNORE`). L'épreuve annonçait donc
« code 77 absent » alors qu'il était là.

> **Le nom d'un banc apparaît dans plusieurs rôles de la même sortie. S'ancrer
> sur le nom, c'est laisser le hasard de l'ordre choisir la ligne qu'on lit.**

C'est la même faute que citer `[OK] … aucune erreur` comme cause d'un échec :
**chercher un mot là où il faut chercher une structure.** Corrigé en exigeant la
forme de la ligne de tableau (`+(oui|non|saute) `). Et j'ai vérifié la sortie
réelle **avant** de toucher à l'assertion — un contrôle qu'on « répare » sans
avoir prouvé qu'il avait tort est un contrôle qu'on vient d'aveugler.

---

## `NkMsaaDeviceCheck` ne vire PAS au vert — et il a raison de rester rouge

On m'a annoncé que le MSAA était corrigé sur les quatre backends et que ce banc
**virerait au vert de lui-même**. **Mesuré, puis lu : non, et c'est justifié.**

### 1. Mesure sur CETTE référence — `feat/verificateur`, 23/08

`NkMsaaDeviceCheck` en Debug : **code de sortie 1**, `4 OK / 5 FAIL`. Les cinq
échecs sont « le contrat REFUSE, la carte ACCEPTE » pour **3, 7, 16, 32 et 64**.

Ce n'est pas un banc périmé : le défaut est **littéralement présent sur cette
branche**. `NkDirectX11Device.cpp` y pose toujours `td.SampleDesc.Count = 1`
quand `CheckMultisampleQualityLevels` rend 0, **puis crée la texture**. Le banc
mesure ce qui est là.

### 2. Le correctif existe — mais sur une branche non fusionnée, et il est PARTIEL

Le bloc corrigé vit sur **`feat/rendu-temps-reel`**, pas ici, pas sur `main`.
Et son propre commentaire dit qu'il ne fait que la **moitié** du travail :

> *PREMIER TEMPS DE LA CORRECTION : LE RAPPORT.* […]
> *SECOND TEMPS, PAS ENCORE FAIT : le REFUS des nombres qui n'existent pas.*
> *Ici, 3 et 7 sont SIGNALES puis plafonnés.*

**Signalés puis plafonnés — donc la texture est toujours créée.** Or ce banc
compare `obtenu = h.IsValid()` (`main.cpp:104`). Une texture plafonnée reste
valide. **Le banc restera donc rouge sur `feat/rendu-temps-reel` aussi**, tant
que le second temps n'est pas fait.

⚠️ **Ce dernier point est une LECTURE DE CODE, pas une mesure** : je n'ai pas
construit NKRHI sur `feat/rendu-temps-reel`. Je le note comme lecture, parce
qu'une supposition consignée comme mesure se propage avec l'autorité d'une
mesure.

Les deux chantiers ne mesurent pas la même chose, et aucun des deux n'a tort :
le correctif a rendu la substitution **visible** ; ce banc demande qu'elle soit
**refusable**. Le vert viendra du second temps, pas du premier.

### 3. ⚠️ Ce que ce banc ne mesure pas, et que sa fiche laissait croire

`config/bancs.list` annonçait « (DX11, puis DX12, puis OpenGL) ». C'est une
**chaîne de repli, pas un balayage** : `main.cpp:167` fait `break` au premier
périphérique qui s'ouvre. Sur cette machine, **DX11 s'ouvre — donc DX12 et
OpenGL n'ont jamais été mesurés**, 9 confrontations en tout.

Conséquence directe : les deux faits qu'on m'a transmis — **DX12 n'a jamais su
créer une cible MSAA**, **OpenGL mentait par zéro** — sont **hors d'atteinte de
ce banc sur toute machine où DX11 s'ouvre**. Je ne peux ni les confirmer ni les
infirmer. La fiche est corrigée pour le dire.

### 4. IGNORÉ comme troisième état : vérifié dans le code, pas sur parole

`NB_IGNORE` est un compteur **distinct** de `NB_OK` et `NB_ECHEC`
(`verif_bancs.sh:926`, incrémenté l.934), **nommé dans les deux lignes de
verdict** (l.979 et l.981), et la passe annonce explicitement combien de bancs
ont été ignorés (l.989). `code+ignore:0` est **refusé délibérément** (l.726).
La condition est tenue.

---

## ⚠️ Le lanceur décide : un `libstdc++` étranger en tête de PATH corrompt le tas

**Mesuré le 2026-08-23, sur cette référence, `NkSLComputeCheck` en Release.**

Il circulait que **glslang corrompt le tas dès sa première compilation
GLSL→SPIR-V en Release**, et que tout ce qui tournait ne tournait que grâce à un
**cache chaud**. **Les deux moitiés sont fausses.**

`NkSLComputeCheck` n'utilise **aucun cache** — `NkSLCompiler c;` prend le
`cacheDir` vide par défaut (`Kernel/Runtime/NKSL/src/NKSL/Compiler/NkSLCompiler.h:83`).
Il compile donc à froid **à chaque exécution**. Et il **passe en Release**,
code de sortie **0**, 153 lignes, glslang compilant réellement.

### Ce qui décide vraiment

| lancement | binaire | code de sortie |
|---|---|---|
| **git-bash** | Release | **0xC0000374** `STATUS_HEAP_CORRUPTION`, dès la 1re compilation SPIR-V |
| **PowerShell** | Release | **0** — 6 essais sur 6 |
| **git-bash** | Debug | **0** |
| **git-bash**, cible 0 (GLSL-OpenGL, sans glslang) | Release | **0** |

Déterministe des deux côtés. `env -i` ne change rien : ce n'est pas une variable
d'environnement.

**La cause est mécanique.** L'exécutable importe `api-ms-win-crt-*` : il est bâti
**ucrt64**. Il dépend de `libstdc++-6.dll` et `libgcc_s_seh-1.dll`, **absents de
son dossier** — il les résout donc par le PATH :

- sous PowerShell, le PATH n'offre que `C:\msys64\ucrt64\bin` → libstdc++ de
  **2 667 044** octets, **celui contre lequel il a été lié** ;
- sous git-bash, le PATH commence par `/mingw64/bin` = `C:\msys64\mingw64\bin`,
  variante **msvcrt** → libstdc++ de **2 667 040** octets, **chargé en premier**.

Un exécutable ucrt64 avec la `libstdc++` msvcrt, ce sont **deux allocateurs
différents** : le tas part à la première allocation C++ lourde — chez glslang.

**Preuve par une seule variable changée**, même binaire, même machine :

```bash
PATH="/c/msys64/ucrt64/bin:$PATH" ./…/NkSLComputeCheck.exe 2   # EXIT=0
PATH="/c/msys64/ucrt64/bin:$PATH" ./…/NkSLComputeCheck.exe     # EXIT=0, 153 lignes
```

### Ce que ça impose au vérificateur — et c'est un aveu sur ses propres mesures

`verif_bancs.sh` **est un script bash** et **ne touche pas au PATH**. Il tourne
en **Debug par défaut**, et Debug passe : **les passes actuelles ne sont pas
faussées**. Mais il accepte `--config Release`.

**Donc, aujourd'hui, une passe `--config Release` lancée depuis bash verrait tout
banc touchant glslang planter, et l'outil écrirait ÉCHEC en accusant le code.**
Un rouge reproductible, argumenté, situé — et **faux** : le défaut serait dans le
PATH du lanceur, pas dans le code accusé. C'est *une supposition consignée comme
mesure*, retournée contre le code.

Le remède **n'est pas** que l'outil force le PATH : ce serait **armer un filet
avant son contrôle**, l'outil réparerait l'environnement qu'il est censé mesurer
et ne saurait plus dire qu'il est cassé. **Il doit détecter et refuser de
démarrer.** Décision en attente de Rodolf (voir *Dettes ouvertes*).

---

## ⚠️ Un filet armé avant son contrôle détruit ce que le contrôle protège

Payé cette nuit, sur un script d'épreuve jetable.

Le script devait injecter un défaut dans `NkIDevice.h` puis le restaurer. Il
avait les deux bonnes idées — un `trap 'restaurer' EXIT` et un contrôle négatif
qui **refuse de démarrer sur un arbre sale** — et il les avait **dans le mauvais
ordre** :

```bash
trap 'restaurer' EXIT INT TERM HUP          # ← armé en premier
sale=$(git status --porcelain -- "$H")
if [ -n "$sale" ]; then exit 2; fi          # ← son refus déclenche le trap
```

Le contrôle négatif a fait son travail : il a vu l'arbre sale et a refusé. **Et
son `exit 2` a déclenché le `git checkout` qui a détruit la modification non
commitée qu'il refusait justement d'écraser.**

> **Le contrôle négatif d'abord, le piège ensuite. Un filet armé avant le
> contrôle détruit exactement ce que le contrôle existe pour protéger.**

C'est l'avertissement que `contre_epreuve_capacites.sh` porte déjà, mot pour
mot — *« partir d'un arbre sale reviendrait à jeter ton travail en cours à la
restauration »*. Je l'avais écrit ; je ne l'ai pas appliqué dans le script
suivant. **Écrire un piège ne protège pas celui qui l'écrit.**

⚠️ **Et le vrai coût n'est pas la modification perdue** (49 lignes, réappliquées
en une minute) : c'est que le commit qui a suivi a laissé, pendant quelques
minutes, un dépôt où **un banc référençait une fonction absente**. Le banc avait
compilé *avant* la destruction ; son binaire était vert alors que la source ne
compilait plus. **Exactement l'ancien binaire qui tourne — la panne qui a motivé
tout ce chantier.**

---

## Le format de `config/bancs.list`

```
<projet> | <classe> | <mode> | <arguments> | <verdict> | <note>
```

- **classe** : `banc` · `pas-banc` · `banc-arrete`
- **mode** : `rapide` · `complet`
- **verdict** : `code` · `code+absence:<motif>` · `code+presence:<motif>` ·
    `sans-verdict` • `code+ignore:<n>`
- **`code+ignore:<n>`** : trois états. `0` → OK, `<n>` → **IGNORÉ**, tout
  autre code → ÉCHEC. À déclarer sur tout banc qui exige quelque chose que la
  machine peut ne pas avoir. `<n>` est une **donnée de la ligne**, pas une
  constante devinée par le script ; `0` y est refusé (ce serait confondre le
  succès et le « rien mesuré »).

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

⚠️ **IGNORÉ n'a pas de code à lui, et c'est voulu.** Un banc ignoré ne casse
rien : la passe peut sortir **0** avec des bancs ignorés. Ce qu'il change, c'est
ce que la passe **dit** — compte séparé sur la ligne de verdict, section
`IGNORES`, et avertissement sous le verdict. *Le code de sortie répond « quelque
chose est-il cassé ? » ; le rapport répond « qu'a-t-on réellement mesuré ? ».
Ce ne sont pas la même question, et les faire porter par le même nombre
obligerait à mentir sur l'une des deux.*

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

---

## Dettes ouvertes — pour ne pas repartir de zéro le jour où on les prend

### 🔴 Le cache de Jenga est invalidé par le CHANGEMENT DE CIBLE, pas par un changement de source

**Non ouvert, et c'est un choix : six chantiers tournent, ce n'est pas le
moment d'en ouvrir un septième.** Mais tout est mesuré, et reproductible en
trois commandes — le jour où on le prend, on part de là.

**Reproduction, arbre chaud, aucune source modifiée entre les trois :**

| séquence | durée | fichiers recompilés |
|---|---|---|
| `jenga build --target NkSLCheck` deux fois (**même** cible) | **4,6 s** | **0** |
| puis `jenga build --target NkSLComputeCheck` | **48,3 s** | **219** |
| puis `jenga build --target NkSLCheck` à nouveau | **49,5 s** | **219** |

**Aucune source n'a bougé entre ces trois commandes.** Le cache d'objets tient
parfaitement tant qu'on reste sur la même cible, et s'effondre à chaque bascule.

**Ce que ça coûte, chiffré** : le vérificateur enchaîne 8 cibles.
8 × ~50 s de recompilation inutile ≈ **7 min sur les 13 à 18** d'une passe.
Autrement dit : **le coût du vérificateur ne vient pas du nombre de bancs, il
vient de ce défaut.** Avec un cache qui tiendrait, la passe à chaud tomberait
vers **une minute**.

**Où c'est** : dépôt Jenga (`D:\Projets\MacShared\Projets\Jenga`), séparé. Le
corriger depuis ici serait sortir du périmètre.

⚠️ **Et une mesure qui délimite ce défaut, faite le 23/08** : Jenga suit
**correctement les en-têtes**. Un changement dans `NkIDevice.h` a bien fait
recompiler **33 fichiers** au build suivant, puis 0 aux deux builds d'après.
Le défaut porte donc sur le **changement de cible**, et sur lui seul — ne pas
l'élargir en « le cache ne marche pas ».

**Piste non explorée**, à mesurer avant de promettre : un seul `jenga build`
sans `--target` (workspace entier), suivi de 8 `--target` devenus no-op à 4,6 s.
Le workspace compte ~146 projets dont 50 applications fenêtrées — **ça peut
coûter plus que ça ne rapporte**, et ça ne se saura qu'en le chronométrant.

### 🟠 Les 27 fichiers « copy » attendent une suppression sur `main`

Prouvés morts (voir *Les fichiers « copy » versionnés*). La suppression ne se
fait **pas depuis une branche** pendant que plusieurs agents travaillent.
`./preuve_copies_mortes.sh --morts` redonne la liste à jour le jour venu.

### 🟢 Le banc de l'échantillonnage impossible — LIVRÉ (2026-08-23)

`NkMsaaContractCheck`, 8 cas, 16 combinaisons de drapeaux, CPU pur, mode
`rapide`. Voir *`NkMsaaContractCheck` — le nombre d'échantillons impossible*.

**Ce qui reste ouvert derrière lui** : `NkVulkanDevice::ToVkSamples()` garde son
`default: return VK_SAMPLE_COUNT_1_BIT`. Le banc rend le mensonge **évitable**
(la question est dicible avant d'atteindre le backend) ; il ne le **supprime**
pas. Mesurer le chemin réel demande un périphérique GPU — un banc en mode
`complet`, pas celui-ci.

### 🟡 Les lignes `non-examine` de `config/capacites.list` — 117 → 102

Classées pour que la garde soit totale dès le premier jour, **pas examinées une
par une**. Politique tranchée : **chaque agent qui passe sur un de ces symboles
corrige sa ligne au passage** — même règle que les 111 `non-examine` de
`config/bancs.list`. Une grande revue en une fois n'arrive jamais.

**Appliquée le 23/08 : les 24 lignes `A-DATER` ont été examinées une par une**
et leurs notes réécrites (compte d'appelants avec `fichier:ligne`, ce qui casse
à l'abandon, recommandation). `non-examine` **passe de 117 à 102**. Aucune
classe, aucune date, aucun plafond n'a bougé. Voir `CAPACITES_A_DATER.md`.

### 🔴 `NkGpuProbe` est classé `code`, et devrait être `code+ignore:<n>`

Sa dette est déjà écrite plus haut (*IGNORÉ est un troisième état*) : il rend
**1** quand aucun périphérique n'existe, donc il **confond « pas de carte » et
« la carte est cassée »**. `NkMsaaDeviceCheck` a été écrit avec le troisième
état ; `NkGpuProbe` ne l'a pas encore.

⚠️ **Et la passe du 23/08 montre pourquoi ça n'a pas encore mordu** : sur cette
machine, **les deux périphériques se sont ouverts**. Son ÉCHEC n'est donc **pas**
le faux ÉCHEC redouté — c'en est un vrai (voir ci-dessous). Le défaut de dessin
reste entier, il ne s'est simplement pas manifesté ici. **Un chemin d'erreur que
rien n'a jamais emprunté est du code dont on ne sait pas s'il est juste.**

---

## Passe `complet` du 23/08 — 12 bancs, 10 OK, 2 ÉCHEC, **0 IGNORÉ**

Première passe `complet` mesurée après fusion de `main` à jour (`9b31cc8f`),
retard **0 commit**. **19 min 43 s** (le gros du temps est le défaut de cache de
Jenga documenté plus haut, aggravé ici par la fusion qui a invalidé NKRHI).

| banc | verdict | ce que ça dit |
|---|---|---|
| les 10 autres | **OK** | — |
| `NkGpuProbe` | **ÉCHEC** | un **vrai** échec, pas un manque de matériel |
| `NkMsaaDeviceCheck` | **ÉCHEC** | **4 OK / 5 FAIL**, et il a raison |

### `IGNORÉ` = 0, et c'est une information, pas un silence

Le troisième état est **implémenté, compté et annoncé** — la ligne de verdict
porte `0 ignore(s)` même à zéro, exactement pour que « aucun ignoré » et « la
section n'existe pas » ne se ressemblent pas.

⚠️ **Mais zéro ne veut pas dire « prouvé aujourd'hui ».** Aucun banc n'a pris ce
chemin cette fois **parce que la machine avait une carte**. Ce qui prouve le
troisième état reste `./epreuve_ignore_gpu.sh`, qui injecte deux défauts réels
sur deux chemins distincts et exige deux raisons **différentes**. La passe le
**compte** ; elle ne le **prouve** pas.

### `NkGpuProbe` — ÉCHEC réel, et une branche à regarder avant de diagnostiquer

Les deux périphériques se sont **ouverts** et ont annoncé `compute shaders :
oui`. Le calcul a rendu `C = [0 0 0 0]` au lieu de `58 64 139 154`, sur **DX11
et DX12**. La ligne de cause :

```
[NkRHI_DX11][ERR] DX11 shader: manque le source HLSL (stage 0)
[NkRHI_DX12][ERR] Shader stage missing/empty for stage 32
[NkML] Erreur compilation shader: MatMul
```

Ce n'est donc **pas** le chemin « pas de carte » : c'est le noyau compute de
`NkML` qui ne trouve pas la source de son shader `MatMul`.

⚠️ **Et l'outil a fait exactement ce pour quoi il a été outillé** : il nomme
**`feat/rendu-temps-reel` (55 commits inédits)** comme branche non fusionnée qui
touche `NkDirectX11Device.cpp`/`.h`. **Je n'ai donc rien diagnostiqué** —
*fusionner d'abord, remesurer ensuite, ne diagnostiquer qu'après*. C'est la
règle de lecture de ce document, appliquée au premier rouge qu'elle rencontre.

*(Fait mesuré qui délimite : le même échec figurait déjà dans le journal de
10 h 52, **avant** la fusion de `main`. Ce n'est pas une régression de la
fusion.)*

### `NkMsaaDeviceCheck` — rouge, et il a raison de l'être

**4 OK / 5 FAIL**, sur DX11 — **le chiffre exact de la mesure du 23/08 au
matin**. Le correctif MSAA du chantier rendu **n'a rien changé ici**, et c'est
attendu : il **plafonne** 3 et 7 au lieu de les **refuser**.

```
[FAIL]  3 echantillons : le contrat REFUSE, la carte ACCEPTE.
[FAIL]  7 echantillons : le contrat REFUSE, la carte ACCEPTE.
[FAIL] 16 / 32 / 64 : idem.
```

`caps.SupportsSamples(3)` répond **non**, et `CreateTexture(samples=3)`
**réussit** — DX11 ramène `SampleDesc.Count` à 1 en silence
(`NkDirectX11Device.cpp:969-977`). **La ressource existe, et rien ne dit combien
d'échantillons elle porte réellement.** C'est le mensonge visé, mot pour mot.

⚠️ Et l'outil dit l'**absence** aussi fort que la présence : *« branches non
fusionnées touchant les fichiers accusés : AUCUNE (15 branches examinées) — le
défaut est bien sur CETTE référence. »* **Ce rouge-là ne se règle pas par une
fusion.** Il se règle par un type qui n'accepte que les comptes valides — voir
`NkRendererConfig::msaaSamples` (E1) dans `CAPACITES_A_DATER.md`, **même sujet**.

---

# Journée du 24 août — le détecteur ne voyait pas tout, et le contrôle qui le prouve

## ⚠️ Le onzième cas était surtout une mesure sur **mon** détecteur

Le chantier rendu a trouvé un onzième paramètre déclaré non honoré :
`NkRendererConfig::pipeline`. `ForFilm()` et `ForArchviz()` posent
`pipeline = NK_DEFERRED`, et le SSR fonctionne quand même — parce que
`NkRendererImpl.cpp:829` lit **`mCfg.deferred`**, un *autre* champ, déclaré
`false` à la ligne 410 et que les deux préréglages ne touchent pas.

> **Qui demande `ForFilm()` obtient du FORWARD pendant que sa configuration dit
> DEFERRED.**

La première chose à faire n'était pas de le classer. C'était de vérifier que
**mon détecteur l'attrapait**. *Il ne l'attrapait pas.* Et un onzième cas qui
échappe veut dire qu'il y en a d'autres : **il y en avait quatorze.**

### Défaut 1 — le commentaire était amputé, la **chaîne de caractères** non

```
logger.Info("[NkRender3D] Shadow pipeline create: shader_valid={0} …")
```

Cette ligne porte le jeton `pipeline`, et ce jeton était compté comme une
**lecture** du champ. Une seule lecture suffit à clore le dossier — c'est même
une optimisation assumée du détecteur (17 s → 5 s).

⚠️ **C'est exactement la faute qu'ampute la ligne au-dessus**, dans un second
vêtement. Le code disait déjà, en toutes lettres : *« une mention dans un
commentaire n'est pas une lecture, et prendre l'une pour l'autre est exactement
chercher un mot au lieu d'un état »*. **Un nom dans une chaîne n'est pas plus une
lecture qu'un nom dans un commentaire** — et j'avais écrit la règle sans voir son
second cas.

### Défaut 2 — le repli « ce fichier nomme le type » attrape les **variables locales**

`NkRender3D.cpp` nomme `NkRendererConfig` **une fois** et déclare partout un
`NkPipelineHandle pipeline` **local** :

```cpp
NkPipelineHandle pipeline = mPBRPipeline;
cmd->BindGraphicsPipeline(pipeline);      // comptée comme lecture du CHAMP
```

**La règle juste est celle du langage** : un membre ne se lit que par `x.nom`,
`x->nom`, ou **nu dans le fichier qui le déclare** (ses méthodes en ligne, où le
`this->` est implicite). Le fichier déclarant garde donc l'exemption ; partout
ailleurs, un jeton nu n'est pas un accès au membre.

### La direction de l'erreur, et c'est pour ça qu'on ose

Les deux corrections ne peuvent qu'**ajouter** des candidats, jamais en retirer.

> **Un candidat de trop se classe une fois ; un candidat manqué ne se voit
> jamais.**

**Mesure : 128 → 143 candidats.**

### ⚠️ Défaut 3 — né de la correction des deux premiers : quatre faux positifs

**Quatre des quinze étaient faux**, et je les ai trouvés en contre-vérifiant à la
main, pas par un outil.

```cpp
srcStage |= toStage(bb[i].srcStage);   // une LOCALE à gauche, une VRAIE LECTURE à droite
```

Le test d'écriture cherchait `nom … =` **n'importe où sur la ligne** : il
attrapait la locale, classait la ligne « écriture », et le champ ne pouvait plus
jamais compter de lecture. `NkBufferBarrier::srcStage`, `::dstStage` et leurs deux
jumeaux de `NkTextureBarrier` sont **réellement lus**
(`NkVulkanCommandBuffer.cpp:400-401` et `:456-457`).

**Parade : la même règle que pour la lecture.** Hors du fichier déclarant, une
écriture de membre porte elle aussi un `.` ou un `->` juste devant le nom.
`c.pipeline = X`, `mCaps.x = true` et l'initialisation désignée `{.pipeline = X}`
la portent toutes les trois ; `srcStage |=` nu, non.

> ⚠️ **La leçon, et elle est pour moi.** J'avais justifié les deux premières
> corrections par la direction de l'erreur — *« un candidat de trop se classe une
> fois »*. **C'est vrai du COÛT, pas de la VÉRITÉ** : un faux positif classé
> `vision-assumee` aurait fait dire au fichier que quatre champs lus par le
> backend Vulkan ne servaient à rien. Les vérifier un par un n'était pas du zèle,
> c'était le travail.

**Reste 139 candidats — onze capacités réellement découvertes**, dont
`NkRendererConfig::hdr` et `::vsync`, deux voisines de `pipeline` dans la même
structure.

### ⚠️ Et la règle (a) ne voit **toujours** pas ce cas-là

`vision-assumee` est interdit « dès qu'un drapeau annonce `true` à du code ». Le
détecteur ne reconnaît la promesse que si elle s'écrit littéralement `= true`.
Ici la promesse est faite par une **valeur d'énumération** différente du défaut
déclaré — `pipeline = NK_DEFERRED`. **C'est le même mensonge dans un autre
vêtement, et il passe.**

Je ne l'ai **pas** corrigé : la règle juste demanderait de comparer chaque
écriture au défaut déclaré, et je ne veux pas d'une règle que je n'ai pas
mesurée. Elle est **signalée** dans la note de `capacites.list`, et le classement
de `pipeline` y est dit **provisoire pour cette raison**.

### Le cliquet relevé de 24 à 25 — délibérément

`NkRendererConfig::vsync` **ne peut pas** être `vision-assumee` : six démos
livrées écrivent `rcfg.vsync = true`, et **le détecteur a refusé mon
classement**. Il avait raison. Les deux autres sorties étaient `implementee`
(faux) et `morte-a-retirer` (une décision produit qui n'est pas la mienne).

> **Le cliquet reposait sur une prémisse — « le stock est un héritage qui ne se
> renouvelle pas » — et elle ne tenait que tant que le détecteur voyait tout le
> stock. Il ne le voyait pas.**

Ce `+1` n'est pas une capacité nouvelle : c'est de l'héritage qui était
invisible. La décision est écrite dans `config/capacites.list`, dans la fiche
**C5** de `CAPACITES_A_DATER.md`, et dans une section dédiée de ce document-là.

⚠️ **Ce que ça dit du chiffre 24 lui-même :** il n'a jamais mesuré « les capacités
creuses du dépôt ». Il mesurait **celles que mon outil savait voir**.

---

## `NkMsaaVulkanCheck` — `ToVkSamples()` exercé, et il trouve autre chose

`NkMsaaDeviceCheck` essaie DX11, puis DX12, puis OpenGL, et **s'arrête au premier
qui s'ouvre**. Sur cette machine c'est DX11. **Le chemin Vulkan n'avait jamais
tourné** — et `NkVulkanDevice::ToVkSamples()`, l'origine écrite de toute
l'affaire, restait une déduction par lecture.

Le nouveau banc **n'a aucune chaîne de repli** : il ouvre Vulkan ou il rend `77`
(IGNORÉ). *Se replier reproduirait trait pour trait le défaut qui a laissé
`ToVkSamples` non mesuré pendant deux jours.*

**Vulkan s'ouvre ici** — il n'avait simplement jamais été demandé. Première
course, `main` à jour :

```
--- peripherique : Vulkan ---
    drapeaux MSAA : 2x=oui 4x=oui 8x=oui 16x=non   MaxSamples()=8
  [TEMOIN] obtenue.
  [OK]    1, 2, 4, 8       [FAIL]  3, 7, 16, 32, 64
=== Resultat : 4 OK / 5 FAIL ===
```

### 🔴 Les cinq FAIL ne sont **pas** tous `ToVkSamples`

Le banc n'imprime sa note `[LECTURE]` que pour **3, 7, 32, 64** — celles qui
tombent dans le `default:`. Pour **16**, il ne l'imprime pas : 16 **a** un `case`,
le nombre atteint le pilote tel quel. **Et 16 est accusé quand même.** Donc la
cause de ce FAIL-là est ailleurs, et elle est pire :

```
NkVulkanDevice.cpp:2475      mCaps.msaa2x = mCaps.msaa4x = mCaps.msaa8x = true;
NkDirectX11Device.cpp:1754   idem
NkDirectX12Device.cpp:3021   idem
NkOpenglDevice.cpp:1040-43   mCaps.msaaNx = maxS >= N     ← le seul qui MESURE
```

> **Trois backends sur quatre n'interrogent pas la carte. Ils écrivent trois
> littéraux à `true` et ne touchent JAMAIS `msaa16x`, qui reste `false` par
> défaut.** `framebufferColorSampleCounts` n'est jamais lu côté Vulkan.

`SupportsSamples()` — la fonction qui rend la question dicible **avant** qu'elle
n'atteigne un backend, le socle entier de `NkMsaaContractCheck` — repose donc, sur
trois backends sur quatre, sur **trois littéraux et un oubli**. Ce n'est pas un
contrat sur la carte : c'est une constante qui a la forme d'une réponse.

⚠️ **Sans la distinction `[LECTURE]` / pas de `[LECTURE]`, j'aurais mis les cinq
FAIL sur le dos de `ToVkSamples` et réparé la mauvaise ligne.** Un banc qui dit
**pourquoi** il accuse, cas par cas, vaut mieux qu'un banc qui compte.

### Ce que ce banc ne mesure pas, et il le dit

Il ne relit pas le `VkImage` créé. « La texture porte 1 échantillon au lieu de
32 » reste une **lecture** du code (`NkVulkanDevice.cpp:2787`, appelé ligne 1271),
étiquetée `[LECTURE]` dans sa sortie. Son **discriminant** (une valeur avec `case`
refusée + 32/64 acceptées) est imprimé comme **information**, jamais comme
verdict : il repose sur la monotonie de `framebufferColorSampleCounts`, que Vulkan
ne garantit pas — **la supposition est nommée dans la sortie elle-même**. Sur
cette carte il est sorti **NON CONCLUANT**, et ce n'est pas un succès.

**Défaut trouvé dans mon propre banc en l'écrivant** : un tableau `gAttendu`
écrit à chaque mesure et lu par personne. *Le défaut que ce banc traque, dans le
banc.* Retiré.

---

## 🔴 « Un contrôle qui n'a jamais rougi n'est pas un contrôle, c'est une intention »

Le chantier design a écrit un contrôle avec `\b` dans une chaîne Python **non
brute** — où `\b` n'est pas une frontière de mot mais le caractère **retour
arrière**. Le contrôle tournait, ne trouvait rien, et **rendait vert**.

⚠️ **C'est ma propre règle dans sa version la plus sournoise** : l'`ok` était bien
**après** l'écriture, et il ne prouvait rien — **parce qu'il portait sur zéro cas
examiné**. *« Je n'ai rien trouvé » et « je ne lis plus le fichier » rendent la
même sortie verte.*

**Donc la question « ce contrôle a-t-il déjà rougi ? » ne doit pas se poser à la
mémoire de celui qui l'a écrit. Elle se lit.**

### Trois fichiers, le dessin de `bancs.list` et de `capacites.list`

| | |
|---|---|
| `config/controles.list` | l'**inventaire** — une donnée, jamais une heuristique |
| `controles_rouges.journal` | ce qui a **été vu** rougir : date, contrôle, épreuve, `sha`, **et la ligne exacte** |
| `verif_controles.sh` | la garde, et le mode `--enregistrer <épreuve>` |

`--enregistrer` lance l'épreuve et **n'écrit une ligne que si le motif déclaré est
trouvé** dans la sortie obtenue. Motif absent → **rien n'est écrit, et c'est dit**.

⚠️ **Le motif est une chaîne fixe (`grep -F`), jamais une expression régulière.**
Une regex ici rejouerait le défaut même du chantier design : *un motif qui ne peut
pas apparaître rend vert.*

⚠️ **Et si l'épreuve elle-même échoue, rien n'est écrit.** Une épreuve ratée ne
dit pas que les contrôles n'ont pas rougi : elle dit **qu'on ne sait pas**.

### La mesure du 24/08 — les cinq épreuves lancées pour de vrai

```
45 contrôles inventoriés   —   31 ONT DÉJÀ ROUGI   —   14 JAMAIS
```

**Et les quatorze ont tous la même raison : aucune épreuve ne sait les faire
rougir.** Ce sont les **préconditions** et les **témoins d'instrument** :

```
bancs/precond-sous-modules      bancs/precond-liste-absente
bancs/instr-temoin-inventaire   bancs/verdict-absence-motif
bancs/verdict-presence-motif    bancs/sans-verdict-jamais-vert
bancs/delai-banc-muet           cap/autocontrole-format-awk
cap/precond-entete-introuvable  cap/precond-liste-absente
cap/instr-nb-motifs             cap/instr-nb-hits
cap/temoin-D1                   cap/temoin-D2
```

> ⚠️ **C'est-à-dire précisément ceux dont le métier est de distinguer « je n'ai
> rien trouvé » de « je ne lis plus le fichier ».** Aujourd'hui, personne ne peut
> distinguer « il garde » de « il ne garde plus » pour ces quatorze-là.

*(Certains ont rougi pendant leur écriture — l'autocontrôle awk a été payé trois
fois le 22/08. Mais je n'en ai **aucune trace mécanique**, et une trace qui vit
dans un souvenir n'est pas une trace. Le journal ne consigne que ce qu'une épreuve
a fait apparaître.)*

### Il ne rougit pas sur un contrôle jamais rouge, et c'est délibéré

Un contrôle jamais rouge n'est pas **cassé**, il est **suspect**. Le rougir ferait
désactiver cet outil-ci dans la semaine — **même arbitrage que pour `A-DATER` et
pour `bancs.list`**, et pour la même raison mesurée. Ce qui garde, c'est le
**cliquet** : `PLAFOND-JAMAIS-ROUGE = 14`, il peut descendre, il ne peut pas
monter. Un contrôle **neuf** se fait rougir **à la naissance**.

### Et il a sa contre-épreuve, sinon il serait ce qu'il dénonce

Un tableau de bord des contrôles jamais rouges, jamais rouge lui-même, serait
l'illustration parfaite du problème. `contre_epreuve_controles.sh` :

| | attendu | vu |
|---|---|---|
| **J** un contrôle neuf jamais rouge | `CLIQUET ROMPU`, code 5 | ✅ et le fantôme est **nommé** |
| **K** un motif introuvable | rien enregistré, et c'est dit | ✅ aucune ligne pour lui |
| **L** plus de directive de plafond | `ECHEC D'INSTRUMENT`, code 2 | ✅ jamais un vert |
| **M** après retrait | vert, arbre identique | ✅ journal **octet pour octet** |

⚠️ **Ce qu'une entrée de journal prouve, et rien de plus** : que l'**épreuve** a
affirmé avoir vu ce contrôle rougir. Sa force est celle de l'épreuve. C'est pour
cela que les cinq épreuves ont toutes un **contrôle négatif** — elles refusent de
démarrer sur un arbre déjà rouge ou déjà sale. *Sans ce refus, un `[vu]` vaudrait
ce que valait l'`ok` du chantier design.*

⚠️ Et le contrôle négatif passe **avant** le `trap … EXIT`. *Un filet armé avant
son contrôle détruit ce que le contrôle refusait d'écraser* — c'est arrivé ici le
22/08, ça a coûté 49 lignes.

---

## Passe `complet` du 24/08 — 13 bancs, 10 OK, 3 ÉCHEC, **0 IGNORÉ**

Sur `feat/verificateur` **après fusion de `main`** (`0b918850`, 8 commits, conflit
`.gitignore` résolu en gardant les deux blocs). *Remesurer sur une photo du matin
aurait produit des chiffres crédibles et faux — ma propre leçon.*

```
VERDICT : ECHEC — 3 banc(s) en echec, 10 au vert, 0 ignore(s), 0 indetermine(s)
mode complet : 13 banc(s) lance(s) sur 125 projet(s) classe(s)
capacites : toutes classees        (139 candidats, 25 dettes A-DATER)
```

| banc | verdict | à qui |
|---|---|---|
| `NkGpuProbe` | ÉCHEC | **pas à moi** — et l'outil le situe tout seul (ci-dessous) |
| `NkMsaaDeviceCheck` | ÉCHEC 4 OK / 5 FAIL | **il a raison de rester rouge** |
| `NkMsaaVulkanCheck` | ÉCHEC 4 OK / 5 FAIL | **neuf, rouge dès sa naissance, et il a raison** |
| les 10 autres | OK | |

### `NkGpuProbe` — l'outil situe, il ne tranche pas

```
code de sortie 1 : [NkRHI_DX11][ERR] DX11 shader: manque le source HLSL (stage 0)
⚠️ 1 branche(s) non fusionnee(s) touchent les fichiers accuses
   ET en different encore aujourd'hui :
   feat/rendu-temps-reel   (63 commit(s) inedit(s))
     Kernel/Runtime/NKRHI/src/NKRHI/DirectX11/NkDirectX11Device.cpp
```

**Ce rouge-là peut disparaître par une fusion.** Les deux rouges MSAA, eux,
portent *« branches non fusionnées : AUCUNE (16 examinées) — le défaut est bien
sur CETTE référence »*. **L'outil dit l'absence aussi fort que la présence**, et
c'est ce qui sépare « fusionne d'abord » de « cherche ici ».

### ⚠️ Une anomalie que je n'explique pas, et je ne l'invente pas

Sur la **première** passe complète (27 min, lancée en tâche de fond derrière un
`timeout` et un `grep | tail`), une ligne parasite est apparue :

```
./verif_bancs.sh: line 914: ire: command not found
```

…et le rapport final était remplacé, dans le flux capturé, par la section
`--liste`. **Non reproductible** : quatre relances de la même commande, avec et
sans le même tuyau, donnent le rapport complet et aucune erreur. `bash -n` passe.
Ma modification du jour sur ce fichier est **un bloc de commentaires**, vérifiable
au diff.

> **Je note l'observation sans lui donner de cause.** `ire` est `dire` amputé de sa
> première lettre, ce qui *ressemble* à un flux tronqué par l'enveloppe de la
> tâche de fond — mais « ça ressemble à » n'est pas une mesure, et *une supposition
> consignée comme mesure se propage avec l'autorité d'une mesure.* Si ça revient,
> ça se mesure ; en attendant, ça se dit.

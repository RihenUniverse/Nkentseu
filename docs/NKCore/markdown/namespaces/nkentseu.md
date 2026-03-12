# 🗂️ Namespace `nkentseu`

> 66 éléments

[🏠 Accueil](../index.md) | [🗂️ Namespaces](./index.md)

## Éléments

### 🏛️ Classs (1)

- **[`NkSourceLocation`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nksourcelocation)** — Informations de localisation dans le code source

### 🔢 Enums (1)

- **[`Value`](../files/NkTypes.h.md#nkentseu-byte-value)** — Valeurs constantes prédéfinies

### ⚙️ Functions (3)

- **[`NkSafeCast`](../files/NkTypeUtils.h.md#nkentseu-nksafecast)** — Cast sécurisé avec vérification debug
- **[`SafeConstCast`](../files/NkTypes.h.md#nkentseu-safeconstcast)** — Conversion sécurisée de ConstVoidPtr vers T*
- **[`from`](../files/NkTypes.h.md#nkentseu-from)** — Conversion sécurisée depuis un type

### 🔣 Macros (4)

- **[`NKENTSEU_CORE_DEPRECATED_API_MSG`](../files/NkCoreExport.h.md#nkentseu-nkentseu_core_deprecated_api_msg)** — API Core dépréciée avec message
- **[`NKENTSEU_GRAPHICS_NVN_AVAILABLE`](../files/NkCGXDetect.h.md#nkentseu-nkentseu_graphics_nvn_available)** — NVN (NVIDIA API) disponible sur Switch
- **[`NkConstCast`](../files/NkTypeUtils.h.md#nkentseu-nkconstcast)** — Const cast
- **[`NkCurrentSourceLocation`](../files/NkPlatform.h.md#nkentseu-nkcurrentsourcelocation)** — Capture la localisation actuelle

### 🔧 Methods (31)

- **[`Byte`](../files/NkTypes.h.md#nkentseu-byte-byte)** — Constructeur explicite
- **[`Byte`](../files/NkTypes.h.md#nkentseu-byte-byte)** — Constructeur par défaut (valeur 0)
- **[`Column`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-column)** — Récupère le numéro de colonne
- **[`FileName`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-filename)** — Constructeur par défaut
- **[`FileName`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-filename)** — Récupère le nom du fichier source
- **[`FunctionName`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-functionname)** — Récupère le nom de la fonction
- **[`Line`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-line)** — Récupère le numéro de ligne
- **[`NkAlignAddress`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkalignaddress)** — Aligne une adresse vers le haut
- **[`NkGetAllocationGranularity`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetallocationgranularity)** — Récupère la granularité d'allocation mémoire
- **[`NkGetArchitectureName`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetarchitecturename)** — Récupère le nom de l'architecture
- **[`NkGetAvailableMemory`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetavailablememory)** — Récupère la RAM disponible
- **[`NkGetBuildType`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetbuildtype)** — Récupère le type de build
- **[`NkGetCPUCoreCount`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetcpucorecount)** — Récupère le nombre de cœurs CPU physiques
- **[`NkGetCPUThreadCount`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetcputhreadcount)** — Récupère le nombre total de threads matériels
- **[`NkGetCacheLineSize`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetcachelinesize)** — Récupère la taille de ligne de cache
- **[`NkGetEndianness`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetendianness)** — Détecte l'endianness du système
- **[`NkGetL1CacheSize`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetl1cachesize)** — Récupère la taille du cache L1
- **[`NkGetL2CacheSize`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetl2cachesize)** — Récupère la taille du cache L2
- **[`NkGetL3CacheSize`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetl3cachesize)** — Récupère la taille du cache L3
- **[`NkGetPageSize`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetpagesize)** — Récupère la taille de page mémoire
- **[`NkGetPlatformInfo`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetplatforminfo)** — Fonctions pour récupérer les informations plateforme
- **[`NkGetPlatformInfo`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetplatforminfo)** — Récupère les informations complètes de la plateforme
- **[`NkGetPlatformName`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgetplatformname)** — Récupère le nom de la plateforme
- **[`NkGetTotalMemory`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkgettotalmemory)** — Récupère la RAM totale du système
- **[`NkHasSIMDFeature`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkhassimdfeature)** — Vérifie le support d'une fonctionnalité SIMD
- **[`NkInitializePlatformInfo`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkinitializeplatforminfo)** — Initialise explicitement les informations plateforme
- **[`NkIs64Bit`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkis64bit)** — Vérifie si l'architecture est 64-bit
- **[`NkIsAligned`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkisaligned)** — Vérifie si une adresse est alignée
- **[`NkIsDebugBuild`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkisdebugbuild)** — Vérifie si c'est un build debug
- **[`NkIsSharedLibrary`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkissharedlibrary)** — Vérifie si c'est une bibliothèque partagée
- **[`NkPrintPlatformInfo`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-nkprintplatforminfo)** — Affiche les informations plateforme sur stdout

### 🏗️ Structs (4)

- **[`Byte`](../files/NkTypes.h.md#nkentseu-byte-byte)** — Structure wrapper pour byte avec opérateurs * Encapsule un uint8 avec des opérations bitwise et conversions sécurisées. *
- **[`NkPlatformInfo`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nkplatforminfo)** — Informations complètes sur la plateforme runtime
- **[`NkVersionInfo`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkversioninfo)** — Informations de version (Major.Minor.Patch)
- **[`int128`](../files/NkTypes.h.md#nkentseu-int128)** — Conversion vers un type

### 📝 Typedefs (21)

- **[`Boolean`](../files/NkTypes.h.md#nkentseu-boolean)** — Booléen 8-bit (0 = false, 1 = true)
- **[`Char`](../files/NkTypes.h.md#nkentseu-char)** — Caractère par défaut du framework
- **[`NkHandle`](../files/NkTypes.h.md#nkentseu-nkhandle)** — Handle opaque (pointeur ou index)
- **[`NkHash32`](../files/NkTypes.h.md#nkentseu-nkhash32)** — Hash 32-bit explicite
- **[`NkHash64`](../files/NkTypes.h.md#nkentseu-nkhash64)** — Hash 64-bit explicite
- **[`NkHashValue`](../files/NkTypes.h.md#nkentseu-nkhashvalue)** — Type pour hash values (32-bit ou 64-bit selon arch)
- **[`NkID32`](../files/NkTypes.h.md#nkentseu-nkid32)** — Handle invalide
- **[`NkID32`](../files/NkTypes.h.md#nkentseu-nkid32)** — Identifiant unique 32-bit
- **[`NkID64`](../files/NkTypes.h.md#nkentseu-nkid64)** — Identifiant unique 64-bit
- **[`NkTimestamp`](../files/NkTypes.h.md#nkentseu-nktimestamp)** — ID invalide 64-bit
- **[`NkTimestamp`](../files/NkTypes.h.md#nkentseu-nktimestamp)** — Timestamp en microsecondes (64-bit)
- **[`PTR`](../files/NkTypes.h.md#nkentseu-ptr)** — Valeur true pour Boolean
- **[`PTR`](../files/NkTypes.h.md#nkentseu-ptr)** — Valeur false pour Boolean
- **[`bool32`](../files/NkTypes.h.md#nkentseu-bool32)** — Booléen 32-bit (pour alignement)
- **[`float32`](../files/NkTypes.h.md#nkentseu-float32)** — Flottant simple précision 32-bit
- **[`float64`](../files/NkTypes.h.md#nkentseu-float64)** — Flottant double précision 64-bit
- **[`float80`](../files/NkTypes.h.md#nkentseu-float80)** — Flottant étendu précision 80-bit
- **[`int8`](../files/NkTypes.h.md#nkentseu-int8)** — Entier signé 8-bit
- **[`uint8`](../files/NkTypes.h.md#nkentseu-uint8)** — Entier non-signé 8-bit
- **[`uintl32`](../files/NkTypes.h.md#nkentseu-uintl32)** — Entier non-signé 32-bit (long)
- **[`usize_cpu`](../files/NkTypes.h.md#nkentseu-usize_cpu)** — Conversion sécurisée de VoidPtr vers T*

### 📦 Variables (1)

- **[`file`](../files/NkPlatform.h.md#nkentseu-nkplatformtype-nkplatformtype-nkarchitecturetype-nkdisplaytype-nkgraphicsapi-nkendianness-nkversioninfo-nkplatforminfo-nksourcelocation-file)** — Crée une instance avec la localisation actuelle


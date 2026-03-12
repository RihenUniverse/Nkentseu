# 📄 NkVersion.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Versioning du framework NKCore

**Auteur:** Rihen

**Chemin:** `src\NKCore\NkVersion.h`

### 📦 Fichiers Inclus

- [`NkMacros.h`](./NkMacros.h.md)

### 🔗 Inclus Par

- [`NkConfig.h`](./NkConfig.h.md)
- [`NkMacros.h`](./NkMacros.h.md)
- [`NkPlatform.h`](./NkPlatform.h.md)

## 🎯 Éléments (13)

### 🔣 Macros (13)

<a name="nkentseu_api_experimental"></a>

#### 🔣 `NKENTSEU_API_EXPERIMENTAL`

```cpp
#define NKENTSEU_API_EXPERIMENTAL NKENTSEU_DEPRECATED_MSG("This API is experimental and may change")
```

**Macro pour marquer une API comme expérimentale**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:162`*


---

<a name="nkentseu_api_internal"></a>

#### 🔣 `NKENTSEU_API_INTERNAL`

```cpp
#define NKENTSEU_API_INTERNAL NKENTSEU_HIDDEN
```

**Macro pour marquer une API comme interne (non publique)**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:168`*


---

<a name="nkentseu_api_stable"></a>

#### 🔣 `NKENTSEU_API_STABLE`

```cpp
#define NKENTSEU_API_STABLE /**
```

**Macro pour marquer une API comme stable**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:156`*


---

<a name="nkentseu_api_version"></a>

#### 🔣 `NKENTSEU_API_VERSION`

```cpp
#define NKENTSEU_API_VERSION NkVersionEncode(NKENTSEU_API_VERSION_MAJOR, \
```

**Version API complète encodée**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:148`*


---

<a name="nkentseu_build_date"></a>

#### 🔣 `NKENTSEU_BUILD_DATE`

```cpp
#define NKENTSEU_BUILD_DATE __DATE__
```

**Date de compilation (format: "YYYY-MM-DD")**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:89`*


---

<a name="nkentseu_build_time"></a>

#### 🔣 `NKENTSEU_BUILD_TIME`

```cpp
#define NKENTSEU_BUILD_TIME __TIME__
```

**Heure de compilation (format: "HH:MM:SS")**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:95`*


---

<a name="nkentseu_build_timestamp"></a>

#### 🔣 `NKENTSEU_BUILD_TIMESTAMP`

```cpp
#define NKENTSEU_BUILD_TIMESTAMP NKENTSEU_BUILD_DATE " " NKENTSEU_BUILD_TIME
```

**Timestamp de compilation complet**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:101`*


---

<a name="nkentseu_framework_core_full_name"></a>

#### 🔣 `NKENTSEU_FRAMEWORK_CORE_FULL_NAME`

```cpp
#define NKENTSEU_FRAMEWORK_CORE_FULL_NAME NKENTSEU_FRAMEWORK_CORE_NAME " v" NKENTSEU_VERSION_CORE_STRING
```

**Nom complet avec version**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:74`*


---

<a name="nkentseu_framework_core_name"></a>

#### 🔣 `NKENTSEU_FRAMEWORK_CORE_NAME`

```cpp
#define NKENTSEU_FRAMEWORK_CORE_NAME "NKCore"
```

**Nom complet du framework**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:68`*


---

<a name="nkentseu_version_at_least"></a>

#### 🔣 `NKENTSEU_VERSION_AT_LEAST`

```cpp
#define NKENTSEU_VERSION_AT_LEAST \
```

**Macro pour vérifier la version minimale requise**

**Retour:** true si version actuelle >= version demandée

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:183`*


---

<a name="nkentseu_version_core"></a>

#### 🔣 `NKENTSEU_VERSION_CORE`

```cpp
#define NKENTSEU_VERSION_CORE NkVersionEncode(NKENTSEU_VERSION_CORE_MAJOR, NKENTSEU_VERSION_CORE_MINOR, NKENTSEU_VERSION_CORE_PATCH)
```

**Version complète encodée (32-bit)**

> 📝 **Note:** Format: 0xMMmmpppp (Major, minor, patch)

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:53`*


---

<a name="nkentseu_version_core_string"></a>

#### 🔣 `NKENTSEU_VERSION_CORE_STRING`

```cpp
#define NKENTSEU_VERSION_CORE_STRING NkStringify(NKENTSEU_VERSION_CORE_MAJOR) "." \
```

**Version sous forme de chaîne**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:60`*


---

<a name="nkentseu_version_equals"></a>

#### 🔣 `NKENTSEU_VERSION_EQUALS`

```cpp
#define NKENTSEU_VERSION_EQUALS \
```

**Macro pour vérifier version exacte**

**Retour:** true si version actuelle == version spécifiée

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkVersion.h:195`*


---


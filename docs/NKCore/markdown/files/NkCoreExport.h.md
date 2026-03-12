# 📄 NkCoreExport.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** API d'export pour le module Core de Nkentseu

**Auteur:** Rihen

**Chemin:** `src\NKCore\NkCoreExport.h`

### 📦 Fichiers Inclus

- [`NkExport.h`](./NkExport.h.md)
- [`Platform/NkCoreExport.h`](./NkCoreExport.h.md)
- [`Platform/NkTypes.h`](./NkTypes.h.md)

### 🔗 Inclus Par

- [`NkCoreExport.h`](./NkCoreExport.h.md)
- [`NkPlatform.h`](./NkPlatform.h.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`core`](../namespaces/core.md)

## 🎯 Éléments (15)

### 🔣 Macros (15)

<a name="nkentseu_container_api"></a>

#### 🔣 `NKENTSEU_CONTAINER_API`

```cpp
#define NKENTSEU_CONTAINER_API NKENTSEU_CORE_API
```

**API pour les containers**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:141`*


---

<a name="nkentseu_core_c_api"></a>

#### 🔣 `NKENTSEU_CORE_C_API`

```cpp
#define NKENTSEU_CORE_C_API NKENTSEU_EXTERN_C NKENTSEU_CORE_API NKENTSEU_CALL
```

**API C du module Core**

**Exemples:**

```cpp
* NKENTSEU_CORE_C_API void* nkCoreAllocateMemory(nk_size size);
 * NKENTSEU_CORE_C_API void nkCoreFreeMemory(void* ptr);
 *
```

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:88`*


---

<a name="nkentseu_core_deprecated_api"></a>

#### 🔣 `NKENTSEU_CORE_DEPRECATED_API`

```cpp
#define NKENTSEU_CORE_DEPRECATED_API NKENTSEU_DEPRECATED NKENTSEU_CORE_API
```

**API Core dépréciée**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:240`*


---

<a name="nkentseu-nkentseu_core_deprecated_api_msg"></a>

#### 🔣 `NKENTSEU_CORE_DEPRECATED_API_MSG`

```cpp
#define NKENTSEU_CORE_DEPRECATED_API_MSG NKENTSEU_DEPRECATED_MSG(msg) NKENTSEU_CORE_API
```

**API Core dépréciée avec message**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:247`*


---

<a name="nkentseu_core_internal"></a>

#### 🔣 `NKENTSEU_CORE_INTERNAL`

```cpp
#define NKENTSEU_CORE_INTERNAL NKENTSEU_SYMBOL_INTERNAL
```

**Symbole visible mais non exporté**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:199`*


---

<a name="nkentseu_core_private"></a>

#### 🔣 `NKENTSEU_CORE_PRIVATE`

```cpp
#define NKENTSEU_CORE_PRIVATE NKENTSEU_SYMBOL_HIDDEN
```

**Symboles privés du module Core**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:112`*


---

<a name="nkentseu_core_public"></a>

#### 🔣 `NKENTSEU_CORE_PUBLIC`

```cpp
#define NKENTSEU_CORE_PUBLIC NKENTSEU_CORE_API
```

**Symboles publics du module Core**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:103`*


---

<a name="nkentseu_core_template"></a>

#### 🔣 `NKENTSEU_CORE_TEMPLATE`

```cpp
#define NKENTSEU_CORE_TEMPLATE NKENTSEU_CORE_API
```

**Pour les fonctions templates exportées**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:226`*


---

<a name="nkentseu_error_api"></a>

#### 🔣 `NKENTSEU_ERROR_API`

```cpp
#define NKENTSEU_ERROR_API NKENTSEU_CORE_API
```

**API pour la gestion des erreurs**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:186`*


---

<a name="nkentseu_log_api"></a>

#### 🔣 `NKENTSEU_LOG_API`

```cpp
#define NKENTSEU_LOG_API NKENTSEU_CORE_API
```

**API pour le logging**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:159`*


---

<a name="nkentseu_memory_api"></a>

#### 🔣 `NKENTSEU_MEMORY_API`

```cpp
#define NKENTSEU_MEMORY_API NKENTSEU_CORE_API
```

**API pour la gestion de mémoire**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:132`*


---

<a name="nkentseu_system_api"></a>

#### 🔣 `NKENTSEU_SYSTEM_API`

```cpp
#define NKENTSEU_SYSTEM_API NKENTSEU_CORE_API
```

**API pour les utilitaires système**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:177`*


---

<a name="nkentseu_thread_api"></a>

#### 🔣 `NKENTSEU_THREAD_API`

```cpp
#define NKENTSEU_THREAD_API NKENTSEU_CORE_API
```

**API pour la gestion des threads**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:168`*


---

<a name="nkentseu_type_api"></a>

#### 🔣 `NKENTSEU_TYPE_API`

```cpp
#define NKENTSEU_TYPE_API NKENTSEU_CORE_API
```

**API pour le système de types**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:150`*


---

<a name="nk_core_api"></a>

#### 🔣 `NK_CORE_API`

```cpp
#define NK_CORE_API NKENTSEU_CORE_API
```

**Alias court pour NKENTSEU_CORE_API**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkCoreExport.h:121`*


---


# 📄 NkExport.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Système d'export/import multiplateforme pour bibliothèques Nkentseu

**Auteur:** Rihen

**Chemin:** `src\NKCore\NkExport.h`

### 📦 Fichiers Inclus

- `NkPlatformDetect.h`
- `NkArchDetect.h`
- [`NkExport.h`](./NkExport.h.md)
- [`NkExport.h`](./NkExport.h.md)

### 🔗 Inclus Par

- [`NkAssertHandler.h`](./NkAssertHandler.h.md)
- [`NkAtomic.h`](./NkAtomic.h.md)
- [`NkBits.h`](./NkBits.h.md)
- [`NkCoreExport.h`](./NkCoreExport.h.md)
- [`NkExport.h`](./NkExport.h.md)
- [`NkGraphicsExport.h`](./NkGraphicsExport.h.md)
- [`NkLimits.h`](./NkLimits.h.md)
- [`NkPlatform.h`](./NkPlatform.h.md)

## 🎯 Éléments (24)

### 🔣 Macros (24)

<a name="nkentseu_cdecl"></a>

#### 🔣 `NKENTSEU_CDECL`

```cpp
#define NKENTSEU_CDECL __cdecl
```

**Convention d'appel C standard**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:181`*


---

<a name="nkentseu_concat3_impl"></a>

#### 🔣 `NKENTSEU_CONCAT3_IMPL`

```cpp
#define NKENTSEU_CONCAT3_IMPL a ## b ## c
```

**Concaténation de 3 tokens**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:368`*


---

<a name="nkentseu_concat_impl"></a>

#### 🔣 `NKENTSEU_CONCAT_IMPL`

```cpp
#define NKENTSEU_CONCAT_IMPL a ## b
```

**Concaténation de tokens (niveau 1)**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:361`*


---

<a name="nkentseu_core_api"></a>

#### 🔣 `NKENTSEU_CORE_API`

```cpp
#define NKENTSEU_CORE_API NKENTSEU_API
```

**Alias court pour NKENTSEU_API**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:465`*


---

<a name="nkentseu_c_api"></a>

#### 🔣 `NKENTSEU_C_API`

```cpp
#define NKENTSEU_C_API NKENTSEU_EXTERN_C NKENTSEU_API NKENTSEU_CALL
```

**API C de Nkentseu**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:444`*


---

<a name="nkentseu_define_module_api"></a>

#### 🔣 `NKENTSEU_DEFINE_MODULE_API`

```cpp
#define NKENTSEU_DEFINE_MODULE_API \
```

**Macro pour définir une API de module**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:287`*


---

<a name="nkentseu_deprecated"></a>

#### 🔣 `NKENTSEU_DEPRECATED`

```cpp
#define NKENTSEU_DEPRECATED [[deprecated]]
```

**Marque une API comme dépréciée**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:524`*


---

<a name="nkentseu_deprecated_api"></a>

#### 🔣 `NKENTSEU_DEPRECATED_API`

```cpp
#define NKENTSEU_DEPRECATED_API NKENTSEU_DEPRECATED NKENTSEU_API
```

**Marque une API publique comme dépréciée**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:541`*


---

<a name="nkentseu_extern_c"></a>

#### 🔣 `NKENTSEU_EXTERN_C`

```cpp
#define NKENTSEU_EXTERN_C extern "C"
```

**Linkage C**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:250`*


---

<a name="nkentseu_fastcall"></a>

#### 🔣 `NKENTSEU_FASTCALL`

```cpp
#define NKENTSEU_FASTCALL __fastcall
```

**Convention d'appel optimisée**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:193`*


---

<a name="nkentseu_if"></a>

#### 🔣 `NKENTSEU_IF`

```cpp
#define NKENTSEU_IF NKENTSEU_IF_IMPL(COND, THEN, ELSE)
```

**Macro conditionnelle simple**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:389`*


---

<a name="nkentseu_private"></a>

#### 🔣 `NKENTSEU_PRIVATE`

```cpp
#define NKENTSEU_PRIVATE NKENTSEU_SYMBOL_HIDDEN
```

**Symboles privés de Nkentseu**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:458`*


---

<a name="nkentseu_public"></a>

#### 🔣 `NKENTSEU_PUBLIC`

```cpp
#define NKENTSEU_PUBLIC NKENTSEU_API
```

**Symboles publics de Nkentseu**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:451`*


---

<a name="nkentseu_stdcall"></a>

#### 🔣 `NKENTSEU_STDCALL`

```cpp
#define NKENTSEU_STDCALL __stdcall
```

**Convention d'appel Windows standard**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:187`*


---

<a name="nkentseu_wasm_export"></a>

#### 🔣 `NKENTSEU_WASM_EXPORT`

```cpp
#define NKENTSEU_WASM_EXPORT __attribute__((export_name(#name)))
```

**Export WebAssembly avec nom**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:482`*


---

<a name="nkentseu_wasm_import"></a>

#### 🔣 `NKENTSEU_WASM_IMPORT`

```cpp
#define NKENTSEU_WASM_IMPORT __attribute__((import_name(#name)))
```

**Import WebAssembly avec nom**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:488`*


---

<a name="nkentseu_wasm_keep"></a>

#### 🔣 `NKENTSEU_WASM_KEEP`

```cpp
#define NKENTSEU_WASM_KEEP __attribute__((used))
```

**Garde un symbole WebAssembly**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:494`*


---

<a name="nkentseu_wasm_main"></a>

#### 🔣 `NKENTSEU_WASM_MAIN`

```cpp
#define NKENTSEU_WASM_MAIN __attribute__((export_name("main")))
```

**Définit la fonction main pour WebAssembly**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:500`*


---

<a name="_nkentseu_define_module_c_api"></a>

#### 🔣 `_NKENTSEU_DEFINE_MODULE_C_API`

```cpp
#define _NKENTSEU_DEFINE_MODULE_C_API \
```

**Définit la macro d'export C du module**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:339`*


---

<a name="_nkentseu_define_module_export"></a>

#### 🔣 `_NKENTSEU_DEFINE_MODULE_EXPORT`

```cpp
#define _NKENTSEU_DEFINE_MODULE_EXPORT \
```

**Définit la macro d'export principale du module**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:326`*


---

<a name="_nkentseu_define_module_visibility"></a>

#### 🔣 `_NKENTSEU_DEFINE_MODULE_VISIBILITY`

```cpp
#define _NKENTSEU_DEFINE_MODULE_VISIBILITY \
```

**Définit les macros de visibilité du module**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:347`*


---

<a name="_nkentseu_detect_module_context"></a>

#### 🔣 `_NKENTSEU_DETECT_MODULE_CONTEXT`

```cpp
#define _NKENTSEU_DETECT_MODULE_CONTEXT \
```

**Détecte si on build ou utilise un module**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:315`*


---

<a name="_nkentseu_if_building"></a>

#### 🔣 `_NKENTSEU_IF_BUILDING`

```cpp
#define _NKENTSEU_IF_BUILDING \
```

**Conditionnel pour build de module**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:375`*


---

<a name="_nkentseu_if_shared"></a>

#### 🔣 `_NKENTSEU_IF_SHARED`

```cpp
#define _NKENTSEU_IF_SHARED \
```

**Conditionnel pour build partagé**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkExport.h:382`*


---


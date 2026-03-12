# 📄 NkCompilerDetect.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Détection compilateur et features C++

**Auteur:** Rihen

**Chemin:** `src\NKPlatform\NkCompilerDetect.h`

### 🔗 Inclus Par

- [`NkPlatformConfig.h`](./NkPlatformConfig.h.md)

## 🎯 Éléments (19)

### 🔣 Macros (18)

<a name="nkentseu_compiler_emscripten"></a>

#### 🔣 `NKENTSEU_COMPILER_EMSCRIPTEN`

```cpp
#define NKENTSEU_COMPILER_EMSCRIPTEN #endif
```

**Détection Emscripten**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:124`*


---

<a name="nkentseu_compiler_gcc"></a>

#### 🔣 `NKENTSEU_COMPILER_GCC`

```cpp
#define NKENTSEU_COMPILER_GCC #define NKENTSEU_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
```

**Détection compilateur GCC**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:102`*


---

<a name="nkentseu_compiler_intel"></a>

#### 🔣 `NKENTSEU_COMPILER_INTEL`

```cpp
#define NKENTSEU_COMPILER_INTEL #define NKENTSEU_COMPILER_VERSION __INTEL_COMPILER
```

**Détection compilateur Intel**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:113`*


---

<a name="nkentseu_compiler_nvcc"></a>

#### 🔣 `NKENTSEU_COMPILER_NVCC`

```cpp
#define NKENTSEU_COMPILER_NVCC #endif
```

**Détection NVCC (CUDA)**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:134`*


---

<a name="nkentseu_compiler_sunpro"></a>

#### 🔣 `NKENTSEU_COMPILER_SUNPRO`

```cpp
#define NKENTSEU_COMPILER_SUNPRO #endif
```

**Détection SunPro**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:144`*


---

<a name="nkentseu_compiler_xlc"></a>

#### 🔣 `NKENTSEU_COMPILER_XLC`

```cpp
#define NKENTSEU_COMPILER_XLC #endif
```

**Détection IBM XL**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:154`*


---

<a name="nkentseu_cpp11"></a>

#### 🔣 `NKENTSEU_CPP11`

```cpp
#define NKENTSEU_CPP11 #define NKENTSEU_CPP_VERSION 11
```

**Standard C++11 ou supérieur**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:206`*


---

<a name="nkentseu_cpp14"></a>

#### 🔣 `NKENTSEU_CPP14`

```cpp
#define NKENTSEU_CPP14 #define NKENTSEU_CPP_VERSION 14
```

**Standard C++14 ou supérieur**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:198`*


---

<a name="nkentseu_cpp17"></a>

#### 🔣 `NKENTSEU_CPP17`

```cpp
#define NKENTSEU_CPP17 #define NKENTSEU_CPP_VERSION 17
```

**Standard C++17 ou supérieur**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:190`*


---

<a name="nkentseu_cpp23"></a>

#### 🔣 `NKENTSEU_CPP23`

```cpp
#define NKENTSEU_CPP23 #define NKENTSEU_CPP_VERSION 23
```

**Standard C++23 ou supérieur**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:172`*


---

<a name="nkentseu_cpp98"></a>

#### 🔣 `NKENTSEU_CPP98`

```cpp
#define NKENTSEU_CPP98 #define NKENTSEU_CPP_VERSION 98
```

**Standard C++98/C++03**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:214`*


---

<a name="nkentseu_file_name"></a>

#### 🔣 `NKENTSEU_FILE_NAME`

```cpp
#define NKENTSEU_FILE_NAME __FILE__
```

**Fichier source actuel**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:588`*


---

<a name="nkentseu_has_cpp11"></a>

#### 🔣 `NKENTSEU_HAS_CPP11`

```cpp
#define NKENTSEU_HAS_CPP11 #define NKENTSEU_HAS_NULLPTR
```

**Support général C++11**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:235`*


---

<a name="nkentseu_has_cpp14"></a>

#### 🔣 `NKENTSEU_HAS_CPP14`

```cpp
#define NKENTSEU_HAS_CPP14 #define NKENTSEU_HAS_GENERIC_LAMBDAS
```

**Support général C++14**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:266`*


---

<a name="nkentseu_has_cpp17"></a>

#### 🔣 `NKENTSEU_HAS_CPP17`

```cpp
#define NKENTSEU_HAS_CPP17 #define NKENTSEU_HAS_INLINE_VARIABLES
```

**Support général C++17**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:289`*


---

<a name="nkentseu_has_cpp20"></a>

#### 🔣 `NKENTSEU_HAS_CPP20`

```cpp
#define NKENTSEU_HAS_CPP20 #define NKENTSEU_HAS_CONCEPTS
```

**Support général C++20**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:315`*


---

<a name="nkentseu_has_cpp23"></a>

#### 🔣 `NKENTSEU_HAS_CPP23`

```cpp
#define NKENTSEU_HAS_CPP23 #define NKENTSEU_HAS_DEDUCING_THIS
```

**Support général C++23**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:341`*


---

<a name="nkentseu_line_number"></a>

#### 🔣 `NKENTSEU_LINE_NUMBER`

```cpp
#define NKENTSEU_LINE_NUMBER __LINE__
```

**Ligne source actuelle**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:595`*


---

### 📝 Typedefs (1)

<a name="nkentseu_int128"></a>

#### 📝 `NKENTSEU_int128`

```cpp
typedef __int128 NKENTSEU_int128
```

**Support 128-bit integers**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCompilerDetect.h:629`*


---


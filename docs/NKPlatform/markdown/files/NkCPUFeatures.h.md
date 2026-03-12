# 📄 NkCPUFeatures.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Advanced CPU feature detection (SIMD, cache, performance)

**Auteur:** Rihen

**Chemin:** `src\NKPlatform\NkCPUFeatures.h`

### 📦 Fichiers Inclus

- `../Core/NkTypes.h`
- `../Core/NkString.h`
- `NkArchDetect.h`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`platform`](../namespaces/platform.md)

## 🎯 Éléments (5)

### 🏗️ Structs (5)

<a name="cacheinfo-cputopology-simdfeatures-extendedfeatures-cpufeatures-cpufeatures"></a>

#### 🏗️ `CPUFeatures`

```cpp
struct CPUFeatures
```

**Complete CPU features structure * Détecte et stocke toutes les capacités CPU disponibles. Utilise CPUID sur x86/x64 et runtime checks sur ARM. ***

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCPUFeatures.h:105`*


---

<a name="cacheinfo-cputopology-cputopology"></a>

#### 🏗️ `CPUTopology`

```cpp
struct CPUTopology
```

**CPU topology information**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCPUFeatures.h:34`*


---

<a name="cacheinfo-cacheinfo"></a>

#### 🏗️ `CacheInfo`

```cpp
struct CacheInfo
```

**CPU cache information**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCPUFeatures.h:21`*


---

<a name="cacheinfo-cputopology-simdfeatures-extendedfeatures-extendedfeatures"></a>

#### 🏗️ `ExtendedFeatures`

```cpp
struct ExtendedFeatures
```

**Other CPU features**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCPUFeatures.h:75`*


---

<a name="cacheinfo-cputopology-simdfeatures-simdfeatures"></a>

#### 🏗️ `SIMDFeatures`

```cpp
struct SIMDFeatures
```

**SIMD instruction set support**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkCPUFeatures.h:46`*


---


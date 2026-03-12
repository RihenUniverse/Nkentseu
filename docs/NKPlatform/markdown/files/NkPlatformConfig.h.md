# 📄 NkPlatformConfig.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Platform configuration and feature detection

**Auteur:** Rihen

**Chemin:** `src\NKPlatform\NkPlatformConfig.h`

### 📦 Fichiers Inclus

- `NkPlatformDetect.h`
- `NkArchDetect.h`
- [`NkCompilerDetect.h`](./NkCompilerDetect.h.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`platform`](../namespaces/platform.md)

## 🎯 Éléments (9)

### 🔧 Methods (6)

<a name="platformconfig-platformcapabilities-getarchname"></a>

#### 🔧 `GetArchName`

```cpp
char* GetArchName()
```

**Get architecture name string**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:291`*


---

<a name="platformconfig-platformcapabilities-getcompilername"></a>

#### 🔧 `GetCompilerName`

```cpp
char* GetCompilerName()
```

**Get compiler name string**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:310`*


---

<a name="platformconfig-platformcapabilities-getplatformcapabilities"></a>

#### 🔧 `GetPlatformCapabilities`

```cpp
PlatformCapabilities& GetPlatformCapabilities()
```

**Get platform capabilities (runtime detection)**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:88`*


---

<a name="platformconfig-platformcapabilities-getplatformname"></a>

#### 🔧 `GetPlatformName`

```cpp
char* GetPlatformName()
```

**Get platform name string**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:270`*


---

<a name="platformconfig-platformcapabilities-is64bit"></a>

#### 🔧 `Is64Bit`

```cpp
inline bool Is64Bit()
```

**Check if running on 64-bit architecture**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:325`*


---

<a name="platformconfig-islittleendian"></a>

#### 🔧 `IsLittleEndian`

```cpp
inline bool IsLittleEndian()
```

**Check if running on little-endian system**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:336`*


---

### 🏗️ Structs (3)

<a name="platformconfig-platformcapabilities-platformcapabilities"></a>

#### 🏗️ `PlatformCapabilities`

```cpp
struct PlatformCapabilities
```

**Get platform configuration**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:55`*


---

<a name="platformconfig-platformcapabilities-platformcapabilities-platformcapabilities"></a>

#### 🏗️ `PlatformCapabilities`

```cpp
struct PlatformCapabilities
```

**Platform capabilities**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:60`*


---

<a name="platformconfig-platformconfig"></a>

#### 🏗️ `PlatformConfig`

```cpp
struct PlatformConfig
```

**Platform configuration structure**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKPlatform\src\NKPlatform\NkPlatformConfig.h:26`*


---


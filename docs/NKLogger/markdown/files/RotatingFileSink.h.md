# 📄 RotatingFileSink.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Sink pour fichiers avec rotation basée sur la taille.

**Auteur:** Rihen

**Chemin:** `src\Logger\Sinks\RotatingFileSink.h`

### 📦 Fichiers Inclus

- [`Logger/Sinks/FileSink.h`](./FileSink.h.md)

### 🔗 Inclus Par

- [`RotatingFileSink.cpp`](./RotatingFileSink.cpp.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`logger`](../namespaces/logger.md)

## 🎯 Éléments (6)

### ⚙️ Functions (6)

<a name="getmaxsize"></a>

#### ⚙️ `GetMaxSize`

`const`

```cpp
size_t GetMaxSize() const
```

**Obtient la taille maximum des fichiers**

**Retour:** Taille en octets

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\RotatingFileSink.h:60`*


---

<a name="log"></a>

#### ⚙️ `Log`

```cpp
void Log(const LogMessage& message) override
```

**Constructeur avec configuration de rotation**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `message` | `const LogMessage&` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\RotatingFileSink.h:28`*


---

<a name="log"></a>

#### ⚙️ `Log`

```cpp
void Log(const LogMessage& message) override
```

**Destructeur**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `message` | `const LogMessage&` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\RotatingFileSink.h:36`*


---

<a name="log"></a>

#### ⚙️ `Log`

```cpp
void Log(const LogMessage& message) override
```

**Logge un message avec vérification de rotation**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `message` | `const LogMessage&` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\RotatingFileSink.h:45`*


---

<a name="setmaxfiles"></a>

#### ⚙️ `SetMaxFiles`

```cpp
void SetMaxFiles(size_t maxFiles)
```

**Définit le nombre maximum de fichiers**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `maxFiles` | `size_t` | [in] Nombre maximum |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\RotatingFileSink.h:66`*


---

<a name="setmaxsize"></a>

#### ⚙️ `SetMaxSize`

```cpp
void SetMaxSize(size_t maxSize)
```

**Définit la taille maximum des fichiers**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `maxSize` | `size_t` | [in] Taille en octets |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\RotatingFileSink.h:54`*


---


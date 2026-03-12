# 📄 LogLevel.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Définition des niveaux de log et fonctions utilitaires.

**Auteur:** Rihen

**Chemin:** `src\Logger\LogLevel.h`

### 📦 Fichiers Inclus

- `string`
- `NKCore/NkTypes.h`

### 🔗 Inclus Par

- [`Formatter.cpp`](./Formatter.cpp.md)
- [`Logger.h`](./Logger.h.md)
- [`LogLevel.cpp`](./LogLevel.cpp.md)
- [`LogMessage.h`](./LogMessage.h.md)
- [`ConsoleSink.cpp`](./ConsoleSink.cpp.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`logger`](../namespaces/logger.md)

## 🎯 Éléments (8)

### ⚙️ Functions (8)

<a name="logleveltoansicolor"></a>

#### ⚙️ `LogLevelToANSIColor`

```cpp
char* LogLevelToANSIColor(LogLevel level)
```

**Obtient la couleur ANSI associée à un niveau de log**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `level` | `LogLevel` | [in] Niveau de log |

**Retour:** Code couleur ANSI (pour console)

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:81`*


---

<a name="logleveltoshortstring"></a>

#### ⚙️ `LogLevelToShortString`

```cpp
char* LogLevelToShortString(LogLevel level)
```

**Convertit un LogLevel en chaîne courte (3 caractères)**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `level` | `LogLevel` | [in] Niveau de log à convertir |

**Retour:** Chaîne courte représentant le niveau de log

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:60`*


---

<a name="logleveltostring"></a>

#### ⚙️ `LogLevelToString`

```cpp
char* LogLevelToString(LogLevel level)
```

**Convertit un LogLevel en chaîne de caractères**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `level` | `LogLevel` | [in] Niveau de log à convertir |

**Retour:** Chaîne représentant le niveau de log

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:53`*


---

<a name="logleveltostring"></a>

#### ⚙️ `LogLevelToString`

```cpp
char* LogLevelToString(LogLevel level)
```

**Niveau fatal - messages fatals (arrêt de l'application)**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `level` | `LogLevel` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:42`*


---

<a name="logleveltostring"></a>

#### ⚙️ `LogLevelToString`

```cpp
char* LogLevelToString(LogLevel level)
```

**Désactivation complète du logging**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `level` | `LogLevel` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:45`*


---

<a name="logleveltowindowscolor"></a>

#### ⚙️ `LogLevelToWindowsColor`

```cpp
uint16 LogLevelToWindowsColor(LogLevel level)
```

**Obtient la couleur Windows associée à un niveau de log**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `level` | `LogLevel` | [in] Niveau de log |

**Retour:** Code couleur Windows (pour console Windows)

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:88`*


---

<a name="shortstringtologlevel"></a>

#### ⚙️ `ShortStringToLogLevel`

```cpp
LogLevel ShortStringToLogLevel(const char* str)
```

**Convertit une chaîne courte en LogLevel**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `str` | `const char*` | [in] Chaîne courte à convertir |

**Retour:** LogLevel correspondant, LogLevel::Info par défaut

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:74`*


---

<a name="stringtologlevel"></a>

#### ⚙️ `StringToLogLevel`

```cpp
LogLevel StringToLogLevel(const char* str)
```

**Convertit une chaîne en LogLevel**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `str` | `const char*` | [in] Chaîne à convertir |

**Retour:** LogLevel correspondant, LogLevel::Info par défaut

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogLevel.h:67`*


---


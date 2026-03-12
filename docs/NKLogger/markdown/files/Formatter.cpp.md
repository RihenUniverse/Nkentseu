# 📄 Formatter.cpp

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Implémentation du formatter de messages de log.

**Auteur:** Rihen

**Chemin:** `src\Logger\Formatter.cpp`

### 📦 Fichiers Inclus

- [`Logger/Formatter.h`](./Formatter.h.md)
- [`Logger/LogLevel.h`](./LogLevel.h.md)
- `sstream`
- `iomanip`
- `iostream`
- `ctime`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`logger`](../namespaces/logger.md)

## 🎯 Éléments (7)

### ⚙️ Functions (5)

<a name="format"></a>

#### ⚙️ `Format`

```cpp
return Format(message , false )
```

**Obtient le pattern courant**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `message` |  |
| `` | `false` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:73`*


---

<a name="format"></a>

#### ⚙️ `Format`

```cpp
return Format(message , false )
```

**Formate un message de log**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `message` |  |
| `` | `false` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:80`*


---

<a name="logleveltoansicolor"></a>

#### ⚙️ `LogLevelToANSIColor`

```cpp
return LogLevelToANSIColor(level )
```

**Formate un nombre avec padding**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `level` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:308`*


---

<a name="logleveltoansicolor"></a>

#### ⚙️ `LogLevelToANSIColor`

```cpp
return LogLevelToANSIColor(level )
```

**Obtient le code couleur ANSI pour un niveau de log**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `level` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:317`*


---

<a name="for"></a>

#### ⚙️ `for`

```cpp
performance for(const auto& token : m_Tokens)
```

**Formate un message de log avec des couleurs**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `m_Tokens` | `const auto& token :` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:87`*


---

### 📦 Variables (2)

<a name="i"></a>

#### 📦 `i`

```cpp
size_t i
```

**Parse le pattern en tokens**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:105`*


---

<a name="tm"></a>

#### 📦 `tm`

```cpp
auto tm
```

**Formate un token individuel**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Formatter.cpp:168`*


---


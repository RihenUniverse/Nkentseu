# 📄 LogMessage.cpp

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Implémentation de la structure LogMessage.

**Auteur:** Rihen

**Chemin:** `src\Logger\LogMessage.cpp`

### 📦 Fichiers Inclus

- [`Logger/LogMessage.h`](./LogMessage.h.md)
- `chrono`
- `ctime`
- `thread`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`logger`](../namespaces/logger.md)

## 🎯 Éléments (2)

### ⚙️ Functions (2)

<a name="gmtime_s"></a>

#### ⚙️ `gmtime_s`

```cpp
_WIN32 gmtime_s(&utcTime , &time )
```

**Obtient l'heure sous forme de structure tm (UTC)**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `&utcTime` |  |
| `` | `&time` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogMessage.cpp:110`*


---

<a name="localtime_s"></a>

#### ⚙️ `localtime_s`

```cpp
_WIN32 localtime_s(&localTime , &time )
```

**Obtient l'heure sous forme de structure tm**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `&localTime` |  |
| `` | `&time` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\LogMessage.cpp:94`*


---


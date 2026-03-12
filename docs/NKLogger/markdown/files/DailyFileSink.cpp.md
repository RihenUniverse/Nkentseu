# 📄 DailyFileSink.cpp

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Implémentation du sink avec rotation quotidienne.

**Auteur:** Rihen

**Chemin:** `src\Logger\Sinks\DailyFileSink.cpp`

### 📦 Fichiers Inclus

- [`Logger/Sinks/DailyFileSink.h`](./DailyFileSink.h.md)
- `filesystem`
- `sstream`
- `iomanip`
- `ctime`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`logger`](../namespaces/logger.md)

## 🎯 Éléments (7)

### ⚙️ Functions (3)

<a name="localtime_s"></a>

#### ⚙️ `localtime_s`

```cpp
_WIN32 localtime_s(&m_CurrentDate , &now )
```

**Constructeur avec configuration**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `&m_CurrentDate` |  |
| `` | `&now` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:17`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Obtient l'heure de rotation**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:58`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Obtient le nombre maximum de jours à conserver**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:82`*


---

### 📦 Variables (4)

<a name="m_maxdays"></a>

#### 📦 `m_MaxDays`

```cpp
return m_MaxDays
```

**Définit le nombre maximum de jours à conserver**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:74`*


---

<a name="m_rotationminute"></a>

#### 📦 `m_RotationMinute`

```cpp
return m_RotationMinute
```

**Obtient la minute de rotation**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:66`*


---

<a name="now"></a>

#### 📦 `now`

```cpp
auto now
```

**Vérifie et effectue la rotation si nécessaire**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:98`*


---

<a name="rotatedfile"></a>

#### 📦 `rotatedFile`

```cpp
string rotatedFile
```

**Effectue la rotation quotidienne**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Sinks\DailyFileSink.cpp:133`*


---


# 📄 Logger.cpp

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Implémentation de la classe Logger principale.

**Auteur:** Rihen

**Chemin:** `src\Logger\Logger.cpp`

### 📦 Fichiers Inclus

- [`Logger/Logger.h`](./Logger.h.md)
- [`Logger/LogMessage.h`](./LogMessage.h.md)
- `cstdarg`
- `chrono`
- `iostream`
- `thread`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`logger`](../namespaces/logger.md)

## 🎯 Éléments (15)

### ⚙️ Functions (9)

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Destructeur du logger**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:36`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Ajoute un sink au logger**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:43`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Supprime tous les sinks du logger**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:52`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Obtient le nombre de sinks attachés**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

**Retour:** Nombre de sinks

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:60`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Définit le pattern de formatage**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:78`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Obtient le niveau de log courant**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

**Retour:** Niveau de log

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:98`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Log fatal avec stream style**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:386`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Force le flush de tous les sinks**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:394`*


---

<a name="lock"></a>

#### ⚙️ `lock`

```cpp
lock_guard<std::mutex> lock(m_Mutex )
```

**Active ou désactive le logger**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `` | `m_Mutex` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:422`*


---

### 📦 Variables (6)

<a name="argscopy"></a>

#### 📦 `argsCopy`

```cpp
va_list argsCopy
```

**Formatage variadique**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:166`*


---

<a name="false"></a>

#### 📦 `false`

```cpp
return false
```

**Vérifie si un niveau devrait être loggé**

**Retour:** true si le niveau est >= niveau minimum, false sinon

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:107`*


---

<a name="m_enabled"></a>

#### 📦 `m_Enabled`

```cpp
return m_Enabled
```

**Vérifie si le logger est actif**

**Retour:** true si actif, false sinon

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:414`*


---

<a name="message"></a>

#### 📦 `message`

```cpp
string message
```

**Log avec message string et informations de source**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:214`*


---

<a name="message"></a>

#### 📦 `message`

```cpp
string message
```

**Log avec format string, informations de source et va_list**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:222`*


---

<a name="msg"></a>

#### 📦 `msg`

```cpp
LogMessage msg
```

**Log interne avec informations de source**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKLogger\src\Logger\Logger.cpp:117`*


---


# 📄 NkGamepadSystem.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\NkGamepadSystem.h`

### 📦 Fichiers Inclus

- `Events/NkGamepadEvents.h`
- `functional`
- `memory`
- `array`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (17)

### 🏛️ Classs (3)

<a name="inkgamepadbackend-inkgamepadbackend"></a>

#### 🏛️ `INkGamepadBackend`

```cpp
class INkGamepadBackend
```

**Platform backend interface for gamepad polling and control.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:61`*


---

<a name="inkgamepadbackend-nkgamepadsystem-nkgamepadsystem"></a>

#### 🏛️ `NkGamepadSystem`

```cpp
class NkGamepadSystem
```

**Cross-platform gamepad system facade. * PollGamepads() updates backend state, emits callbacks and injects NK_GAMEPAD_* events into the EventSystem queue.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:102`*


---

<a name="nkgamepadsystem-nkgamepadsystem-nkgamepadsystem"></a>

#### 🏛️ `NkGamepadSystem`

```cpp
class NkGamepadSystem
```

**Gyro/accéléromètre disponible ?**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:94`*


---

### ⚙️ Functions (4)

<a name="getconnectedcount"></a>

#### ⚙️ `GetConnectedCount`

`const` `virtual`

```cpp
virtual NkU32 GetConnectedCount() const = 0
```

**Nombre de manettes actuellement connectées.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:77`*


---

<a name="getinfo"></a>

#### ⚙️ `GetInfo`

`const`

```cpp
NkGamepadInfo& GetInfo(NkU32 idx) const = 0
```

**Infos sur la manette à l'indice idx (0-based).**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `idx` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:80`*


---

<a name="getstate"></a>

#### ⚙️ `GetState`

`const`

```cpp
NkGamepadStateData& GetState(NkU32 idx) const = 0
```

**Snapshot complet de l'état courant.**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `idx` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:83`*


---

<a name="rumble"></a>

#### ⚙️ `Rumble`

`virtual`

```cpp
virtual void Rumble(NkU32 idx, float motorLow, float motorHigh, float triggerLeft, float triggerRight, NkU32 durationMs) = 0
```

**Lance une vibration. Implémentation peut ignorer si non supporté.**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `idx` | `NkU32` |  |
| `motorLow` | `float` |  |
| `motorHigh` | `float` |  |
| `triggerLeft` | `float` |  |
| `triggerRight` | `float` |  |
| `durationMs` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:86`*


---

### 🔧 Methods (5)

<a name="nkgamepadsystem-nkgamepadsystem-getconnectedcount"></a>

#### 🔧 `GetConnectedCount`

`const`

```cpp
NkU32 GetConnectedCount() const = 0
```

**Number of connected gamepads.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:145`*


---

<a name="inkgamepadbackend-nkgamepadsystem-init"></a>

#### 🔧 `Init`

`virtual`

```cpp
virtual bool Init() = 0
```

**Initialise le backend (ouvre les devices, enregistre les callbacks OS…)**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:68`*


---

<a name="nkgamepadsystem-nkgamepadsystem-instance"></a>

#### 🔧 `Instance`

`static`

```cpp
static NkGamepadSystem& Instance()
```

**Access singleton instance.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:110`*


---

<a name="nkgamepadsystem-nkgamepadsystem-isconnected"></a>

#### 🔧 `IsConnected`

`const`

```cpp
bool IsConnected(NkU32 idx) const = 0
```

**True if gamepad index is connected.**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `idx` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:147`*


---

<a name="inkgamepadbackend-nkgamepadsystem-shutdown"></a>

#### 🔧 `Shutdown`

`virtual`

```cpp
virtual void Shutdown() = 0
```

**Libère toutes les ressources.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:71`*


---

### 📦 Variables (5)

<a name="inkgamepadbackend-false"></a>

#### 📦 `false`

```cpp
return false
```

**Pompe les événements gamepad et remplit les états internes.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:74`*


---

<a name="nkgamepadsystem-nkgamepadsystem-mready"></a>

#### 📦 `mReady`

```cpp
return mReady
```

**Initialize backend and internal state.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:120`*


---

<a name="nkgamepadsystem-nkgamepadsystem-mready"></a>

#### 📦 `mReady`

```cpp
return mReady
```

**Shutdown backend and clear state.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:122`*


---

<a name="nkgamepadsystem-nkgamepadsystem-motorlow"></a>

#### 📦 `motorLow`

```cpp
float motorLow
```

**Device info for a connected gamepad index.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:149`*


---

<a name="nkgamepadsystem-nkgamepadsystem-motorlow"></a>

#### 📦 `motorLow`

```cpp
float motorLow
```

**Snapshot state for a gamepad index.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkGamepadSystem.h:151`*


---


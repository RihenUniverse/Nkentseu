# 📄 NkClock.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Time\NkClock.h`

### 📦 Fichiers Inclus

- [`NkDuration.h`](./NkDuration.h.md)
- `NkPlatformDetect.h`
- `chrono`
- `thread`

### 🔗 Inclus Par

- [`NkStopwatch.h`](./NkStopwatch.h.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (7)

### 🏛️ Classs (1)

<a name="nkclock-nkclock"></a>

#### 🏛️ `NkClock`

```cpp
class NkClock
```

**Cross-platform steady clock utilities. * NkClock centralizes time acquisition and sleep/yield primitives. On WebAssembly, Sleep/Yield use emscripten cooperative APIs.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:16`*


---

### ⚙️ Functions (1)

<a name="tonkduration"></a>

#### ⚙️ `ToNkDuration`

`static`

```cpp
static NkDuration ToNkDuration(const Clock::duration& d)
```

**Convert std::chrono duration to NkDuration.**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `d` | `const Clock::duration&` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:80`*


---

### 🔧 Methods (3)

<a name="nkclock-now"></a>

#### 🔧 `Now`

`static`

```cpp
static TimePoint Now()
```

**Current monotonic timestamp.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:28`*


---

<a name="nkclock-sleepmilliseconds"></a>

#### 🔧 `SleepMilliseconds`

`static`

```cpp
static void SleepMilliseconds(NkU32 milliseconds) = 0
```

**Sleep for a number of milliseconds.**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `milliseconds` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:58`*


---

<a name="nkclock-yieldthread"></a>

#### 🔧 `YieldThread`

`static`

```cpp
static void YieldThread()
```

**Yield execution to other tasks/threads.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:70`*


---

### 📦 Variables (2)

<a name="nkclock-nanoseconds"></a>

#### 📦 `nanoseconds`

`const`

```cpp
const NkI64 nanoseconds
```

**Elapsed duration since**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:34`*


---

<a name="nkclock-nanoseconds"></a>

#### 📦 `nanoseconds`

`const`

```cpp
const NkI64 nanoseconds
```

**Sleep for a duration (cooperative on WASM).**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkClock.h:40`*


---


# 📄 NkStopwatch.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Time\NkStopwatch.h`

### 📦 Fichiers Inclus

- [`NkClock.h`](./NkClock.h.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (4)

### 🏛️ Classs (1)

<a name="nkstopwatch-nkstopwatch"></a>

#### 🏛️ `NkStopwatch`

```cpp
class NkStopwatch
```

**Simple elapsed-time accumulator. * NkStopwatch can be started/stopped multiple times while preserving accumulated elapsed time.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkStopwatch.h:8`*


---

### 🔧 Methods (3)

<a name="nkstopwatch-isrunning"></a>

#### 🔧 `IsRunning`

`const`

```cpp
bool IsRunning() const
```

**True when stopwatch is currently running.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkStopwatch.h:54`*


---

<a name="nkstopwatch-restart"></a>

#### 🔧 `Restart`

```cpp
void Restart()
```

**Reset and start immediately.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkStopwatch.h:46`*


---

<a name="nkstopwatch-start"></a>

#### 🔧 `Start`

```cpp
void Start()
```

**Start timing if currently stopped.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Time\NkStopwatch.h:19`*


---


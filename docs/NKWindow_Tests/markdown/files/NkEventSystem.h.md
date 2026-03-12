# 📄 NkEventSystem.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\NkEventSystem.h`

### 📦 Fichiers Inclus

- `NkEvent.h`
- `NkTypedEvents.h`
- [`IEventImpl.h`](./IEventImpl.h.md)
- `functional`
- `unordered_map`
- `typeindex`
- `vector`
- `algorithm`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (8)

### 🏛️ Classs (1)

<a name="eventsystem-eventsystem"></a>

#### 🏛️ `EventSystem`

```cpp
class EventSystem
```

**Central event dispatcher and polling queue. * EventSystem aggregates platform event implementations, dispatches global and typed callbacks, and optionally exposes a queue API via PollEvent().**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:42`*


---

### 🔧 Methods (7)

<a name="eventsystem-attachimpl"></a>

#### 🔧 `AttachImpl`

```cpp
void AttachImpl(IEventImpl* impl)
```

**Lie une IEventImpl concrète (appelé par Window::Create). Plusieurs implémentations peuvent être liées simultanément (plusieurs fenêtres).**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `impl` | `IEventImpl*` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:61`*


---

<a name="eventsystem-detachimpl"></a>

#### 🔧 `DetachImpl`

```cpp
void DetachImpl(IEventImpl* impl)
```

**Détache une implémentation (appelé quand la fenêtre est fermée).**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `impl` | `IEventImpl*` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:68`*


---

<a name="eventsystem-pollevent"></a>

#### 🔧 `PollEvent`

```cpp
NkEvent* PollEvent()
```

**Retourne le prochain événement de la queue, ou nullptr si vide. * Le pointeur est valide jusqu'au prochain appel de PollEvent ou PollEvents. Ne pas stocker ce pointeur. * Si la queue est vide, PollEvent() pompe automatiquement les événements (callbacks + queue) en interne. À utiliser seul (sans PollEvents() juste avant).**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:81`*


---

<a name="eventsystem-pollevent"></a>

#### 🔧 `PollEvent`

```cpp
bool PollEvent(NkEvent& event)
```

**Variante style SFML: copie le prochain événement dans**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `event` | `NkEvent&` |  |

**Retour:** true si un événement a été lu, sinon false.

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:93`*


---

<a name="eventsystem-pollevents"></a>

#### 🔧 `PollEvents`

```cpp
void PollEvents()
```

**Pompe les événements OS et exécute uniquement les callbacks. * Ce mode ne fournit pas de batch lisible par PollEvent(). Ne pas combiner avec while(PollEvent()) dans la même trame.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:73`*


---

<a name="eventsystem-removeeventcallback"></a>

#### 🔧 `RemoveEventCallback`

```cpp
template<typename T> void RemoveEventCallback()
```

**Supprime le callback typé pour T.**

**Paramètres Template:**

- `typename T`

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:124`*


---

<a name="eventsystem-setglobaleventcallback"></a>

#### 🔧 `SetGlobalEventCallback`

```cpp
void SetGlobalEventCallback(NkGlobalEventCallback callback)
```

**Callback reçoit TOUS les événements (avant la queue).**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `callback` | `NkGlobalEventCallback` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkEventSystem.h:101`*


---


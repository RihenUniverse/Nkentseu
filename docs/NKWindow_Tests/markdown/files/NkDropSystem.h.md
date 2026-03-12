# 📄 NkDropSystem.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\NkDropSystem.h`

### 📦 Fichiers Inclus

- `Events/NkDropEvents.h`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (2)

### ⚙️ Functions (2)

<a name="nkdisabledroptarget"></a>

#### ⚙️ `NkDisableDropTarget`

```cpp
void NkDisableDropTarget(void* nativeHandle)
```

**Désactive le support drag&drop.**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `nativeHandle` | `void*` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkDropSystem.h:28`*


---

<a name="nkenabledroptarget"></a>

#### ⚙️ `NkEnableDropTarget`

```cpp
void NkEnableDropTarget(void* nativeHandle)
```

**Active le support drag&drop sur la fenêtre donnée (handle natif).**

Appelé automatiquement par IEventImpl::Initialize si DropEnabled=true dans NkWindowConfig.

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `nativeHandle` | `void*` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkDropSystem.h:24`*


---


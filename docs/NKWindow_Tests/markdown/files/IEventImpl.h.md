# 📄 IEventImpl.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\IEventImpl.h`

### 📦 Fichiers Inclus

- `NkEvent.h`
- `queue`
- `cstddef`
- `functional`
- `string`

### 🔗 Inclus Par

- [`NkEventSystem.h`](./NkEventSystem.h.md)
- [`NkWindow.h`](./NkWindow.h.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (1)

### ⚙️ Functions (1)

<a name="shutdown"></a>

#### ⚙️ `Shutdown`

`virtual`

```cpp
virtual void Shutdown(void* nativeHandle) = 0
```

**Appelé par IWindowImpl::Close() avant destruction du handle.**

Désenregistre la fenêtre.

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `nativeHandle` | `void*` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\IEventImpl.h:51`*


---


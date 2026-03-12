# 📄 NkWindow.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\NkWindow.h`

### 📦 Fichiers Inclus

- `NkWindowConfig.h`
- [`NkSafeArea.h`](./NkSafeArea.h.md)
- [`NkSurface.h`](./NkSurface.h.md)
- `NkEvent.h`
- `IWindowImpl.h`
- [`IEventImpl.h`](./IEventImpl.h.md)
- [`NkSystem.h`](./NkSystem.h.md)
- `memory`
- `string`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (2)

### 🏛️ Classs (1)

<a name="window-window"></a>

#### 🏛️ `Window`

```cpp
class Window
```

**Cross-platform window facade. * Window wraps platform implementations (Win32/X11/Wayland/Cocoa/UIKit/...) and exposes a unified API for lifecycle, sizing, input policy and native surface retrieval for rendering backends.**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkWindow.h:45`*


---

### 🔧 Methods (1)

<a name="window-create"></a>

#### 🔧 `Create`

```cpp
bool Create(const NkWindowConfig& config)
```

**Crée la fenêtre. NkInitialise() doit avoir été appelé avant.**

Utilise automatiquement l'IEventImpl fourni par NkSystem.

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `config` | `const NkWindowConfig&` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\NkWindow.h:71`*


---


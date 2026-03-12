# 📄 NkKeyboardEvents.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\Events\NkKeyboardEvents.h`

### 📦 Fichiers Inclus

- `NkEventTypes.h`
- [`NkScancode.h`](./NkScancode.h.md)
- `string`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (1)

### ⚙️ Functions (1)

<a name="ispress"></a>

#### ⚙️ `IsPress`

`const`

```cpp
bool IsPress() const = 0
```

**< true = touche maintenue, l'OS génère des répétitions.**

< L'état reste NK_PRESSED pour les répétitions (pas NK_RELEASED).

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkKeyboardEvents.h:54`*


---


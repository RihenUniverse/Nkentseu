# 📄 NkScancode.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Chemin:** `src\NKWindow\Core\Events\NkScancode.h`

### 📦 Fichiers Inclus

- `cstring`
- `../NkTypes.h`
- `string`

### 🔗 Inclus Par

- [`NkKeyboardEvents.h`](./NkKeyboardEvents.h.md)

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)

## 🎯 Éléments (6)

### ⚙️ Functions (6)

<a name="nkscancodefromdomcode"></a>

#### ⚙️ `NkScancodeFromDOMCode`

```cpp
NkScancode NkScancodeFromDOMCode(const char* domCode)
```

**Convertit un DOM KeyboardEvent.code (WASM) vers NkScancode USB HID**

domCode exemple: "KeyA", "Space", "ArrowLeft", "Numpad0"

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `domCode` | `const char*` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkScancode.h:206`*


---

<a name="nkscancodefromlinux"></a>

#### ⚙️ `NkScancodeFromLinux`

```cpp
NkScancode NkScancodeFromLinux(NkU32 linuxKeycode)
```

**Convertit un keycode Linux evdev (XCB/XLib) vers NkScancode USB HID**

linux_keycode = xcb_keycode_t ou XLib keycode (soustrait -8 pour evdev)

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `linuxKeycode` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkScancode.h:199`*


---

<a name="nkscancodefrommac"></a>

#### ⚙️ `NkScancodeFromMac`

```cpp
NkScancode NkScancodeFromMac(NkU32 macKeycode)
```

**Convertit un keyCode macOS (NSEvent.keyCode) vers NkScancode USB HID**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `macKeycode` | `NkU32` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkScancode.h:203`*


---

<a name="nkscancodefromwin32"></a>

#### ⚙️ `NkScancodeFromWin32`

```cpp
NkScancode NkScancodeFromWin32(NkU32 win32Scancode, bool extended)
```

**Convertit un scancode Win32 Set-1 (MAPVK_VK_TO_VSC) vers NkScancode USB HID**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `win32Scancode` | `NkU32` |  |
| `extended` | `bool` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkScancode.h:196`*


---

<a name="nkscancodetokey"></a>

#### ⚙️ `NkScancodeToKey`

```cpp
NkKey NkScancodeToKey(NkScancode sc)
```

**Convertit NkScancode vers NkKey (position US-QWERTY invariante)**

Ce mapping est la table de référence keycode ← scancode

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `sc` | `NkScancode` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkScancode.h:210`*


---

<a name="nkscancodetostring"></a>

#### ⚙️ `NkScancodeToString`

```cpp
char* NkScancodeToString(NkScancode sc)
```

**Nom lisible du scancode (ex: "SC_A", "SC_SPACE")**

**Paramètres:**

| Nom | Type | Description |
|-----|------|-------------|
| `sc` | `NkScancode` |  |

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKWindow\src\NKWindow\Core\Events\NkScancode.h:193`*


---


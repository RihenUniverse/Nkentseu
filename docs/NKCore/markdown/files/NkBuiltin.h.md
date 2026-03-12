# 📄 NkBuiltin.h

[🏠 Accueil](../index.md) | [📁 Fichiers](./index.md)

## Informations

**Description:** Macros pour fonctionnalités intégrées du compilateur

**Auteur:** Rihen

**Chemin:** `src\NKCore\NkBuiltin.h`

### 🗂️ Namespaces

- [`nkentseu`](../namespaces/nkentseu.md)
- [`debug`](../namespaces/debug.md)
- [`log`](../namespaces/log.md)

## 🎯 Éléments (19)

### 🔣 Macros (19)

<a name="nkentseu_assert_msg"></a>

#### 🔣 `NKENTSEU_ASSERT_MSG`

```cpp
#define NKENTSEU_ASSERT_MSG \
```

**Assertion avec message personnalisé**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:183`*


---

<a name="nkentseu_builtin_date"></a>

#### 🔣 `NKENTSEU_BUILTIN_DATE`

```cpp
#define NKENTSEU_BUILTIN_DATE __DATE__
```

**Date de compilation courante**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:63`*


---

<a name="nkentseu_builtin_file"></a>

#### 🔣 `NKENTSEU_BUILTIN_FILE`

```cpp
#define NKENTSEU_BUILTIN_FILE __FILE__
```

**Nom du fichier source courant**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:23`*


---

<a name="nkentseu_builtin_line"></a>

#### 🔣 `NKENTSEU_BUILTIN_LINE`

```cpp
#define NKENTSEU_BUILTIN_LINE __LINE__
```

**Numéro de ligne courante**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:30`*


---

<a name="nkentseu_builtin_time"></a>

#### 🔣 `NKENTSEU_BUILTIN_TIME`

```cpp
#define NKENTSEU_BUILTIN_TIME __TIME__
```

**Heure de compilation courante**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:70`*


---

<a name="nkentseu_builtin_timestamp"></a>

#### 🔣 `NKENTSEU_BUILTIN_TIMESTAMP`

```cpp
#define NKENTSEU_BUILTIN_TIMESTAMP __DATE__ " " __TIME__
```

**Timestamp de compilation**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:77`*


---

<a name="nkentseu_check_continue"></a>

#### 🔣 `NKENTSEU_CHECK_CONTINUE`

```cpp
#define NKENTSEU_CHECK_CONTINUE \
```

**Vérifie une condition et continue si fausse**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:313`*


---

<a name="nkentseu_check_return"></a>

#### 🔣 `NKENTSEU_CHECK_RETURN`

```cpp
#define NKENTSEU_CHECK_RETURN \
```

**Vérifie une condition et retourne si fausse**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:298`*


---

<a name="nkentseu_declare_with_context"></a>

#### 🔣 `NKENTSEU_DECLARE_WITH_CONTEXT`

```cpp
#define NKENTSEU_DECLARE_WITH_CONTEXT \
```

**Définit une variable locale avec contexte de débogage**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:286`*


---

<a name="nkentseu_fixme"></a>

#### 🔣 `NKENTSEU_FIXME`

```cpp
#define NKENTSEU_FIXME \
```

**Message FIXME avec localisation**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:141`*


---

<a name="nkentseu_log_error"></a>

#### 🔣 `NKENTSEU_LOG_ERROR`

```cpp
#define NKENTSEU_LOG_ERROR \
```

**Macro pour logger une erreur avec contexte**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:240`*


---

<a name="nkentseu_log_info"></a>

#### 🔣 `NKENTSEU_LOG_INFO`

```cpp
#define NKENTSEU_LOG_INFO \
```

**Macro pour logger une information avec contexte**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:258`*


---

<a name="nkentseu_log_warning"></a>

#### 🔣 `NKENTSEU_LOG_WARNING`

```cpp
#define NKENTSEU_LOG_WARNING \
```

**Macro pour logger un avertissement avec contexte**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:249`*


---

<a name="nkentseu_note"></a>

#### 🔣 `NKENTSEU_NOTE`

```cpp
#define NKENTSEU_NOTE \
```

**Message NOTE avec localisation**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:151`*


---

<a name="nkentseu_simple_assert"></a>

#### 🔣 `NKENTSEU_SIMPLE_ASSERT`

```cpp
#define NKENTSEU_SIMPLE_ASSERT \
```

**Assertion basique avec informations contextuelles**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:170`*


---

<a name="nkentseu_test_assert"></a>

#### 🔣 `NKENTSEU_TEST_ASSERT`

```cpp
#define NKENTSEU_TEST_ASSERT \
```

**Assertion de test avec message**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:336`*


---

<a name="nkentseu_todo"></a>

#### 🔣 `NKENTSEU_TODO`

```cpp
#define NKENTSEU_TODO \
```

**Message TODO avec localisation**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:131`*


---

<a name="nkentseu_unique_id"></a>

#### 🔣 `NKENTSEU_UNIQUE_ID`

```cpp
#define NKENTSEU_UNIQUE_ID __LINE__
```

**Macro de base pour créer un identifiant unique**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:93`*


---

<a name="nkentseu_unique_name"></a>

#### 🔣 `NKENTSEU_UNIQUE_NAME`

```cpp
#define NKENTSEU_UNIQUE_NAME NKENTSEU_CONCAT(prefix, NKENTSEU_UNIQUE_ID)
```

**Crée un identifiant unique avec préfixe**

*Défini dans: `E:\Projets\MacShared\Projets\Jenga\Jenga\Exemples\NKWindow\NKCore\src\NKCore\NkBuiltin.h:100`*


---


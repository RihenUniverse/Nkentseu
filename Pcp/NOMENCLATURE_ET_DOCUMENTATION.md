# Guide de Nomenclature et Documentation - Nkentseu Framework

> **Version:** 1.0  
> **Date:** Février 2026  
> **Auteur:** Rihen  
> **Statut:** Standard Obligatoire

---

## 📋 Table des Matières

1. [Règles de Nomenclature](#1-règles-de-nomenclature)
2. [Structure des Fichiers](#2-structure-des-fichiers)
3. [Documentation du Code](#3-documentation-du-code)
4. [Commentaires](#4-commentaires)
5. [Exemples Complets](#5-exemples-complets)
6. [Génération Documentation](#6-génération-documentation)
7. [Outils et Automatisation](#7-outils-et-automatisation)

---

## 1. Règles de Nomenclature

### 1.1 Convention Générale

| Type | Convention | Exemple | Usage |
|------|-----------|---------|-------|
| **Fichiers** | `PascalCase` | `NkBitManipulation.h` | Tous les fichiers |
| **Classes/Structs** | `PascalCase` | `class RenderSystem` | Types |
| **Enums** | `PascalCase` | `enum class LogLevel` | Énumérations |
| **Unions** | `PascalCase` | `union DataUnion` | Unions |
| **Fonctions/Méthodes** | `PascalCase` | `void Initialize()` | Toutes méthodes |
| **Propriétés publiques** | `camelCase` | `int entityCount` | Membres publics |
| **Propriétés privées/protected** | `mPascalCase` | `int mMaxEntities` | Membres privés |
| **Membres statiques** | `sPascalCase` | `int sInstanceCount` | Statiques classe |
| **Globales** | `gPascalCase` | `Logger* gLogger` | Variables globales |
| **Constantes** | `UPPER_SNAKE_CASE` | `const int MAX_SIZE` | Constantes |
| **Macros** | `UPPER_SNAKE_CASE` | `#define NK_ASSERT` | Préprocesseur |
| **Namespaces** | `lowercase` | `namespace nkentseu` | Espaces de noms |

---

### 1.2 Règles Détaillées

#### 1.2.1 Fichiers

**Headers (.h) :**
```cpp
NkBitManipulation.h      // Classe/Struct
NkVector3.h              // Type mathématique
NkAllocator.h            // Interface
NkMemoryUtils.h          // Fonctions utilitaires
```

**Sources (.cpp) :**
```cpp
NkBitManipulation.cpp
NkVector3.cpp
NkAllocator.cpp
NkMemoryUtils.cpp
```

**Convention :**
- Toujours `PascalCase`
- Préfixe `Nk` pour tous les fichiers du framework
- Nom descriptif et concis
- Un fichier = une classe/struct principale (sauf utilitaires)

---

#### 1.2.2 Classes, Structs, Enums, Unions

**Classes :**
```cpp
class RenderSystem { };           // Système de rendu
class PhysicsEngine { };          // Moteur physique
class AssetManager { };           // Gestionnaire
class IAllocator { };             // Interface (préfixe I)
class GraphicsDeviceVulkan { };   // Implémentation spécifique
```

**Structs :**
```cpp
struct Vector3 { };               // Structure de données
struct TransformComponent { };    // Component ECS
struct RenderCommand { };         // Commande
```

**Enums :**
```cpp
enum class LogLevel {             // Enum class (préféré)
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

enum class GraphicsAPI {
    OPENGL,
    VULKAN,
    DIRECTX12,
    METAL,
    WEBGL
};
```

**Unions :**
```cpp
union ColorUnion {
    uint32_t rgba;
    struct {
        uint8_t r, g, b, a;
    };
};
```

**Règles Supplémentaires :**
- **Interfaces** : Préfixe `I` (ex: `IAllocator`, `IRenderer`)
- **Classes abstraites** : Pas de préfixe spécial, mais suffixe `Base` optionnel
- **Templates** : Paramètres en `PascalCase` avec préfixe `T` (ex: `template<typename TKey, typename TValue>`)

---

#### 1.2.3 Fonctions et Méthodes

**Méthodes Publiques :**
```cpp
class RenderSystem {
public:
    void Initialize();              // Action
    void Update(float deltaTime);   // Action avec paramètre
    void Shutdown();                // Action
    
    Mesh* CreateMesh();             // Factory
    void DestroyMesh(Mesh* mesh);   // Cleanup
    
    bool IsInitialized() const;     // Query (prédicat)
    int GetEntityCount() const;     // Getter
    void SetViewport(int w, int h); // Setter
    
    Vector3 CalculateNormal() const;// Calcul
    void ProcessEvents();           // Traitement
};
```

**Méthodes Privées :**
```cpp
class RenderSystem {
private:
    void InternalUpdate();          // Méthode interne
    void ProcessRenderQueue();      // Traitement privé
    void ValidateState();           // Validation
};
```

**Méthodes Statiques :**
```cpp
class Math {
public:
    static float Sqrt(float value);
    static Vector3 Normalize(const Vector3& v);
    static float Dot(const Vector3& a, const Vector3& b);
};
```

**Opérateurs :**
```cpp
Vector3 operator+(const Vector3& other) const;
Vector3& operator+=(const Vector3& other);
bool operator==(const Vector3& other) const;
```

**Convention :**
- Verbes pour actions : `Initialize`, `Update`, `Process`
- Préfixe `Is`/`Has`/`Can` pour booléens : `IsValid()`, `HasComponent()`
- Préfixe `Get`/`Set` pour accesseurs : `GetPosition()`, `SetPosition()`
- Pas de préfixe pour calculs : `Normalize()`, `CalculateDistance()`

---

#### 1.2.4 Propriétés et Variables Membres

**Membres Publics :**
```cpp
class Vector3 {
public:
    float x;           // camelCase pour public
    float y;
    float z;
};

struct TransformComponent {
public:
    Vector3 position;       // camelCase
    Quaternion rotation;    // camelCase
    Vector3 scale;          // camelCase
};
```

**Membres Privés/Protected :**
```cpp
class RenderSystem {
private:
    int mMaxEntities;              // mPascalCase pour privé
    float mDeltaTime;              // mPascalCase
    bool mIsInitialized;           // mPascalCase
    
    std::vector<Mesh*> mMeshes;    // mPascalCase
    Camera* mActiveCamera;         // mPascalCase
    
protected:
    GraphicsDevice* mDevice;       // mPascalCase pour protected
    SwapChain* mSwapChain;         // mPascalCase
};
```

**Membres Statiques :**
```cpp
class Logger {
private:
    static Logger* sInstance;           // sPascalCase (statique privé)
    static int sLoggerCount;            // sPascalCase
    
public:
    static const int sMaxLoggers;       // sPascalCase (statique public)
};
```

**Variables Locales :**
```cpp
void Update(float deltaTime) {
    int entityCount = 0;        // camelCase pour locales
    float elapsedTime = 0.0f;   // camelCase
    bool isValid = true;        // camelCase
    
    for (int i = 0; i < count; ++i) {      // camelCase
        Entity* entity = entities[i];      // camelCase
        // ...
    }
}
```

**Paramètres de Fonction :**
```cpp
void ProcessEntity(
    Entity* entity,              // camelCase
    float deltaTime,             // camelCase
    const RenderContext& context // camelCase
);
```

---

#### 1.2.5 Variables Globales

**Déclaration :**
```cpp
// Dans un header
extern Logger* gLogger;              // gPascalCase
extern AssetManager* gAssetManager;  // gPascalCase
extern int gMaxThreads;              // gPascalCase

// Dans un source
Logger* gLogger = nullptr;
AssetManager* gAssetManager = nullptr;
int gMaxThreads = 8;
```

**⚠️ IMPORTANT :** Les globales doivent être utilisées avec parcimonie. Préférer :
- Singleton pattern
- Service Locator
- Dependency injection

---

#### 1.2.6 Constantes

**Constantes de Compilation :**
```cpp
constexpr int MAX_ENTITIES = 10000;        // UPPER_SNAKE_CASE
constexpr float PI = 3.14159f;             // UPPER_SNAKE_CASE
constexpr size_t BUFFER_SIZE = 1024;       // UPPER_SNAKE_CASE

const char* const DEFAULT_SHADER_PATH = "shaders/default.glsl";
```

**Constantes de Classe :**
```cpp
class RenderSystem {
public:
    static const int MAX_LIGHTS = 16;      // UPPER_SNAKE_CASE
    static const int MAX_TEXTURES = 32;    // UPPER_SNAKE_CASE
    
private:
    static const size_t COMMAND_BUFFER_SIZE = 4096;  // UPPER_SNAKE_CASE
};
```

**Valeurs d'Énumération :**
```cpp
enum class LogLevel {
    TRACE,      // UPPER_SNAKE_CASE
    DEBUG,      // UPPER_SNAKE_CASE
    INFO,       // UPPER_SNAKE_CASE
    WARNING,    // UPPER_SNAKE_CASE
    ERROR,      // UPPER_SNAKE_CASE
    FATAL       // UPPER_SNAKE_CASE
};

enum class GraphicsAPI {
    OPENGL,     // UPPER_SNAKE_CASE
    VULKAN,     // UPPER_SNAKE_CASE
    DIRECTX12,  // UPPER_SNAKE_CASE (pas de underscore dans acronyme)
    METAL,
    WEBGL       // UPPER_SNAKE_CASE
};
```

---

#### 1.2.7 Macros et Préprocesseur

**Macros :**
```cpp
// Protection d'inclusion (guards)
#ifndef NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED
#define NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED

// Macros fonctionnelles
#define NK_ASSERT(condition, ...) \
    do { \
        if (!(condition)) { \
            nkentseu::core::AssertHandler(__FILE__, __LINE__, #condition, __VA_ARGS__); \
        } \
    } while(0)

#define NK_LOG_INFO(...) nkentseu::logger::Log(nkentseu::logger::LogLevel::INFO, __VA_ARGS__)
#define NK_LOG_ERROR(...) nkentseu::logger::Log(nkentseu::logger::LogLevel::ERROR, __VA_ARGS__)

// Macros de configuration
#define NK_DEBUG
#define NK_PLATFORM_WINDOWS
#define NK_ENABLE_PROFILING

// Macros utilitaires
#define NK_ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define NK_BIT(x) (1 << (x))
#define NK_UNUSED(x) (void)(x)
```

**Convention :**
- Toujours `UPPER_SNAKE_CASE`
- Préfixe `NK_` pour toutes les macros du framework
- Utiliser `do { } while(0)` pour macros multi-lignes
- Toujours parenthéser les arguments de macros

---

#### 1.2.8 Namespaces

**Convention : `lowercase` sans séparateurs**

```cpp
namespace nkentseu {              // Namespace racine (lowercase)
    namespace core {              // Sous-namespace (lowercase)
        namespace memory {        // Sous-sous-namespace
            
            class LinearAllocator { };
            
        } // namespace memory
    } // namespace core
    
    namespace graphics {
        namespace opengl {
            class OpenGLDevice { };
        } // namespace opengl
        
        namespace vulkan {
            class VulkanDevice { };
        } // namespace vulkan
    } // namespace graphics
    
    namespace math {
        struct Vector3 { };
        struct Matrix4 { };
    } // namespace math
    
} // namespace nkentseu
```

**Hiérarchie des Namespaces :**
```
nkentseu                    (racine)
├── core                    (système core)
│   ├── memory             (gestion mémoire)
│   ├── reflection         (réflexion)
│   └── traits             (traits C++)
│
├── containers             (conteneurs)
│   ├── sequential
│   └── associative
│
├── filesystem             (système fichiers)
├── serialization          (sérialisation)
├── threading              (multi-threading)
├── string                 (chaînes de caractères)
├── time                   (temps)
│
├── logger                 (logging)
├── math                   (mathématiques)
│
├── window                 (fenêtrage)
│   └── events            (événements)
│
├── graphics               (futur - rendu)
│   ├── opengl
│   ├── vulkan
│   ├── directx12
│   └── metal
│
├── physics               (futur - physique)
├── audio                 (futur - audio)
├── animation             (futur - animation)
└── network               (futur - réseau)
```

**Règles :**
- Toujours `lowercase` (tout en minuscules)
- Pas d'underscores, pas d'espaces
- Mots collés ensemble
- Commenter la fermeture : `} // namespace xxx`
- Alignement des commentaires de fermeture

---

### 1.3 Règles Supplémentaires

#### 1.3.1 Acronymes et Abréviations

**Dans les Noms de Types :**
```cpp
// CORRECT
class HttpClient { };      // Http, pas HTTP
class XmlParser { };       // Xml, pas XML
class JsonReader { };      // Json, pas JSON
class GpuBuffer { };       // Gpu, pas GPU

// EXCEPTION : Si acronyme fait partie du nom propre
class DirectX12Device { };  // DirectX12 (nom propre)
class OpenGLContext { };    // OpenGL (nom propre)
```

**Dans les Variables :**
```cpp
// Membres
int mHttpPort;           // camelCase avec acronyme en Pascal
bool mIsXmlValid;        // camelCase

// Locales
int httpResponseCode;    // camelCase
std::string jsonData;    // camelCase
```

---

#### 1.3.2 Booléens

**Préfixes :**
```cpp
// Préfixes recommandés : is, has, can, should, will
bool isInitialized;      // État
bool isValid;
bool isEnabled;

bool hasChildren;        // Possession
bool hasTexture;

bool canRender;          // Capacité
bool canMove;

bool shouldUpdate;       // Recommandation
bool shouldRender;

bool willTerminate;      // Future action
```

**Membres de Classe :**
```cpp
class RenderSystem {
private:
    bool mIsInitialized;     // mPascalCase
    bool mHasSwapChain;      // mPascalCase
    bool mCanRender;         // mPascalCase
};
```

---

#### 1.3.3 Pointeurs et Références

**Déclaration :**
```cpp
// Préféré : * et & collés au type
Entity* entity;              // Pointeur
const Vector3& position;     // Référence const

// Acceptable mais moins préféré
Entity *entity;
const Vector3 &position;

// À éviter
Entity * entity;
const Vector3 & position;
```

**Smart Pointers :**
```cpp
std::unique_ptr<Mesh> mesh;
std::shared_ptr<Texture> texture;
std::weak_ptr<Material> material;

// Ou avec alias Nkentseu
nkentseu::UniquePtr<Mesh> mesh;
nkentseu::SharedPtr<Texture> texture;
nkentseu::WeakPtr<Material> material;
```

---

#### 1.3.4 Templates

**Paramètres de Template :**
```cpp
// Préfixe T pour types
template<typename T>
class Vector { };

template<typename TKey, typename TValue>
class HashMap { };

template<typename TAllocator>
class String { };

// Paramètres non-type : PascalCase
template<typename T, size_t Size>
class Array { };

template<int Width, int Height>
struct Resolution { };
```

**Spécialisation :**
```cpp
// Template général
template<typename T>
class Serializer { };

// Spécialisation
template<>
class Serializer<int> { };

template<>
class Serializer<std::string> { };
```

---

## 2. Structure des Fichiers

### 2.1 En-tête de Fichier (Header)

**Template Header (.h) :**

```cpp
// -----------------------------------------------------------------------------
// FICHIER: Core\NKCore\src\NKCore\NkBitManipulation.h
// DESCRIPTION: Déclaration de la classe BitManipulation pour manipulation de bits
// AUTEUR: Rihen
// DATE: 2026
// VERSION: 1.0.0
// -----------------------------------------------------------------------------

#pragma once

#ifndef NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED
#define NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED

// ============================================================
// INCLUDES
// ============================================================

// Nkentseu headers
#include "NKCore/NkExport.h"
#include "NKCore/NkPrimitiveTypes.h"

// Standard library
#include <cstdint>
#include <type_traits>

// ============================================================
// DECLARATIONS
// ============================================================

namespace nkentseu {
    namespace core {
        
        /**
         * @brief Classe utilitaire pour manipulation de bits
         * 
         * Fournit des opérations efficaces pour manipuler des bits
         * individuels, compter les bits, et effectuer des rotations.
         * 
         * @note Toutes les méthodes sont static et inline pour performance
         * @warning Thread-safe uniquement pour lectures
         * 
         * @example
         * @code
         * uint32_t value = 0b10110010;
         * bool bit5 = BitManipulation::GetBit(value, 5);  // true
         * value = BitManipulation::SetBit(value, 3);      // 0b10111010
         * int count = BitManipulation::CountBits(value);   // 5
         * @endcode
         * 
         * @see NkBitSet
         * @see NkFlags
         * 
         * @author Rihen
         * @date 2026-02-06
         * @version 1.0.0
         */
        class NKENTSEU_CORE_API BitManipulation {
        public:
            // ========================================
            // CONSTRUCTORS & DESTRUCTOR
            // ========================================
            
            /**
             * @brief Constructeur supprimé (classe statique)
             */
            BitManipulation() = delete;
            
            // ========================================
            // BIT OPERATIONS
            // ========================================
            
            /**
             * @brief Récupère la valeur d'un bit à une position donnée
             * 
             * @param value Valeur à examiner
             * @param position Position du bit (0 = LSB)
             * @return true si le bit est à 1, false sinon
             * 
             * @pre position < sizeof(T) * 8
             * @complexity O(1)
             * 
             * @example
             * @code
             * uint8_t value = 0b10110010;
             * bool bit5 = BitManipulation::GetBit(value, 5);  // true
             * @endcode
             */
            template<typename T>
            static inline bool GetBit(T value, uint8_t position);
            
            /**
             * @brief Définit un bit à 1 à une position donnée
             * 
             * @param value Valeur à modifier
             * @param position Position du bit à définir
             * @return Nouvelle valeur avec le bit défini
             * 
             * @pre position < sizeof(T) * 8
             * @complexity O(1)
             */
            template<typename T>
            static inline T SetBit(T value, uint8_t position);
            
            /**
             * @brief Compte le nombre de bits à 1 (population count)
             * 
             * @param value Valeur à analyser
             * @return Nombre de bits à 1
             * 
             * @complexity O(1) sur architecture moderne (POPCNT instruction)
             * @complexity O(log n) en fallback software
             * 
             * @note Utilise l'instruction POPCNT si disponible
             */
            template<typename T>
            static inline int CountBits(T value);
            
            // ... Autres méthodes
            
        private:
            // ========================================
            // INTERNAL HELPERS
            // ========================================
            
            /**
             * @brief Compte bits (implémentation software)
             * @internal
             */
            template<typename T>
            static inline int CountBitsSoftware(T value);
        };
        
    } // namespace core
} // namespace nkentseu

// ============================================================
// INLINE IMPLEMENTATIONS
// ============================================================

namespace nkentseu {
    namespace core {
        
        template<typename T>
        inline bool BitManipulation::GetBit(T value, uint8_t position) {
            static_assert(std::is_integral<T>::value, "T must be integral type");
            return (value & (T(1) << position)) != 0;
        }
        
        template<typename T>
        inline T BitManipulation::SetBit(T value, uint8_t position) {
            static_assert(std::is_integral<T>::value, "T must be integral type");
            return value | (T(1) << position);
        }
        
        // ... Autres implémentations inline
        
    } // namespace core
} // namespace nkentseu

#endif // NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED

// ============================================================
// Copyright © 2024-2026 Rihen. All rights reserved.
// Proprietary License - Free to use and modify
//
// Generated by Rihen on 2026-02-06 14:30:00
// Creation Date: 2026-02-06 14:30:00
// ============================================================
```

---

### 2.2 Fichier Source (.cpp)

**Template Source (.cpp) :**

```cpp
// -----------------------------------------------------------------------------
// FICHIER: Core\NKCore\src\NKCore\NkBitManipulation.cpp
// DESCRIPTION: Implémentation de la classe BitManipulation
// AUTEUR: Rihen
// DATE: 2026
// VERSION: 1.0.0
// -----------------------------------------------------------------------------

// ============================================================
// INCLUDES
// ============================================================

// Header correspondant (TOUJOURS EN PREMIER)
#include "NkBitManipulation.h"

// Nkentseu headers
#include "NKCore/Assert/NkAssert.h"
#include "NKCore/NkPlatform.h"

// Platform-specific
#ifdef NK_PLATFORM_WINDOWS
    #include <intrin.h>
#endif

// Standard library
#include <bit>      // C++20 std::popcount
#include <cstring>

// ============================================================
// USING DECLARATIONS (scope limité au .cpp)
// ============================================================

using namespace nkentseu::core;

// ============================================================
// ANONYMOUS NAMESPACE (fonctions/variables internes au .cpp)
// ============================================================

namespace {
    
    /**
     * @brief Table de lookup pour count bits (8-bit)
     * @internal
     */
    constexpr uint8_t BIT_COUNT_TABLE[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        // ... (256 valeurs)
    };
    
    /**
     * @brief Vérifie si l'instruction POPCNT est disponible
     * @internal
     */
    bool HasPopCountInstruction() {
        #ifdef NK_PLATFORM_WINDOWS
            int cpuInfo[4];
            __cpuid(cpuInfo, 1);
            return (cpuInfo[2] & (1 << 23)) != 0;  // Check POPCNT bit
        #else
            return false;  // Implement for other platforms
        #endif
    }
    
} // anonymous namespace

// ============================================================
// IMPLEMENTATIONS
// ============================================================

namespace nkentseu {
    namespace core {
        
        // Si implémentations non-inline nécessaires
        
        template<typename T>
        int BitManipulation::CountBitsSoftware(T value) {
            static_assert(std::is_integral<T>::value, "T must be integral type");
            
            int count = 0;
            
            // Utilisation table lookup pour performance
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
            for (size_t i = 0; i < sizeof(T); ++i) {
                count += BIT_COUNT_TABLE[bytes[i]];
            }
            
            return count;
        }
        
        // Explicit template instantiations si nécessaire
        template int BitManipulation::CountBitsSoftware<uint8_t>(uint8_t);
        template int BitManipulation::CountBitsSoftware<uint16_t>(uint16_t);
        template int BitManipulation::CountBitsSoftware<uint32_t>(uint32_t);
        template int BitManipulation::CountBitsSoftware<uint64_t>(uint64_t);
        
    } // namespace core
} // namespace nkentseu

// ============================================================
// Copyright © 2024-2026 Rihen. All rights reserved.
// Proprietary License - Free to use and modify
//
// Generated by Rihen on 2026-02-06 14:30:00
// Creation Date: 2026-02-06 14:30:00
// ============================================================
```

---

### 2.3 Organisation des Sections

**Ordre des Sections (Header) :**

1. **En-tête** : Informations fichier
2. **#pragma once** et include guard
3. **Includes** : Groupés par catégorie
4. **Forward declarations** (si nécessaire)
5. **Namespace ouverture**
6. **Declarations** : Classes, structs, fonctions
7. **Inline implementations** (pour templates)
8. **Namespace fermeture**
9. **Include guard fermeture**
10. **Pied de page** : Copyright

**Ordre des Sections (Source) :**

1. **En-tête** : Informations fichier
2. **Include header correspondant** (PREMIER)
3. **Autres includes**
4. **Using declarations** (scope .cpp uniquement)
5. **Anonymous namespace** (helpers internes)
6. **Implementations**
7. **Template instantiations explicites**
8. **Pied de page** : Copyright

---

### 2.4 Include Guards

**Convention Stricte :**

```cpp
#ifndef NK_<MODULE>_<SUBMODULE>_<PATH>_<FILE>_H_INCLUDED
#define NK_<MODULE>_<SUBMODULE>_<PATH>_<FILE>_H_INCLUDED

// Contenu

#endif // NK_<MODULE>_<SUBMODULE>_<PATH>_<FILE>_H_INCLUDED
```

**Exemples :**

```cpp
// Core/NKCore/src/NKCore/NkBitManipulation.h
#ifndef NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED
#define NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED
// ...
#endif // NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED

// Core/Nkentseu/src/Nkentseu/Containers/NkVector.h
#ifndef NK_NKENTSEU_SRC_NKENTSEU_CONTAINERS_NKVECTOR_H_INCLUDED
#define NK_NKENTSEU_SRC_NKENTSEU_CONTAINERS_NKVECTOR_H_INCLUDED
// ...
#endif // NK_NKENTSEU_SRC_NKENTSEU_CONTAINERS_NKVECTOR_H_INCLUDED

// Core/NKMath/src/NKMath/Vector/NkVector3.h
#ifndef NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_H_INCLUDED
#define NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_H_INCLUDED
// ...
#endif // NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_H_INCLUDED
```

**Règles :**
- Toujours `UPPER_SNAKE_CASE`
- Préfixe `NK_`
- Chemin complet du module
- Suffixe `_H_INCLUDED`
- Backslashes Windows convertis en underscores

---

## 3. Documentation du Code

### 3.1 Format de Documentation

**Style Doxygen/JavaDoc :**

```cpp
/**
 * @brief Description courte (une ligne)
 * 
 * Description détaillée sur plusieurs lignes si nécessaire.
 * Peut contenir plusieurs paragraphes.
 * 
 * @param paramName Description du paramètre
 * @param[in] inputParam Paramètre d'entrée uniquement
 * @param[out] outputParam Paramètre de sortie uniquement
 * @param[in,out] inoutParam Paramètre entrée/sortie
 * 
 * @return Description de la valeur de retour
 * @retval VALUE1 Description de la valeur spécifique
 * @retval VALUE2 Description d'une autre valeur
 * 
 * @throw ExceptionType Description de quand l'exception est levée
 * @throws AnotherException Description
 * 
 * @pre Précondition : état requis avant appel
 * @post Postcondition : état garanti après appel
 * @invariant Invariant : propriété toujours vraie
 * 
 * @note Note importante pour l'utilisateur
 * @warning Avertissement sur usage dangereux
 * @attention Point d'attention particulier
 * @remark Remarque additionnelle
 * 
 * @see RelatedClass Référence croisée
 * @sa AnotherFunction Voir aussi
 * 
 * @example
 * @code
 * // Exemple d'utilisation
 * MyClass obj;
 * obj.MyMethod(42);
 * @endcode
 * 
 * @complexity O(n log n) Complexité algorithmique
 * @performance Considérations de performance
 * 
 * @threadsafe Cette fonction est thread-safe
 * @notthreadsafe Cette fonction n'est PAS thread-safe
 * 
 * @deprecated Utiliser NewFunction() à la place
 * @since Version 1.2.0
 * @version 1.0.0
 * 
 * @author Rihen
 * @date 2026-02-06
 * 
 * @internal Documentation interne (non publiée)
 * @private Détails privés d'implémentation
 */
```

---

### 3.2 Documentation des Classes

**Classe Complète :**

```cpp
/**
 * @brief Gestionnaire de rendu 2D optimisé avec batching
 * 
 * Renderer2D fournit un système de rendu 2D haute performance
 * avec batching automatique des sprites, textes, et primitives.
 * Supporte transparence, rotations, et effets de shaders.
 * 
 * Le renderer utilise un système de batching intelligent qui
 * regroupe les draw calls par texture et shader pour minimiser
 * les appels GPU.
 * 
 * @note Thread-safe pour soumission de commandes (thread-local queues)
 * @warning Ne pas appeler depuis le render thread directement
 * 
 * @example Usage Basique
 * @code
 * // Création
 * auto renderer = Renderer2D::Create(window);
 * 
 * // Rendu
 * renderer->BeginFrame();
 * renderer->DrawSprite(sprite, position, rotation, scale);
 * renderer->DrawText("Hello", font, position, color);
 * renderer->EndFrame();
 * @endcode
 * 
 * @example Batching Avancé
 * @code
 * renderer->BeginFrame();
 * 
 * // Ces draw calls seront batchés ensemble (même texture)
 * for (int i = 0; i < 1000; ++i) {
 *     renderer->DrawSprite(sprite, positions[i]);
 * }
 * 
 * renderer->EndFrame();  // 1 seul draw call GPU!
 * @endcode
 * 
 * @performance 
 * - 10K+ sprites @ 60 FPS (batched)
 * - Draw calls < 100 par frame typique
 * - Batching ratio > 90%
 * 
 * @see Renderer3D Pour rendu 3D
 * @see SpriteBatch Pour batching manuel
 * 
 * @author Rihen
 * @date 2026-02-06
 * @version 1.0.0
 * @since Nkentseu 0.2.0
 */
class NKENTSEU_CORE_API Renderer2D {
public:
    // ... membres
};
```

---

### 3.3 Documentation des Fonctions

**Fonction Simple :**

```cpp
/**
 * @brief Normalise le vecteur (longueur = 1)
 * 
 * Modifie le vecteur pour avoir une longueur de 1 tout en
 * préservant sa direction. Si le vecteur est nul (longueur = 0),
 * aucune modification n'est effectuée.
 * 
 * @pre Le vecteur ne doit pas être verrouillé
 * @post Si longueur > 0, alors Length() == 1.0f
 * 
 * @complexity O(1)
 * @notthreadsafe
 * 
 * @example
 * @code
 * Vector3 v(3, 4, 0);
 * v.Normalize();
 * // v est maintenant (0.6, 0.8, 0)
 * assert(v.Length() == 1.0f);
 * @endcode
 * 
 * @see Normalized() Pour version const
 */
void Normalize();
```

**Fonction avec Paramètres :**

```cpp
/**
 * @brief Calcule le produit scalaire de deux vecteurs
 * 
 * Le produit scalaire (dot product) de deux vecteurs est défini comme:
 * dot(a, b) = a.x * b.x + a.y * b.y + a.z * b.z
 * 
 * Propriétés mathématiques:
 * - dot(a, b) = |a| * |b| * cos(angle)
 * - dot(a, b) = 0 si perpendiculaires
 * - dot(a, b) > 0 si angle < 90°
 * - dot(a, b) < 0 si angle > 90°
 * 
 * @param[in] a Premier vecteur
 * @param[in] b Deuxième vecteur
 * 
 * @return Produit scalaire (valeur réelle)
 * @retval 0.0f Si les vecteurs sont perpendiculaires
 * @retval >0.0f Si l'angle est aigu
 * @retval <0.0f Si l'angle est obtus
 * 
 * @pre Aucune précondition
 * @post Résultat dans [-|a|*|b|, |a|*|b|]
 * 
 * @complexity O(1)
 * @threadsafe
 * 
 * @example
 * @code
 * Vector3 forward(0, 0, 1);
 * Vector3 right(1, 0, 0);
 * float dot = Vector3::Dot(forward, right);
 * // dot == 0.0f (perpendiculaires)
 * @endcode
 * 
 * @see Cross() Pour produit vectoriel
 * @see Angle() Pour calculer l'angle directement
 */
static float Dot(const Vector3& a, const Vector3& b);
```

**Fonction avec Exceptions :**

```cpp
/**
 * @brief Charge une texture depuis un fichier
 * 
 * Charge une texture depuis le disque et la transfère en mémoire GPU.
 * Supporte PNG, JPG, TGA, BMP, et formats compressés (DDS, KTX).
 * 
 * Le chargement est asynchrone si async = true, auquel cas la fonction
 * retourne immédiatement avec un handle valide mais les données GPU
 * ne sont pas encore disponibles.
 * 
 * @param[in] filepath Chemin vers le fichier texture
 * @param[in] async Chargement asynchrone (défaut: false)
 * @param[in] generateMipmaps Générer mipmaps automatiquement
 * 
 * @return Handle vers la texture chargée
 * 
 * @throw FileNotFoundException Si le fichier n'existe pas
 * @throw InvalidFormatException Si le format n'est pas supporté
 * @throw OutOfMemoryException Si mémoire GPU insuffisante
 * 
 * @pre filepath ne doit pas être vide
 * @pre Le fichier doit exister et être lisible
 * @post Si succès, handle.IsValid() == true
 * 
 * @warning En mode async, ne pas utiliser immédiatement la texture
 * @note Les textures sont automatiquement libérées (RAII)
 * 
 * @complexity O(n) où n = taille du fichier
 * @threadsafe
 * 
 * @example Chargement Synchrone
 * @code
 * try {
 *     auto texture = Texture::Load("assets/player.png");
 *     // Texture prête immédiatement
 *     renderer->DrawSprite(texture, position);
 * } catch (const FileNotFoundException& e) {
 *     logger->Error("Texture not found: {}", e.what());
 * }
 * @endcode
 * 
 * @example Chargement Asynchrone
 * @code
 * auto texture = Texture::Load("assets/large_texture.png", true);
 * 
 * // Attendre chargement
 * while (!texture.IsReady()) {
 *     std::this_thread::sleep_for(std::chrono::milliseconds(10));
 * }
 * 
 * // Maintenant utilisable
 * renderer->DrawSprite(texture, position);
 * @endcode
 * 
 * @see LoadAsync() Pour API callback-based
 * @see TextureManager Pour cache automatique
 * 
 * @author Rihen
 * @date 2026-02-06
 */
static TextureHandle Load(
    const std::string& filepath,
    bool async = false,
    bool generateMipmaps = true
);
```

---

### 3.4 Documentation des Variables Membres

**Membres Publics :**

```cpp
/**
 * @brief Position du vecteur dans l'espace 3D (composante X)
 * 
 * Coordonnée X dans un repère orthonormé direct.
 * Convention: X positif = droite
 * 
 * @invariant Valeur finie (pas NaN, pas Inf)
 */
float x;

/**
 * @brief Nombre maximal d'entités supportées
 * 
 * Limite hard-coded pour des raisons de performance.
 * Changer cette valeur nécessite recompilation.
 * 
 * @note Peut être augmenté si nécessaire (impact mémoire)
 */
static const int MAX_ENTITIES = 10000;
```

**Membres Privés :**

```cpp
/**
 * @brief Device graphique actif
 * @private
 * 
 * Pointeur non-owning vers le device graphique utilisé
 * pour toutes les opérations de rendu.
 * 
 * @invariant mDevice != nullptr après Initialize()
 * @see Initialize()
 */
GraphicsDevice* mDevice;

/**
 * @brief Indique si le système est initialisé
 * @private
 * 
 * Flag booléen mis à true après Initialize() et à false
 * après Shutdown().
 * 
 * @invariant mIsInitialized == (mDevice != nullptr)
 */
bool mIsInitialized;
```

---

### 3.5 Documentation des Enums

```cpp
/**
 * @brief Niveaux de logging disponibles
 * 
 * Les niveaux sont ordonnés par sévérité croissante.
 * Un logger configuré à un niveau N affiche tous les
 * messages de niveau >= N.
 * 
 * @example
 * @code
 * logger->SetLevel(LogLevel::WARNING);
 * 
 * logger->Trace("Not shown");    // Ignoré
 * logger->Debug("Not shown");    // Ignoré
 * logger->Info("Not shown");     // Ignoré
 * logger->Warning("Shown!");     // Affiché
 * logger->Error("Shown!");       // Affiché
 * @endcode
 * 
 * @see Logger::SetLevel()
 */
enum class LogLevel {
    /**
     * @brief Niveau de trace (très verbeux)
     * 
     * Utilisé pour traçage détaillé de l'exécution.
     * Typiquement désactivé en production.
     */
    TRACE = 0,
    
    /**
     * @brief Niveau de debug
     * 
     * Informations utiles pour le debugging.
     * Désactivé en Release/Distribution.
     */
    DEBUG = 1,
    
    /**
     * @brief Niveau d'information
     * 
     * Messages informatifs normaux.
     * Actif même en Release.
     */
    INFO = 2,
    
    /**
     * @brief Niveau d'avertissement
     * 
     * Situations anormales mais non-critiques.
     * Nécessite attention mais pas urgence.
     */
    WARNING = 3,
    
    /**
     * @brief Niveau d'erreur
     * 
     * Erreurs récupérables ayant échoué.
     * Nécessite investigation.
     */
    ERROR = 4,
    
    /**
     * @brief Niveau fatal/critique
     * 
     * Erreurs non-récupérables causant arrêt.
     * Typiquement suivi d'un crash ou abort.
     */
    FATAL = 5
};
```

---

**[Document continue...]**

Le document est trop long. Je vais créer la suite dans un fichier séparé.

## 4. Commentaires

### 4.1 Commentaires de Code

**Commentaires sur une ligne :**
```cpp
// GOOD: Explique le "pourquoi", pas le "quoi"
int timeout = 5000;  // 5 secondes: délai réseau typique

// BAD: Répète le code
int timeout = 5000;  // Assigne 5000 à timeout

// GOOD: Contexte important
mutex.lock();  // Lock requis car accès depuis render thread
```

**Commentaires multi-lignes :**
```cpp
/*
 * Algorithme de pathfinding A*
 * 
 * Complexité: O((V + E) log V) où V = nodes, E = edges
 * Heuristique: Distance euclidienne
 * 
 * Note: Optimisé pour grilles 2D régulières
 */
Path FindPath(Node* start, Node* end);
```

**Commentaires TODO/FIXME/NOTE/HACK :**
```cpp
// TODO(rihen): Implémenter cache pour éviter recalculs
// FIXME: Bug quand count == 0 (division par zéro)
// NOTE: Cette approche est 3x plus rapide que l'ancienne
// HACK: Workaround temporaire pour bug driver NVIDIA
// OPTIMIZE: Utiliser SIMD ici pour 4x speedup
// WARNING: Ne pas appeler depuis interrupt handler
```

---

### 4.2 Commentaires de Section

**Séparation de sections dans un fichier :**
```cpp
// ============================================================
// CONSTRUCTORS & DESTRUCTOR
// ============================================================

MyClass();
MyClass(const MyClass& other);
~MyClass();

// ============================================================
// PUBLIC METHODS
// ============================================================

void Initialize();
void Update(float deltaTime);
void Shutdown();

// ============================================================
// GETTERS & SETTERS
// ============================================================

int GetCount() const;
void SetCount(int value);

// ============================================================
// PRIVATE HELPERS
// ============================================================

void InternalUpdate();
void ValidateState();
```

**Sections Standards (dans l'ordre) :**
1. INCLUDES
2. FORWARD DECLARATIONS
3. CONSTANTS
4. TYPE DEFINITIONS
5. CONSTRUCTORS & DESTRUCTOR
6. OPERATORS
7. PUBLIC METHODS
8. GETTERS & SETTERS
9. STATIC METHODS
10. PROTECTED METHODS
11. PRIVATE METHODS
12. PUBLIC MEMBERS
13. PROTECTED MEMBERS
14. PRIVATE MEMBERS
15. FRIEND DECLARATIONS

---

### 4.3 Commentaires de Fermeture

**Namespaces :**
```cpp
namespace nkentseu {
    namespace core {
        namespace memory {
            
            // Code...
            
        } // namespace memory
    } // namespace core
} // namespace nkentseu
```

**Blocs longs :**
```cpp
if (condition) {
    // Beaucoup de code...
    // ... 50+ lignes ...
    
} // if (condition)

for (int i = 0; i < count; ++i) {
    // Beaucoup de code...
    // ... 50+ lignes ...
    
} // for i
```

**Include Guards :**
```cpp
#ifndef NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED
#define NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED

// Contenu

#endif // NK_CORE_NKCORE_SRC_NKCORE_NKBITMANIPULATION_H_INCLUDED
```

---

## 5. Exemples Complets

### 5.1 Classe Complète (Header)

```cpp
// -----------------------------------------------------------------------------
// FICHIER: Core\NKMath\src\NKMath\Vector\NkVector3.h
// DESCRIPTION: Vecteur 3D avec opérations mathématiques optimisées
// AUTEUR: Rihen
// DATE: 2026
// VERSION: 1.0.0
// -----------------------------------------------------------------------------

#pragma once

#ifndef NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_H_INCLUDED
#define NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_H_INCLUDED

// ============================================================
// INCLUDES
// ============================================================

#include "NKMath/NkExport.h"
#include <cmath>
#include <iosfwd>

// ============================================================
// FORWARD DECLARATIONS
// ============================================================

namespace nkentseu {
    namespace math {
        struct Vector2;
        struct Vector4;
        struct Matrix3;
        struct Matrix4;
    } // namespace math
} // namespace nkentseu

// ============================================================
// DECLARATIONS
// ============================================================

namespace nkentseu {
    namespace math {
        
        /**
         * @brief Vecteur 3D pour représenter positions, directions, et vélocités
         * 
         * Structure légère (12 bytes) représentant un vecteur 3D.
         * Fournit toutes les opérations vectorielles standard avec
         * optimisations SIMD quand disponibles.
         * 
         * Layout mémoire: [x][y][z] (contiguous, cache-friendly)
         * Alignment: 4 bytes (peut être 16 bytes si SIMD enabled)
         * 
         * @note POD type (Plain Old Data) - peut être memcpy
         * @note Toutes les opérations sont inlinées pour performance
         * 
         * @example Usage Basique
         * @code
         * Vector3 position(10.0f, 5.0f, 0.0f);
         * Vector3 velocity(1.0f, 0.0f, 0.0f);
         * 
         * position += velocity * deltaTime;
         * 
         * float distance = position.Length();
         * Vector3 direction = position.Normalized();
         * @endcode
         * 
         * @example Opérations Statiques
         * @code
         * Vector3 a(1, 0, 0);
         * Vector3 b(0, 1, 0);
         * 
         * float dot = Vector3::Dot(a, b);        // 0.0
         * Vector3 cross = Vector3::Cross(a, b);  // (0, 0, 1)
         * float angle = Vector3::Angle(a, b);    // 90 degrees
         * @endcode
         * 
         * @performance
         * - Addition/Soustraction: ~1-2 cycles
         * - Multiplication scalaire: ~2-3 cycles
         * - Dot product: ~3-4 cycles
         * - Length: ~10-15 cycles (sqrt)
         * - Normalize: ~15-20 cycles
         * 
         * @threadsafe Oui (pas de state partagé)
         * 
         * @see Vector2 Pour vecteurs 2D
         * @see Vector4 Pour vecteurs 4D ou homogènes
         * @see Quaternion Pour rotations 3D
         * 
         * @author Rihen
         * @date 2026-02-06
         * @version 1.0.0
         */
        struct NKENTSEU_CORE_API Vector3 {
            // ========================================
            // PUBLIC MEMBERS
            // ========================================
            
            /**
             * @brief Composante X (axe horizontal, droite = positif)
             */
            float x;
            
            /**
             * @brief Composante Y (axe vertical, haut = positif)
             */
            float y;
            
            /**
             * @brief Composante Z (axe profondeur, avant = positif)
             */
            float z;
            
            // ========================================
            // CONSTRUCTORS
            // ========================================
            
            /**
             * @brief Constructeur par défaut (vecteur nul)
             * @post x == 0, y == 0, z == 0
             */
            Vector3();
            
            /**
             * @brief Constructeur avec composantes explicites
             * @param x Composante X
             * @param y Composante Y
             * @param z Composante Z
             */
            Vector3(float x, float y, float z);
            
            /**
             * @brief Constructeur avec scalaire (toutes composantes égales)
             * @param scalar Valeur pour x, y, et z
             * @post x == scalar, y == scalar, z == scalar
             */
            explicit Vector3(float scalar);
            
            /**
             * @brief Constructeur depuis Vector2 + z
             * @param xy Vecteur 2D pour x et y
             * @param z Composante z (défaut: 0)
             */
            explicit Vector3(const Vector2& xy, float z = 0.0f);
            
            // ========================================
            // ARITHMETIC OPERATORS
            // ========================================
            
            /**
             * @brief Addition de vecteurs
             * @complexity O(1)
             */
            Vector3 operator+(const Vector3& other) const;
            
            /**
             * @brief Soustraction de vecteurs
             * @complexity O(1)
             */
            Vector3 operator-(const Vector3& other) const;
            
            /**
             * @brief Multiplication par scalaire
             * @complexity O(1)
             */
            Vector3 operator*(float scalar) const;
            
            /**
             * @brief Division par scalaire
             * @pre scalar != 0
             * @complexity O(1)
             */
            Vector3 operator/(float scalar) const;
            
            /**
             * @brief Négation (inverse direction)
             * @complexity O(1)
             */
            Vector3 operator-() const;
            
            // ========================================
            // COMPOUND ASSIGNMENT OPERATORS
            // ========================================
            
            Vector3& operator+=(const Vector3& other);
            Vector3& operator-=(const Vector3& other);
            Vector3& operator*=(float scalar);
            Vector3& operator/=(float scalar);
            
            // ========================================
            // COMPARISON OPERATORS
            // ========================================
            
            /**
             * @brief Égalité exacte
             * @note Pour comparaison avec epsilon, voir Equals()
             */
            bool operator==(const Vector3& other) const;
            
            bool operator!=(const Vector3& other) const;
            
            // ========================================
            // METHODS
            // ========================================
            
            /**
             * @brief Calcule la longueur (magnitude) du vecteur
             * @return sqrt(x² + y² + z²)
             * @complexity O(1) - environ 10-15 cycles
             */
            float Length() const;
            
            /**
             * @brief Calcule la longueur au carré (évite sqrt)
             * @return x² + y² + z²
             * @note Plus rapide que Length() si comparaison seulement
             * @complexity O(1) - environ 3-4 cycles
             */
            float LengthSquared() const;
            
            /**
             * @brief Normalise le vecteur (longueur = 1)
             * @pre Length() > 0
             * @post Length() ~= 1.0f (avec erreur float)
             * @note Si vecteur nul, aucun changement
             */
            void Normalize();
            
            /**
             * @brief Retourne version normalisée (const)
             * @return Vecteur de longueur 1 dans même direction
             */
            Vector3 Normalized() const;
            
            /**
             * @brief Teste si le vecteur est nul (epsilon)
             * @param epsilon Tolérance (défaut: 1e-6f)
             * @return true si Length() < epsilon
             */
            bool IsZero(float epsilon = 1e-6f) const;
            
            /**
             * @brief Teste égalité avec epsilon
             * @param other Vecteur à comparer
             * @param epsilon Tolérance par composante
             * @return true si |x-other.x| < epsilon pour x,y,z
             */
            bool Equals(const Vector3& other, float epsilon = 1e-6f) const;
            
            // ========================================
            // STATIC METHODS
            // ========================================
            
            /**
             * @brief Produit scalaire (dot product)
             * @param a Premier vecteur
             * @param b Deuxième vecteur
             * @return a.x*b.x + a.y*b.y + a.z*b.z
             * @complexity O(1)
             */
            static float Dot(const Vector3& a, const Vector3& b);
            
            /**
             * @brief Produit vectoriel (cross product)
             * @param a Premier vecteur
             * @param b Deuxième vecteur
             * @return Vecteur perpendiculaire à a et b
             * @note |result| = |a| * |b| * sin(angle)
             * @note Direction: règle de la main droite
             */
            static Vector3 Cross(const Vector3& a, const Vector3& b);
            
            /**
             * @brief Distance entre deux points
             */
            static float Distance(const Vector3& a, const Vector3& b);
            
            /**
             * @brief Interpolation linéaire
             * @param a Point de départ (t=0)
             * @param b Point d'arrivée (t=1)
             * @param t Facteur d'interpolation [0, 1]
             * @return a + t * (b - a)
             * @note t n'est pas clampé automatiquement
             */
            static Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
            
            /**
             * @brief Angle entre deux vecteurs
             * @return Angle en radians [0, π]
             */
            static float Angle(const Vector3& a, const Vector3& b);
            
            /**
             * @brief Réflexion d'un vecteur par rapport à une normale
             * @param incident Vecteur incident
             * @param normal Normale de surface (doit être normalisée)
             * @return Vecteur réfléchi
             */
            static Vector3 Reflect(const Vector3& incident, const Vector3& normal);
            
            /**
             * @brief Projection de a sur b
             */
            static Vector3 Project(const Vector3& a, const Vector3& b);
            
            // ========================================
            // STATIC CONSTANTS
            // ========================================
            
            /**
             * @brief Vecteur nul (0, 0, 0)
             */
            static const Vector3 ZERO;
            
            /**
             * @brief Vecteur unitaire (1, 1, 1)
             */
            static const Vector3 ONE;
            
            /**
             * @brief Axe X unitaire (1, 0, 0) - Droite
             */
            static const Vector3 RIGHT;
            
            /**
             * @brief Axe Y unitaire (0, 1, 0) - Haut
             */
            static const Vector3 UP;
            
            /**
             * @brief Axe Z unitaire (0, 0, 1) - Avant
             */
            static const Vector3 FORWARD;
            
            /**
             * @brief -X (gauche)
             */
            static const Vector3 LEFT;
            
            /**
             * @brief -Y (bas)
             */
            static const Vector3 DOWN;
            
            /**
             * @brief -Z (arrière)
             */
            static const Vector3 BACK;
        };
        
        // ========================================
        // NON-MEMBER OPERATORS
        // ========================================
        
        /**
         * @brief Multiplication scalaire (scalar * vector)
         * @note Permet écriture: 2.0f * vec au lieu de vec * 2.0f
         */
        NKENTSEU_CORE_API Vector3 operator*(float scalar, const Vector3& vec);
        
        /**
         * @brief Stream output operator
         */
        NKENTSEU_CORE_API std::ostream& operator<<(std::ostream& os, const Vector3& vec);
        
    } // namespace math
} // namespace nkentseu

// ============================================================
// INLINE IMPLEMENTATIONS
// ============================================================

#include "NkVector3.inl"

#endif // NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_H_INCLUDED

// ============================================================
// Copyright © 2024-2026 Rihen. All rights reserved.
// Proprietary License - Free to use and modify
//
// Generated by Rihen on 2026-02-06 15:00:00
// Creation Date: 2026-02-06 15:00:00
// ============================================================
```

---

### 5.2 Classe Complète (Source)

```cpp
// -----------------------------------------------------------------------------
// FICHIER: Core\NKMath\src\NKMath\Vector\NkVector3.cpp
// DESCRIPTION: Implémentation du vecteur 3D
// AUTEUR: Rihen
// DATE: 2026
// VERSION: 1.0.0
// -----------------------------------------------------------------------------

// ============================================================
// INCLUDES
// ============================================================

#include "NkVector3.h"
#include "NkVector2.h"
#include "NKCore/Assert/NkAssert.h"
#include <cmath>
#include <ostream>

// ============================================================
// USING DECLARATIONS
// ============================================================

using namespace nkentseu::math;

// ============================================================
// STATIC CONSTANTS
// ============================================================

const Vector3 Vector3::ZERO(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::ONE(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::RIGHT(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::UP(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::FORWARD(0.0f, 0.0f, 1.0f);
const Vector3 Vector3::LEFT(-1.0f, 0.0f, 0.0f);
const Vector3 Vector3::DOWN(0.0f, -1.0f, 0.0f);
const Vector3 Vector3::BACK(0.0f, 0.0f, -1.0f);

// ============================================================
// IMPLEMENTATIONS
// ============================================================

namespace nkentseu {
    namespace math {
        
        // ========================================
        // CONSTRUCTORS
        // ========================================
        
        Vector3::Vector3()
            : x(0.0f), y(0.0f), z(0.0f)
        {
        }
        
        Vector3::Vector3(float x, float y, float z)
            : x(x), y(y), z(z)
        {
        }
        
        Vector3::Vector3(float scalar)
            : x(scalar), y(scalar), z(scalar)
        {
        }
        
        Vector3::Vector3(const Vector2& xy, float z)
            : x(xy.x), y(xy.y), z(z)
        {
        }
        
        // ========================================
        // METHODS
        // ========================================
        
        float Vector3::Length() const {
            return std::sqrt(x * x + y * y + z * z);
        }
        
        float Vector3::LengthSquared() const {
            return x * x + y * y + z * z;
        }
        
        void Vector3::Normalize() {
            float len = Length();
            if (len > 1e-6f) {  // Éviter division par zéro
                float invLen = 1.0f / len;
                x *= invLen;
                y *= invLen;
                z *= invLen;
            }
        }
        
        Vector3 Vector3::Normalized() const {
            Vector3 result = *this;
            result.Normalize();
            return result;
        }
        
        bool Vector3::IsZero(float epsilon) const {
            return LengthSquared() < epsilon * epsilon;
        }
        
        bool Vector3::Equals(const Vector3& other, float epsilon) const {
            return std::abs(x - other.x) < epsilon &&
                   std::abs(y - other.y) < epsilon &&
                   std::abs(z - other.z) < epsilon;
        }
        
        // ========================================
        // STATIC METHODS
        // ========================================
        
        float Vector3::Dot(const Vector3& a, const Vector3& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
        
        Vector3 Vector3::Cross(const Vector3& a, const Vector3& b) {
            return Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }
        
        float Vector3::Distance(const Vector3& a, const Vector3& b) {
            return (b - a).Length();
        }
        
        Vector3 Vector3::Lerp(const Vector3& a, const Vector3& b, float t) {
            return a + (b - a) * t;
        }
        
        float Vector3::Angle(const Vector3& a, const Vector3& b) {
            float dot = Dot(a, b);
            float lenProduct = a.Length() * b.Length();
            
            if (lenProduct < 1e-6f) {
                return 0.0f;
            }
            
            // Clamp pour éviter erreurs numériques avec acos
            float cosAngle = std::clamp(dot / lenProduct, -1.0f, 1.0f);
            return std::acos(cosAngle);
        }
        
        Vector3 Vector3::Reflect(const Vector3& incident, const Vector3& normal) {
            NK_ASSERT(std::abs(normal.Length() - 1.0f) < 1e-3f, 
                     "Normal must be normalized");
            
            return incident - normal * (2.0f * Dot(incident, normal));
        }
        
        Vector3 Vector3::Project(const Vector3& a, const Vector3& b) {
            float lenSq = b.LengthSquared();
            if (lenSq < 1e-6f) {
                return Vector3::ZERO;
            }
            
            return b * (Dot(a, b) / lenSq);
        }
        
        // ========================================
        // NON-MEMBER OPERATORS
        // ========================================
        
        Vector3 operator*(float scalar, const Vector3& vec) {
            return vec * scalar;
        }
        
        std::ostream& operator<<(std::ostream& os, const Vector3& vec) {
            os << "Vector3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
            return os;
        }
        
    } // namespace math
} // namespace nkentseu

// ============================================================
// Copyright © 2024-2026 Rihen. All rights reserved.
// Proprietary License - Free to use and modify
//
// Generated by Rihen on 2026-02-06 15:00:00
// Creation Date: 2026-02-06 15:00:00
// ============================================================
```

---

### 5.3 Fichier Inline (.inl)

```cpp
// -----------------------------------------------------------------------------
// FICHIER: Core\NKMath\src\NKMath\Vector\NkVector3.inl
// DESCRIPTION: Implémentations inline du vecteur 3D
// AUTEUR: Rihen
// DATE: 2026
// VERSION: 1.0.0
// -----------------------------------------------------------------------------

#ifndef NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_INL_INCLUDED
#define NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_INL_INCLUDED

namespace nkentseu {
    namespace math {
        
        // ========================================
        // ARITHMETIC OPERATORS
        // ========================================
        
        inline Vector3 Vector3::operator+(const Vector3& other) const {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }
        
        inline Vector3 Vector3::operator-(const Vector3& other) const {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }
        
        inline Vector3 Vector3::operator*(float scalar) const {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }
        
        inline Vector3 Vector3::operator/(float scalar) const {
            NK_ASSERT(scalar != 0.0f, "Division by zero");
            float inv = 1.0f / scalar;
            return Vector3(x * inv, y * inv, z * inv);
        }
        
        inline Vector3 Vector3::operator-() const {
            return Vector3(-x, -y, -z);
        }
        
        // ========================================
        // COMPOUND ASSIGNMENT OPERATORS
        // ========================================
        
        inline Vector3& Vector3::operator+=(const Vector3& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }
        
        inline Vector3& Vector3::operator-=(const Vector3& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }
        
        inline Vector3& Vector3::operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }
        
        inline Vector3& Vector3::operator/=(float scalar) {
            NK_ASSERT(scalar != 0.0f, "Division by zero");
            float inv = 1.0f / scalar;
            x *= inv;
            y *= inv;
            z *= inv;
            return *this;
        }
        
        // ========================================
        // COMPARISON OPERATORS
        // ========================================
        
        inline bool Vector3::operator==(const Vector3& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
        
        inline bool Vector3::operator!=(const Vector3& other) const {
            return !(*this == other);
        }
        
    } // namespace math
} // namespace nkentseu

#endif // NK_NKMATH_SRC_NKMATH_VECTOR_NKVECTOR3_INL_INCLUDED

// ============================================================
// Copyright © 2024-2026 Rihen. All rights reserved.
// Proprietary License - Free to use and modify
//
// Generated by Rihen on 2026-02-06 15:00:00
// Creation Date: 2026-02-06 15:00:00
// ============================================================
```

---

**[Suite dans le prochain message pour éviter trop de longueur...]**

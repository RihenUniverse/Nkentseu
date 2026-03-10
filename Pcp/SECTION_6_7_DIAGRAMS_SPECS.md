# Sections 6-7 : Diagrammes et Spécifications Techniques

## 6. Diagrammes de Conception

### 6.1 Diagramme de Classes - Core Memory System

```
                    ┌─────────────────────────┐
                    │      IAllocator         │
                    │  <<interface>>          │
                    ├─────────────────────────┤
                    │ + Allocate(size)        │
                    │ + Deallocate(ptr)       │
                    │ + GetUsedMemory()       │
                    │ + GetTotalMemory()      │
                    └────────────▲────────────┘
                                 │
                                 │ implements
          ┌──────────────────────┼──────────────────────┐
          │                      │                      │
┌─────────┴────────────┐ ┌───────┴────────┐  ┌─────────┴──────────┐
│  LinearAllocator     │ │PoolAllocator   │  │  FreeListAllocator │
├──────────────────────┤ ├────────────────┤  ├────────────────────┤
│- buffer: void*       │ │- chunks: []    │  │- freeBlocks: List  │
│- offset: size_t      │ │- chunkSize     │  │- allocations: Map  │
├──────────────────────┤ ├────────────────┤  ├────────────────────┤
│+ Allocate()          │ │+ Allocate()    │  │+ Allocate()        │
│+ Reset()             │ │+ Free()        │  │+ Deallocate()      │
└──────────────────────┘ └────────────────┘  │+ CoalesceBlocks()  │
                                             └────────────────────┘

┌────────────────────┐         ┌────────────────────────┐
│ ProxyAllocator     │─────────│   MemoryTracker       │
├────────────────────┤ wraps   ├────────────────────────┤
│- underlying: ptr   │────────▶│- allocations: HashMap  │
│- tracker: ptr      │         │- totalAllocated        │
├────────────────────┤         │- allocationCount       │
│+ Allocate()        │         ├────────────────────────┤
│+ Deallocate()      │         │+ RecordAllocation()    │
│+ GetStats()        │         │+ RecordDeallocation()  │
└────────────────────┘         │+ GetReport()           │
                               │+ DetectLeaks()         │
                               └────────────────────────┘

              ┌────────────────────────┐
              │   MemoryArena          │
              ├────────────────────────┤
              │- allocator: IAllocator*│
              │- allocations: []       │
              ├────────────────────────┤
              │+ AllocateBlock(size)   │
              │+ FreeAll()             │
              │+ Reset()               │
              └────────────────────────┘
```

**Relations:**
- `IAllocator` : Interface de base pour tous les allocateurs
- `LinearAllocator` : Allocation séquentielle (bump allocator)
- `PoolAllocator` : Objets de taille fixe
- `FreeListAllocator` : General purpose avec réutilisation
- `ProxyAllocator` : Wrapper avec tracking
- `MemoryTracker` : Profiling et leak detection
- `MemoryArena` : Grouped allocations avec lifetime lié

---

### 6.2 Diagramme de Séquence - Window Creation et Event Loop

```
User          WindowFactory    Window(Platform)   OS/Platform    EventManager    InputManager
 │                 │                  │                │               │               │
 │─CreateWindow──>│                  │                │               │               │
 │                 │                  │                │               │               │
 │                 │─Create(config)─>│                │               │               │
 │                 │                  │                │               │               │
 │                 │                  │─CreateWindow──>│               │               │
 │                 │                  │                │               │               │
 │                 │                  │<──HWND/Handle──│               │               │
 │                 │                  │                │               │               │
 │                 │                  │─InitGL/VK/DX──>│               │               │
 │                 │                  │                │               │               │
 │                 │                  │<──Context──────┤               │               │
 │                 │                  │                │               │               │
 │                 │                  │─Register──────────────────────>│               │
 │                 │                  │                │               │               │
 │                 │<──Window ptr─────│                │               │               │
 │                 │                  │                │               │               │
 │<──Window ptr───┤                  │                │               │               │
 │                                    │                │               │               │
│─────────────────── GAME LOOP ───────────────────────────────────────────────────────│
 │                                    │                │               │               │
│─PollEvents()──────────────────────>│                │               │               │
 │                                    │                │               │               │
 │                                    │─GetMessages───>│               │               │
 │                                    │                │               │               │
 │                                    │<──MSG array────┤               │               │
 │                                    │                │               │               │
 │                                    │─TranslateEvent─────────────────>│               │
 │                                    │                │               │               │
 │                                    │                │               │<─OnKeyPress───┤
 │                                    │                │               │               │
 │                                    │                │               │─UpdateState──>│
 │                                    │                │               │               │
 │                                    │<───Event───────────────────────┤               │
 │                                    │                │               │               │
 │<───────────────────────────────────│                │               │               │
 │                                                                                     │
│─SetEventCallback(lambda)──────────────────────────────────────────>│               │
 │                                                                     │               │
[User presses key]                                                    │               │
 │                                                                     │               │
 │<─OnEvent(KeyPressedEvent)──────────────────────────────────────────┤               │
```

---

### 6.3 Diagramme d'États - Application Lifecycle

```
                         ┌─────────┐
                         │ START   │
                         └────┬────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │ INITIALIZATION   │
                    │                  │
                    │ • Parse args     │
                    │ • Load config    │
                    │ • Init subsystems│
                    │ • Create window  │
                    │ • Load initial   │
                    │   assets         │
                    └────────┬─────────┘
                             │
                             │ success
                             ▼
                      ┌──────────────┐
                      │   RUNNING    │◄──────────────┐
                      │              │               │
                      │ • Poll events│               │
                      │ • Update     │               │
                      │ • Render     │               │
                      │ • Present    │               │
                      └──┬───────┬───┘               │
                         │       │                   │
              ┌──────────┘       └────────┐          │
              │                           │          │
              ▼                           ▼          │
        ┌──────────┐              ┌──────────────┐   │
        │  PAUSED  │              │  LOADING     │   │
        │          │              │              │   │
        │ • No     │              │ • Stream     │   │
        │   update │              │   assets     │   │
        │ • Show   │              │ • Show       │   │
        │   menu   │              │   progress   │   │
        └────┬─────┘              └──────┬───────┘   │
             │                           │           │
             │ resume                    │ complete  │
             └───────────────────────────┴───────────┘
                         │
                         │ quit requested
                         │ (ESC, Alt+F4, etc.)
                         ▼
                 ┌───────────────┐
                 │   SHUTDOWN    │
                 │               │
                 │ • Save state  │
                 │ • Free GPU    │
                 │   resources   │
                 │ • Free CPU    │
                 │   memory      │
                 │ • Close       │
                 │   window      │
                 │ • Cleanup     │
                 │   subsystems  │
                 └───────┬───────┘
                         │
                         ▼
                    ┌─────────┐
                    │   END   │
                    └─────────┘
```

**Transitions:**
- `START → INITIALIZATION` : Automatique
- `INITIALIZATION → RUNNING` : Si init succès
- `INITIALIZATION → END` : Si init échoue
- `RUNNING → PAUSED` : Sur pause input ou focus perdu
- `RUNNING → LOADING` : Sur load scene/level
- `PAUSED → RUNNING` : Sur resume
- `LOADING → RUNNING` : Quand load terminé
- `RUNNING → SHUTDOWN` : Sur quit request
- `SHUTDOWN → END` : Cleanup terminé

---

### 6.4 Diagramme de Déploiement Multi-Plateforme

```
┌─────────────────────────────────────────────────────────────────┐
│                   DEVELOPMENT ENVIRONMENT                       │
│                                                                 │
│  ┌──────────────────┐              ┌──────────────────┐        │
│  │  Source Code     │              │  Build System    │        │
│  │  (.h / .cpp)     │─────────────▶│  (.jenga files)  │        │
│  └──────────────────┘              └─────────┬────────┘        │
│                                               │                 │
│                                               ▼                 │
│                                    ┌──────────────────┐         │
│                                    │   Compiler       │         │
│                                    │  MSVC/GCC/Clang  │         │
│                                    └─────────┬────────┘         │
│                                              │                  │
└──────────────────────────────────────────────┼──────────────────┘
                                               │
                ┌──────────────────────────────┼──────────────────┐
                │                              │                  │
                ▼                              ▼                  ▼
    ┌─────────────────────┐      ┌─────────────────────┐  ┌──────────────┐
    │   WINDOWS TARGET    │      │    LINUX TARGET     │  │ macOS TARGET │
    │                     │      │                     │  │              │
    │  ┌───────────────┐  │      │  ┌───────────────┐  │  │ ┌──────────┐│
    │  │  .exe         │  │      │  │  ELF binary   │  │  │ │  .app    ││
    │  │  + DLLs       │  │      │  │  + .so libs   │  │  │ │  bundle  ││
    │  │  + assets/    │  │      │  │  + assets/    │  │  │ │ +assets/ ││
    │  └───────────────┘  │      │  └───────────────┘  │  │ └──────────┘│
    │                     │      │                     │  │              │
    │  Graphics:          │      │  Graphics:          │  │  Graphics:   │
    │  • DirectX 12       │      │  • OpenGL 4.6       │  │  • Metal 2   │
    │  • Vulkan           │      │  • Vulkan           │  │  • Vulkan    │
    └─────────────────────┘      └─────────────────────┘  └──────────────┘

    ┌─────────────────────┐      ┌─────────────────────┐  ┌──────────────┐
    │   MOBILE TARGETS    │      │    WEB TARGET       │  │   CONSOLE    │
    │                     │      │                     │  │              │
    │  ┌───────────────┐  │      │  ┌───────────────┐  │  │ ┌──────────┐│
    │  │  Android      │  │      │  │  .wasm        │  │  │ │PlayStation│
    │  │  .apk         │  │      │  │  .html        │  │  │ │  Xbox    ││
    │  │  ARM64-v8a    │  │      │  │  .js          │  │  │ │  Switch  ││
    │  └───────────────┘  │      │  └───────────────┘  │  │ └──────────┘│
    │                     │      │                     │  │              │
    │  ┌───────────────┐  │      │  Graphics:          │  │  Requires    │
    │  │  iOS          │  │      │  • WebGL 2.0        │  │  NDAs &      │
    │  │  .ipa         │  │      │  • WebGPU (future)  │  │  SDKs        │
    │  │  ARM64        │  │      │                     │  │              │
    │  └───────────────┘  │      │  Emscripten compile │  │              │
    │                     │      │                     │  │              │
    │  Graphics:          │      └─────────────────────┘  └──────────────┘
    │  • OpenGL ES 3.2    │
    │  • Vulkan Mobile    │
    │  • Metal (iOS)      │
    └─────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                       ASSET PIPELINE                            │
│                                                                 │
│  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐  │
│  │  Raw Assets  │─────▶│  Asset       │─────▶│  Platform    │  │
│  │              │      │  Processor   │      │  Optimized   │  │
│  │ • .png/.jpg  │      │              │      │  Assets      │  │
│  │ • .fbx/.obj  │      │ • Compress   │      │              │  │
│  │ • .wav/.ogg  │      │ • Convert    │      │ • DDS (Win)  │  │
│  │ • .glsl      │      │ • Optimize   │      │ • KTX (GL)   │  │
│  └──────────────┘      └──────────────┘      │ • PVR (iOS)  │  │
│                                              │ • ASTC (Mob) │  │
│                                              └──────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

### 6.5 Diagramme de Flux de Données - Frame Rendering

```
 Frame Start
      │
      ▼
┌───────────────────┐
│  Input Polling    │
│  • Keyboard       │
│  • Mouse          │
│  • Gamepad        │
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Game Logic       │
│  Update           │
│  • AI             │
│  • Scripts        │
│  • Animations     │
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Physics          │
│  Simulation       │
│  • Collision      │
│  • Integration    │
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Scene Graph      │
│  Update           │
│  • Transforms     │
│  • Hierarchies    │
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Culling          │
│  • Frustum        │
│  • Occlusion      │
│  • LOD selection  │
└─────────┬─────────┘
          │
          ▼
┌───────────────────┐
│  Render Queue     │
│  Building         │
│  • Sort opaque    │
│  • Sort transp    │
│  • Batching       │
└─────────┬─────────┘
          │
          ├─────────────────────────────┐
          │                             │
          ▼                             ▼
┌───────────────────┐         ┌───────────────────┐
│  Shadow Pass      │         │  Main Pass        │
│  • Render depth   │         │  • Opaque         │
│  • Cascades       │         │  • Skybox         │
└─────────┬─────────┘         │  • Transparent    │
          │                   └─────────┬─────────┘
          │                             │
          └──────────────┬──────────────┘
                         │
                         ▼
              ┌───────────────────┐
              │  Post-Processing  │
              │  • SSAO           │
              │  • Bloom          │
              │  • Tonemap        │
              │  • AA             │
              └─────────┬─────────┘
                        │
                        ▼
              ┌───────────────────┐
              │  UI Rendering     │
              │  • Overlays       │
              │  • HUD            │
              │  • Debug          │
              └─────────┬─────────┘
                        │
                        ▼
              ┌───────────────────┐
              │  Present          │
              │  • Swap buffers   │
              │  • VSync          │
              └───────────────────┘
                        │
                        ▼
                   Frame End
```

---

## 7. Spécifications Techniques

### 7.1 Standards et Technologies

#### 7.1.1 Langages et Standards C++

**Minimum Required:**
- C++17
- Compatible STL
- Exception handling enabled
- RTTI enabled (pour réflexion)

**Recommended:**
- C++20 (concepts, coroutines, modules)
- C++23 ready (std::expected, etc.)

**Forbidden Features:**
- `goto` statements (except for error cleanup in C-style APIs)
- `union` with non-POD types
- Multiple inheritance (except for interfaces)
- `new`/`delete` direct (use smart pointers)

**Compiler Support:**
```cpp
// Minimum versions
MSVC    : 19.20+ (VS 2019)
GCC     : 9.0+
Clang   : 10.0+
AppleClang: 12.0+
Emscripten: 3.1.0+
```

---

#### 7.1.2 Graphics APIs

**OpenGL:**
- Version: 4.5+ (Core Profile)
- Extensions: ARB_direct_state_access, ARB_multi_draw_indirect
- Loader: GLAD or custom

**Vulkan:**
- Version: 1.3
- Extensions: VK_KHR_swapchain, VK_KHR_dynamic_rendering
- Validation layers en debug

**DirectX 12:**
- Feature Level: 12_0 minimum
- Windows 10 1903+
- DirectX Shader Compiler (DXC)

**Metal:**
- Version: 2.4+
- macOS 10.15+, iOS 13+
- Metal Shading Language 2.4

**WebGL:**
- Version: 2.0
- Extensions: WEBGL_multi_draw, EXT_color_buffer_float
- WebGPU en considération pour l'avenir

---

#### 7.1.3 Shader Languages

**Cross-Platform Strategy:**
- Source: GLSL 4.50 (base)
- Transpilation: SPIRV-Cross
- Targets: GLSL, HLSL, MSL, SPIRV

**Example Pipeline:**
```
MyShader.vert.glsl ──▶ glslangValidator ──▶ MyShader.spv
                                              │
                      ┌───────────────────────┼───────────────────┐
                      │                       │                   │
                      ▼                       ▼                   ▼
              SPIRV-Cross               SPIRV-Cross         SPIRV-Cross
                      │                       │                   │
                      ▼                       ▼                   ▼
               MyShader.vert.glsl      MyShader.hlsl       MyShader.metal
               (OpenGL/WebGL)          (DirectX 12)        (Metal)
```

**Shader Reflection:**
- Automatic uniform detection
- Texture binding points
- Vertex attribute locations
- Push constants (Vulkan)

---

#### 7.1.4 Audio APIs

**Primary:**
- OpenAL Soft (cross-platform)
- Supports: WAV, OGG, MP3, FLAC

**Platform-Specific:**
- Windows: XAudio2 (optionnel)
- macOS/iOS: CoreAudio (optionnel)
- Android: OpenSL ES
- Web: Web Audio API

**3D Audio:**
- HRTF support
- Doppler effect
- Reverb zones
- Occlusion/obstruction

---

### 7.2 Conventions de Code

#### 7.2.1 Naming Conventions

```cpp
// Namespaces: PascalCase
namespace Nk {
namespace Graphics {
namespace Internal {
}}}

// Classes, Structs, Enums: PascalCase
class RenderSystem { };
struct TransformComponent { };
enum class LogLevel { Trace, Debug, Info };

// Functions, Methods: PascalCase
void Initialize();
void UpdateTransform(Entity entity);
Matrix4 GetViewMatrix() const;

// Variables, Parameters: camelCase
int entityCount = 0;
float deltaTime = 0.016f;
const char* filename = "data.bin";

// Member variables: camelCase with optional 'm_' prefix
class Player {
private:
    int health;         // Preferred
    int m_maxHealth;    // Acceptable (for clarity)
    float m_speed;
};

// Constants: UPPER_SNAKE_CASE or kPascalCase
const int MAX_ENTITIES = 10000;
constexpr float kPi = 3.14159f;
constexpr size_t kDefaultBufferSize = 1024;

// Macros: UPPER_SNAKE_CASE with NK_ prefix
#define NK_ASSERT(condition, ...) /* ... */
#define NK_LOG_INFO(...) /* ... */

// Template parameters: PascalCase with 'T' prefix (optional)
template<typename T>
template<typename TKey, typename TValue>

// File names: Match class name
// RenderSystem.h / RenderSystem.cpp
// Vector3.h / Vector3.cpp

// Acronyms: PascalCase (not all caps in names)
class HttpClient { };  // Not HTTPClient
class XmlParser { };   // Not XMLParser
void ProcessAIBehavior(); // AIBehavior OK (clearly readable)
```

---

#### 7.2.2 File Organization

**Header (.h):**
```cpp
#pragma once  // Preferred over include guards

#include "NKCore/NkExport.h"  // Public API export macro
#include <cstdint>             // System headers
#include <string>

// Forward declarations
namespace Nk {
    class Texture;
    class Shader;
}

namespace Nk {

/**
 * @brief Represents a 3D vector
 * 
 * Provides common vector operations including arithmetic,
 * normalization, and dot/cross products.
 * 
 * @example
 * Vector3 v1(1, 2, 3);
 * Vector3 v2(4, 5, 6);
 * Vector3 result = v1 + v2;
 * float length = result.Length();
 */
class NKENTSEU_CORE_API Vector3 {
public:
    // ===== Constructors =====
    Vector3();
    Vector3(float x, float y, float z);
    explicit Vector3(float scalar);
    
    // ===== Operators =====
    Vector3 operator+(const Vector3& other) const;
    Vector3& operator+=(const Vector3& other);
    
    // ===== Methods =====
    float Length() const;
    float LengthSquared() const;
    void Normalize();
    Vector3 Normalized() const;
    
    // ===== Static Methods =====
    static Vector3 Zero();
    static Vector3 One();
    static float Dot(const Vector3& a, const Vector3& b);
    static Vector3 Cross(const Vector3& a, const Vector3& b);
    
    // ===== Public Members =====
    float x, y, z;
    
private:
    // ===== Private Methods =====
    void InternalUpdate();
};

} // namespace Nk
```

**Source (.cpp):**
```cpp
#include "NKMath/Vector3.h"
#include <cmath>

namespace Nk {

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

Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3& Vector3::operator+=(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

float Vector3::Length() const {
    return std::sqrt(x * x + y * y + z * z);
}

float Vector3::LengthSquared() const {
    return x * x + y * y + z * z;
}

void Vector3::Normalize() {
    float len = Length();
    if (len > 0.0f) {
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

Vector3 Vector3::Zero() {
    return Vector3(0.0f, 0.0f, 0.0f);
}

Vector3 Vector3::One() {
    return Vector3(1.0f, 1.0f, 1.0f);
}

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

} // namespace Nk
```

---

#### 7.2.3 Code Style Guidelines

**Bracing Style: Allman (opening brace on new line)**
```cpp
void Function()
{
    if (condition)
    {
        // code
    }
    else
    {
        // code
    }
}
```

**Indentation:**
- 4 spaces (no tabs)
- Continuation lines: +4 spaces

**Line Length:**
- Soft limit: 100 characters
- Hard limit: 120 characters

**Include Order:**
```cpp
// 1. Corresponding header (for .cpp files)
#include "MyClass.h"

// 2. Blank line

// 3. Nkentseu headers
#include "NKCore/Memory.h"
#include "NKMath/Vector3.h"

// 4. Blank line

// 5. Third-party headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// 6. Blank line

// 7. Standard library headers
#include <vector>
#include <memory>
#include <string>
```

**Comments:**
```cpp
// Single-line comment for brief explanations

/**
 * Multi-line comment for detailed explanations,
 * function documentation, class descriptions.
 * 
 * @param deltaTime Time since last frame in seconds
 * @return True if update was successful
 */
bool Update(float deltaTime);

// TODO: Implement feature X
// FIXME: Bug in edge case Y
// NOTE: This is important information
// HACK: Temporary workaround for Z
```

**Const Correctness:**
```cpp
// Prefer const for variables that don't change
const int maxEntities = 1000;
const std::string& GetName() const; // const method

// Prefer const references for input parameters
void ProcessEntity(const Entity& entity);

// Use constexpr for compile-time constants
constexpr float kPi = 3.14159f;
constexpr size_t kBufferSize = 1024;
```

**Modern C++ Features:**
```cpp
// Use auto for complex types
auto it = container.begin();
auto ptr = std::make_unique<MyClass>();

// Range-based for loops
for (const auto& entity : entities) {
    entity.Update();
}

// nullptr instead of NULL or 0
MyClass* ptr = nullptr;

// enum class instead of enum
enum class State { Idle, Running, Paused };

// = default and = delete
MyClass() = default;
MyClass(const MyClass&) = delete;

// Smart pointers instead of raw new/delete
auto ptr = std::make_unique<MyClass>();
auto shared = std::make_shared<MyClass>();
```

---

### 7.3 Build System Configuration

#### 7.3.1 .jenga File Format

```lua
-- Nkentseu.jenga (solution root)
solution "Nkentseu"
    configurations { "Debug", "Release", "Distribution" }
    platforms { "x64", "ARM64" }
    language "C++"
    cppdialect "C++17"
    
    -- Output directories
    targetdir ("bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
    objdir ("bin-int/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")
    
    -- Global include directories
    includedirs {
        "Core/NKCore/src",
        "Core/Nkentseu/src",
        "Core/NKLogger/src",
        "Core/NKMath/src",
        "Core/NKWindow/src",
        "externals/glad/include",
        "externals/glm/include"
    }
    
    -- Platform-specific settings
    filter "system:windows"
        systemversion "latest"
        defines { "NK_PLATFORM_WINDOWS" }
        
    filter "system:linux"
        defines { "NK_PLATFORM_LINUX" }
        links { "pthread", "dl", "X11", "Xxf86vm", "Xrandr", "Xi" }
        
    filter "system:macosx"
        defines { "NK_PLATFORM_MACOS" }
        links { "Cocoa.framework", "IOKit.framework", "CoreVideo.framework" }
    
    -- Configuration-specific settings
    filter "configurations:Debug"
        defines { "NK_DEBUG", "NK_ENABLE_ASSERTS", "NK_ENABLE_PROFILING" }
        symbols "On"
        runtime "Debug"
        optimize "Off"
        
    filter "configurations:Release"
        defines { "NK_RELEASE" }
        symbols "On"
        runtime "Release"
        optimize "Speed"
        
    filter "configurations:Distribution"
        defines { "NK_DIST" }
        symbols "Off"
        runtime "Release"
        optimize "Full"
        flags { "LinkTimeOptimization", "NoRuntimeChecks" }
    
    -- Projects
    include "Core/NKCore"
    include "Core/Nkentseu"
    include "Core/NKLogger"
    include "Core/NKMath"
    include "Core/NKWindow"
    include "Sandbox/SandboxCore"
```

```lua
-- NKCore/NKCore.jenga (project)
project "NKCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
    
    -- Precompiled header
    pchheader "pch.h"
    pchsource "pch/pch.cpp"
    
    files {
        "src/**.h",
        "src/**.cpp",
        "pch/**.h",
        "pch/**.cpp"
    }
    
    includedirs {
        "src",
        "pch",
        "%{wks.location}/externals/spdlog/include"
    }
    
    defines {
        "NK_CORE_BUILD"
    }
    
    filter "system:windows"
        systemversion "latest"
        defines { "_CRT_SECURE_NO_WARNINGS" }
        
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "NK_ENABLE_MEMORY_TRACKING" }
        
    filter "configurations:Release"
        runtime "Release"
        optimize "on"
```

---

#### 7.3.2 CMake Alternative (pour compatibilité)

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.20)

project(Nkentseu 
    VERSION 1.0.0
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Options
option(NK_BUILD_TESTS "Build tests" ON)
option(NK_BUILD_EXAMPLES "Build examples" ON)
option(NK_ENABLE_PROFILING "Enable profiling" OFF)

# Platform detection
if(WIN32)
    add_definitions(-DNK_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DNK_PLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DNK_PLATFORM_MACOS)
endif()

# Configuration-specific defines
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNK_DEBUG -DNK_ENABLE_ASSERTS)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DNK_RELEASE)
endif()

# Sub-projects
add_subdirectory(Core/NKCore)
add_subdirectory(Core/Nkentseu)
add_subdirectory(Core/NKLogger)
add_subdirectory(Core/NKMath)
add_subdirectory(Core/NKWindow)

if(NK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

**[Continue dans le fichier suivant...]**

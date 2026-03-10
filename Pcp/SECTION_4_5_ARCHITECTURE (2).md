# Sections 4-5 : Architecture Système et Modules

## 4. Architecture Système Complète

### 4.1 Vue d'Ensemble en Couches

```
┌─────────────────────────────────────────────────────────────────────┐
│                      APPLICATION LAYER                              │
│  Jeux, Simulations, Apps VR/AR, Outils, Applications Métier        │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
┌───────────────────────────────┴─────────────────────────────────────┐
│                    HIGH-LEVEL SYSTEMS LAYER                         │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│ │  Scene   │ │Animation │ │ Scripting│ │   AI     │ │  Editor  │  │
│ │  Graph   │ │  System  │ │  Engine  │ │  System  │ │  Tools   │  │
│ └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐               │
│ │   GUI    │ │ Network  │ │  VR/AR   │ │ Particles│               │
│ │  System  │ │  Layer   │ │  Support │ │  System  │               │
│ └──────────┘ └──────────┘ └──────────┘ └──────────┘               │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
┌───────────────────────────────┴─────────────────────────────────────┐
│                     MIDDLEWARE SYSTEMS LAYER                        │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│ │ Graphics │ │ Physics  │ │  Audio   │ │  Asset   │ │  Input   │  │
│ │ Renderer │ │  Engine  │ │  Engine  │ │ Manager  │ │  System  │  │
│ └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐                             │
│ │  Camera  │ │Animation │ │   ECS    │                             │
│ │  System  │ │  Engine  │ │ Registry │                             │
│ └──────────┘ └──────────┘ └──────────┘                             │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
┌───────────────────────────────┴─────────────────────────────────────┐
│                       CORE FRAMEWORK LAYER                          │
│ ┌─────────────────────────────┐ ┌─────────────────────────────┐    │
│ │         NKCore              │ │        Nkentseu             │    │
│ │ • Memory Management         │ │ • Containers (20+ types)    │    │
│ │ • Reflection System         │ │ • FileSystem                │    │
│ │ • Type System               │ │ • Serialization             │    │
│ │ • 7 Allocators              │ │ • Threading (10+ primitives)│    │
│ │ • Smart Pointers            │ │ • String Utilities          │    │
│ │ • Assertions                │ │ • Time & Profiling          │    │
│ └─────────────────────────────┘ └─────────────────────────────┘    │
│ ┌─────────────────────────────┐ ┌─────────────────────────────┐    │
│ │        NKLogger             │ │         NKMath              │    │
│ │ • Async Logging             │ │ • Vector2/3/4               │    │
│ │ • Multiple Sinks            │ │ • Matrix2/3/4               │    │
│ │ • Pattern Formatting        │ │ • Quaternions               │    │
│ │ • Filtering System          │ │ • Geometric Shapes          │    │
│ └─────────────────────────────┘ └─────────────────────────────┘    │
│ ┌─────────────────────────────┐                                    │
│ │        NKWindow             │                                    │
│ │ • Platform Abstraction      │                                    │
│ │ • Event System              │                                    │
│ │ • Input Management          │                                    │
│ │ • 6 Platform Backends       │                                    │
│ └─────────────────────────────┘                                    │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
┌───────────────────────────────┴─────────────────────────────────────┐
│                         PLATFORM LAYER                              │
│ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐       │
│ │ Windows │ │  Linux  │ │  macOS  │ │ Android │ │   iOS   │       │
│ │  Win32  │ │   XCB   │ │  Cocoa  │ │   NDK   │ │  UIKit  │       │
│ └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘       │
│ ┌─────────┐                                                        │
│ │   Web   │                                                        │
│ │  WASM   │                                                        │
│ └─────────┘                                                        │
└─────────────────────────────────────────────────────────────────────┘
```

### 4.2 Diagramme de Dépendances entre Modules

```
                         ┌──────────────┐
                         │ Application  │
                         └──────┬───────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         │                      │                      │
    ┌────▼─────┐          ┌────▼─────┐          ┌────▼─────┐
    │ Graphics │          │ Physics  │          │  Audio   │
    └────┬─────┘          └────┬─────┘          └────┬─────┘
         │                      │                      │
         └──────────┬───────────┴───────────┬──────────┘
                    │                       │
               ┌────▼─────┐           ┌────▼─────┐
               │ NKWindow │           │  NKMath  │
               └────┬─────┘           └────┬─────┘
                    │                       │
                    └───────────┬───────────┘
                                │
                         ┌──────▼───────┐
                         │   Nkentseu   │
                         └──────┬───────┘
                                │
                    ┌───────────┴───────────┐
                    │                       │
               ┌────▼─────┐          ┌─────▼─────┐
               │  NKCore  │◄─────────│  NKLogger │
               └──────────┘          └───────────┘
```

**Règles de Dépendance:**
1. ✅ Dépendances unidirectionnelles (pas de cycles)
2. ✅ NKCore est la fondation (zéro dépendance externe)
3. ✅ NKLogger dépend uniquement de NKCore
4. ✅ Modules de haut niveau peuvent dépendre de n'importe quel module de niveau inférieur
5. ❌ Interdiction absolue de dépendances circulaires

---

### 4.3 Patterns Architecturaux Implémentés

#### 4.3.1 Entity Component System (ECS)

**Motivation:**  
Séparation données/logique, optimisation cache, parallélisation facile.

**Implémentation:**

```cpp
// Entity: Simple ID unique
using EntityID = uint64_t;

// Component: Pure data (POD preferred)
struct TransformComponent {
    Nk::Vector3 position{0, 0, 0};
    Nk::Quaternion rotation = Nk::Quaternion::Identity();
    Nk::Vector3 scale{1, 1, 1};
    
    Nk::Matrix4 GetMatrix() const {
        return Nk::Matrix4::TRS(position, rotation, scale);
    }
};

struct RenderComponent {
    Nk::MeshHandle mesh;
    Nk::MaterialHandle material;
    bool castShadows = true;
    bool receiveShadows = true;
    uint32_t renderLayer = 0;
};

struct RigidBodyComponent {
    Nk::Vector3 velocity{0, 0, 0};
    Nk::Vector3 angularVelocity{0, 0, 0};
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    bool isKinematic = false;
};

// System: Logic operating on components
class RenderSystem : public Nk::System {
public:
    void Update(Nk::Registry& registry, float deltaTime) override {
        // View tous les entities avec Transform + Render
        auto view = registry.View<TransformComponent, RenderComponent>();
        
        // Iteration cache-friendly (SOA layout)
        for (auto entity : view) {
            auto [transform, render] = view.Get<TransformComponent, 
                                                RenderComponent>(entity);
            
            // Submit draw call
            Nk::Renderer::DrawMesh(
                render.mesh,
                transform.GetMatrix(),
                render.material
            );
        }
    }
};

class PhysicsSystem : public Nk::System {
public:
    void FixedUpdate(Nk::Registry& registry, float fixedDeltaTime) override {
        auto view = registry.View<TransformComponent, RigidBodyComponent>();
        
        // Parallel iteration possible
        registry.ParallelFor(view, [&](auto entity) {
            auto [transform, body] = view.Get<TransformComponent, 
                                              RigidBodyComponent>(entity);
            
            if (!body.isKinematic) {
                // Appliquer gravité
                body.velocity += Nk::Vector3(0, -9.81f, 0) * fixedDeltaTime;
                
                // Intégration
                transform.position += body.velocity * fixedDeltaTime;
            }
        });
    }
};

// Usage
Nk::Registry registry;

// Créer entity
auto player = registry.CreateEntity();

// Ajouter components
registry.AddComponent<TransformComponent>(player);
registry.AddComponent<RenderComponent>(player, meshHandle, materialHandle);
registry.AddComponent<RigidBodyComponent>(player, 80.0f); // 80kg

// Systems
auto renderSystem = Nk::MakeUnique<RenderSystem>();
auto physicsSystem = Nk::MakeUnique<PhysicsSystem>();

// Game loop
while (running) {
    physicsSystem->FixedUpdate(registry, 1.0f / 60.0f);
    renderSystem->Update(registry, deltaTime);
}
```

**Avantages:**
- ✅ Cache locality (iterate contiguous data)
- ✅ Facile à paralléliser
- ✅ Composition > héritage
- ✅ Flexible (add/remove components runtime)
- ✅ Testable (systèmes indépendants)

---

#### 4.3.2 Service Locator Pattern

**Motivation:**  
Accès global aux services sans couplage fort ni singletons.

```cpp
class ServiceLocator {
public:
    // Register service
    template<typename T>
    static void Provide(T* service) {
        std::type_index typeIndex = typeid(T);
        services[typeIndex] = static_cast<void*>(service);
    }
    
    // Get service
    template<typename T>
    static T* Get() {
        std::type_index typeIndex = typeid(T);
        auto it = services.find(typeIndex);
        
        if (it != services.end()) {
            return static_cast<T*>(it->second);
        }
        
        NK_ASSERT(false, "Service not registered: {}", typeid(T).name());
        return nullptr;
    }
    
    // Check if service exists
    template<typename T>
    static bool Has() {
        return services.find(typeid(T)) != services.end();
    }
    
    // Clear all services
    static void Clear() {
        services.clear();
    }
    
private:
    static std::unordered_map<std::type_index, void*> services;
};

// Implementation
std::unordered_map<std::type_index, void*> ServiceLocator::services;

// Usage
int main() {
    // Create and register services
    auto logger = Nk::MakeUnique<Nk::Logger>("Main");
    auto audioEngine = Nk::MakeUnique<Nk::AudioEngine>();
    auto renderer = Nk::MakeUnique<Nk::Renderer>();
    
    ServiceLocator::Provide(logger.get());
    ServiceLocator::Provide(audioEngine.get());
    ServiceLocator::Provide(renderer.get());
    
    // Access from anywhere
    auto log = ServiceLocator::Get<Nk::Logger>();
    log->Info("Application started");
    
    auto audio = ServiceLocator::Get<Nk::AudioEngine>();
    audio->PlayMusic("bgm.ogg");
    
    // Cleanup
    ServiceLocator::Clear();
}
```

---

#### 4.3.3 Observer Pattern (Event System)

**Motivation:**  
Découplage producteur/consommateur d'événements.

```cpp
// Event base class
class Event {
public:
    virtual ~Event() = default;
    virtual EventType GetType() const = 0;
    virtual const char* GetName() const = 0;
    
    bool handled = false;
};

// Concrete events
class KeyPressedEvent : public Event {
public:
    KeyPressedEvent(KeyCode key, int repeatCount)
        : key(key), repeatCount(repeatCount) {}
    
    KeyCode GetKeyCode() const { return key; }
    int GetRepeatCount() const { return repeatCount; }
    
    EventType GetType() const override { return EventType::KeyPressed; }
    const char* GetName() const override { return "KeyPressed"; }
    
private:
    KeyCode key;
    int repeatCount;
};

// Event bus with type-safe dispatch
class EventBus {
public:
    template<typename EventType>
    void Subscribe(std::function<void(const EventType&)> callback) {
        auto typeIndex = std::type_index(typeid(EventType));
        
        listeners[typeIndex].push_back([callback](const Event& e) {
            callback(static_cast<const EventType&>(e));
        });
    }
    
    template<typename EventType>
    void Publish(const EventType& event) {
        auto typeIndex = std::type_index(typeid(EventType));
        auto it = listeners.find(typeIndex);
        
        if (it != listeners.end()) {
            for (auto& listener : it->second) {
                listener(event);
                
                if (event.handled) break; // Stop propagation
            }
        }
    }
    
private:
    using Listener = std::function<void(const Event&)>;
    std::unordered_map<std::type_index, std::vector<Listener>> listeners;
};

// Usage
EventBus eventBus;

// Subscribe
eventBus.Subscribe<KeyPressedEvent>([](const KeyPressedEvent& e) {
    if (e.GetKeyCode() == KeyCode::Escape) {
        Application::Quit();
    }
});

eventBus.Subscribe<MouseMovedEvent>([](const MouseMovedEvent& e) {
    camera.OnMouseMove(e.GetX(), e.GetY());
});

// Publish
eventBus.Publish(KeyPressedEvent(KeyCode::Space, 0));
eventBus.Publish(MouseMovedEvent(mouseX, mouseY));
```

---

#### 4.3.4 Command Pattern (Undo/Redo)

**Motivation:**  
Historique d'actions, undo/redo, macros.

```cpp
// Command interface
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual String GetName() const = 0;
};

// Concrete commands
class MoveEntityCommand : public ICommand {
public:
    MoveEntityCommand(Entity* entity, Vector3 newPosition)
        : entity(entity)
        , oldPosition(entity->GetPosition())
        , newPosition(newPosition) {}
    
    void Execute() override {
        entity->SetPosition(newPosition);
    }
    
    void Undo() override {
        entity->SetPosition(oldPosition);
    }
    
    String GetName() const override {
        return "Move " + entity->GetName();
    }
    
private:
    Entity* entity;
    Vector3 oldPosition;
    Vector3 newPosition;
};

class DeleteEntityCommand : public ICommand {
public:
    DeleteEntityCommand(Scene* scene, Entity* entity)
        : scene(scene), entity(entity) {
        // Serialize entity state
        entityData = entity->Serialize();
    }
    
    void Execute() override {
        scene->DestroyEntity(entity);
    }
    
    void Undo() override {
        entity = scene->CreateEntity();
        entity->Deserialize(entityData);
    }
    
    String GetName() const override {
        return "Delete Entity";
    }
    
private:
    Scene* scene;
    Entity* entity;
    JSONValue entityData;
};

// Command history manager
class CommandHistory {
public:
    void Execute(UniquePtr<ICommand> command) {
        command->Execute();
        
        // Clear redo stack
        while (!redoStack.empty()) {
            redoStack.pop();
        }
        
        undoStack.push(std::move(command));
        
        // Limit history size
        if (undoStack.size() > maxHistorySize) {
            undoStack.pop_front();
        }
    }
    
    void Undo() {
        if (undoStack.empty()) return;
        
        auto command = std::move(undoStack.top());
        undoStack.pop();
        
        command->Undo();
        redoStack.push(std::move(command));
    }
    
    void Redo() {
        if (redoStack.empty()) return;
        
        auto command = std::move(redoStack.top());
        redoStack.pop();
        
        command->Execute();
        undoStack.push(std::move(command));
    }
    
    bool CanUndo() const { return !undoStack.empty(); }
    bool CanRedo() const { return !redoStack.empty(); }
    
private:
    std::stack<UniquePtr<ICommand>> undoStack;
    std::stack<UniquePtr<ICommand>> redoStack;
    size_t maxHistorySize = 100;
};

// Usage
CommandHistory history;

// Execute commands
history.Execute(MakeUnique<MoveEntityCommand>(player, Vector3(10, 0, 0)));
history.Execute(MakeUnique<DeleteEntityCommand>(scene, enemy));

// Undo
history.Undo(); // Restore enemy
history.Undo(); // Restore player position

// Redo
history.Redo(); // Move player again
```

---

#### 4.3.5 Object Pool Pattern

**Motivation:**  
Éviter allocations/deallocations fréquentes (particules, bullets, enemies).

```cpp
template<typename T, size_t PoolSize = 1000>
class ObjectPool {
public:
    ObjectPool() {
        // Pre-allocate all objects
        objects.reserve(PoolSize);
        for (size_t i = 0; i < PoolSize; ++i) {
            objects.emplace_back(MakeUnique<T>());
            freeList.push_back(objects.back().get());
        }
    }
    
    T* Acquire() {
        if (freeList.empty()) {
            // Pool exhausted, log warning
            NK_LOG_WARN("ObjectPool exhausted for type {}", typeid(T).name());
            return nullptr;
        }
        
        T* obj = freeList.back();
        freeList.pop_back();
        activeObjects.insert(obj);
        
        // Reset object state
        if constexpr (requires { obj->Reset(); }) {
            obj->Reset();
        }
        
        return obj;
    }
    
    void Release(T* obj) {
        if (!obj) return;
        
        auto it = activeObjects.find(obj);
        if (it != activeObjects.end()) {
            activeObjects.erase(it);
            freeList.push_back(obj);
        }
    }
    
    size_t GetActiveCount() const { return activeObjects.size(); }
    size_t GetFreeCount() const { return freeList.size(); }
    
private:
    std::vector<UniquePtr<T>> objects;
    std::vector<T*> freeList;
    std::unordered_set<T*> activeObjects;
};

// Usage
class Bullet {
public:
    void Reset() {
        position = Vector3::Zero();
        velocity = Vector3::Zero();
        lifetime = 0.0f;
        active = false;
    }
    
    void Update(float deltaTime) {
        if (!active) return;
        
        position += velocity * deltaTime;
        lifetime += deltaTime;
        
        if (lifetime > maxLifetime) {
            active = false;
        }
    }
    
    Vector3 position;
    Vector3 velocity;
    float lifetime = 0.0f;
    float maxLifetime = 5.0f;
    bool active = false;
};

// In game code
ObjectPool<Bullet, 500> bulletPool;

// Shoot bullet
void Gun::Shoot() {
    Bullet* bullet = bulletPool.Acquire();
    if (bullet) {
        bullet->position = muzzlePosition;
        bullet->velocity = forward * bulletSpeed;
        bullet->active = true;
    }
}

// Update bullets
void BulletSystem::Update(float deltaTime) {
    // Iterate only active bullets (managed by pool)
    for (size_t i = 0; i < bulletPool.GetActiveCount(); ++i) {
        // Update and check if should be returned to pool
    }
}
```

---

### 4.4 Threading Model Détaillé

```
┌─────────────────────────────────────────────────────────────┐
│                      MAIN THREAD                            │
│  • Event polling & dispatch                                 │
│  • High-level game logic                                    │
│  • Command buffer submission                                │
│  • Synchronization barriers                                 │
└────────────┬────────────────────────────────────────────────┘
             │
   ┌─────────┼─────────┐
   ▼         ▼         ▼
┌────────┐┌────────┐┌────────┐
│ Render ││Physics ││ Audio  │
│ Thread ││ Thread ││ Thread │
└────┬───┘└────┬───┘└────┬───┘
     │         │         │
     │         │         │ (Optionnel, peut être sur main)
     │         │         │
     ▼         ▼         ▼
   GPU     Physics    Audio
  Commands   World    Device
```

**Main Thread Responsibilities:**
- Poll OS events (input, window, etc.)
- Update game logic (high-level)
- Submit render commands to queue
- Coordinate other threads

**Render Thread Responsibilities:**
- Process render command queue
- Build GPU command buffers
- Submit to GPU
- SwapBuffers synchronization

**Physics Thread Responsibilities:**
- Collision detection
- Constraint solving
- Integration
- Write results to shared state (double-buffered)

**Job System (Work-Stealing Queue):**

```cpp
class JobSystem {
public:
    void Initialize(uint32_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
        }
        
        workers.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this, i]() {
                WorkerThread(i);
            });
        }
    }
    
    void Schedule(std::function<void()> job) {
        // Add to global queue
        {
            std::lock_guard lock(queueMutex);
            globalQueue.push(std::move(job));
        }
        queueCV.notify_one();
    }
    
    void WaitAll() {
        std::unique_lock lock(queueMutex);
        queueCV.wait(lock, [this]() {
            return globalQueue.empty() && activeJobs == 0;
        });
    }
    
private:
    void WorkerThread(uint32_t threadID) {
        while (!shutdownFlag) {
            std::function<void()> job;
            
            {
                std::unique_lock lock(queueMutex);
                queueCV.wait(lock, [this]() {
                    return !globalQueue.empty() || shutdownFlag;
                });
                
                if (shutdownFlag) break;
                
                if (!globalQueue.empty()) {
                    job = std::move(globalQueue.front());
                    globalQueue.pop();
                    activeJobs++;
                }
            }
            
            if (job) {
                job();
                
                {
                    std::lock_guard lock(queueMutex);
                    activeJobs--;
                }
                queueCV.notify_all();
            }
        }
    }
    
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> globalQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<bool> shutdownFlag{false};
    std::atomic<uint32_t> activeJobs{0};
};

// Usage
JobSystem jobs;
jobs.Initialize(8); // 8 worker threads

// Parallel for
jobs.Schedule([&]() {
    UpdateTransforms(entities, 0, count / 4);
});
jobs.Schedule([&]() {
    UpdateTransforms(entities, count / 4, count / 2);
});
jobs.Schedule([&]() {
    UpdateTransforms(entities, count / 2, 3 * count / 4);
});
jobs.Schedule([&]() {
    UpdateTransforms(entities, 3 * count / 4, count);
});

jobs.WaitAll(); // Wait for all transforms to complete
```

---

## 5. Architecture Détaillée par Module

### 5.1 NKGraphics (À Créer - Phase 2)

**Responsabilité:** Rendu 2D/3D multi-API avec performance optimale.

```
NKGraphics/
├── Core/
│   ├── GraphicsDevice.h         # Abstraction device
│   ├── GraphicsContext.h        # Contexte de rendu
│   ├── CommandBuffer.h          # Enregistrement commandes
│   ├── RenderPass.h             # Organisation rendu
│   ├── SwapChain.h              # Présentation frames
│   └── GraphicsAPI.h            # Enum API (GL/VK/DX/Metal)
│
├── Resources/
│   ├── Buffer.h                 # Vertex/Index/Uniform/Storage
│   ├── Texture.h                # 1D/2D/3D/Cube/Array
│   ├── Sampler.h                # Filtrage et wrapping
│   ├── Shader.h                 # Vertex/Fragment/Geometry/Compute
│   ├── ShaderCompiler.h         # Compilation cross-platform
│   ├── Pipeline.h               # Pipeline state object
│   ├── RenderTarget.h           # Framebuffer/rendertexture
│   └── Material.h               # Shader + paramètres
│
├── Renderer2D/
│   ├── Renderer2D.h             # API rendu 2D
│   ├── SpriteBatch.h            # Batching sprites
│   ├── QuadRenderer.h           # Quads texturés
│   ├── LineRenderer.h           # Lignes et shapes
│   ├── TextRenderer.h           # Rendu texte (FreeType)
│   └── ParticleBatch.h          # Particules 2D
│
├── Renderer3D/
│   ├── Renderer3D.h             # API rendu 3D
│   ├── MeshRenderer.h           # Rendu meshes
│   ├── SkinnedMeshRenderer.h    # Meshes animés
│   ├── TerrainRenderer.h        # Rendu terrain
│   ├── WaterRenderer.h          # Rendu eau
│   └── SkyboxRenderer.h         # Skybox/skydome
│
├── Lighting/
│   ├── Light.h                  # Base class
│   ├── DirectionalLight.h       # Lumière directionnelle
│   ├── PointLight.h             # Lumière ponctuelle
│   ├── SpotLight.h              # Lumière spot
│   ├── AreaLight.h              # Lumière surface
│   └── LightingSystem.h         # Gestion éclairage
│
├── Shadows/
│   ├── ShadowMap.h              # Shadow mapping
│   ├── CascadedShadowMap.h      # CSM pour directional
│   ├── OmnidirectionalShadow.h  # Cubemap shadows
│   └── ShadowAtlas.h            # Atlas ombres
│
├── PostProcessing/
│   ├── PostProcessStack.h       # Stack d'effets
│   ├── Bloom.h                  # HDR bloom
│   ├── SSAO.h                   # Screen-space AO
│   ├── Tonemapping.h            # HDR -> LDR
│   ├── ColorGrading.h           # Correction couleur
│   ├── MotionBlur.h             # Motion blur
│   └── AntiAliasing.h           # FXAA/SMAA/TAA
│
├── Culling/
│   ├── FrustumCulling.h         # Frustum culling
│   ├── OcclusionCulling.h       # Occlusion queries
│   └── LODManager.h             # Level of detail
│
└── Backends/
    ├── OpenGL/
    │   ├── GLDevice.h
    │   ├── GLBuffer.h
    │   ├── GLTexture.h
    │   ├── GLShader.h
    │   └── GLPipeline.h
    │
    ├── Vulkan/
    │   ├── VKDevice.h
    │   ├── VKBuffer.h
    │   ├── VKTexture.h
    │   ├── VKShader.h
    │   └── VKPipeline.h
    │
    ├── DirectX12/
    │   ├── D3D12Device.h
    │   └── ...
    │
    ├── Metal/
    │   ├── MetalDevice.h
    │   └── ...
    │
    └── WebGL/
        ├── WebGLDevice.h
        └── ...
```

**API Example:**

```cpp
// Device creation
Nk::GraphicsConfig config;
config.api = Nk::GraphicsAPI::Vulkan;
config.vsync = true;
config.msaa = 4;
config.hdr = true;

auto device = Nk::GraphicsDevice::Create(config, window);

// Renderer 3D
auto renderer = Nk::Renderer3D::Create(device);

// Load assets
auto mesh = Nk::Mesh::Load("model.obj");
auto albedo = Nk::Texture::Load("albedo.png");
auto normal = Nk::Texture::Load("normal.png");
auto metallic = Nk::Texture::Load("metallic.png");
auto roughness = Nk::Texture::Load("roughness.png");

// PBR material
auto material = Nk::Material::CreatePBR();
material->SetTexture("albedoMap", albedo);
material->SetTexture("normalMap", normal);
material->SetTexture("metallicMap", metallic);
material->SetTexture("roughnessMap", roughness);

// Camera
auto camera = Nk::Camera::CreatePerspective(45.0f, aspect, 0.1f, 1000.0f);
camera->SetPosition({0, 5, 10});
camera->LookAt({0, 0, 0});

// Lighting
auto sun = Nk::DirectionalLight::Create();
sun->SetDirection({-0.5f, -1.0f, -0.3f});
sun->SetColor({1.0f, 0.95f, 0.8f});
sun->SetIntensity(2.0f);
sun->EnableShadows(true);
sun->SetShadowCascades(4);

// Render loop
while (running) {
    renderer->BeginFrame(camera);
    
    // Opaque pass
    renderer->BeginOpaquePass();
    renderer->DrawMesh(mesh, transform, material);
    renderer->EndOpaquePass();
    
    // Transparent pass
    renderer->BeginTransparentPass();
    // Draw transparent objects
    renderer->EndTransparentPass();
    
    // Post-processing
    renderer->ApplyPostProcessing(postProcessStack);
    
    renderer->EndFrame();
    
    device->Present();
}
```

---

**[Document continue dans les prochains fichiers...]**

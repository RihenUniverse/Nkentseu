# Nkentseu Framework - Document de Conception Complet

> **Version:** 1.0  
> **Date:** Février 2026  
> **Auteur:** Équipe Nkentseu  
> **Statut:** Document Vivant

---

## 📋 Table des Matières

1. [Vue d'Ensemble du Projet](#1-vue-densemble-du-projet)
2. [Analyse des Besoins](#2-analyse-des-besoins)
3. [Cas d'Utilisation](#3-cas-dutilisation)
4. [Architecture Système](#4-architecture-système)
5. [Architecture Détaillée par Module](#5-architecture-détaillée-par-module)
6. [Diagrammes de Conception](#6-diagrammes-de-conception)
7. [Spécifications Techniques](#7-spécifications-techniques)
8. [Roadmap et Planification](#8-roadmap-et-planification)
9. [Annexes](#9-annexes)

---

## 1. Vue d'Ensemble du Projet

### 1.1 Description

**Nkentseu** est un framework C++ modulaire haute performance conçu pour le développement d'applications multimédias avancées, incluant :

- 🎮 Moteurs de jeux 2D/3D
- 🔬 Applications de simulation
- 🥽 Systèmes VR/AR/XR
- 🎬 Outils d'animation 2D/3D
- 🤖 Applications IA multimodales
- 🛠️ Environnements de développement intégrés (IDE)

### 1.2 Vision

Créer un écosystème complet permettant aux développeurs de construire n'importe quel type d'application graphique ou multimédia avec une architecture unifiée, performante et multi-plateforme.

### 1.3 Objectifs Stratégiques

1. **Modularité** : Architecture en couches indépendantes et réutilisables
2. **Performance** : Optimisation mémoire et temps réel
3. **Multi-plateforme** : Windows, Linux, macOS, iOS, Android, Web (Emscripten)
4. **Extensibilité** : Système de plugins et d'extensions
5. **Productivité** : API intuitive et documentation complète

### 1.4 Analyse de la Structure Actuelle

Votre projet possède déjà une architecture solide avec:

```
Core/
├── NKCore          # Fondations (mémoire, réflexion, types)
├── Nkentseu        # Bibliothèque haut niveau (conteneurs, filesystem, threading)
├── NKLogger        # Système de logging
├── NKMath          # Math 3D (vecteurs, matrices, quaternions)
└── NKWindow        # Fenêtrage multi-plateforme
```

**État Actuel:**
- ✅ Fondations système complètes
- ✅ Gestion mémoire avancée (7 types d'allocateurs)
- ✅ Système de réflexion runtime
- ✅ Conteneurs optimisés (séquentiels, associatifs, cache-friendly)
- ✅ Threading et synchronisation
- ✅ Sérialisation multi-format (Binary, JSON, XML, YAML)
- ✅ Math 3D avec support SIMD
- 🚧 Fenêtrage multi-plateforme (Win32, XCB, macOS, iOS, Android, Emscripten)

---

## 2. Analyse des Besoins

### 2.1 Besoins Fonctionnels Critiques

#### BF-001 : Gestion Mémoire Avancée
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Description:**  
Système de gestion mémoire personnalisable avec multiples stratégies d'allocation.

**Implémentation Actuelle:**
- ✅ LinearAllocator (allocation séquentielle rapide)
- ✅ StackAllocator (LIFO, scope-based)
- ✅ PoolAllocator (objets taille fixe)
- ✅ FreeListAllocator (general purpose)
- ✅ TLSFAllocator (Two-Level Segregated Fit)
- ✅ ProxyAllocator (wrapper avec tracking)
- ✅ MallocAllocator (fallback système)

**Critères d'Acceptation:**
- [x] Support de multiples stratégies d'allocation
- [x] Tracking mémoire en mode debug (MemoryTracker)
- [x] Alignement mémoire configurable
- [ ] Optimisation SIMD complète

---

#### BF-002 : Système de Réflexion
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Implémentation Actuelle:**
- ✅ NkType (méta-informations)
- ✅ NkClass (hiérarchie classes)
- ✅ NkProperty (champs membres)
- ✅ NkMethod (fonctions membres)
- ✅ NkRegistry (enregistrement global)

**Utilisation Future:**
- Sérialisation automatique
- Binding scripting (Lua/Python)
- Éditeur visuel (inspector)
- Hot-reload de code

---

#### BF-003 : Conteneurs STL-Like Optimisés
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Implémentation Actuelle:**

**Séquentiels:**
- ✅ Vector (tableau dynamique)
- ✅ List (liste simple chaînée)
- ✅ DoubleList (liste doublement chaînée)
- ✅ Deque (double-ended queue)

**Associatifs:**
- ✅ Map (Red-Black tree)
- ✅ Set
- ✅ HashMap (table de hachage)
- ✅ UnorderedMap/UnorderedSet
- ✅ BinaryTree, BTree
- ✅ Trie (prefix tree)
- ✅ PriorityQueue (heap)

**Cache-Friendly:**
- ✅ Array (SOA - Structure of Arrays)
- ✅ Pool (object pool aligné cache)
- ✅ RingBuffer (circular buffer)

**Spécialisés:**
- ✅ Graph (graphes orientés/non-orientés)
- ✅ QuadTree (partitionnement spatial 2D)

---

#### BF-004 : Threading et Concurrence
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Implémentation Actuelle:**
- ✅ Thread (wrapper std::thread)
- ✅ ThreadPool (pool de workers)
- ✅ Mutex, RecursiveMutex, SharedMutex
- ✅ SpinLock (busy-wait)
- ✅ Semaphore, ConditionVariable
- ✅ Future/Promise (résultats asynchrones)
- ✅ ThreadLocal (TLS)
- ✅ Barrier, Latch, Event, ReaderWriterLock

**À Ajouter:**
- [ ] Job System pour parallélisme de tâches
- [ ] Work-stealing queue
- [ ] Fiber-based multitasking

---

#### BF-005 : Logging Multi-Niveaux
**Priorité:** ⭐⭐⭐⭐ Haute

**Implémentation Actuelle:**
- ✅ Logger synchrone et AsyncLogger
- ✅ Niveaux: Trace, Debug, Info, Warn, Error, Fatal
- ✅ Sinks: Console, File, Rotating, Daily, Async, Distributing, Null
- ✅ Formatage patterns personnalisables
- ✅ Filtres: Level, Pattern, Thread

**Patterns Supportés:**
```
%v - Message
%l - Log level
%t - Timestamp
%n - Logger name
%P - Process ID
%T - Thread ID
%s - Source file
%# - Line number
%! - Function name
```

---

#### BF-006 : Mathématiques 3D
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Implémentation Actuelle:**
- ✅ Vector2, Vector3, Vector4
- ✅ Matrix2, Matrix3, Matrix4
- ✅ Quaternion (rotations 3D)
- ✅ Angle (degrés/radians)
- ✅ EulerAngle (pitch/yaw/roll)
- ✅ Color (RGBA)
- ✅ Random (génération nombres aléatoires)
- ✅ Range, Rectangle, Segment

**À Optimiser:**
- [ ] SIMD complet (SSE/AVX)
- [ ] Transformations affines optimisées
- [ ] Frustum culling helpers

---

#### BF-007 : Fenêtrage Multi-Plateforme
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Implémentation Actuelle:**

**Plateformes:**
- ✅ Windows (Win32)
- ✅ Linux (XCB)
- ✅ macOS (Cocoa)
- ✅ iOS (UIKit)
- ✅ Android (NDK)
- ✅ Web (Emscripten/WebGL)

**Événements:**
- ✅ Keyboard (KeyPress, KeyRelease, KeyRepeat)
- ✅ Mouse (Move, Click, Scroll, Drag)
- ✅ Gamepad (Button, Axis, Connected)
- ✅ Window (Resize, Close, Focus)
- ✅ App (Quit)

**Gestionnaire:**
- ✅ EventManager (dispatch)
- ✅ EventBroker (pub/sub)
- ✅ EventFilter (filtrage)
- ✅ InputManager (état global)
- ✅ InputController (mapping actions)

---

#### BF-008 : Système de Fichiers
**Priorité:** ⭐⭐⭐⭐ Haute

**Implémentation Actuelle:**
- ✅ Path (manipulation chemins)
- ✅ File (lecture/écriture)
- ✅ Directory (navigation)
- ✅ FileWatcher (notification modifications)
- ✅ FileSystem (abstraction)

**À Ajouter:**
- [ ] VFS (Virtual File System)
- [ ] Compression (ZIP)
- [ ] Chiffrement assets
- [ ] Async I/O

---

#### BF-009 : Sérialisation Multi-Format
**Priorité:** ⭐⭐⭐⭐ Haute

**Implémentation Actuelle:**
- ✅ Binary (BinaryReader/BinaryWriter)
- ✅ JSON (JSONReader/JSONWriter/JSONValue)
- ✅ XML (XMLReader/XMLWriter)
- ✅ YAML (YAMLReader/YAMLWriter)
- ✅ Archive (abstraction)

**Intégration Future:**
- Sérialisation automatique via réflexion
- Versioning de formats
- Migration données

---

### 2.2 Besoins Fonctionnels Futurs

#### BF-010 : Rendu Graphique 2D/3D
**Priorité:** ⭐⭐⭐⭐⭐ Critique (Phase 2)

**Composants à Créer:**

**NKGraphics:**
```
NKGraphics/
├── Core/
│   ├── GraphicsDevice       # Abstraction API graphique
│   ├── CommandBuffer        # Enregistrement commandes
│   ├── RenderPass          # Organisation du rendu
│   └── SwapChain           # Présentation frames
│
├── Resources/
│   ├── Buffer              # Vertex/Index/Uniform buffers
│   ├── Texture             # Textures 1D/2D/3D/Cube
│   ├── Sampler             # Filtrage textures
│   ├── Shader              # Vertex/Fragment/Compute shaders
│   └── Pipeline            # État de rendu
│
├── Renderer2D/
│   ├── SpriteBatch         # Batching sprites
│   ├── TextRenderer        # Rendu texte
│   └── ShapeRenderer       # Primitives 2D
│
├── Renderer3D/
│   ├── MeshRenderer        # Rendu meshes
│   ├── Material            # PBR materials
│   ├── Light               # Point/Directional/Spot
│   └── Camera              # Perspective/Orthographic
│
└── Backends/
    ├── OpenGL/             # OpenGL 4.5+
    ├── Vulkan/             # Vulkan 1.3
    ├── DirectX12/          # DirectX 12
    ├── Metal/              # Metal 2.0
    └── WebGL/              # WebGL 2.0
```

**Features Clés:**
- Batching automatique (sprites, meshes similaires)
- PBR (Physically Based Rendering)
- Deferred/Forward rendering
- Post-processing (bloom, SSAO, tonemapping)
- HDR rendering
- Shadows (cascaded shadow maps)
- Culling (frustum, occlusion)

**Critères de Performance:**
- 60 FPS stable avec 10K sprites (2D)
- 60 FPS stable avec 100K+ triangles (3D)
- Draw calls < 1000 par frame
- Batching ratio > 80%

---

#### BF-011 : Système de Physique
**Priorité:** ⭐⭐⭐⭐ Haute (Phase 3)

**NKPhysics:**
```
NKPhysics/
├── Core/
│   ├── PhysicsWorld        # Monde physique
│   ├── RigidBody          # Corps rigides
│   ├── Collider           # Formes collision
│   └── PhysicsMaterial    # Friction, restitution
│
├── Collision/
│   ├── BroadPhase         # Détection grossière
│   │   ├── SweepAndPrune
│   │   ├── DynamicAABBTree
│   │   └── SpatialHash
│   │
│   ├── NarrowPhase        # Détection précise
│   │   ├── GJK           # Gilbert-Johnson-Keerthi
│   │   ├── EPA           # Expanding Polytope Algorithm
│   │   └── SAT           # Separating Axis Theorem
│   │
│   └── Shapes/
│       ├── Sphere
│       ├── Box
│       ├── Capsule
│       ├── Cylinder
│       ├── ConvexHull
│       └── Mesh
│
├── Constraints/
│   ├── Joint              # Articulations
│   │   ├── Fixed
│   │   ├── Hinge
│   │   ├── Ball
│   │   └── Slider
│   │
│   └── Contact           # Résolution contacts
│
└── Integration/
    ├── Euler             # Intégration simple
    ├── Verlet            # Stable
    └── RK4               # Runge-Kutta 4
```

**Features:**
- Rigid body dynamics 2D/3D
- Collision detection optimisée
- Constraints et joints
- Continuous collision detection (CCD)
- Triggers et queries
- Ragdoll physics
- Particle systems

**Performance:**
- 1000+ rigid bodies à 60 Hz
- Collision detection < 5ms/frame
- Spatial partitioning (octree/grid)

**Option Backends:**
- Custom engine
- Box2D (2D)
- Bullet Physics (3D)
- PhysX (3D)

---

#### BF-012 : Système Audio
**Priorité:** ⭐⭐⭐ Moyenne (Phase 4)

**NKAudio:**
```
NKAudio/
├── Core/
│   ├── AudioDevice        # Device audio
│   ├── AudioContext       # Contexte
│   └── AudioListener      # Position écoute (3D)
│
├── Sources/
│   ├── AudioSource        # Source audio
│   ├── AudioClip          # Données audio
│   └── AudioStream        # Streaming
│
├── Effects/
│   ├── Reverb
│   ├── Echo
│   ├── Filter            # Lowpass/Highpass
│   ├── Distortion
│   └── Compressor
│
├── Mixer/
│   ├── AudioMixer        # Mixage multi-channel
│   ├── AudioGroup        # Groupes audio
│   └── AudioBus          # Routing
│
└── Backends/
    ├── OpenAL/           # OpenAL Soft
    ├── XAudio2/          # Windows
    ├── CoreAudio/        # macOS/iOS
    └── FMOD/             # Commercial (optionnel)
```

**Formats Supportés:**
- WAV (PCM)
- MP3 (décodage)
- OGG Vorbis
- FLAC

**Features:**
- Audio 3D spatial
- HRTF (Head-Related Transfer Function)
- Doppler effect
- Occlusion/obstruction
- DSP effects chain
- Low-latency mixing (<10ms)

---

#### BF-013 : Animation
**Priorité:** ⭐⭐⭐⭐ Haute (Phase 3)

**NKAnimation:**
```
NKAnimation/
├── Core/
│   ├── AnimationClip      # Données animation
│   ├── Animator           # Contrôleur animation
│   └── AnimationCurve     # Courbes d'animation
│
├── Skeletal/
│   ├── Skeleton           # Hiérarchie os
│   ├── Bone               # Os individuel
│   ├── Skin               # Skinning mesh
│   └── SkeletalAnimation  # Animation squelette
│
├── Blending/
│   ├── BlendTree          # Arbre blending
│   ├── BlendSpace1D       # Blending 1D (vitesse)
│   ├── BlendSpace2D       # Blending 2D (direction+vitesse)
│   └── LayerBlending      # Couches animation
│
├── StateMachine/
│   ├── AnimationState     # État
│   ├── Transition         # Transition
│   └── StateMachine       # Machine à états
│
├── IK/
│   ├── IKChain           # Chaîne IK
│   ├── TwoBoneIK         # IK 2 os (bras/jambe)
│   └── FABRIK            # Forward And Backward Reaching IK
│
└── Procedural/
    ├── ProceduralAnimation
    ├── SpringBone         # Physique secondaire
    └── LookAt             # Rotation regard
```

**Features:**
- Skeletal animation (skinning GPU)
- Blend trees complexes
- State machines avec transitions
- IK (Inverse Kinematics)
- Animation events/notifications
- Root motion
- Animation compression

---

#### BF-014 : Networking
**Priorité:** ⭐⭐⭐ Moyenne (Phase 4)

**NKNetwork:**
```
NKNetwork/
├── Core/
│   ├── NetworkManager     # Gestionnaire réseau
│   ├── Connection         # Connexion individuelle
│   └── Packet             # Paquet réseau
│
├── Transport/
│   ├── TCPSocket          # Socket TCP
│   ├── UDPSocket          # Socket UDP
│   └── WebSocket          # WebSocket (web)
│
├── Serialization/
│   ├── NetworkSerializer  # Sérialisation réseau
│   ├── BitPacker          # Compression bits
│   └── DeltaCompression   # Compression delta
│
├── Replication/
│   ├── NetworkObject      # Objet réseau
│   ├── NetworkVariable    # Variable répliquée
│   └── RPC                # Remote Procedure Call
│
└── Sync/
    ├── ClientPrediction   # Prédiction client
    ├── ServerReconciliation # Correction serveur
    ├── Interpolation      # Lissage mouvement
    └── Lag Compensation   # Compensation latence
```

**Architectures:**
- Client-Server
- Peer-to-Peer
- Relay server

**Features:**
- Reliable/unreliable channels
- Ordering garantie
- Fragmentation large messages
- Compression
- Encryption (optionnel)
- NAT traversal

**Performance:**
- Latency < 100ms (LAN)
- 20-60 tick/sec
- Support 100+ clients simultanés

---

#### BF-015 : Scripting
**Priorité:** ⭐⭐⭐ Moyenne (Phase 5)

**NKScript:**
```
NKScript/
├── Core/
│   ├── ScriptEngine       # Moteur script
│   ├── ScriptContext      # Contexte exécution
│   └── ScriptObject       # Objet script
│
├── Lua/
│   ├── LuaEngine         # Intégration Lua
│   ├── LuaBinding        # Binding C++
│   └── LuaDebugger       # Debugger
│
├── Python/               # Optionnel
│   ├── PythonEngine
│   └── PythonBinding
│
└── Bindings/
    ├── AutoBinding       # Via réflexion
    ├── ManualBinding
    └── BindingGenerator  # Code gen
```

**Features:**
- Lua 5.4 integration
- Binding automatique (réflexion)
- Hot-reload scripts
- Debugging (breakpoints, step)
- Sandboxing (sécurité)
- Performance profiling

**Bindings:**
- Tous modules Nkentseu
- ECS components
- Event system

---

#### BF-016 : Éditeur Visuel
**Priorité:** ⭐⭐⭐⭐ Haute (Phase 6)

**NKEditor:**
```
NKEditor/
├── Core/
│   ├── EditorApplication  # Application éditeur
│   ├── EditorWindow       # Fenêtre éditeur
│   └── EditorContext      # Contexte global
│
├── Windows/
│   ├── SceneView         # Vue 3D interactive
│   ├── Hierarchy         # Arborescence scène
│   ├── Inspector         # Propriétés objet
│   ├── AssetBrowser      # Navigateur assets
│   ├── Console           # Console logs
│   ├── Profiler          # Profiling performance
│   └── AnimationTimeline # Timeline animation
│
├── Tools/
│   ├── Gizmo             # Manipulation 3D
│   │   ├── TranslateGizmo
│   │   ├── RotateGizmo
│   │   └── ScaleGizmo
│   │
│   ├── Grid              # Grille 3D
│   ├── Snapping          # Accrochage
│   └── Selection         # Sélection objets
│
├── Commands/
│   ├── Command           # Interface commande
│   ├── UndoStack         # Pile undo/redo
│   └── CommandHistory
│
└── Importers/
    ├── ModelImporter     # Import 3D (FBX, OBJ, GLTF)
    ├── TextureImporter   # Import textures
    └── AudioImporter     # Import audio
```

**Features:**
- WYSIWYG scene editing
- Multi-viewport (perspectives différentes)
- Play mode (test in-editor)
- Prefab system
- Asset pipeline
- Material editor visuel
- Shader node editor
- Particle system editor
- Animation timeline

---

### 2.3 Besoins Non-Fonctionnels

#### BNF-001 : Performance
**Catégorie:** Performance  
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Objectifs:**
- Framerate stable 60 FPS (16.67ms/frame)
- Allocations mémoire < 1MB/frame
- Temps chargement scène < 3s
- Utilisation CPU < 70% (monothread)
- Utilisation GPU < 90%

**Métriques:**
- Frame time moyen/min/max/99th percentile
- Memory usage (heap, stack, GPU)
- Draw calls, triangles, textures
- Cache miss rate
- Thread utilization

**Outils:**
- Profilers intégrés
- Tracy Profiler integration
- RenderDoc captures
- GPU profiling (Nsight, PIX)

---

#### BNF-002 : Portabilité
**Catégorie:** Compatibilité  
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Plateformes Desktop:**
- Windows 10/11 (x64)
- Linux (Ubuntu 22.04+, Fedora 38+)
- macOS 12+ (Intel + Apple Silicon)

**Plateformes Mobile:**
- iOS 15+ (iPhone, iPad)
- Android 10+ (API 29+)

**Web:**
- WebAssembly (Emscripten)
- WebGL 2.0

**Compilateurs:**
- MSVC 2019+ (Windows)
- GCC 9+ (Linux)
- Clang 10+ (macOS/Linux)
- Emscripten latest

**Standards:**
- C++17 minimum
- C++20 recommandé
- C++23 compatible

---

#### BNF-003 : Maintenabilité
**Catégorie:** Qualité Code  
**Priorité:** ⭐⭐⭐⭐ Haute

**Code Quality:**
- Couverture tests > 80%
- Complexité cyclomatique < 15
- Code duplication < 5%
- Documentation API 100%

**Processus:**
- Code review obligatoire (2+ approvals)
- CI/CD automatisé
- Analyse statique (Clang-Tidy, Cppcheck)
- Sanitizers (ASan, TSan, UBSan)

**Documentation:**
- API documentation (Doxygen)
- Architecture documentation
- Tutorials et exemples
- FAQ et troubleshooting

---

#### BNF-004 : Scalabilité
**Catégorie:** Architecture  
**Priorité:** ⭐⭐⭐⭐ Haute

**Capacité:**
- Scènes avec 100K+ entités
- 1M+ triangles rendus
- 10K+ particules actives
- 1000+ rigid bodies physique
- 100+ audio sources

**Techniques:**
- Streaming assets asynchrone
- LOD (Level of Detail) automatique
- Culling (frustum, occlusion)
- Job system scalable (1-64 threads)
- Data-oriented design (ECS)

---

#### BNF-005 : Sécurité
**Catégorie:** Sécurité  
**Priorité:** ⭐⭐⭐ Moyenne

**Mesures:**
- Validation inputs utilisateur
- Protection buffer overflow
- Sanitization chemins fichiers
- Sandboxing scripts
- Chiffrement assets (optionnel)

**Audits:**
- Security scanning (Coverity)
- Dependency vulnerability checks
- Penetration testing (networking)

---

#### BNF-006 : Extensibilité
**Catégorie:** Architecture  
**Priorité:** ⭐⭐⭐⭐ Haute

**Mécanismes:**
- Système plugins (.dll/.so)
- API stable (versioning sémantique)
- Hooks et callbacks
- Scripting (Lua/Python)
- Custom allocators
- Custom renderers

**Compatibilité:**
- ABI stability (major versions)
- Plugin API versioning
- Graceful degradation

---

#### BNF-007 : Utilisabilité
**Catégorie:** UX  
**Priorité:** ⭐⭐⭐⭐ Haute

**API Design:**
- API intuitive et cohérente
- Naming conventions claires
- Erreurs explicites avec contexte
- Defaults raisonnables
- Type safety

**Documentation:**
- Getting started < 15 min
- 20+ exemples couvrant features
- Video tutorials
- Interactive playground

**Outils:**
- Templates de projets
- Code snippets
- Visual debugging tools

---

#### BNF-008 : Fiabilité
**Catégorie:** Qualité  
**Priorité:** ⭐⭐⭐⭐⭐ Critique

**Objectifs:**
- Crash rate < 0.1%
- Mean time between failures > 100h
- Graceful error recovery
- Zero memory leaks
- No undefined behavior

**Validation:**
- Extensive unit testing
- Integration testing
- Stress testing
- Fuzz testing
- Regression testing

**Monitoring:**
- Crash reporting
- Telemetry (opt-in)
- Performance metrics
- Error logging

---

**[Suite du document dans le prochain message...]**

## 3. Cas d'Utilisation Détaillés

### 3.1 Acteurs du Système

#### 👨‍💻 Développeur de Jeux
**Description:** Utilise Nkentseu pour créer des jeux vidéo (indie, AA, AAA)  
**Compétences:** C++, graphiques 3D, gameplay programming  
**Objectifs:** Prototypage rapide, performance optimale, multi-plateforme  
**Besoins:** API intuitive, documentation complète, exemples variés

#### 🔬 Développeur d'Applications Scientifiques
**Description:** Crée des simulations, visualisations, outils de recherche  
**Compétences:** C++, mathématiques, domaine spécifique  
**Objectifs:** Précision calculs, visualisation données, performance  
**Besoins:** Flexibilité, extensions personnalisées, stabilité

#### 🎨 Artiste Technique
**Description:** Crée du contenu (modèles, animations, shaders, effets)  
**Compétences:** Outils 3D (Blender, Maya), shaders, scripting léger  
**Objectifs:** Workflow fluide, preview temps réel, itération rapide  
**Besoins:** Éditeur visuel, hot-reload assets, documentation visuelle

#### 🥽 Développeur VR/AR
**Description:** Crée des expériences immersives VR/AR/XR  
**Compétences:** C++, graphics, interaction 3D, UX immersive  
**Objectifs:** 90+ FPS stable, faible latence, confort utilisateur  
**Besoins:** Support casques, tracking précis, haptic feedback

#### 🤝 Contributeur Open Source
**Description:** Contribue au développement du framework  
**Compétences:** C++, architecture logicielle, optimisation  
**Objectifs:** Code de qualité, impact communauté  
**Besoins:** Guidelines claires, reviews constructives, reconnaissance

---

### 3.2 Cas d'Utilisation Principaux

#### 🎮 CU-001 : Développement Jeu de Plateforme 2D

**Acteur Principal:** Développeur de Jeux  
**Niveau:** Essentiel  
**Fréquence:** Quotidienne

**Préconditions:**
- Nkentseu installé et configuré
- Projet créé (template "2D Game")
- Assets préparés (sprites, sons, niveaux)

**Scénario Principal:**

1. **Initialisation Projet**
   ```cpp
   #include <Nkentseu/Nkentseu.h>
   
   int main() {
       // Configuration fenêtre
       Nk::WindowConfig config;
       config.title = "Super Platformer";
       config.width = 1280;
       config.height = 720;
       config.vsync = true;
       
       auto window = Nk::Window::Create(config);
       auto renderer = Nk::Renderer2D::Create(window);
       
       // Initialisation systèmes
       Nk::PhysicsWorld2D physics;
       Nk::AudioEngine audio;
       Nk::SceneManager scenes;
   }
   ```

2. **Chargement Assets**
   ```cpp
   // Chargement asynchrone
   auto assetManager = Nk::AssetManager::Create();
   
   auto playerSpriteSheet = assetManager->LoadAsync<Nk::Texture>(
       "assets/sprites/player.png"
   );
   
   auto jumpSound = assetManager->LoadAsync<Nk::AudioClip>(
       "assets/audio/jump.wav"
   );
   
   auto level1 = assetManager->LoadAsync<Nk::Scene>(
       "assets/levels/level1.json"
   );
   
   // Attente chargement
   assetManager->WaitAll();
   ```

3. **Création Entités (ECS)**
   ```cpp
   auto scene = Nk::Scene::Create("MainLevel");
   auto& registry = scene->GetRegistry();
   
   // Joueur
   auto player = registry.CreateEntity("Player");
   registry.AddComponent<Nk::Transform2D>(player, 
       Nk::Vector2(100, 300), 0.0f, Nk::Vector2(1, 1)
   );
   registry.AddComponent<Nk::Sprite>(player,
       playerSpriteSheet, Nk::Rect(0, 0, 32, 32)
   );
   registry.AddComponent<Nk::RigidBody2D>(player,
       Nk::BodyType::Dynamic, 1.0f
   );
   registry.AddComponent<Nk::BoxCollider2D>(player,
       32, 32
   );
   registry.AddComponent<PlayerController>(player,
       10.0f, // moveSpeed
       500.0f // jumpForce
   );
   
   // Plateforme
   auto platform = registry.CreateEntity("Platform");
   registry.AddComponent<Nk::Transform2D>(platform,
       Nk::Vector2(400, 500), 0.0f, Nk::Vector2(5, 1)
   );
   registry.AddComponent<Nk::Sprite>(platform,
       platformTexture, Nk::Color::Gray()
   );
   registry.AddComponent<Nk::RigidBody2D>(platform,
       Nk::BodyType::Static
   );
   registry.AddComponent<Nk::BoxCollider2D>(platform,
       160, 32
   );
   ```

4. **Logique Gameplay (Component System)**
   ```cpp
   class PlayerController : public Nk::Component {
   public:
       float moveSpeed;
       float jumpForce;
       bool isGrounded = false;
       
       void OnUpdate(float deltaTime) override {
           auto& transform = GetComponent<Nk::Transform2D>();
           auto& rigidbody = GetComponent<Nk::RigidBody2D>();
           
           // Input horizontal
           float horizontal = 0.0f;
           if (Nk::Input::IsKeyPressed(Nk::Key::A)) horizontal -= 1.0f;
           if (Nk::Input::IsKeyPressed(Nk::Key::D)) horizontal += 1.0f;
           
           // Mouvement
           rigidbody.SetVelocityX(horizontal * moveSpeed);
           
           // Saut
           if (Nk::Input::IsKeyJustPressed(Nk::Key::Space) && isGrounded) {
               rigidbody.ApplyImpulse(Nk::Vector2(0, jumpForce));
               Nk::Audio::PlayOneShot(jumpSound);
               isGrounded = false;
           }
           
           // Animation
           auto& sprite = GetComponent<Nk::Sprite>();
           if (horizontal != 0.0f) {
               sprite.SetAnimation("Run");
               sprite.SetFlipX(horizontal < 0.0f);
           } else {
               sprite.SetAnimation("Idle");
           }
       }
       
       void OnCollisionEnter(Nk::Collision2D collision) override {
           if (collision.normal.y > 0.5f) {
               isGrounded = true;
           }
       }
   };
   ```

5. **Boucle de Jeu**
   ```cpp
   Nk::Clock clock;
   float fixedDeltaTime = 1.0f / 60.0f;
   float accumulator = 0.0f;
   
   while (window->IsOpen()) {
       float deltaTime = clock.Restart();
       accumulator += deltaTime;
       
       // Input
       window->PollEvents();
       
       // Fixed Update (Physique)
       while (accumulator >= fixedDeltaTime) {
           physics.Step(fixedDeltaTime);
           scene->FixedUpdate(fixedDeltaTime);
           accumulator -= fixedDeltaTime;
       }
       
       // Variable Update (Logique)
       scene->Update(deltaTime);
       
       // Render
       renderer->BeginFrame();
       renderer->Clear(Nk::Color(0.2f, 0.3f, 0.8f, 1.0f));
       
       // Rendu scène
       scene->Render(renderer);
       
       // UI
       Nk::GUI::Text("FPS: " + std::to_string(1.0f / deltaTime), 
                     Nk::Vector2(10, 10));
       
       renderer->EndFrame();
       window->SwapBuffers();
   }
   ```

**Scénarios Alternatifs:**

**3a. Asset manquant:**
- Le système log un warning
- Affiche un placeholder (texture rose)
- L'application continue

**4a. Erreur de script:**
- Le système catch l'exception
- Log l'erreur avec stack trace
- Désactive le component fautif
- Notifie l'utilisateur

**5a. Performance insuffisante:**
- Le profiler détecte framerate < 60 FPS
- Log les systèmes coûteux
- Suggestions d'optimisation (réduire entités, LOD, etc.)

**Postconditions:**
- Jeu fonctionnel avec physique, input, rendu
- Performance 60 FPS stable
- Logs sans erreurs critiques

**Métriques de Succès:**
- Temps de développement < 2 jours (prototype)
- Code < 500 lignes (logique gameplay)
- Performance 60 FPS avec 100+ entités

---

#### 🔬 CU-002 : Visualisation Scientifique 3D

**Acteur Principal:** Développeur d'Applications Scientifiques  
**Niveau:** Essentiel  
**Fréquence:** Hebdomadaire

**Contexte:**  
Un chercheur veut visualiser des données de simulation moléculaire en 3D avec manipulation interactive.

**Scénario Principal:**

1. **Configuration Rendu 3D Haute Qualité**
   ```cpp
   Nk::WindowConfig config;
   config.api = Nk::GraphicsAPI::Vulkan; // Meilleure perf
   config.samples = 8; // MSAA 8x
   config.vsync = false; // Refresh rate max
   
   auto window = Nk::Window::Create(config);
   auto renderer = Nk::Renderer3D::Create(window);
   renderer->SetHDR(true);
   renderer->SetToneMapping(Nk::ToneMapping::ACES);
   ```

2. **Import Données Scientifiques**
   ```cpp
   class MoleculeData {
   public:
       struct Atom {
           Nk::Vector3 position;
           float radius;
           Nk::Color color;
           std::string element;
       };
       
       struct Bond {
           uint32_t atom1, atom2;
           float strength;
       };
       
       std::vector<Atom> atoms;
       std::vector<Bond> bonds;
       
       static MoleculeData LoadFromPDB(const std::string& filepath) {
           MoleculeData data;
           // Parser PDB format
           auto file = Nk::File::Open(filepath);
           // ... parsing ...
           return data;
       }
   };
   
   auto molecule = MoleculeData::LoadFromPDB("data/protein.pdb");
   ```

3. **Génération Géométrie Procédurale**
   ```cpp
   class MoleculeRenderer {
       Nk::SharedPtr<Nk::Mesh> GenerateAtomMesh(const Atom& atom) {
           // Sphere avec LOD basé sur distance
           auto mesh = Nk::Mesh::CreateSphere(atom.radius, 32, 32);
           
           // Material PBR
           auto material = Nk::Material::CreatePBR();
           material->SetAlbedo(atom.color);
           material->SetMetallic(0.0f);
           material->SetRoughness(0.4f);
           material->SetEmissive(atom.color * 0.1f); // Légère émission
           
           mesh->SetMaterial(material);
           return mesh;
       }
       
       Nk::SharedPtr<Nk::Mesh> GenerateBondMesh(const Bond& bond) {
           auto& atom1 = molecule.atoms[bond.atom1];
           auto& atom2 = molecule.atoms[bond.atom2];
           
           Nk::Vector3 direction = atom2.position - atom1.position;
           float length = direction.Length();
           
           // Cylindre entre atomes
           auto mesh = Nk::Mesh::CreateCylinder(0.1f, length, 16);
           
           // Orientation
           Nk::Vector3 up(0, 1, 0);
           Nk::Quaternion rotation = Nk::Quaternion::FromToRotation(up, direction);
           
           mesh->SetPosition((atom1.position + atom2.position) * 0.5f);
           mesh->SetRotation(rotation);
           
           return mesh;
       }
   };
   ```

4. **Caméra Orbitale Interactive**
   ```cpp
   class OrbitCamera {
       Nk::Vector3 target;
       float distance = 50.0f;
       float theta = 0.0f;   // Azimuth
       float phi = 45.0f;    // Elevation
       
       void OnMouseDrag(Nk::MouseEvent& e) {
           if (e.button == Nk::MouseButton::Left) {
               // Rotation
               theta += e.deltaX * 0.5f;
               phi += e.deltaY * 0.5f;
               phi = Nk::Clamp(phi, -89.0f, 89.0f);
           } else if (e.button == Nk::MouseButton::Right) {
               // Pan
               Nk::Vector3 right = camera->GetRight();
               Nk::Vector3 up = camera->GetUp();
               target += (right * -e.deltaX + up * e.deltaY) * 0.01f * distance;
           }
       }
       
       void OnScroll(Nk::ScrollEvent& e) {
           // Zoom
           distance *= (1.0f - e.deltaY * 0.1f);
           distance = Nk::Clamp(distance, 1.0f, 1000.0f);
       }
       
       void UpdateCamera(Nk::Camera& camera) {
           // Position sphérique
           float thetaRad = Nk::Radians(theta);
           float phiRad = Nk::Radians(phi);
           
           Nk::Vector3 position;
           position.x = distance * cos(phiRad) * cos(thetaRad);
           position.y = distance * sin(phiRad);
           position.z = distance * cos(phiRad) * sin(thetaRad);
           position += target;
           
           camera.SetPosition(position);
           camera.LookAt(target);
       }
   };
   ```

5. **Rendu avec Éclairage Avancé**
   ```cpp
   // Setup lighting
   auto dirLight = Nk::DirectionalLight::Create();
   dirLight->SetDirection(Nk::Vector3(-1, -1, -1).Normalized());
   dirLight->SetColor(Nk::Color::White());
   dirLight->SetIntensity(1.0f);
   
   auto ambientLight = Nk::AmbientLight::Create();
   ambientLight->SetColor(Nk::Color(0.2f, 0.2f, 0.3f));
   
   // Environment map pour réflections
   auto envMap = Nk::Cubemap::Load("assets/env/lab.hdr");
   renderer->SetEnvironmentMap(envMap);
   
   // Render loop
   while (window->IsOpen()) {
       renderer->BeginFrame();
       renderer->Clear(Nk::Color::Black());
       
       // Rendu molécule
       for (auto& atom : molecule.atoms) {
           auto mesh = atomMeshes[&atom];
           renderer->DrawMesh(mesh);
       }
       
       for (auto& bond : molecule.bonds) {
           auto mesh = bondMeshes[&bond];
           renderer->DrawMesh(mesh);
       }
       
       // Post-processing
       renderer->ApplySSAO();      // Ambient occlusion
       renderer->ApplyBloom(0.3f); // Glow
       renderer->ApplyToneMapping();
       
       renderer->EndFrame();
   }
   ```

6. **Interface Analyse**
   ```cpp
   Nk::GUI::Begin("Molecule Analyzer");
   
   // Sélection atome
   static int selectedAtom = -1;
   if (Nk::GUI::ListBox("Atoms", &selectedAtom, atomNames)) {
       // Highlight atome sélectionné
       camera.FocusOn(molecule.atoms[selectedAtom].position);
   }
   
   // Affichage informations
   if (selectedAtom >= 0) {
       auto& atom = molecule.atoms[selectedAtom];
       Nk::GUI::Text("Element: " + atom.element);
       Nk::GUI::Text("Position: " + atom.position.ToString());
       Nk::GUI::Text("Radius: " + std::to_string(atom.radius));
   }
   
   // Contrôles visualisation
   static bool showBonds = true;
   Nk::GUI::Checkbox("Show Bonds", &showBonds);
   
   static float atomScale = 1.0f;
   Nk::GUI::Slider("Atom Scale", &atomScale, 0.5f, 2.0f);
   
   Nk::GUI::End();
   ```

**Postconditions:**
- Visualisation 3D interactive fonctionnelle
- Performance 60+ FPS avec 10K+ atomes
- Export images haute résolution

---

#### 🥽 CU-003 : Formation VR Industrielle

**Acteur Principal:** Développeur VR  
**Niveau:** Critique  
**Fréquence:** Projet complet

**Contexte:**  
Créer une simulation VR pour former des techniciens à la maintenance d'équipements industriels complexes.

**Scénario Principal:**

1. **Initialisation VR**
   ```cpp
   // Configuration VR
   Nk::VRConfig vrConfig;
   vrConfig.headset = Nk::VRHeadset::AutoDetect; // Quest, Index, PSVR2
   vrConfig.renderScale = 1.4f; // Supersampling
   vrConfig.targetFramerate = 90; // 90 FPS minimum VR
   
   auto vrSystem = Nk::VRSystem::Create(vrConfig);
   
   // Contrôleurs
   auto leftHand = vrSystem->GetController(Nk::VRHand::Left);
   auto rightHand = vrSystem->GetController(Nk::VRHand::Right);
   
   // Tracking
   auto headset = vrSystem->GetHeadset();
   ```

2. **Environnement 3D Interactif**
   ```cpp
   // Chargement scène usine
   auto factory = Nk::Scene::Load("assets/scenes/factory_floor.scene");
   
   // Équipement à maintenir
   auto machine = factory->FindEntity("MaintenanceMachine");
   
   // Rendre pièces interactives
   auto cover = machine->FindChild("Cover");
   cover->AddComponent<Nk::Grabbable>();
   cover->AddComponent<Nk::RigidBody>();
   
   auto screws = machine->FindChildren("Screw");
   for (auto screw : screws) {
       screw->AddComponent<Nk::Interactable>();
       screw->AddComponent<ScrewComponent>(4); // 4 tours pour retirer
   }
   ```

3. **Interaction Manuelle**
   ```cpp
   class VRInteractionSystem {
       void Update() {
           // Raycasting depuis contrôleur
           auto ray = rightHand->GetRay();
           auto hit = physics->Raycast(ray, 10.0f);
           
           if (hit) {
               // Highlight objet pointé
               if (auto interactable = hit.entity->GetComponent<Interactable>()) {
                   interactable->Highlight(true);
                   
                   // Afficher tooltip
                   ShowTooltip(hit.point, interactable->GetName());
               }
           }
       }
       
       void OnGripPressed() {
           auto hit = GetPointedObject();
           if (!hit) return;
           
           if (auto grabbable = hit->GetComponent<Grabbable>()) {
               // Préhension objet
               grabbedObject = hit;
               grabbedObject->AttachTo(rightHand);
               
               // Feedback haptique
               rightHand->Vibrate(0.3f, 50); // intensity, duration ms
               
               // Audio
               Nk::Audio::PlayOneShot3D(grabSound, hit->GetPosition());
           }
       }
       
       void OnGripReleased() {
           if (grabbedObject) {
               grabbedObject->Detach();
               grabbedObject->GetComponent<RigidBody>()->SetVelocity(
                   rightHand->GetVelocity()
               );
               grabbedObject = nullptr;
           }
       }
   };
   ```

4. **Outils Virtuels**
   ```cpp
   class VRToolSystem {
       enum class Tool {
           None,
           Screwdriver,
           Wrench,
           Multimeter
       };
       
       Tool currentTool = Tool::None;
       
       void EquipTool(Tool tool) {
           // Destroy current tool
           if (toolMesh) {
               toolMesh->Destroy();
           }
           
           currentTool = tool;
           
           // Create new tool
           switch (tool) {
               case Tool::Screwdriver:
                   toolMesh = Nk::Mesh::Load("assets/tools/screwdriver.obj");
                   toolMesh->AttachTo(rightHand, Nk::Vector3(0, -0.1f, 0));
                   break;
               case Tool::Wrench:
                   toolMesh = Nk::Mesh::Load("assets/tools/wrench.obj");
                   toolMesh->AttachTo(rightHand, Nk::Vector3(0, -0.12f, 0.05f));
                   break;
           }
       }
       
       void UseTool() {
           auto hit = GetPointedObject();
           if (!hit) return;
           
           if (currentTool == Tool::Screwdriver) {
               if (auto screw = hit->GetComponent<ScrewComponent>()) {
                   // Rotation trigger
                   float trigger = rightHand->GetTriggerValue();
                   if (trigger > 0.5f) {
                       screw->Unscrew(deltaTime);
                       
                       // Vibration proportionnelle
                       rightHand->Vibrate(trigger, 20);
                       
                       if (screw->IsFullyUnscrewed()) {
                           screw->GetEntity()->Detach();
                           OnStepCompleted("Remove Screw");
                       }
                   }
               }
           }
       }
   };
   ```

5. **Guidage et Formation**
   ```cpp
   class TrainingModule {
       struct Step {
           std::string description;
           std::function<bool()> checkCompletion;
           Nk::Vector3 highlightPosition;
       };
       
       std::vector<Step> steps;
       int currentStep = 0;
       
       void InitializeSteps() {
           steps = {
               {
                   "Mettez les lunettes de protection",
                   []() { return safetyGlasses->IsWorn(); },
                   safetyGlasses->GetPosition()
               },
               {
                   "Prenez le tournevis",
                   []() { return currentTool == Tool::Screwdriver; },
                   toolRack->GetPosition()
               },
               {
                   "Retirez les 4 vis du capot",
                   []() { return screwsRemoved == 4; },
                   cover->GetPosition()
               },
               // ... more steps
           };
       }
       
       void Update() {
           auto& step = steps[currentStep];
           
           // Afficher instruction en VR
           ShowInstructionPanel(step.description);
           
           // Highlight zone d'action
           DrawArrow(headset->GetPosition(), step.highlightPosition);
           HighlightZone(step.highlightPosition, 0.5f);
           
           // Vérifier complétion
           if (step.checkCompletion()) {
               CompleteStep();
           }
       }
       
       void CompleteStep() {
           // Audio feedback
           Nk::Audio::Play(successSound);
           
           // Visual feedback
           ShowSuccessEffect(steps[currentStep].highlightPosition);
           
           // Progression
           currentStep++;
           if (currentStep >= steps.size()) {
               OnTrainingCompleted();
           }
       }
   };
   ```

6. **Analyse Performance**
   ```cpp
   class TrainingAnalytics {
       struct Session {
           float totalTime;
           int errorsCount;
           std::vector<float> stepDurations;
           float score;
       };
       
       Session currentSession;
       
       void RecordError(const std::string& errorType) {
           currentSession.errorsCount++;
           
           // Feedback immédiat
           ShowErrorMessage(errorType);
           Nk::Audio::Play(errorSound);
           rightHand->Vibrate(1.0f, 200); // Vibration forte
       }
       
       void GenerateReport() {
           float avgStepTime = 0.0f;
           for (float duration : currentSession.stepDurations) {
               avgStepTime += duration;
           }
           avgStepTime /= currentSession.stepDurations.size();
           
           // Calcul score
           float timeScore = Nk::Clamp(60.0f / avgStepTime, 0.0f, 100.0f);
           float accuracyScore = Nk::Clamp(
               100.0f - currentSession.errorsCount * 10.0f, 
               0.0f, 100.0f
           );
           
           currentSession.score = (timeScore + accuracyScore) / 2.0f;
           
           // Sauvegarde
           SaveToDatabase(currentSession);
           
           // Affichage
           ShowReportPanel(currentSession);
       }
   };
   ```

**Exigences Techniques VR:**
- 90 FPS minimum (11.1ms par frame)
- Motion-to-photon latency < 20ms
- IPD adjustment automatique
- Confort (pas de nausée)
- Spatial audio 3D

**Postconditions:**
- Trainee complète procédure correctement
- Données de performance enregistrées
- Certification délivrée si score > 80%

---

### 3.3 Diagrammes de Cas d'Utilisation

```
                    ┌──────────────────┐
                    │  Nkentseu        │
                    │  Framework       │
                    └──────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│ Développeur   │   │   Artiste     │   │ Contributeur  │
│ de Jeux       │   │  Technique    │   │ Open Source   │
└───────┬───────┘   └───────┬───────┘   └───────┬───────┘
        │                   │                   │
        │                   │                   │
        ├─ Créer Jeu 2D/3D │                   │
        ├─ Implémenter     │                   │
        │  Gameplay         │                   │
        ├─ Optimiser Perf  │                   │
        │                   │                   │
        │                   ├─ Créer Assets    │
        │                   ├─ Éditer Matériaux│
        │                   ├─ Animer Modèles  │
        │                   │                   │
        │                   │                   ├─ Contribuer Code
        │                   │                   ├─ Écrire Tests
        │                   │                   ├─ Documenter API
        │                   │                   │
        └───────────────────┴───────────────────┘
                            │
                            │ utilise
                            ▼
            ┌───────────────────────────────┐
            │      Core Services            │
            ├───────────────────────────────┤
            │ • Window Management           │
            │ • Input Handling              │
            │ • Asset Loading               │
            │ • Rendering                   │
            │ • Physics Simulation          │
            │ • Audio Playback              │
            │ • Scene Management            │
            │ • Scripting                   │
            └───────────────────────────────┘
```

---

## 4. Architecture Système

### 4.1 Architecture en Couches

```
╔══════════════════════════════════════════════════════════════════╗
║                      APPLICATION LAYER                           ║
║  User Applications, Games, Simulations, Tools, Editors          ║
╚══════════════════════════════════════════════════════════════════╝
                              ▲
                              │
╔══════════════════════════════════════════════════════════════════╗
║                    HIGH-LEVEL SYSTEMS                            ║
║ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐            ║
║ │  Scene   │ │Animation │ │Scripting │ │   AI     │            ║
║ │  Graph   │ │  System  │ │  Engine  │ │  System  │            ║
║ └──────────┘ └──────────┘ └──────────┘ └──────────┘            ║
║ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐            ║
║ │  Editor  │ │   GUI    │ │ Network  │ │  VR/AR   │            ║
║ │  Tools   │ │  System  │ │  Layer   │ │  Support │            ║
║ └──────────┘ └──────────┘ └──────────┘ └──────────┘            ║
╚══════════════════════════════════════════════════════════════════╝
                              ▲
                              │
╔══════════════════════════════════════════════════════════════════╗
║                    MIDDLEWARE SYSTEMS                            ║
║ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐            ║
║ │ Graphics │ │ Physics  │ │  Audio   │ │  Asset   │            ║
║ │ Renderer │ │  Engine  │ │  Engine  │ │ Manager  │            ║
║ └──────────┘ └──────────┘ └──────────┘ └──────────┘            ║
║ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐            ║
║ │  Input   │ │  Camera  │ │Particles │ │Animation │            ║
║ │  System  │ │  System  │ │  System  │ │  Engine  │            ║
║ └──────────┘ └──────────┘ └──────────┘ └──────────┘            ║
╚══════════════════════════════════════════════════════════════════╝
                              ▲
                              │
╔══════════════════════════════════════════════════════════════════╗
║                      CORE FRAMEWORK                              ║
║ ┌────────────────────┐  ┌────────────────────┐                  ║
║ │    NKCore          │  │    Nkentseu        │                  ║
║ │ • Memory Mgmt      │  │ • Containers       │                  ║
║ │ • Reflection       │  │ • FileSystem       │                  ║
║ │ • Type System      │  │ • Serialization    │                  ║
║ │ • Allocators       │  │ • Threading        │                  ║
║ └────────────────────┘  └────────────────────┘                  ║
║ ┌────────────────────┐  ┌────────────────────┐                  ║
║ │    NKLogger        │  │     NKMath         │                  ║
║ │ • Async Logging    │  │ • Vectors/Matrices │                  ║
║ │ • Multi-Sink       │  │ • Quaternions      │                  ║
║ └────────────────────┘  └────────────────────┘                  ║
║ ┌────────────────────┐                                          ║
║ │    NKWindow        │                                          ║
║ │ • Windowing        │                                          ║
║ │ • Event System     │                                          ║
║ └────────────────────┘                                          ║
╚══════════════════════════════════════════════════════════════════╝
                              ▲
                              │
╔══════════════════════════════════════════════════════════════════╗
║                      PLATFORM LAYER                              ║
║ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐            ║
║ │ Windows  │ │  Linux   │ │  macOS   │ │ Android  │            ║
║ │  Win32   │ │   XCB    │ │  Cocoa   │ │   NDK    │            ║
║ └──────────┘ └──────────┘ └──────────┘ └──────────┘            ║
║ ┌──────────┐ ┌──────────┐                                      ║
║ │   iOS    │ │   Web    │                                      ║
║ │  UIKit   │ │   WASM   │                                      ║
║ └──────────┘ └──────────┘                                      ║
╚══════════════════════════════════════════════════════════════════╝
```

### 4.2 Graphe de Dépendances des Modules

```
Application
    │
    ├─→ SceneGraph ──→ ECS ──→ Nkentseu
    ├─→ Graphics ──┬→ NKWindow ──→ NKCore
    │              └→ NKMath
    ├─→ Physics ───→ NKMath ──→ NKCore
    ├─→ Audio ─────→ NKCore
    ├─→ Animation ─→ NKMath
    └─→ Scripting ─→ Reflection (NKCore)

NKLogger ──→ NKCore
NKWindow ──→ NKCore
NKMath ───→ NKCore
Nkentseu ─→ NKCore

NKCore (pas de dépendances internes)
```

**Règles de Dépendances:**
1. Pas de dépendances circulaires
2. Les couches supérieures dépendent des couches inférieures uniquement
3. NKCore est la base (aucune dépendance)
4. Modules optionnels peuvent être désactivés (Physics, Audio, VR)

---

### 4.3 Architecture ECS (Entity Component System)

```
┌─────────────────────────────────────────────────────────┐
│                       Registry                          │
│  Central storage for all entities and components       │
├─────────────────────────────────────────────────────────┤
│  • EntityPool: SparseSet<EntityID>                     │
│  • ComponentPools: Map<TypeID, ComponentPool>          │
│  • Systems: Vector<System*>                            │
└─────────────────────────────────────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        │                │                │
        ▼                ▼                ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│   Entity    │  │ Component   │  │   System    │
│             │  │   Pools     │  │             │
├─────────────┤  ├─────────────┤  ├─────────────┤
│ • ID: u64   │  │ Transform   │  │ Render      │
│ • Version   │  │ Sprite      │  │ Physics     │
│ • Name      │  │ RigidBody   │  │ Animation   │
│             │  │ Script      │  │ AI          │
│             │  │ ...         │  │ Input       │
└─────────────┘  └─────────────┘  └─────────────┘

Data Layout (SOA - Structure of Arrays):

ComponentPool<Transform>:
┌─────────────────────────────────────────┐
│ positions:  [Vec3, Vec3, Vec3, ...]    │ ← Contiguous
│ rotations:  [Quat, Quat, Quat, ...]    │ ← Cache-friendly
│ scales:     [Vec3, Vec3, Vec3, ...]    │ ← SIMD-friendly
│ entities:   [E1, E2, E3, ...]          │
└─────────────────────────────────────────┘
```

**Exemple d'Utilisation:**

```cpp
// Création entité
auto entity = registry.CreateEntity("Player");

// Ajout components
registry.AddComponent<Transform>(entity, 
    Vector3(0, 0, 0), Quaternion::Identity(), Vector3(1, 1, 1)
);
registry.AddComponent<Sprite>(entity, spriteTexture);
registry.AddComponent<RigidBody2D>(entity, BodyType::Dynamic);
registry.AddComponent<PlayerController>(entity);

// System update (batch processing)
class RenderSystem : public System {
    void Update(Registry& registry, float dt) override {
        // View = itération efficace sur entités avec composants spécifiques
        auto view = registry.View<Transform, Sprite>();
        
        for (auto entity : view) {
            auto& transform = view.Get<Transform>(entity);
            auto& sprite = view.Get<Sprite>(entity);
            
            renderer->DrawSprite(sprite.texture, transform.GetMatrix());
        }
    }
};
```

**Avantages ECS:**
- ✅ Data locality (cache-friendly)
- ✅ Parallélisation facile (systèmes indépendants)
- ✅ Flexibilité (composition vs héritage)
- ✅ Performance prévisible

---

### 4.4 Architecture Rendu (RenderGraph)

```
                    ┌──────────────┐
                    │ RenderGraph  │
                    │  Builder     │
                    └──────┬───────┘
                           │
                           │ configure
                           ▼
        ┌──────────────────────────────────────┐
        │          Render Passes               │
        ├──────────────────────────────────────┤
        │  1. Shadow Pass                      │
        │     └→ Output: ShadowMap             │
        │  2. Geometry Pass (Deferred)         │
        │     └→ Outputs: GBuffer (albedo,     │
        │                normal, metallic,      │
        │                roughness, depth)      │
        │  3. SSAO Pass                        │
        │     └→ Input: GBuffer                │
        │     └→ Output: AO texture            │
        │  4. Lighting Pass                    │
        │     └→ Inputs: GBuffer, ShadowMap, AO│
        │     └→ Output: HDR color             │
        │  5. Transparent Pass                 │
        │     └→ Input: HDR color, depth       │
        │     └→ Output: HDR color             │
        │  6. Post-Process Pass                │
        │     ├→ Bloom                         │
        │     ├→ Tone Mapping                  │
        │     └→ FXAA                          │
        │     └→ Output: LDR color (backbuffer)│
        └──────────────────────────────────────┘
                           │
                           │ compile
                           ▼
            ┌──────────────────────────┐
            │  Optimized Command       │
            │  Buffer                  │
            ├──────────────────────────┤
            │ • Resource barriers      │
            │ • State transitions      │
            │ • Draw calls batched     │
            │ • Async compute tasks    │
            └──────────────────────────┘
```

**Code Exemple:**

```cpp
// Configuration render graph
auto renderGraph = RenderGraph::Create();

// Shadow pass
auto shadowPass = renderGraph->AddPass("Shadows");
shadowPass->SetRenderTarget(shadowMap, AttachmentLoad::Clear);
shadowPass->Execute([](CommandBuffer& cmd, const Scene& scene) {
    for (auto& light : scene.GetDirectionalLights()) {
        cmd.SetShader(shadowShader);
        cmd.SetViewProjection(light.GetViewProjection());
        scene.DrawOpaque(cmd);
    }
});

// Geometry pass
auto gBufferPass = renderGraph->AddPass("GBuffer");
gBufferPass->SetRenderTargets({
    {albedoRT, AttachmentLoad::Clear},
    {normalRT, AttachmentLoad::Clear},
    {metallicRoughnessRT, AttachmentLoad::Clear},
    {depthRT, AttachmentLoad::Clear}
});
gBufferPass->Execute([](CommandBuffer& cmd, const Scene& scene) {
    cmd.SetShader(gBufferShader);
    scene.DrawOpaque(cmd);
});

// Lighting pass
auto lightingPass = renderGraph->AddPass("Lighting");
lightingPass->ReadTexture(gBufferPass, "albedo");
lightingPass->ReadTexture(gBufferPass, "normal");
lightingPass->ReadTexture(shadowPass, "shadowMap");
lightingPass->SetRenderTarget(hdrRT, AttachmentLoad::DontCare);
lightingPass->Execute([](CommandBuffer& cmd) {
    cmd.SetShader(deferredLightingShader);
    cmd.DrawFullscreenQuad();
});

// Compile et execute
renderGraph->Compile();
renderGraph->Execute(commandBuffer);
```

---

**[Suite dans le prochain message: Sections 5-9]**

## 3. Cas d'Utilisation Détaillés

### 3.1 Acteurs du Système

#### 👨‍💻 Développeur de Jeux
**Description:** Utilise Nkentseu pour créer des jeux vidéo 2D/3D  
**Compétences:** C++, graphiques 3D, gameplay programming  
**Objectifs:** 
- Prototyper rapidement
- Performance optimale
- Déploiement multi-plateforme
- Contrôle total sur le code

**Frustrations actuelles:**
- Unity/Unreal trop "boîte noire"
- C# GC pauses inacceptables
- Royalties sur revenus
- Compilation lente

---

#### 🎨 Artiste Technique
**Description:** Crée du contenu (modèles, animations, shaders, effets)  
**Compétences:** Blender/Maya, shading, scripting Python  
**Objectifs:**
- Pipeline d'import fluide
- Preview temps réel
- Itération rapide
- Feedback visuel immédiat

**Besoins:**
- Éditeur visuel WYSIWYG
- Hot-reload assets
- Shader node editor
- Material editor interactif

---

#### 🔬 Développeur d'Applications Scientifiques
**Description:** Crée des simulations, visualisations, applications VR de formation  
**Compétences:** C++, mathématiques, physique, visualisation de données  
**Objectifs:**
- Précision calculs
- Performance temps réel
- Visualisation 3D complexe
- Export données

**Cas d'usage:**
- Simulations physiques
- Visualisation données médicales
- Training VR industriel
- Twins numériques

---

#### 🎓 Étudiant en Informatique
**Description:** Apprend le développement de moteurs de jeu  
**Compétences:** C++ débutant/intermédiaire, bases graphiques  
**Objectifs:**
- Comprendre l'architecture
- Expérimenter avec code source
- Projets portfolio
- Apprentissage par la pratique

**Besoins:**
- Documentation pédagogique
- Code lisible et commenté
- Exemples progressifs
- Communauté d'entraide

---

#### 🏢 Studio Indie/AA
**Description:** Équipe 5-50 personnes développant un jeu commercial  
**Compétences:** Multidisciplinaire (prog, art, design, audio)  
**Objectifs:**
- Pas de royalties engine
- Workflow collaboratif
- Build pipeline robuste
- Support technique

**Contraintes:**
- Budget limité
- Deadline serrée
- Qualité AAA attendue
- Multi-plateforme obligatoire

---

### 3.2 Scénarios d'Usage Complets

#### 📱 Scénario A : Jeu Mobile 2D "Platformer"

**Contexte:**  
Un développeur indie veut créer un jeu de plateforme 2D stylisé pour iOS/Android avec physique, animations, et monétisation.

**Étapes Détaillées:**

**1. Setup Projet (10 minutes)**
```bash
# Création projet via CLI
nkentseu create project MyPlatformer --template=2d-platformer
cd MyPlatformer

# Structure générée
MyPlatformer/
├── assets/
│   ├── sprites/
│   ├── audio/
│   └── levels/
├── src/
│   ├── main.cpp
│   ├── Player.h
│   └── GameScene.h
└── MyPlatformer.jenga
```

**2. Configuration Window et Renderer (5 minutes)**
```cpp
// main.cpp
#include <Nkentseu/Nkentseu.h>

int main() {
    // Configuration fenêtre mobile
    Nk::WindowConfig config;
    config.title = "My Platformer";
    config.width = 1920;   // Résolution logique
    config.height = 1080;
    config.resizable = false;
    config.fullscreen = true; // Mobile
    
    auto window = Nk::Window::Create(config);
    
    // Renderer 2D optimisé
    Nk::Renderer2DConfig rendererConfig;
    rendererConfig.maxSprites = 10000;
    rendererConfig.maxBatchSize = 1000;
    rendererConfig.enableMSAA = true;
    rendererConfig.msaaSamples = 4;
    
    auto renderer = Nk::Renderer2D::Create(window, rendererConfig);
    
    // Scene management
    auto sceneManager = Nk::MakeUnique<Nk::SceneManager>();
    sceneManager->LoadScene("assets/levels/level1.scene");
    
    // Game loop
    Nk::Clock clock;
    while (window->IsOpen()) {
        float deltaTime = clock.Restart().AsSeconds();
        
        // Input
        window->PollEvents();
        
        // Update
        sceneManager->Update(deltaTime);
        
        // Render
        renderer->BeginFrame();
        renderer->Clear(Nk::Color(0.2f, 0.6f, 1.0f)); // Sky blue
        sceneManager->Render(renderer);
        renderer->EndFrame();
        
        window->SwapBuffers();
    }
    
    return 0;
}
```

**3. Création du Joueur (20 minutes)**
```cpp
// Player.h
#pragma once
#include <Nkentseu/Nkentseu.h>

class Player : public Nk::Entity {
public:
    Player() {
        // Sprite component
        auto sprite = AddComponent<Nk::SpriteComponent>();
        sprite->SetTexture(Nk::Texture::Load("assets/sprites/player.png"));
        sprite->SetTextureRect({0, 0, 64, 64}); // Frame initial
        sprite->SetOrigin(32, 32); // Centre
        
        // Physics component
        auto body = AddComponent<Nk::RigidBody2D>();
        body->SetType(Nk::BodyType::Dynamic);
        body->SetMass(1.0f);
        body->SetFriction(0.3f);
        body->SetFixedRotation(true); // Pas de rotation
        
        // Collider
        auto collider = AddComponent<Nk::BoxCollider2D>();
        collider->SetSize(48, 60);
        collider->SetOffset(0, 2);
        
        // Animation
        auto animator = AddComponent<Nk::Animator>();
        animator->AddAnimation("idle", CreateIdleAnimation());
        animator->AddAnimation("run", CreateRunAnimation());
        animator->AddAnimation("jump", CreateJumpAnimation());
        animator->Play("idle");
        
        // Controller component (custom)
        AddComponent<PlayerController>();
    }
    
private:
    Nk::Animation* CreateIdleAnimation() {
        auto anim = Nk::MakeShared<Nk::Animation>();
        anim->AddFrame({0, 0, 64, 64}, 0.1f);
        anim->AddFrame({64, 0, 64, 64}, 0.1f);
        anim->SetLoop(true);
        return anim;
    }
    
    // ... autres animations
};

// PlayerController.h - Logique gameplay
class PlayerController : public Nk::Component {
public:
    void Update(float deltaTime) override {
        auto body = GetEntity()->GetComponent<Nk::RigidBody2D>();
        auto animator = GetEntity()->GetComponent<Nk::Animator>();
        auto input = Nk::Input::Get();
        
        // Mouvement horizontal
        float moveX = 0.0f;
        if (input->IsKeyPressed(Nk::Key::Left) || input->IsKeyPressed(Nk::Key::A)) {
            moveX = -1.0f;
        }
        if (input->IsKeyPressed(Nk::Key::Right) || input->IsKeyPressed(Nk::Key::D)) {
            moveX = 1.0f;
        }
        
        // Support tactile mobile
        if (input->IsTouchDown()) {
            Nk::Vector2 touchPos = input->GetTouchPosition();
            Nk::Vector2 playerPos = GetEntity()->GetPosition();
            if (touchPos.x < playerPos.x - 50) moveX = -1.0f;
            if (touchPos.x > playerPos.x + 50) moveX = 1.0f;
        }
        
        // Appliquer vélocité
        Nk::Vector2 velocity = body->GetVelocity();
        velocity.x = moveX * moveSpeed;
        
        // Saut
        if (isGrounded && (input->IsKeyJustPressed(Nk::Key::Space) || 
                           input->IsTouchJustDown())) {
            velocity.y = -jumpForce;
            isGrounded = false;
            animator->Play("jump");
        }
        
        body->SetVelocity(velocity);
        
        // Animation state
        if (isGrounded) {
            if (Nk::Math::Abs(moveX) > 0.01f) {
                animator->Play("run");
                // Flip sprite
                auto sprite = GetEntity()->GetComponent<Nk::SpriteComponent>();
                sprite->SetFlipX(moveX < 0);
            } else {
                animator->Play("idle");
            }
        }
    }
    
    void OnCollisionEnter(Nk::Collision2D collision) override {
        if (collision.normal.y < -0.5f) { // Contact par le bas
            isGrounded = true;
        }
    }
    
private:
    float moveSpeed = 300.0f;
    float jumpForce = 500.0f;
    bool isGrounded = false;
};
```

**4. Création Niveau (15 minutes)**
```cpp
// Level.h
class Level : public Nk::Scene {
public:
    void OnLoad() override {
        // Background parallax
        CreateBackground();
        
        // Plateformes
        CreatePlatform({0, 400}, {800, 50});
        CreatePlatform({900, 300}, {400, 50});
        CreatePlatform({1400, 200}, {300, 50});
        
        // Collectibles
        for (int i = 0; i < 10; ++i) {
            CreateCoin({100.0f + i * 150.0f, 200});
        }
        
        // Ennemis
        CreateEnemy({600, 350});
        CreateEnemy({1200, 150});
        
        // Joueur
        player = CreateEntity<Player>();
        player->SetPosition(100, 100);
        
        // Camera follow
        auto camera = GetMainCamera();
        camera->AddComponent<Nk::CameraFollow>()->SetTarget(player);
        camera->SetBounds({0, 0, 2000, 1080});
    }
    
private:
    void CreateBackground() {
        auto layers = Nk::MakeUnique<Nk::ParallaxBackground>();
        layers->AddLayer("assets/bg/sky.png", 0.1f);
        layers->AddLayer("assets/bg/clouds.png", 0.3f);
        layers->AddLayer("assets/bg/mountains.png", 0.5f);
        layers->AddLayer("assets/bg/trees.png", 0.8f);
        AddSystem(std::move(layers));
    }
    
    void CreatePlatform(Nk::Vector2 pos, Nk::Vector2 size) {
        auto platform = CreateEntity();
        platform->SetPosition(pos);
        
        auto sprite = platform->AddComponent<Nk::SpriteComponent>();
        sprite->SetTexture(Nk::Texture::Load("assets/sprites/platform.png"));
        sprite->SetSize(size);
        
        auto body = platform->AddComponent<Nk::RigidBody2D>();
        body->SetType(Nk::BodyType::Static);
        
        auto collider = platform->AddComponent<Nk::BoxCollider2D>();
        collider->SetSize(size);
    }
    
    Nk::Entity* player = nullptr;
};
```

**5. Audio et Effets (10 minutes)**
```cpp
// AudioManager setup
auto audioManager = Nk::AudioManager::Get();
audioManager->LoadMusic("assets/audio/bgm_level1.ogg", "level1_music");
audioManager->PlayMusic("level1_music", true); // Loop
audioManager->SetMusicVolume(0.6f);

// Sound effects
auto jumpSound = Nk::AudioClip::Load("assets/audio/sfx_jump.wav");
auto coinSound = Nk::AudioClip::Load("assets/audio/sfx_coin.wav");

// Dans PlayerController::Update()
if (justJumped) {
    Nk::AudioSource::PlayOneShot(jumpSound, GetEntity()->GetPosition());
}

// Particle effects pour coin collection
void Coin::OnCollect() {
    auto particles = Nk::ParticleSystem::Create();
    particles->SetPosition(GetPosition());
    particles->SetEmissionRate(50);
    particles->SetLifetime(0.5f);
    particles->SetStartColor(Nk::Color::Yellow());
    particles->SetEndColor(Nk::Color(1, 1, 0, 0));
    particles->SetStartSize(10);
    particles->SetEndSize(2);
    particles->Emit(20);
}
```

**6. Build et Déploiement Mobile (20 minutes)**
```bash
# Configuration iOS
nkentseu configure ios --bundle-id com.mystudio.platformer
nkentseu build ios --config Release --architecture arm64

# Configuration Android
nkentseu configure android --package com.mystudio.platformer
nkentseu build android --config Release --abi arm64-v8a

# Test sur device
nkentseu deploy ios --device "iPhone 14"
nkentseu deploy android --device emulator-5554
```

**Résultat Final:**
- ✅ Jeu fonctionnel en 80 minutes
- ✅ 60 FPS stable sur mobile
- ✅ Physics réactive
- ✅ Animations fluides
- ✅ Audio immersif
- ✅ Déployable iOS + Android

---

#### 🎮 Scénario B : Jeu 3D "Third-Person Shooter"

**Contexte:**  
Studio indie de 5 personnes créant un TPS avec combat, exploration, et RPG elements.

**Temps estimé:** 3-6 mois de développement

**Phase 1: Prototype Jouable (1 mois)**

```cpp
// Player3D.h - Contrôleur third-person
class Player3D : public Nk::Entity {
public:
    Player3D() {
        // Modèle 3D avec animations
        auto mesh = AddComponent<Nk::MeshRenderer>();
        mesh->SetMesh(Nk::Model::Load("assets/models/player.fbx"));
        mesh->SetMaterial(CreatePlayerMaterial());
        
        // Skeletal animator
        auto animator = AddComponent<Nk::SkeletalAnimator>();
        animator->LoadAnimations({
            {"idle", "assets/anims/idle.fbx"},
            {"walk", "assets/anims/walk.fbx"},
            {"run", "assets/anims/run.fbx"},
            {"shoot", "assets/anims/shoot.fbx"},
            {"reload", "assets/anims/reload.fbx"}
        });
        
        // Blend tree pour mouvement
        auto blendTree = animator->CreateBlendTree2D("locomotion");
        blendTree->AddAnimation("idle", {0, 0});
        blendTree->AddAnimation("walk", {0, 0.5f});
        blendTree->AddAnimation("run", {0, 1.0f});
        
        // Physics capsule
        auto body = AddComponent<Nk::RigidBody>();
        body->SetMass(80.0f); // kg
        body->SetFriction(0.5f);
        body->SetAngularFactor({0, 0, 0}); // Pas de rotation physique
        
        auto capsule = AddComponent<Nk::CapsuleCollider>();
        capsule->SetHeight(1.8f);
        capsule->SetRadius(0.3f);
        
        // Camera controller
        auto camController = AddComponent<ThirdPersonCamera>();
        camController->SetDistance(5.0f);
        camController->SetHeight(2.0f);
        camController->SetSensitivity(0.3f);
        
        // Weapon system
        auto weaponSystem = AddComponent<WeaponController>();
        weaponSystem->EquipWeapon(CreateRifle());
    }
};

// ThirdPersonCamera.h
class ThirdPersonCamera : public Nk::Component {
public:
    void Update(float deltaTime) override {
        auto input = Nk::Input::Get();
        auto player = GetEntity();
        
        // Rotation caméra avec souris
        if (input->IsMouseButtonPressed(Nk::MouseButton::Right)) {
            Nk::Vector2 mouseDelta = input->GetMouseDelta();
            yaw += mouseDelta.x * sensitivity;
            pitch -= mouseDelta.y * sensitivity;
            pitch = Nk::Math::Clamp(pitch, -80.0f, 80.0f);
        }
        
        // Position caméra behind player
        Nk::Vector3 offset(
            distance * Nk::Math::Cos(Nk::Angle::Degrees(pitch)) * 
                       Nk::Math::Sin(Nk::Angle::Degrees(yaw)),
            distance * Nk::Math::Sin(Nk::Angle::Degrees(pitch)) + height,
            distance * Nk::Math::Cos(Nk::Angle::Degrees(pitch)) * 
                       Nk::Math::Cos(Nk::Angle::Degrees(yaw))
        );
        
        Nk::Vector3 targetPos = player->GetPosition() + offset;
        
        // Smooth follow
        Nk::Vector3 camPos = Nk::Vector3::Lerp(
            camera->GetPosition(), 
            targetPos, 
            deltaTime * followSpeed
        );
        
        camera->SetPosition(camPos);
        camera->LookAt(player->GetPosition() + Nk::Vector3(0, 1.5f, 0));
        
        // Collision caméra avec environnement
        RaycastCameraCollision(player->GetPosition(), camPos);
    }
    
private:
    void RaycastCameraCollision(Nk::Vector3 target, Nk::Vector3& camPos) {
        Nk::Ray ray(target, camPos - target);
        Nk::RaycastHit hit;
        
        if (Nk::Physics::Raycast(ray, hit, distance)) {
            camPos = hit.point - ray.direction * 0.2f; // Petit offset
        }
    }
    
    float yaw = 0.0f;
    float pitch = 20.0f;
    float distance = 5.0f;
    float height = 2.0f;
    float sensitivity = 0.3f;
    float followSpeed = 10.0f;
};

// WeaponController.h
class WeaponController : public Nk::Component {
public:
    void Update(float deltaTime) override {
        auto input = Nk::Input::Get();
        
        // Tir
        if (input->IsMouseButtonPressed(Nk::MouseButton::Left) && 
            canShoot && currentAmmo > 0) {
            Shoot();
        }
        
        // Reload
        if (input->IsKeyJustPressed(Nk::Key::R) && currentAmmo < maxAmmo) {
            StartReload();
        }
        
        // Recoil recovery
        recoilOffset = Nk::Vector3::Lerp(recoilOffset, Nk::Vector3::Zero(), 
                                         deltaTime * recoilRecovery);
        
        // Update weapon position
        UpdateWeaponPosition();
    }
    
private:
    void Shoot() {
        // Raycast pour hit detection
        auto camera = Nk::Camera::GetMain();
        Nk::Ray ray = camera->ScreenPointToRay(Nk::Input::Get()->GetMousePosition());
        
        Nk::RaycastHit hit;
        if (Nk::Physics::Raycast(ray, hit, 1000.0f)) {
            // Hit effects
            CreateBulletHole(hit.point, hit.normal);
            
            // Damage
            if (auto damageable = hit.entity->GetComponent<Damageable>()) {
                damageable->TakeDamage(weaponDamage);
            }
            
            // Blood/sparks particles
            CreateHitEffect(hit.point, hit.entity->GetTag());
        }
        
        // Muzzle flash
        muzzleFlash->Play();
        
        // Sound
        Nk::AudioSource::PlayOneShot(shootSound, GetEntity()->GetPosition());
        
        // Camera shake
        Nk::Camera::GetMain()->Shake(0.1f, 0.2f);
        
        // Recoil
        ApplyRecoil();
        
        currentAmmo--;
        fireTimer = fireRate;
        canShoot = false;
    }
    
    void ApplyRecoil() {
        recoilOffset.y += Nk::Random::Range(0.05f, 0.1f);
        recoilOffset.x += Nk::Random::Range(-0.03f, 0.03f);
    }
    
    void StartReload() {
        auto animator = GetEntity()->GetComponent<Nk::SkeletalAnimator>();
        animator->Play("reload");
        
        reloadTimer = reloadTime;
        isReloading = true;
    }
    
    Nk::SharedPtr<Weapon> currentWeapon;
    int currentAmmo = 30;
    int maxAmmo = 30;
    float weaponDamage = 25.0f;
    float fireRate = 0.1f; // 10 rounds/sec
    float reloadTime = 2.0f;
    Nk::Vector3 recoilOffset;
    float recoilRecovery = 5.0f;
};
```

**Phase 2: Systèmes de Combat (1 mois)**

```cpp
// AI Enemy
class EnemyAI : public Nk::Component {
public:
    void Update(float deltaTime) override {
        switch (currentState) {
            case AIState::Patrol:
                UpdatePatrol(deltaTime);
                break;
            case AIState::Chase:
                UpdateChase(deltaTime);
                break;
            case AIState::Attack:
                UpdateAttack(deltaTime);
                break;
            case AIState::Cover:
                UpdateCover(deltaTime);
                break;
        }
        
        // Check for player visibility
        if (CanSeePlayer()) {
            if (currentState == AIState::Patrol) {
                TransitionTo(AIState::Chase);
            }
        }
    }
    
private:
    void UpdateChase(float deltaTime) {
        auto player = Nk::Scene::GetActive()->FindEntity("Player");
        Nk::Vector3 playerPos = player->GetPosition();
        
        // Pathfinding vers joueur
        auto path = Nk::NavMesh::FindPath(GetEntity()->GetPosition(), playerPos);
        
        if (!path.empty()) {
            Nk::Vector3 nextPoint = path[0];
            MoveTowards(nextPoint, deltaTime);
            
            // Si assez proche, attaquer
            float distance = Nk::Vector3::Distance(GetEntity()->GetPosition(), 
                                                   playerPos);
            if (distance < attackRange) {
                TransitionTo(AIState::Attack);
            }
        }
    }
    
    bool CanSeePlayer() {
        auto player = Nk::Scene::GetActive()->FindEntity("Player");
        Nk::Vector3 toPlayer = player->GetPosition() - GetEntity()->GetPosition();
        
        // Vision cone
        float angle = Nk::Vector3::Angle(GetEntity()->GetForward(), toPlayer);
        if (angle > visionAngle) return false;
        
        // Raycast pour obstacles
        Nk::Ray ray(GetEntity()->GetPosition() + Nk::Vector3(0, 1.5f, 0), 
                    toPlayer.Normalized());
        Nk::RaycastHit hit;
        
        if (Nk::Physics::Raycast(ray, hit, visionRange)) {
            return hit.entity == player;
        }
        
        return false;
    }
    
    enum class AIState { Patrol, Chase, Attack, Cover };
    AIState currentState = AIState::Patrol;
    float visionRange = 30.0f;
    float visionAngle = 60.0f;
    float attackRange = 15.0f;
};
```

**Phase 3: Environnement et Assets (2 mois)**

```cpp
// Level loading with streaming
class OpenWorld : public Nk::Scene {
public:
    void OnLoad() override {
        // Terrain
        auto terrain = CreateTerrain("assets/terrain/heightmap.png");
        terrain->SetSize(1000, 100, 1000);
        terrain->SetMaterial(CreateTerrainMaterial());
        
        // Vegetation system
        auto vegSystem = AddSystem<VegetationSystem>();
        vegSystem->PlaceTrees("assets/models/tree_*.fbx", 1000);
        vegSystem->PlaceGrass("assets/textures/grass.png", 10000);
        
        // LOD system
        auto lodManager = AddSystem<LODManager>();
        lodManager->SetLODDistances({50, 150, 500});
        
        // Streaming zones
        CreateStreamingZones();
        
        // Lighting
        SetupLighting();
        
        // Weather system
        auto weather = AddSystem<WeatherSystem>();
        weather->SetTime(12.0f); // Noon
        weather->EnableDynamicWeather(true);
    }
    
private:
    void SetupLighting() {
        // Directional light (sun)
        auto sun = CreateEntity();
        auto dirLight = sun->AddComponent<Nk::DirectionalLight>();
        dirLight->SetDirection({-0.5f, -1.0f, -0.3f});
        dirLight->SetColor({1.0f, 0.95f, 0.8f});
        dirLight->SetIntensity(1.5f);
        dirLight->EnableShadows(true);
        dirLight->SetShadowCascades(4);
        
        // Ambient light
        Nk::Renderer::SetAmbientLight({0.3f, 0.35f, 0.4f});
        
        // Skybox
        auto skybox = Nk::Skybox::Load("assets/skybox/day_*.hdr");
        Nk::Renderer::SetSkybox(skybox);
    }
};
```

**Résultat après 3-6 mois:**
- ✅ Prototype jouable complet
- ✅ Mouvement third-person fluide
- ✅ Système de combat fonctionnel
- ✅ AI ennemis avec pathfinding
- ✅ Monde ouvert avec streaming
- ✅ Graphismes AAA (PBR, shadows, post-FX)
- ✅ Performance 60 FPS sur PC
- ✅ Ready pour playtesting

---

#### 🥽 Scénario C : Application VR de Formation Industrielle

**Contexte:**  
Entreprise crée une simulation VR pour former techniciens à la maintenance de machines complexes.

**Objectifs:**
- Formation immersive sans risque
- Procédures étape par étape
- Feedback temps réel
- Tracking performance
- Multi-utilisateurs

**Implémentation (2 mois):**

```cpp
// VRApplication.cpp
class IndustrialTraining : public Nk::VRApplication {
public:
    void OnLoad() override {
        // VR setup
        Nk::VRConfig vrConfig;
        vrConfig.trackingSpace = Nk::VRTrackingSpace::RoomScale;
        vrConfig.renderScale = 1.4f; // Supersampling
        
        vrSystem = Nk::VRSystem::Create(vrConfig);
        
        // Controllers
        leftHand = vrSystem->GetController(Nk::VRHand::Left);
        rightHand = vrSystem->GetController(Nk::VRHand::Right);
        
        // Hand models
        leftHand->SetModel("assets/models/hand_left.fbx");
        rightHand->SetModel("assets/models/hand_right.fbx");
        
        // Load factory environment
        factory = Nk::Scene::Load("assets/scenes/factory_floor.scene");
        
        // Load machine to maintain
        machine = factory->FindEntity("TurbineGenerator");
        
        // Setup training module
        trainingModule = Nk::MakeUnique<TrainingModule>();
        trainingModule->LoadProcedure("assets/procedures/turbine_maintenance.json");
        
        // UI overlay
        vrUI = Nk::MakeUnique<VRUIManager>();
        vrUI->CreateInstructionPanel();
        vrUI->CreateProgressTracker();
    }
    
    void Update(float deltaTime) override {
        // Update VR tracking
        vrSystem->Update();
        
        // Handle controller input
        HandleGrip();
        HandleTeleport();
        
        // Update training logic
        trainingModule->Update(deltaTime);
        
        // Update UI
        vrUI->Update(deltaTime);
    }
    
private:
    void HandleGrip() {
        // Right hand grip
        if (rightHand->GetGripValue() > 0.8f && !isGripping) {
            TryGrabObject(rightHand);
            isGripping = true;
        } else if (rightHand->GetGripValue() < 0.2f && isGripping) {
            ReleaseObject(rightHand);
            isGripping = false;
        }
        
        // Update grabbed object position
        if (grippedObject) {
            grippedObject->SetPosition(rightHand->GetPosition());
            grippedObject->SetRotation(rightHand->GetRotation());
        }
    }
    
    void TryGrabObject(Nk::VRController* hand) {
        Nk::Vector3 handPos = hand->GetPosition();
        Nk::Quaternion handRot = hand->GetRotation();
        
        // Raycast from hand
        Nk::Ray ray(handPos, handRot * Nk::Vector3::Forward());
        Nk::RaycastHit hit;
        
        if (Nk::Physics::Raycast(ray, hit, 0.3f)) {
            if (auto grabbable = hit.entity->GetComponent<Grabbable>()) {
                grippedObject = hit.entity;
                grippedObject->GetComponent<Nk::RigidBody>()->SetKinematic(true);
                
                // Haptic feedback
                hand->Vibrate(0.5f, 50); // intensity, duration ms
                
                // Visual feedback
                grabbable->Highlight(true);
                
                // Training validation
                trainingModule->OnObjectGrabbed(grippedObject->GetName());
            }
        }
    }
    
    void HandleTeleport() {
        // Thumbstick for teleport arc
        Nk::Vector2 thumbstick = leftHand->GetThumbstick();
        
        if (thumbstick.y > 0.5f) {
            // Show teleport arc
            Nk::Vector3 startPos = leftHand->GetPosition();
            Nk::Vector3 direction = leftHand->GetRotation() * 
                                    Nk::Vector3::Forward();
            
            teleportArc = CalculateTeleportArc(startPos, direction);
            ShowTeleportArc(teleportArc);
            
            // Teleport on trigger release
            if (leftHand->IsTriggerJustReleased()) {
                TeleportPlayer(teleportArc.endPoint);
            }
        } else {
            HideTeleportArc();
        }
    }
    
    Nk::UniquePtr<Nk::VRSystem> vrSystem;
    Nk::VRController* leftHand;
    Nk::VRController* rightHand;
    Nk::Entity* grippedObject = nullptr;
};

// TrainingModule.h - Gestion des étapes de formation
class TrainingModule {
public:
    void LoadProcedure(const Nk::String& filepath) {
        auto json = Nk::JSON::Load(filepath);
        
        for (auto& stepJson : json["steps"]) {
            TrainingStep step;
            step.id = stepJson["id"];
            step.description = stepJson["description"];
            step.instruction = stepJson["instruction"];
            step.requiredAction = stepJson["action"];
            step.targetObject = stepJson["target"];
            step.validationFunc = CreateValidator(stepJson["validation"]);
            
            steps.push_back(step);
        }
        
        currentStepIndex = 0;
    }
    
    void Update(float deltaTime) {
        if (currentStepIndex >= steps.size()) {
            OnTrainingComplete();
            return;
        }
        
        auto& currentStep = steps[currentStepIndex];
        
        // Check validation
        if (currentStep.validationFunc()) {
            OnStepCompleted(currentStep);
            currentStepIndex++;
            
            if (currentStepIndex < steps.size()) {
                OnStepStarted(steps[currentStepIndex]);
            }
        }
        
        // Update timer
        currentStep.timeSpent += deltaTime;
        
        // Hint system
        if (currentStep.timeSpent > 30.0f && !hintShown) {
            ShowHint(currentStep.hint);
            hintShown = true;
        }
    }
    
    void OnObjectGrabbed(const Nk::String& objectName) {
        auto& currentStep = steps[currentStepIndex];
        
        if (objectName == currentStep.targetObject) {
            // Correct object
            PlayPositiveFeedback();
            currentStep.correctActions++;
        } else {
            // Wrong object
            PlayNegativeFeedback();
            currentStep.errors++;
        }
    }
    
private:
    void OnStepCompleted(const TrainingStep& step) {
        // Visual feedback
        ShowCompletionAnimation();
        
        // Audio feedback
        Nk::AudioSource::PlayOneShot(successSound);
        
        // Haptic feedback
        rightHand->Vibrate(1.0f, 200);
        
        // Analytics
        RecordStepMetrics(step);
        
        // UI update
        vrUI->UpdateProgress(currentStepIndex + 1, steps.size());
    }
    
    void RecordStepMetrics(const TrainingStep& step) {
        StepMetrics metrics;
        metrics.stepId = step.id;
        metrics.timeSpent = step.timeSpent;
        metrics.errors = step.errors;
        metrics.correctActions = step.correctActions;
        metrics.hintsUsed = step.hintsUsed;
        
        sessionMetrics.steps.push_back(metrics);
    }
    
    struct TrainingStep {
        Nk::String id;
        Nk::String description;
        Nk::String instruction;
        Nk::String targetObject;
        Nk::String requiredAction;
        std::function<bool()> validationFunc;
        float timeSpent = 0.0f;
        int errors = 0;
        int correctActions = 0;
        int hintsUsed = 0;
    };
    
    std::vector<TrainingStep> steps;
    int currentStepIndex = 0;
};
```

**Résultat:**
- ✅ Formation VR immersive
- ✅ Tracking précis des mains
- ✅ Validation étape par étape
- ✅ Feedback multimodal (visuel, audio, haptique)
- ✅ Analytics détaillées
- ✅ Réduction 60% temps formation vs méthode traditionnelle
- ✅ Rétention connaissances +40%

---

### 3.3 Diagrammes de Cas d'Utilisation

#### Diagramme Cas d'Usage Général

```
                    [Nkentseu Framework]
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
   ┌────▼────┐         ┌────▼────┐       ┌─────▼─────┐
   │Développer│        │ Créer   │       │  Éditer   │
   │   Jeu   │        │Application       │  Contenu  │
   └────┬────┘        └────┬────┘       └─────┬─────┘
        │                   │                   │
        │                   │                   │
  ┌─────▼─────┐       ┌─────▼─────┐     ┌─────▼─────┐
  │Développeur│       │Développeur│     │  Artiste  │
  │  de Jeux  │       │   App     │     │ Technique │
  └───────────┘       └───────────┘     └───────────┘

            Cas d'Usage par Acteur
            
Développeur de Jeux:
├── Initialiser Framework
├── Créer Fenêtre
├── Setup Renderer
├── Charger Assets
├── Créer Entités
├── Implémenter Logique
├── Gérer Input
├── Gérer Audio
├── Build et Déployer
└── Debug et Profile

Développeur App:
├── Créer Interface
├── Gérer Données
├── Implémenter Logique Métier
├── Setup VR/AR
├── Networking
└── Export Résultats

Artiste Technique:
├── Importer Assets
├── Créer Matériaux
├── Créer Animations
├── Setup Particules
├── Éditer Scènes
└── Preview Temps Réel
```

---

**[Document trop long - Suite dans le prochain fichier...]**

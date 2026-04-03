// ============================================================================
// Sandbox/src/main.cpp
// Pattern A : Dispatcher typé (push - événementiel)
// ============================================================================

#include "NKWindow/Core/NkWindow.h"
#include "NKWindow/Core/NkSystem.h"
#include "NKWindow/Events/NkEventDispatcher.h"
#include "NKWindow/Events/NkEventSystem.h"
#include "NKWindow/Events/NkGamepadSystem.h"
#include "NKWindow/Core/NkMain.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/NkRendererConfig.h"
#include "NKTime/NkChrono.h"

#include "NKLogger/NkLog.h"
#include "NKMath/NKMath.h"

#include "NKMemory/NkMemory.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <iostream>
#include <assert.h>
#include <random>
#include <cstdlib>

#ifndef NK_SANDBOX_RENDERER_API
#define NK_SANDBOX_RENDERER_API nkentseu::NkRendererApi::NK_SOFTWARE
#endif

using namespace nkentseu;
using namespace nkentseu::math;

// ============================================================================
int nkmain(const nkentseu::NkEntryState& /*state*/)
{
    using namespace nkentseu;
    using namespace NkMath;

    // -------------------------------------------------------------------------
    // 1. Initialisation
    // -------------------------------------------------------------------------
    if (!NkInitialise({ .appName = "NkWindow Sandbox Pattern A" })) {
        logger.Error("[Sandbox] NkInitialise FAILED");
        return -1;
    }

    // -------------------------------------------------------------------------
    // 2. Fenêtre
    // -------------------------------------------------------------------------
    NkWindowConfig cfg;
    cfg.title       = "NkWindow Sandbox - Pattern A (Dispatcher)";
    cfg.width       = 900;
    cfg.height      = 600;
    cfg.centered    = true;
    cfg.resizable   = true;
    cfg.dropEnabled = true;

    NkWindow window(cfg);
    if (!window.IsOpen()) {
        logger.Error("[Sandbox] Window creation FAILED");
        NkClose();
        return -2;
    }

    // -------------------------------------------------------------------------
    // 3. Renderer
    // -------------------------------------------------------------------------
    NkRendererConfig rcfg;
    rcfg.api                   = NK_SANDBOX_RENDERER_API;
    rcfg.autoResizeFramebuffer = true;

    mem::NkUniquePtr<NkRenderer> renderer;
    if (rcfg.api != NkRendererApi::NK_NONE) {
        renderer = mem::NkMakeUnique<NkRenderer>();
        if (!renderer->Create(window, rcfg)) {
            logger.Error("[Sandbox] Renderer creation FAILED");
            NkClose();
            return -3;
        }
    }

    // DEVOIR : TPs Semaines 1 à 3

    const int width = 512, height = 512;
    NkImage img(width, height);
    float s1, s2;
    std::vector<float> v;
    Vec2d u, w, n;
    Vec3d i, j, k;
    Vec4d s, q;
    Mat4d m, r, inv;


    // TP1 : InspectFloat(float x)

    inspectFloat(0.1f);
    inspectFloat(1.0f);
    inspectFloat(1.0f / 0.0f);
    inspectFloat(std::sqrt(-1.0f));
    inspectFloat(-0.0f);
    inspectFloat(0.0f);
    inspectFloat(std::numeric_limits<float>::min());


    // TP2 : Kahan et Welford

    // 1. Tableau de 1 000 000  de copies de 0.1f
    std::vector<float> data(1'000'000, 0.1f);
    
    // 2. Comparaison : Somme accumulate vs Somme Kahan
    s1 = std::accumulate(data.begin(), data.end(), 0.0f);
    s2 = kahanSum(data);
    logger.Info("\nSum with accumulate : {0}\nKahan sum : {1}\nReal value : 100000.0", s1, s2);
    
    // 3. Calcul de variance : Variance naïve vs Variance Welford
    v = std::vector<float>({1e8f, 1e8f, 1.0f, 2.0f});
    logger.Info("\nVariance Naive   : {0}\nVariance de Welford : {1}", varianceNaive(v), varianceWelford(v));

    // 4. Comparaison : Epsilon machine par boucle vs std::numeric_limits<float>::epsilon() 
    logger.Info("\nEpsilon Machine (loop) : {0}\nEpsilon Machine (std)  : {1}", epsilonMachine(), std::numeric_limits<float>::epsilon());
    

    // TP3 : NkMath/Float.h

    // 1. Tests isFiniteValid     
    assert(!isFiniteValid(std::numeric_limits<float>::quiet_NaN())); // 1
    assert(!isFiniteValid(std::numeric_limits<float>::infinity()));  // 2
    assert(!isFiniteValid(-std::numeric_limits<float>::infinity())); // 3
    assert(isFiniteValid(0.0f));                                     // 4
    assert(isFiniteValid(1.0f));                                     // 5

    // 2. Tests nearlyZero 
    assert(nearlyZero(0.0f, 1e-6f));     // 6
    assert(nearlyZero(1e-7f, 1e-6f));    // 7
    assert(!nearlyZero(1e-5f, 1e-6f));   // 8

    assert(nearlyZero(-1e-7f, 1e-6f));   // 9
    assert(!nearlyZero(-1e-5f, 1e-6f));  // 10

    assert(nearlyZero(1e-3f, 1e-2f));    // 11
    assert(!nearlyZero(1e-2f, 1e-3f));   // 12

    assert(nearlyZero(5e-8f, 1e-7f));    // 13

    // 3. Tests approxEq 
    assert(approxEq(1.0f, 1.0f, 1e-6f));           // 14
    assert(approxEq(1.0f, 1.0000001f, 1e-5f));     // 15
    assert(!approxEq(1.0f, 1.1f, 1e-3f));          // 16

    assert(approxEq(0.0f, 1e-7f, 1e-6f));          // 17
    assert(!approxEq(0.0f, 1e-4f, 1e-6f));         // 18

    assert(approxEq(-1.0f, -1.000001f, 1e-5f));    // 19
    assert(!approxEq(-1.0f, -1.1f, 1e-2f));        // 20

    assert(approxEq(1000.0f, 1000.0001f, 1e-3f));  // 21
    assert(approxEq(1000.0f, 1001.0f, 1e-3f));     // 22

    assert(approxEq(1e-7f, 2e-7f, 1e-6f));         // 23

    // 4. Tests kahanSum vs accumulate    
    v = std::vector<float>(1000, 0.1f);
    s1 = std::accumulate(v.begin(), v.end(), 0.0f);
    s2 = kahanSum(v);
    assert(std::fabs(s2 - 100.0f) < std::fabs(s1 - 100.0f)); // 24
    
    v = std::vector<float>(10000, 0.1f);
    s1 = std::accumulate(v.begin(), v.end(), 0.0f);
    s2 = kahanSum(v);
    assert(std::fabs(s2 - 1000.0f) < std::fabs(s1 - 1000.0f)); // 25
    
    v = std::vector<float>({1e8f, 1.0f, -1e8f});
    s1 = std::accumulate(v.begin(), v.end(), 0.0f);
    s2 = kahanSum(v);
    assert(std::fabs(s2 - 1.0f) <= std::fabs(s1 - 1.0f)); // 26

    v = std::vector<float>({1.0f, 1e8f, -1e8f});
    s1 = std::accumulate(v.begin(), v.end(), 0.0f);
    s2 = kahanSum(v);
    assert(std::fabs(s2 - 1.0f) <= std::fabs(s1 - 1.0f)); // 27
    
    v = std::vector<float>(100000, 0.01f);
    s2 = kahanSum(v);
    assert(approxEq(s2, 1000.0f, 1e-2f)); // 28

    v = std::vector<float>(100000, 1e-5f);
    s2 = kahanSum(v);
    assert(approxEq(s2, 1.0f, 1e-3f)); // 29
    
    v = std::vector<float>({0.1f, 0.2f, 0.3f});
    s2 = kahanSum(v);
    assert(approxEq(s2, 0.6f, 1e-6f)); // 30

    v = std::vector<float>(50000, 0.2f);
    s2 = kahanSum(v);
    assert(approxEq(s2, 10000.0f, 1e-2f)); // 31
    
    v = std::vector<float>({1e7f, 1.0f, 1.0f, -1e7f});
    s2 = kahanSum(v);
    assert(approxEq(s2, 2.0f, 1e-3f)); // 32

    v = std::vector<float>(1000000, 0.1f);
    s2 = kahanSum(v);
    assert(approxEq(s2, 100000.0f, 1e-1f)); // 33



    // TP4 : Vec2d complet et implémentations

    // 1. Dot product 
    assert(Dot({1,0}, {0,1}) == 0.0);      // 1
    assert(Dot({1,0}, {1,0}) == 1.0);      // 2
    assert(Dot({3,4}, {3,4}) == 25.0);     // 3
    assert(Dot({-1,0}, {1,0}) == -1.0);    // 4
    assert(Dot({2,3}, {4,5}) == 23.0);     // 5
    assert(Dot({0,0}, {5,7}) == 0.0);      // 6

    // 2. CROSS2D 
    assert(Cross2D({1,0}, {0,1}) == 1.0);   // 7
    assert(Cross2D({0,1}, {1,0}) == -1.0);  // 8
    assert(Cross2D({1,1}, {1,1}) == 0.0);   // 9
    assert(Cross2D({2,0}, {0,2}) == 4.0);   // 10

    // 3. NORMALISATION 
    w = {3,4};
    n = w.Normalized();
    assert(std::fabs(n.Norm() - 1.0) < kEps);   // 11

    // direction conservée
    assert(std::fabs(n.x - 0.6) < kEps);    // 12
    assert(std::fabs(n.y - 0.8) < kEps);    // 13

    // vecteur unitaire reste inchangé
    u = {1,0};
    u = u.Normalized();
    assert(std::fabs(u.x - 1.0) < kEps);   // 14

    // 4. OPERATOR [] 
    w = {10, 20};
    assert(w[0] == 10.0);   // 15
    assert(w[1] == 20.0);   // 16
    w[0] = 30;
    assert(w.x == 30.0);    // 17
    w[1] = 40;
    assert(w.y == 40.0);    // 18
    u = {5, 6};
    assert(u[0] == 5.0);    // 19

    // 5. STATIC ASSERT 
    static_assert(sizeof(Vec2d) == 16, "Vec2d must be 16 bytes"); // 20


    // TP5 : Vec3d avec Gram-Schmidt
    
    // 1 & 2. Cross Product
    i = {1,0,0}, j = {0,1,0}, k = {0,0,1};
    // règle main droite
    assert(ApproxVec(Cross(i, j), k));               // 1
    assert(ApproxVec(Cross(j, i), {0,0,-1}));        // 2
    // base complète
    assert(ApproxVec(Cross(j, k), i));               // 3
    assert(ApproxVec(Cross(k, i), j));               // 4
    // orthogonalité
    assert(approxEq(Dot(Cross(i, j), i), 0));        // 5
    assert(approxEq(Dot(Cross(i, j), j), 0));        // 6

    // 2. Gram-Schmidt sur 10 triplets aléatoires 
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(-10.0, 10.0);

    for(int t = 0; t < 10; ++t) {
        Vec3d a{dist(rng), dist(rng), dist(rng)};
        Vec3d b{dist(rng), dist(rng), dist(rng)};
        Vec3d c{dist(rng), dist(rng), dist(rng)};

        // Gram-Schmidt
        Vec3d ui = a.Normalized();
        Vec3d vi = (b - Project(b, ui)).Normalized();
        Vec3d wi = (c - Project(c, ui) - Project(c, vi)).Normalized();
        // normes
        assert(approxEq(ui.Norm(), 1.0));  // 7
        assert(approxEq(vi.Norm(), 1.0));  // 8
        assert(approxEq(wi.Norm(), 1.0));  // 9

        // orthogonalité
        assert(approxEq(Dot(ui, vi), 0.0));  // 10
        assert(approxEq(Dot(ui, wi), 0.0));  // 11
        assert(approxEq(Dot(vi, wi), 0.0));  // 12
    }

    // 3. Project et Reject
    i = {3,4,0}, j = {1,0,0};
    Vec3d prj = Project(i, j);
    Vec3d rej = Reject(i, j);
    assert(ApproxVec(prj + rej, i)); // 13



     
    // -------------------------------------------------------------------------
    // 5. Boucle principale
    // -------------------------------------------------------------------------
    auto& eventSystem = NkEvents();

    bool running = true;
    float timeSeconds = 0.f;
    NkChrono chrono;
    NkElapsedTime elapsed;

    while (running && window.IsOpen())
    {
        NkElapsedTime e = chrono.Reset();

        // --- Pattern A : Dispatcher typé (OnEvent pour chaque event)
        while (NkEvent* event = eventSystem.PollEvent())
            if (event->Is<nkentseu::NkWindowCloseEvent>())
                running = false;

        if (!running || !window.IsOpen())
            break;

        // --- Cap 60 fps ---
        elapsed = chrono.Elapsed();
        if (elapsed.milliseconds < 16)
            NkChrono::Sleep(16 - elapsed.milliseconds);
        else
            NkChrono::YieldThread();
    }

    // -------------------------------------------------------------------------
    // 6. Nettoyage
    // -------------------------------------------------------------------------
    if (renderer)
        renderer->Shutdown();

    window.Close();
    NkClose();
    return 0;
}

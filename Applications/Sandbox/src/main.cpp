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

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

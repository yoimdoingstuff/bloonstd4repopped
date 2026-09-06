#include "PSPLifecycle.hpp"
#include "rendering/LogicalResolution.hpp"
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("BTD4Repopped", 0, 0, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(64);
PSP_HEAP_SIZE_KB(12 * 1024);

int main() {
    pspDebugScreenInit();
    if (!btd4::installPSPExitCallback()) {
        pspDebugScreenPrintf("Failed to install exit callback.\n");
        sceKernelDelayThread(2000000);
        sceKernelExitGame();
        return 1;
    }
    // Exercise a shared engine function without desktop renderer dependencies.
    const auto viewport = btd4::LogicalResolution::calculateViewport(480, 272);
    pspDebugScreenPrintf("Bloons TD 4 Repopped\n\n");
    pspDebugScreenPrintf("Native PSP bootstrap\n");
    pspDebugScreenPrintf("Engine viewport: %d x %d\n\n", viewport.width, viewport.height);
    pspDebugScreenPrintf("No source game files are required.\n");
    pspDebugScreenPrintf("Gameplay and GU renderer are not connected yet.\n\n");
    pspDebugScreenPrintf("Press START or HOME to exit.\n");
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
    while (true) {
        sceKernelCheckCallback();
        SceCtrlData pad{};
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_START) break;
        sceDisplayWaitVblankStart();
    }
    sceKernelExitGame();
    return 0;
}

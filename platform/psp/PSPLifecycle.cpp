#include "PSPLifecycle.hpp"
#include <pspkernel.h>

namespace {
int exitCallback(int, int, void*) {
    sceKernelExitGame();
    return 0;
}
}
namespace btd4 {
bool installPSPExitCallback() {
    const int callback = sceKernelCreateCallback("BTD4 exit", exitCallback, nullptr);
    if (callback < 0) return false;
    if (sceKernelRegisterExitCallback(callback) < 0) {
        sceKernelDeleteCallback(callback);
        return false;
    }
    return true;
}
}

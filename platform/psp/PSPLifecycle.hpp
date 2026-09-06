#pragma once
namespace btd4 {
// Installs the PSP HOME/exit callback on the calling thread.
// The host loop must service callbacks with sceKernelCheckCallback().
// Returns false if any required kernel object cannot be created.
bool installPSPExitCallback();
}

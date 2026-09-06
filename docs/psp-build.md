# PSP bootstrap and EBOOT build

The PSP target is a minimal native bootstrap, not the playable game. It shows
an SDK debug screen, calls shared engine viewport code at 480x272, and accepts
START or HOME to exit. No original game files, SDL or desktop builder components
are required. GU rendering, shared controller input, audio and filesystem
backends remain separate TODO tasks.

## Toolchain

Use the open-source [PSPDEV/PSPSDK](https://pspdev.github.io/) toolchain.
Set `PSPDEV` to its installation directory and add `$PSPDEV/bin` to PATH.
CMake and a build generator (Make or Ninja) must also be available on the host.
The repository toolchain includes the installed SDK's `pspdev.cmake` rather
than duplicating its ABI/linker setup. C++ exceptions remain enabled because
the internal data loaders use them.

```sh
cmake --preset psp-release
cmake --build build/psp-release --parallel
ctest --test-dir build/psp-release --output-on-failure --no-tests=error
```

For a Ninja installation, add `-G Ninja` to the configure command in a fresh
build directory. The SDK's `psp-cmake` wrapper is also supported:

```sh
psp-cmake -S . -B build/psp-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build/psp-sdk --parallel
ctest --test-dir build/psp-sdk --output-on-failure --no-tests=error
```

The SDK's `create_pbp_file` builds a user-mode PRX, fixes imports, creates SFO
metadata and produces `build/psp-release/EBOOT.PBP`. The default package contains
no external media. Do not explicitly link `pspuser` before libc: the compiler
specs add core SDK stub libraries in the required order.

## Windows validation environment

This task was cross-compiled on Windows using the community
[pspdev-win v2 bundle](https://github.com/dmang-dev/pspdev-win/releases/tag/v2)
(GCC 15.2.0), portable CMake 4.4.3 and Ninja. Tool downloads were kept outside
the repository in `%TEMP%/btd4-toolchain`. PSPDEV and CMake archive checksums
were verified against their published sums. The existing Git for Windows
`usr/bin` supplied the MSYS runtime for this local build; the bundle's
maintainer recommends standalone MSYS2 for a supported installation.
No persistent PATH/environment settings were changed. Temporary tools may be
removed by Windows; set up a durable toolchain for ongoing development.

For this temporary setup, a PowerShell build can use:

```powershell
$env:PSPDEV = "$env:TEMP/btd4-toolchain/pspdev"
$env:Path = "$env:PSPDEV/bin;$env:TEMP/btd4-toolchain/ninja;C:/Program Files/Git/usr/bin;$env:Path"
& "$env:TEMP/btd4-toolchain/cmake-4.4.3-windows-x86_64/bin/cmake.exe" --preset psp-release -G Ninja
& "$env:TEMP/btd4-toolchain/cmake-4.4.3-windows-x86_64/bin/cmake.exe" --build build/psp-release --parallel
& "$env:TEMP/btd4-toolchain/cmake-4.4.3-windows-x86_64/bin/ctest.exe" --test-dir build/psp-release --output-on-failure
```

## Package and launch checks

Place EBOOT.PBP in `PSP/GAME/BTD4REPOPPED/EBOOT.PBP` for a PSP setup that can
run homebrew, or open EBOOT.PBP in PPSSPP. Expected behavior to verify:

- The native bootstrap text appears with a 480 x 272 viewport readout.
- START exits normally.
- HOME invokes the registered exit callback and exits normally.

The host loop services callbacks on the thread which registered them. It
waits for controller sampling/vblank rather than spinning freely. A 64 KiB
main stack and 12 MiB heap are bootstrap settings, not profiled game budgets.

## Verified scope

The shared engine and native PSP entry point compiled and linked successfully.
EBOOT.PBP was generated without SDK import-order warnings. CTest validated its
PBP header, ordered section offsets, PARAM.SFO signature and ELF32 little-endian
MIPS payload. Negative checks rejected missing/truncated files, invalid magic,
out-of-range offsets and a non-MIPS payload.

These are compilation and package-structure checks, not runtime tests. PPSSPP,
real PSP hardware, rendering performance and HOME/START behavior have not yet
been tested. GitHub Actions were updated but were not dispatched in this task.
The standalone PSP workflow now builds, tests, packages and uploads a real
EBOOT.PBP; Combined CI reuses that workflow. There is no EBOOT.BIN fallback.
The container supplies the toolchain; its dependency step verifies those tools.

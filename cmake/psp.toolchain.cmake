# Delegate ABI, compiler and packaging setup to the installed open-source SDK.
# Keep exceptions enabled: internal data loaders report parse errors with them.
if(NOT DEFINED ENV{PSPDEV} OR "$ENV{PSPDEV}" STREQUAL "")
    message(FATAL_ERROR "Set PSPDEV to your PSPDEV installation before configuring PSP.")
endif()
file(TO_CMAKE_PATH "$ENV{PSPDEV}" PSPDEV)
set(_btd4_sdk_toolchain "${PSPDEV}/psp/share/pspdev.cmake")
if(NOT EXISTS "${_btd4_sdk_toolchain}")
    message(FATAL_ERROR "PSPSDK CMake toolchain not found: ${_btd4_sdk_toolchain}")
endif()
# Compiler probes need not produce a bootable PSP executable.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
include("${_btd4_sdk_toolchain}")

# CMake Toolchain File for PlayStation Portable (PSPSDK / PSPDEV)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)
set(CMAKE_CROSSCOMPILING TRUE)

# Locate PSPDEV
if(NOT DEFINED ENV{PSPDEV})
    message(WARNING "PSPDEV environment variable not set. Defaulting to /usr/local/pspdev")
    set(PSPDEV "/usr/local/pspdev")
else()
    set(PSPDEV "$ENV{PSPDEV}")
endif()

set(CMAKE_C_COMPILER "${PSPDEV}/bin/psp-gcc")
set(CMAKE_CXX_COMPILER "${PSPDEV}/bin/psp-g++")
set(CMAKE_AR "${PSPDEV}/bin/psp-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${PSPDEV}/bin/psp-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_STRIP "${PSPDEV}/bin/psp-strip" CACHE FILEPATH "Strip")

# PSP Compile flags
set(PSP_COMPILE_FLAGS "-D__PSP__ -D_PSP_FW_VERSION=371 -G0 -Wall")
set(CMAKE_C_FLAGS_INIT "${PSP_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${PSP_COMPILE_FLAGS} -fno-exceptions -fno-rtti")

# Include directories
include_directories(
    SYSTEM
    "${PSPDEV}/psp/include"
    "${PSPDEV}/psp/sdk/include"
)

# Link directories
link_directories(
    "${PSPDEV}/psp/lib"
    "${PSPDEV}/psp/sdk/lib"
)

# Standard PSP Libraries
set(PSP_STANDARD_LIBS pspdebug pspgu pspge pspdisplay pspctrl pspsdk c pspnet pspnet_inet pspnet_apctl psputility pspuser)

set(CMAKE_FIND_ROOT_PATH "${PSPDEV}/psp")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

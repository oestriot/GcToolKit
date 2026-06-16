cmake_minimum_required(VERSION 3.11)
option(BUILD_PC "Builds PC Utility, GcNetworkBackup" OFF)
option(CROSS "Cross-Compile GcNetworkBackup to either 'WINDOWS' or 'LINUX'" OFF)

if(BUILD_PC OR CROSS)
# build pc program

if(CROSS STREQUAL "LINUX")
    set(CMAKE_TOOLCHAIN_FILE "pc/cross/linux.cmake" CACHE PATH "toolchain file")
	message(STATUS "Cross-Compiling for linux using gcc.")
elseif(CROSS STREQUAL "WINDOWS")
    set(CMAKE_TOOLCHAIN_FILE "pc/cross/win32.cmake" CACHE PATH "toolchain file")
	message(STATUS "Cross-Compiling for win32 using mingw-gcc.")
endif()

project(GcNetworkBackup_build)
add_subdirectory(pc)
else()

# build for psvita ... 
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(GcToolKit_Build)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

add_subdirectory(kern)
add_subdirectory(app)

endif()

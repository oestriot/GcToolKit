cmake_minimum_required(VERSION 3.11)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(GcToolKitBuild)
if(BUILD_PC)
add_subdirectory(pc)
else()
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

add_subdirectory(kern)
add_subdirectory(app)

add_custom_target(${PROJECT_NAME}
	ALL
	DEPENDS GcToolKit.vpk-vpk
	COMMAND mv ${CMAKE_BINARY_DIR}/app/GcToolKit.vpk .
)

endif()


# TODO: find out how to build pc/ folder for !vita.
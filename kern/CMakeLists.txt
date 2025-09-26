cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(GcKernKit)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -std=gnu17")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DENABLE_LOGGING=1")


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(${CMAKE_SOURCE_DIR}/inc)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT_NAME}
	main.c
	io.c
	cobra.c
	otg.c
	format.c
	mod.c
	cmd56.c
	gc.c
)

target_link_libraries(${PROJECT_NAME}
	taihenForKernel_stub
	taihenModuleUtils_stub
	SceRtcForDriver_stub
	SceModulemgrForDriver_stub
	SceThreadmgrForDriver_stub
	SceSblSmCommForKernel_stub
	SceIofilemgrForDriver_stub
	SceSysclibForDriver_stub
	SceSysmemForDriver_stub
	SceDebugForDriver_stub
	ScePowerForDriver_stub
	SceSdifForDriver_stub
	SceSysconForDriver_stub
	SceSblGcAuthMgrGcAuthForDriver_stub
	SceSblGcAuthMgrDrmBBForDriver_stub
)

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.yml
)
vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.yml
  KERNEL
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
  
)

cmake_minimum_required(VERSION 3.11)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(GcToolKit)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "GCToolKit")
set(VITA_TITLEID  "GCTK10000")
set(VITA_VERSION  "01.06")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-format-truncation -std=gnu17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DENABLE_LOGGING=1")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DENABLE_LOGGING=1")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS}")
set(VITA_MAKE_FSELF_FLAGS "${VITA_MAKE_FSELF_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/inc)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/kern
)

add_executable(${PROJECT_NAME}
	aes.c
	aes_cmac.c
	config.c
	err.c
	sha256.c
	sha1.c
	ctrl.c
	draw.c
	device.c
	crypto.c
	kernel.c
	lock.c
	io.c
	gameinfo.c
	sfo.c
	main.c
	menu.c
	ime.c
	net.c
	gc_ident.c
	bgm.cpp
)

# require libGcKernKit.a to be built first, before building GcToolKit ELF.
add_dependencies(${PROJECT_NAME} GcKernKit-stubs)

target_link_libraries(${PROJECT_NAME}
# vita kernel modules
	GcKernKit_stub
	taihen_stub
	
# vita libraries
	vita2d
	soloud
	pthread
	stdc++
	freetype
	png
	jpeg
	m
	z


# vita system libraries

	SceSysmodule_stub
	SceNet_stub
	SceNetCtl_stub_weak
	SceDisplay_stub
	SceGxm_stub
	ScePgf_stub
	ScePvf_stub
	SceCommonDialog_stub
	SceCtrl_stub
	SceVshBridge_stub
	SceAppMgr_stub
	SceAppUtil_stub
	SceIofilemgr_stub
	ScePower_stub
	SceRegistryMgr_stub
	SceShellSvc_stub
	SceAppUtil_stub
	SceIme_stub
	SceAudio_stub
	SceError_stub
)


vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}

  FILE ${CMAKE_BINARY_DIR}/kern/GcKernKit.skprx GcKernKit.skprx
  
  FILE res/sce_sys/icon0.png sce_sys/icon0.png
  FILE res/sce_sys/pic0.png sce_sys/pic0.png
  FILE res/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE res/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  
  FILE res/background.png res/background.png
  FILE res/insertgc.png res/insertgc.png
  FILE res/menuoverlay.png res/menuoverlay.png
  FILE res/bgm.ogg res/bgm.ogg  
)

add_dependencies(${PROJECT_NAME}.vpk-vpk GcKernKit.skprx-self)

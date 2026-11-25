cmake_minimum_required(VERSION 3.10)
project(GcNetworkBackup)

if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -static-libgcc -Wl,-Bstatic -O3")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -O3")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

include_directories(${CMAKE_SOURCE_DIR}/inc)

add_executable(${PROJECT_NAME}
	main.c
)

if(WIN32)
target_link_libraries(${PROJECT_NAME}
	ws2_32
)
endif()

cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME visor)
project(${PROJECT_NAME})

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)

# replace by add_library with "STATIC" later
add_executable(
	${PROJECT_NAME} 
	${SOURCES})

target_include_directories(
	${PROJECT_NAME} 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/external/volk
	${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers/include)

# platform detection
if(WIN32)
	message(STATUS "platform : windows")
	target_compile_definitions(${PROJECT_NAME} PRIVATE VSR_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
	message(STATUS "platform : linux")
	target_compile_definitions(${PROJECT_NAME} PRIVATE VSR_PLATFORM_LINUX)
elseif(APPLE)
	message(FATAL_ERROR "unsupported platform: macOS is not supported")
else()
	message(FATAL_ERROR "unknown platform")
endif()

# graphics API
target_compile_definitions(${PROJECT_NAME} PRIVATE VSR_GRAPHICS_API_VULKAN VK_NO_PROTOTYPES)

add_subdirectory(external/glfw)
add_subdirectory(external/trivex)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw trivex)


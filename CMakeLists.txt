cmake_minimum_required(VERSION 3.20)
project(WindowsAudioPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
	message(FATAL_ERROR "Windows only")
endif()

# Use static runtime: /MT (Release) and /MTd (Debug)
if (MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Output directories (per-arch build directory is selected by -A Win32/x64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# mmkv
add_subdirectory(third_party/MMKV-2.2.4/Core subdir_mmkv)

add_subdirectory(src/main)
add_subdirectory(src/hook)



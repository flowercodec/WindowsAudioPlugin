project(AudioHook LANGUAGES CXX)

add_library(AudioHook SHARED
	dllmain.cpp
	AudioHook.cpp
	ApiInfo.cpp
	AudioHook.def
)

target_sources(AudioHook PRIVATE
	AudioHook.h
	ApiInfo.h
)

target_include_directories(AudioHook PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/third_party/Detours/include
)

# Choose output name by arch (explicitly distinguish)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(AudioHook PROPERTIES OUTPUT_NAME "AudioHook64")
else()
	set_target_properties(AudioHook PROPERTIES OUTPUT_NAME "AudioHook32")
endif()

# Detours root candidates (support both Detours/ and detours/)
set(DETOURS_ROOT_UPPER "${CMAKE_SOURCE_DIR}/third_party/Detours")
set(DETOURS_ROOT_LOWER "${CMAKE_SOURCE_DIR}/third_party/detours")

# Map to common win dir names
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_DET_WIN_DIR "win64")
else()
	set(_DET_WIN_DIR "win32")
endif()

# Locate Release library (Detours.lib / detours.lib)
find_library(LIB_DETOURS_R
	detours.lib
	${CMAKE_SOURCE_DIR}/third_party/detours/${_DET_WIN_DIR}/lib
	NO_DEFAULT_PATH
)

# Locate Debug library (Detoursd.lib / detoursd.lib)
find_library(LIB_DETOURS_D
	detoursd.lib
	${CMAKE_SOURCE_DIR}/third_party/detours/${_DET_WIN_DIR}/lib
	NO_DEFAULT_PATH
)

# Use generator expression to select debug/release
set(DETOURS_LIB
	"$<$<NOT:$<CONFIG:DEBUG>>:${LIB_DETOURS_R}>"
	"$<$<CONFIG:DEBUG>:${LIB_DETOURS_D}>"
)

target_link_libraries(AudioHook
	PRIVATE
	${DETOURS_LIB}
	ole32
	oleaut32
	uuid
	core
)

target_compile_definitions(AudioHook
	PRIVATE UNICODE _UNICODE
)


project(WindowsAudioPluginApp LANGUAGES CXX)

add_executable(WindowsAudioPlugin WIN32
	MainApp.cpp
	HiddenWnd.cpp
	SystemTray.cpp
	PowerMonitor.cpp
	AudioManager.cpp
	Settings.cpp
	ConfigDialog.cpp
	HookInstaller.cpp
	resource.rc
)

# Add headers and resources to project for IDE browsing
target_sources(WindowsAudioPlugin PRIVATE
	MainApp.h
	HiddenWnd.h
	SystemTray.h
	PowerMonitor.h
	AudioManager.h
	Settings.h
	ConfigDialog.h
	HookInstaller.h
	resource.h
	app_icon.ico
)

target_include_directories(WindowsAudioPlugin PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(WindowsAudioPlugin PROPERTIES
	MFC_FLAG 2
)

target_link_libraries(WindowsAudioPlugin
	PRIVATE
	version
	shlwapi
	shell32
	ole32
	oleaut32
	uuid
	user32
	comdlg32
	comctl32
	AudioHook
	core
)

set(CMAKE_MFC_FLAG 1)
target_compile_definitions(WindowsAudioPlugin
	PRIVATE UNICODE _UNICODE
)

# Copy ffplay.exe next to the built WindowsAudioPlugin.exe after build
add_custom_command(TARGET WindowsAudioPlugin POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/tools/ffplay.exe"
		"$<TARGET_FILE_DIR:WindowsAudioPlugin>/ffplay.exe"
	COMMENT "Copy ffplay.exe to runtime output directory"
)


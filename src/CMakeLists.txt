project(OpenFunscripter)


set(OPEN_FUNSCRIPTER_SOURCES
  "main.cpp"
  "OpenFunscripter.cpp"
  "UndoSystem.cpp"
  "Funscript/FunscriptAction.cpp"
  "Funscript/Funscript.cpp"
  "OpenFunscripterSettings.cpp"
  "RawInput.cpp"
  "ScriptingMode.cpp"
  "OpenFunscripterUtil.cpp"

  "UI/KeybindingSystem.cpp"
  "UI/GradientBar.cpp"
  "UI/ScriptPositionsWindow.cpp"
  "UI/OpenFunscripterVideoplayer.cpp"

  "event/EventSystem.cpp"

  "imgui_impl/imgui_impl_opengl3.cpp"
  "imgui_impl/imgui_impl_sdl.cpp"
)

add_executable(${PROJECT_NAME} ${OPEN_FUNSCRIPTER_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
  nlohmann_json
  imgui
  glad
  SDL2-static
  SDL2main
  portable_file_dialogs
)

if(WIN32)
target_include_directories(${PROJECT_NAME} PRIVATE 
	"${CMAKE_SOURCE_DIR}/lib/libmpv/include"
)
target_link_directories(${PROJECT_NAME} PUBLIC 
	"${CMAKE_SOURCE_DIR}/lib/libmpv/"
)
target_link_libraries(${PROJECT_NAME} PUBLIC
	# linking of libmpv can be improved but this works...
	  mpv.lib
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
	"NOMINMAX"
)
else()
# linux etc. 
find_package(PkgConfig REQUIRED) 
pkg_check_modules(mpv REQUIRED IMPORTED_TARGET mpv)

target_link_libraries(${PROJECT_NAME} PUBLIC
	mpv
)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
	"${PROJECT_SOURCE_DIR}/"
	"${PROJECT_SOURCE_DIR}/imgui_impl/"
	"${PROJECT_SOURCE_DIR}/UI/"
	"${PROJECT_SOURCE_DIR}/event/"
	"${PROJECT_SOURCE_DIR}/Funscript/"
	"${CMAKE_SOURCE_DIR}/lib/minimp3/"
)


# c++17
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# warnings as errors
if(MSVC)
	# target_compile_options(${PROJECT_NAME} PRIVATE /W2 /WX)
	# target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else() # clang/gcc
	target_compile_options(${PROJECT_NAME} PUBLIC -fpermissive -Wno-writable-strings)
	# target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror 
	# 	-Wno-error=unused-parameter 
	# 	-Wno-error=unused-variable 
	# 	-Wno-gnu-anonymous-struct
	# 	-Wno-nested-anon-types
	# ) 
endif()
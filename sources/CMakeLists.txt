cmake_minimum_required (VERSION 2.6)
project (GeometryTool)

include_directories(
	"../3rdparty/glew-1.10.0/include"
	"../3rdparty/SDL2-2.0.1/include"
	"../3rdparty/glm-0.9.4.3"
	"../3rdparty/rapidxml-1.13"
)

link_directories(
	"D:/MyEngine/GeometryTool/3rdparty/glew-1.10.0/lib/Release/Win32"
	"D:/MyEngine/GeometryTool/3rdparty/SDL2-2.0.1/lib/x86"
)

set(SOURCES 
	main.cpp
	render_primitives.h
	render_primitives.cpp
	pch.h
	pch.cpp
	window.h
	window.cpp
	stbi_image.h
)

add_executable (Geometry WIN32 ${SOURCES}) 

target_link_libraries (Geometry SDL2 SDL2main glew32 opengl32)

set_target_properties (Geometry PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/../bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/../bin"
)

set_target_properties (Geometry PROPERTIES COMPILE_FLAGS "/Yupch.h")
set_source_files_properties (pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
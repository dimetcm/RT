# Function for building single app
function(buildApp APP_NAME)
	SET(APP_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${APP_NAME})
	message(STATUS "Generating project file for app in ${APP_FOLDER}")
	# Main
	file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${APP_FOLDER}/*.cpp)
	SET(MAIN_CPP ${APP_FOLDER}/${APP_NAME}.cpp)
	if(EXISTS ${APP_FOLDER}/main.cpp)
		SET(MAIN_CPP ${APP_FOLDER}/main.cpp)
	ENDIF()
	if(EXISTS ${APP_FOLDER}/${APP_NAME}.h)
		SET(MAIN_HEADER ${APP_FOLDER}/${APP_NAME}.h)
	ENDIF()

	# Add shaders
	set(SHADER_DIR_GLSL "../shaders/glsl/${APP_NAME}")
	file(GLOB SHADERS_GLSL "${SHADER_DIR_GLSL}/*.vert" "${SHADER_DIR_GLSL}/*.frag" "${SHADER_DIR_GLSL}/*.comp" "${SHADER_DIR_GLSL}/*.geom" "${SHADER_DIR_GLSL}/*.tesc" "${SHADER_DIR_GLSL}/*.tese" "${SHADER_DIR_GLSL}/*.mesh" "${SHADER_DIR_GLSL}/*.task" "${SHADER_DIR_GLSL}/*.rgen" "${SHADER_DIR_GLSL}/*.rchit" "${SHADER_DIR_GLSL}/*.rmiss" "${SHADER_DIR_GLSL}/*.rcall" "${SHADER_DIR_GLSL}/*.rahit" "${SHADER_DIR_GLSL}/*.rint" "${SHADER_DIR_GLSL}/*.glsl")
	set(SHADER_DIR_HLSL "../shaders/hlsl/${APP_NAME}")
	file(GLOB SHADERS_HLSL "${SHADER_DIR_HLSL}/*.vert" "${SHADER_DIR_HLSL}/*.frag" "${SHADER_DIR_HLSL}/*.comp" "${SHADER_DIR_HLSL}/*.geom" "${SHADER_DIR_HLSL}/*.tesc" "${SHADER_DIR_HLSL}/*.tese" "${SHADER_DIR_HLSL}/*.mesh" "${SHADER_DIR_HLSL}/*.task" "${SHADER_DIR_HLSL}/*.rgen" "${SHADER_DIR_HLSL}/*.rchit" "${SHADER_DIR_HLSL}/*.rmiss" "${SHADER_DIR_HLSL}/*.rcall" "${SHADER_DIR_HLSL}/*.rahit" "${SHADER_DIR_HLSL}/*.rint")
	source_group("Shaders\\GLSL" FILES ${SHADERS_GLSL})
	source_group("Shaders\\HLSL" FILES ${SHADERS_HLSL})
	# Add optional readme / tutorial
	file(GLOB README_FILES "${APP_FOLDER}/*.md")

	add_executable(${APP_NAME} WIN32 ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_GLSL} ${SHADERS_HLSL} ${README_FILES})
	target_link_libraries(${APP_NAME} base ${Vulkan_LIBRARY} ${WINLIBS})

	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

	if(RESOURCE_INSTALL_DIR)
		install(TARGETS ${APP_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
	endif()
endfunction(buildApp)

# Build all apps
function(buildApps)
	foreach(APP ${APPS})
		buildApp(${APP})
	endforeach(APP)
endfunction(buildApps)

set(APPS
    app0
)

buildApps()

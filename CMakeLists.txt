cmake_minimum_required (VERSION 3.8)

set(PROJECT "outline-triangulation")
set(TARGET "app")
project(${PROJECT})

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Enable compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(NORMAL_PATH PROJECT_DIR)
set(SOURCE_DIR "${PROJECT_DIR}/src")
set(INCLUDE_DIR "${PROJECT_DIR}/include")
set(SUBMODULES_DIR "${PROJECT_DIR}/submodules")
set(THIRDPARTY_DIR "${PROJECT_DIR}/thirdparty")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_DIR}/bin)

file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.hpp" "${SOURCE_DIR}/*.h" "${INCLUDE_DIR}/*.hpp" "${INCLUDE_DIR}/*.h")

# Set their directories
foreach (SOURCE_FILE ${SOURCES};${HEADERS})

	# Get relative path
	cmake_path(RELATIVE_PATH SOURCE_FILE BASE_DIRECTORY ${PROJECT_DIR})

	# Extract directory
	get_filename_component(SOURCE_FILE_DIR ${SOURCE_FILE} DIRECTORY)
	
	# Set source group
	if (WIN32) # Replace slashes with backslashes for Windows
		string(REPLACE "/" "\\" SOURCE_FILE_DIR ${SOURCE_FILE_DIR})
	endif ()
	source_group("${SOURCE_FILE_DIR}" FILES ${SOURCE_FILE})

endforeach ()

# Thirdparty
set(GLM_DIR "${THIRDPARTY_DIR}/glm")

# Set include directories
set(INCLUDE_DIRS
	"${SOURCE_DIR}/"
	"${INCLUDE_DIR}/"
	"${XDG_SHELL_DIR}/"
	"${GLM_DIR}/"
	)

# Optional Thirdparty
if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
elseif (LINUX)

	set(XDG_SHELL_DIR "${THIRDPARTY_DIR}/wlr-protocols/include")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${XDG_SHELL_DIR})

elseif (WIN32)

	find_package(Vulkan REQUIRED)
	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS})

endif ()

add_executable(${TARGET} ${SOURCES} ${HEADERS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")

	message( FATAL_ERROR "Sorry, bruh, this project is meant to be build only for Linux+Wayland and Windows" )
	add_compile_definitions(__PLATFORM_EMSCRIPTEN__)

else ()

	if (WIN32)

		# Set Windows entry point to main(), instead of WinMain()
		set_target_properties(${TARGET} PROPERTIES
			LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup"
			LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
		# Enable vcpkg
		set_target_properties(${TARGET} PROPERTIES VS_GLOBAL_VcpkgEnabled true)

		target_link_libraries(${TARGET} Vulkan::Vulkan)
		add_compile_definitions(__PLATFORM_WINDOWS__)

	elseif (LINUX)

		target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
		add_subdirectory("${THIRDPARTY_DIR}/wlr-protocols")
		target_link_libraries(${TARGET} wlr-protocols vulkan wayland-client)
		add_compile_definitions(__PLATFORM_LINUX__ __USE_WAYLAND__)

	else ()
		message( FATAL_ERROR "Sorry, bruh, this project is meant to be build only for Linux+Wayland and Windows" )
		add_compile_definitions(__PLATFORM_UNKNOWN__)
	endif ()

endif ()

include_directories(${INCLUDE_DIRS})
add_definitions(-DTARGET="${TARGET}")

# Hide ZERO_CHECK and ALL_BUILD targets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER meta)

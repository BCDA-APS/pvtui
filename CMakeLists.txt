cmake_minimum_required (VERSION 3.22)

project(pvtui
  LANGUAGES CXX
  VERSION 1.0.0
)

# Generate compile_commands.json for LSP's
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ Standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Built binaries in build/bin, libraries in build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# PVTUI build options
option(FETCH_FTXUI "Fetch FTXUI using FetchContent" ON)
option(BUILD_LIB "Build PVTUI library" ON)
option(BUILD_APPS "Build applications in apps/" ON)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)

# Add EPICS_BASE as a cached string option
if(NOT DEFINED EPICS_BASE OR EPICS_BASE STREQUAL "")
    set(EPICS_BASE $ENV{EPICS_BASE})
endif()
set(EPICS_BASE "${EPICS_BASE}" CACHE STRING "Path to EPICS base")

if(NOT BUILD_LIB)
    if(BUILD_APPS OR BUILD_TESTS)
        message(WARNING "Disabling BUILD_APPS and BUILD_TESTS because BUILD_LIB is OFF.")
        set(BUILD_APPS OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    endif()
endif()

if (BUILD_LIB)
    # --- FTXUI --------------------------------------------------------------------
    if(FETCH_FTXUI)
	message(STATUS "Fetching FTXUI using FetchContent.")
	include(FetchContent)

	set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
	set(FETCHCONTENT_QUIET OFF)

	FetchContent_Declare(ftxui
	    GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
	    GIT_TAG         v6.1.9
	    GIT_PROGRESS    TRUE
	    GIT_SHALLOW     TRUE
	    EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(ftxui)

    else()
	message(STATUS "Attempting to find FTXUI on the system.")
	find_package(ftxui REQUIRED CONFIG)
	if(ftxui_FOUND)
	    message(STATUS "ftxui found at: ${ftxui_DIR}")
	else()
	    message(FATAL_ERROR "ftxui not found on your system. Please install it or enable -DFETCH_FTXUI=ON.")
	endif()
    endif()
    # ------------------------------------------------------------------------------


    # --- EPICS Base ---------------------------------------------------------------
    # Get path to EPICS base from environment variable or -DEPICS_BASE
    if(NOT DEFINED EPICS_BASE OR EPICS_BASE STREQUAL "")
	set(EPICS_BASE $ENV{EPICS_BASE})
    endif()
    if(NOT EPICS_BASE OR EPICS_BASE STREQUAL "")
	message(FATAL_ERROR "Missing required variable 'EPICS_BASE'. Provide it via -DEPICS_BASE=... or set the EPICS_BASE environment variable.")
    endif()
    if(NOT IS_DIRECTORY "${EPICS_BASE}")
	message(FATAL_ERROR "Specified EPICS_BASE does not exist: ${EPICS_BASE}")
    endif()

    # Put it into the cache so it’s visible in GUI tools or later CMake runs
    set(EPICS_BASE "${EPICS_BASE}" CACHE STRING "Path to EPICS base")
    message(STATUS "Found EPICS_BASE: ${EPICS_BASE}")

    # Get EPICS host architecture
    set(EPICS_HOST_ARCH "$ENV{EPICS_HOST_ARCH}" CACHE STRING "Host architecture (e.g. linux-x86_64")
    if (NOT EPICS_HOST_ARCH)
	message(FATAL_ERROR "'EPICS_HOST_ARCH' not defined")
    endif()
    message(STATUS "EPICS_HOST_ARCH: ${EPICS_HOST_ARCH}")

    if(APPLE)
	set(EPICS_LIB_EXT ".dylib")
    elseif(UNIX)
	set(EPICS_LIB_EXT ".so")
    else()
	set(EPICS_LIB_EXT ".a")
    endif()

    set(EPICS_BASE_LIBRARIES
	"${EPICS_BASE}/lib/${EPICS_HOST_ARCH}/libpvAccessCA${EPICS_LIB_EXT}"
	"${EPICS_BASE}/lib/${EPICS_HOST_ARCH}/libca${EPICS_LIB_EXT}"
	"${EPICS_BASE}/lib/${EPICS_HOST_ARCH}/libpvAccess${EPICS_LIB_EXT}"
	"${EPICS_BASE}/lib/${EPICS_HOST_ARCH}/libpvData${EPICS_LIB_EXT}"
	"${EPICS_BASE}/lib/${EPICS_HOST_ARCH}/libCom${EPICS_LIB_EXT}"
    )

    # List all possible directories
    set(POTENTIAL_EPICS_INCLUDES
	${EPICS_BASE}/include
	${EPICS_BASE}/include/compiler/gcc
	${EPICS_BASE}/include/compiler/clang
	${EPICS_BASE}/include/pv
	${EPICS_BASE}/include/pva
	${EPICS_BASE}/include/os/Linux
	${EPICS_BASE}/include/os/Darwin
    )

    # Only add directories that actually exist
    set(EPICS_BASE_INCLUDES "")
    foreach(DIR ${POTENTIAL_EPICS_INCLUDES})
	if(IS_DIRECTORY "${DIR}")
	    list(APPEND EPICS_BASE_INCLUDES "${DIR}")
	endif()
    endforeach()
    # ------------------------------------------------------------------------------

    # --- PVTUI static library -----------------------------------------------------
    add_library(pvtui STATIC pvtui/pvtui.cpp pvtui/pvgroup.cpp)
    target_compile_options(pvtui PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(pvtui PUBLIC -std=c++17)
    target_include_directories(pvtui
	PUBLIC
	    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	    $<INSTALL_INTERFACE:include>
    )
    target_include_directories(pvtui
	SYSTEM
	PUBLIC
	    ${EPICS_BASE_INCLUDES}
    )
    target_link_libraries(pvtui
	PUBLIC ${EPICS_BASE_LIBRARIES}
	PUBLIC ftxui::screen
	PUBLIC ftxui::dom
	PUBLIC ftxui::component
    )
    # ------------------------------------------------------------------------------

    # --- Installation/CMake config ------------------------------------------------
    # Install library headers
    install(DIRECTORY pvtui
	    DESTINATION include
	    FILES_MATCHING
		PATTERN "*.hpp"
		PATTERN "*.h")

    # Install the actual library
    install(TARGETS pvtui
	    EXPORT pvtuiTargets
	    ARCHIVE DESTINATION lib
	    LIBRARY DESTINATION lib
	    RUNTIME DESTINATION bin
	    INCLUDES DESTINATION include)

    # Export the targets to a cmake file
    install(EXPORT pvtuiTargets
	    FILE pvtuiTargets.cmake
	    NAMESPACE pvtui::
	    DESTINATION lib/cmake/pvtui)

    include(CMakePackageConfigHelpers)

    configure_package_config_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pvtuiConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/pvtuiConfig.cmake
      INSTALL_DESTINATION lib/cmake/pvtui
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/pvtuiConfig.cmake
      DESTINATION lib/cmake/pvtui)
    # ------------------------------------------------------------------------------
endif() # BUILD_LIB

# --- Build applications -------------------------------------------------------
if(BUILD_APPS)
    message(STATUS "Building applications")
    add_subdirectory(apps)
else()
    message(STATUS "Skipping executables in apps/ (BUILD_APPS=OFF)")
endif()
# ------------------------------------------------------------------------------

# --- Build tests --------------------------------------------------------------
if(BUILD_TESTS)
    message(STATUS "Building tests")
    add_subdirectory(tests)
else()
    message(STATUS "Skipping tests (BUILD_TESTS=OFF)")
endif()
# ------------------------------------------------------------------------------

# --- Doxygen Documentation Generation -----------------------------------------
if (BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    find_program(SPHINX_EXECUTABLE sphinx-build)
    if (NOT DOXYGEN_FOUND)
	message(FATAL_ERROR "Could not find doxygen")
    endif()
    if (NOT SPHINX_EXECUTABLE)
	message(FATAL_ERROR "Could not find sphinx")
    endif()

    configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
	${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
    )

    add_custom_target(
	doc
	# 1. Run Doxygen to generate docs/xml
	COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	# 2. Run Sphinx on the docs folder
	COMMAND ${SPHINX_EXECUTABLE} -b html
		${CMAKE_CURRENT_SOURCE_DIR}/doc
		${CMAKE_CURRENT_BINARY_DIR}/doc/sphinx
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating Doxygen XML and Sphinx HTML"
	VERBATIM
    )
endif()
# ------------------------------------------------------------------------------

# Print a summary of the options
include(FeatureSummary)
message(STATUS "------------------------------------------------")
message(STATUS " Configuration Summary for ${PROJECT_NAME}")
message(STATUS "   BUILD_LIB:   ${BUILD_LIB}")
message(STATUS "   BUILD_APPS:  ${BUILD_APPS}")
message(STATUS "   BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "   BUILD_DOCS:  ${BUILD_DOCS}")
message(STATUS "   FETCH_FTXUI: ${FETCH_FTXUI}")
message(STATUS "   EPICS_BASE:  ${EPICS_BASE}")
message(STATUS "------------------------------------------------")

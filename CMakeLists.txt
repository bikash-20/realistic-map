cmake_minimum_required(VERSION 3.20)
project(realistic_map LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# --- raylib ------------------------------------------------------------------
# Try Homebrew first (Apple Silicon & Intel), then pkg-config, then a manual
# fallback path so this works on distros without pkg-config.
find_program(RAYLIB_BREW brew)
if(RAYLIB_BREW)
    execute_process(COMMAND brew --prefix raylib
        OUTPUT_VARIABLE BREW_RAYLIB_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(RAYLIB_INCLUDE_DIRS "${BREW_RAYLIB_PREFIX}/include")
    set(RAYLIB_LIBRARY_DIRS "${BREW_RAYLIB_PREFIX}/lib")
else()
    set(RAYLIB_INCLUDE_DIRS "/opt/homebrew/include")
    set(RAYLIB_LIBRARY_DIRS "/opt/homebrew/lib")
endif()
set(RAYLIB_LIBRARIES "raylib")

if(NOT EXISTS "${RAYLIB_INCLUDE_DIRS}/raylib.h")
    message(FATAL_ERROR
        "raylib.h not found at ${RAYLIB_INCLUDE_DIRS}.\n"
        "Install with: brew install raylib")
endif()
message(STATUS "Using raylib from ${RAYLIB_INCLUDE_DIRS}")

# --- executable --------------------------------------------------------------
add_executable(realistic_map
    src/main.cpp
    src/NavSystem.cpp
    src/MapData.cpp
    src/AStar.cpp
    src/MapView.cpp
    src/MapRenderer.cpp
    src/Sidebar.cpp
    src/RouteAnimator.cpp
    src/Util.cpp
)
target_include_directories(realistic_map PRIVATE
    ${RAYLIB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)
target_link_directories(realistic_map PRIVATE ${RAYLIB_LIBRARY_DIRS})
target_link_libraries(realistic_map PRIVATE ${RAYLIB_LIBRARIES} m)

# Copy the bundled city data file next to the binary so the app finds it
# regardless of where the user runs it from.
add_custom_command(TARGET realistic_map POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/data/city.json
        $<TARGET_FILE_DIR:realistic_map>/city.json
)

install(TARGETS realistic_map DESTINATION bin)
install(DIRECTORY data DESTINATION share/realistic_map)

cmake_minimum_required(VERSION 3.16)
project(Empire LANGUAGES C CXX)

if(MSVC AND NOT CMAKE_GENERATOR MATCHES "Ninja")
    add_compile_options(/MP)
endif()

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

add_subdirectory(soy/SDL)

# gl loader for win/macos
if(NOT EMSCRIPTEN)
    add_library(glad STATIC soy/glad/src/gl.c)
    target_include_directories(glad PUBLIC soy/glad/include)
endif()

# Codegen tool (native only)
if(NOT EMSCRIPTEN)
    add_executable(Florence src/codegen/florence.c)
    target_include_directories(Florence PRIVATE include)
    target_link_libraries(Florence PRIVATE SDL3::SDL3)
    set_target_properties(Florence PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    
    set(EMPIRE_GENERATED
        ${CMAKE_SOURCE_DIR}/src/generated/assets_generated.c
        ${CMAKE_SOURCE_DIR}/include/Empire/generated/assets_generated.h
    )
    
    add_custom_target(RunFlorence ALL
        COMMAND Florence
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS Florence
        BYPRODUCTS ${EMPIRE_GENERATED}
        COMMENT "Running Florence codegen..."
    )
endif()

# get all the sloppy files
file(GLOB EMPIRE_SOURCES "src/*.c")
file(GLOB EMPIRE_HEADERS "include/Empire/*.h" "include/Empire/*.inl")

add_executable(Empire 
    ${EMPIRE_SOURCES}
    ${EMPIRE_HEADERS}
    ${EMPIRE_GENERATED}
)

target_include_directories(Empire PRIVATE include)
target_link_libraries(Empire PRIVATE SDL3::SDL3)

# make pretty filters in VS
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    source_group("Sources" FILES ${EMPIRE_SOURCES})
    source_group("Sources\\Generated" REGULAR_EXPRESSION "src/generated/.*\\.c")
    source_group("Headers" FILES ${EMPIRE_HEADERS})
    source_group("Headers\\Generated" REGULAR_EXPRESSION "src/generated/.*\\.h")
endif()

if(NOT EMSCRIPTEN)
    target_link_libraries(Empire PRIVATE glad)
    add_dependencies(Empire RunFlorence)
    set_target_properties(Empire PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Copy SDL3 DLL for Windows builds
if(WIN32 AND NOT EMSCRIPTEN)
    add_custom_command(TARGET Florence POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:Florence>
    )
    add_custom_command(TARGET Empire POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:Empire>
    )
endif()

if(EMSCRIPTEN)
    set_target_properties(Empire PROPERTIES SUFFIX ".html")
    target_link_options(Empire PRIVATE 
        "-sALLOW_MEMORY_GROWTH=1"
        "-sUSE_SDL=3"
        "-g3"                                       # Level 3 debug: Includes source maps and line info
        "-sASSERTIONS=1"                            # Turns on Emscripten's internal runtime checks
        "-sSTACK_OVERFLOW_CHECK=2"                  # Catches stack errors (common in WASM)
        "--source-map-base http://localhost:8080/"  # Helps the browser find the source files)
    )
endif()

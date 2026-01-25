cmake_minimum_required(VERSION 3.16)
project(Empire LANGUAGES C CXX)

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
endif()

add_executable(Empire src/main.c src/generated/assets_generated.c src/util.c)
target_include_directories(Empire PRIVATE include)
target_link_libraries(Empire PRIVATE SDL3::SDL3)

# Run codegen before building Empire
if(NOT EMSCRIPTEN)
    add_custom_target(RunFlorence ALL
        COMMAND $<TARGET_FILE:Florence>
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running Florence codegen..."
        DEPENDS Florence
    )
    add_dependencies(Empire RunFlorence)
endif()

if(NOT EMSCRIPTEN)
    target_link_libraries(Empire PRIVATE glad)
endif()

if(WIN32)
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

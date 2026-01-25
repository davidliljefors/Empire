cmake_minimum_required(VERSION 3.16)
project(Empire LANGUAGES C CXX)

add_subdirectory(soy/SDL)

# gl loader for win/macos
if(NOT EMSCRIPTEN)
    add_library(glad STATIC soy/glad/src/gl.c)
    target_include_directories(glad PUBLIC soy/glad/include)
endif()

add_executable(Empire src/main.c)
target_link_libraries(Empire PRIVATE SDL3::SDL3)

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
    target_link_options(Empire PRIVATE "-sALLOW_MEMORY_GROWTH=1")
endif()

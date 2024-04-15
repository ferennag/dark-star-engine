include(FetchContent)
FetchContent_Declare(SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.2
)
FetchContent_MakeAvailable(SDL2)

FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

find_package(Vulkan REQUIRED)

add_library(dark_star_engine SHARED
        src/engine.h
        src/engine.cpp
        src/application.cpp
        src/application.h
        src/renderer/vulkan.cpp
        src/renderer/vulkan.h
        src/core/file.cpp
        src/core/file.h
)

target_link_libraries(dark_star_engine SDL2::SDL2 Vulkan::Vulkan glm)
target_include_directories(dark_star_engine PUBLIC src)
target_compile_options(dark_star_engine PRIVATE -g -Wall)

function(add_shaders TARGET_NAME)
    set(INPUT_FILES ${ARGN})

    set(SHADER_PRODUCTS)
    list(LENGTH INPUT_FILES FILE_COUNT)

    foreach (INPUT_FILE IN LISTS INPUT_FILES)
        add_custom_command(
                OUTPUT ${INPUT_FILE}.spv
                COMMAND Vulkan::glslc shaders/${INPUT_FILE} -o ${CMAKE_BINARY_DIR}/${INPUT_FILE}.spv
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS shaders/${INPUT_FILE}
                VERBATIM
        )
        list(APPEND SHADER_PRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${INPUT_FILE}.spv")
    endforeach ()

    add_custom_target(${TARGET_NAME} ALL DEPENDS ${SHADER_PRODUCTS})
endfunction()

add_shaders(dark_star_engine_shaders basic.vert basic.frag)
add_dependencies(dark_star_engine dark_star_engine_shaders)
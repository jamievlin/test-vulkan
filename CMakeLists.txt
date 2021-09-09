CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
project(vkTest)
set(CMAKE_CXX_STANDARD 17)

if (GENERATE_DEBUG_SYMBOLS)
    if (WIN32)
        string(APPEND CMAKE_CXX_FLAGS " /Zi")
    else()
        string(APPEND CMAKE_CXX_FLAGS " -Og -g")
    endif()
    set(COMPILE_DEFINITIONS DEBUG)
endif(GENERATE_DEBUG_SYMBOLS)


if (WIN32)
    string(APPEND CMAKE_CXX_FLAGS " /EHsc /W3")
else()
    string(APPEND CMAKE_CXX_FLAGS " -Wall")
endif (WIN32)

file(GLOB SOURCES "*.cc")
file(GLOB HEADERS "*.h")
add_executable(vkTest ${SOURCES} ${HEADERS})

if (ENABLE_VALIDATION_LAYERS)
    list(APPEND COMPILE_DEFINITIONS ENABLE_VALIDATION_LAYERS)
endif(ENABLE_VALIDATION_LAYERS)

set(INCLUDE_DIRS ${GLM_PATH})

find_path(GLM_PATH glm/glm.hpp)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

list(APPEND INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
set(LIBRARIES ${Vulkan_LIBRARIES} glfw)

file(GLOB SHADERS **/*.glsl)
set(GLSLC_FLAGS -O -Werror)
set(GLSLC_TARGETS --target-spv=spv1.5 --target-env=vulkan1.2)

foreach(shaderFile IN LISTS SHADERS)
    get_filename_component(baseShaderName ${shaderFile} NAME_WE)
    get_filename_component(shaderNameExt ${shaderFile} NAME_WLE)
    get_filename_component(shaderStageDot ${shaderNameExt} LAST_EXT)
    string(SUBSTRING ${shaderStageDot} 1 -1 shaderStage)
    set(shaderOutName ${shaderNameExt}.spv)
    list(APPEND SHADER_OUTFILES ${shaderOutName})

    set(GLSLC_CMD glslc
            ${GLSLC_FLAGS} ${GLSLC_TARGETS}
            -fshader-stage=${shaderStage}
            ${shaderFile}
            -o ${shaderOutName})
    add_custom_command(
            OUTPUT ${shaderOutName}
            COMMAND ${GLSLC_CMD}
            MAIN_DEPENDENCY ${shaderFile}
    )
endforeach()

add_custom_target(shaders
        DEPENDS ${SHADER_OUTFILES}
)

add_dependencies(vkTest shaders)
target_link_libraries(vkTest PRIVATE ${LIBRARIES})
target_include_directories(vkTest PUBLIC ${INCLUDE_DIRS})
target_compile_definitions(vkTest PUBLIC ${COMPILE_DEFINITIONS})

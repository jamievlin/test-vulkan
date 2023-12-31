CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
project(vkTest)
set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "src/*.cc")
add_executable(vkTest ${SOURCES})

list(APPEND INCLUDE_DIRS include/)
# For vulkan validation layers
if (ENABLE_VALIDATION_LAYERS)
    list(APPEND COMPILE_DEFINITIONS ENABLE_VALIDATION_LAYERS)
endif(ENABLE_VALIDATION_LAYERS)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(PNG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(PNGPP_PATH "png++/color.hpp")
list(APPEND INCLUDE_DIRS ${PNGPP_PATH})

find_package(
        Boost
        REQUIRED
        COMPONENTS regex
)

include(FetchContent)

FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG master
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)


set(LIBRARIES
        Vulkan::Vulkan
        Boost::boost Boost::regex
        glm::glm
        PNG::PNG
        GPUOpen::VulkanMemoryAllocator
        glfw
)

# Shader compilation
file(GLOB SHADERS **/*.hlsl **/*.glsl)
set(GLSLC_FLAGS -Werror)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    list(APPEND GLSLC_FLAGS -g -O0)
elseif (${CMAKE_BUILD_TYPE} MATCHES Release)
    list(APPEND GLSLC_FLAGS -O)
endif()

set(GLSLC_TARGETS --target-env=vulkan1.1)
set(GLSLC_INCLUDE_PATH shaders/ shaders/include/)

set(GLSLC_FULL_FLAGS ${GLSLC_FLAGS} ${GLSLC_TARGETS})
foreach(inclpath IN LISTS GLSLC_INCLUDE_PATH)
    list(APPEND GLSLC_FULL_FLAGS -I${PROJECT_SOURCE_DIR}/${inclpath} )
endforeach()

foreach(shaderFile IN LISTS SHADERS)
    get_filename_component(baseShaderName ${shaderFile} NAME_WE)
    get_filename_component(shaderNameExt ${shaderFile} NAME_WLE)
    get_filename_component(shaderStageDot ${shaderNameExt} LAST_EXT)
    string(SUBSTRING ${shaderStageDot} 1 -1 shaderStage)
    set(shaderOutName ${shaderNameExt}.spv)
    set(shaderDepFile ${shaderOutName}.d)
    list(APPEND SHADER_OUTFILES ${shaderOutName})

    # using glslc

if(${CMAKE_VERSION} VERSION_GREATER "3.20.0")
    set(GLSLC_CMD glslc
            ${GLSLC_FULL_FLAGS}
            -MD
            -fshader-stage=${shaderStage}
            ${shaderFile}
            -o ${shaderOutName}
            -MF ${shaderDepFile})

    add_custom_command(
            OUTPUT ${shaderOutName}
            COMMAND ${GLSLC_CMD}
            BYPRODUCTS ${shaderDepFile}
            MAIN_DEPENDENCY ${shaderFile}
            DEPFILE ${shaderDepFile}
    )
else()
    message("Shader include dependencies will not be properly updated; please use version >= 3.21.0. ")
    set(GLSLC_CMD glslc
            ${GLSLC_FULL_FLAGS}
            -fshader-stage=${shaderStage}
            ${shaderFile}
            -o ${shaderOutName})

    add_custom_command(
            OUTPUT ${shaderOutName}
            COMMAND ${GLSLC_CMD}
            MAIN_DEPENDENCY ${shaderFile}
    )
endif()

endforeach()

add_custom_target(shaders
        DEPENDS ${SHADER_OUTFILES}
)

add_dependencies(vkTest shaders)
target_link_libraries(vkTest PRIVATE ${LIBRARIES})
target_include_directories(vkTest PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(vkTest PRIVATE ${COMPILE_DEFINITIONS})

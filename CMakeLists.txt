CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
project(vkTest)
set(CMAKE_CXX_STANDARD 17)

# Debugging symbols. g++/clang++ for linux, MSVC for windows
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    if (WIN32)
        string(APPEND CMAKE_CXX_FLAGS " /Zi")
    else()
        string(APPEND CMAKE_CXX_FLAGS " -Og -g")
    endif()
    set(COMPILE_DEFINITIONS DEBUG)
elseif (${CMAKE_BUILD_TYPE} MATCHES Release)
    if (WIN32)
        string(APPEND CMAKE_CXX_FLAGS " /O2")
    else()
        string(APPEND CMAKE_CXX_FLAGS " -O3")
    endif()
    set(COMPILE_DEFINITIONS RELEASE)
endif()

# Other compiler defintions
if (WIN32)
    string(APPEND CMAKE_CXX_FLAGS " /EHsc /W3")
else()
    string(APPEND CMAKE_CXX_FLAGS " -Wall")
endif (WIN32)

file(GLOB SOURCES "src/*.cc")
file(GLOB HEADERS "include/*.h")
add_executable(vkTest ${SOURCES} ${HEADERS})

list(APPEND INCLUDE_DIRS include/)
# For vulkan validation layers
if (ENABLE_VALIDATION_LAYERS)
    list(APPEND COMPILE_DEFINITIONS ENABLE_VALIDATION_LAYERS)
endif(ENABLE_VALIDATION_LAYERS)


find_path(PNGPP_PATH "png++/color.hpp")
find_path(GLM_PATH glm/glm.hpp)
list(APPEND INCLUDE_DIRS ${GLM_PATH} ${PNGPP_PATH})

if (WIN32)
    find_package(unofficial-vulkan-memory-allocator REQUIRED)
endif()

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

if (WIN32)
    find_package(libpng REQUIRED)
endif()

if (UNIX)
    list(APPEND INCLUDE_DIRS "./contrib/vkmemalloc/include")
endif()

find_package(Boost REQUIRED
        COMPONENTS regex)

list(APPEND INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
set(LIBRARIES ${Vulkan_LIBRARIES} ${Boost_LIBRARIES} glfw png)

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
endforeach()

add_custom_target(shaders
        DEPENDS ${SHADER_OUTFILES}
)

add_dependencies(vkTest shaders)
target_link_libraries(vkTest PRIVATE ${LIBRARIES})
target_include_directories(vkTest PUBLIC ${INCLUDE_DIRS})
target_compile_definitions(vkTest PUBLIC ${COMPILE_DEFINITIONS})

CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
project(vkTest)
set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    # what?
else()
    set(CMAKE_CXX_FLAGS "-Wall -Og -g")
endif (WIN32)

file(GLOB SOURCES "*.cc")
file(GLOB HEADERS "*.h")
add_executable(vkTest ${SOURCES} ${HEADERS})

set(COMPILE_DEFINITIONS DEBUG)
if (ENABLE_VALIDATION_LAYERS)
    list(APPEND COMPILE_DEFINITIONS ENABLE_VALIDATION_LAYERS)
endif (ENABLE_VALIDATION_LAYERS)

set(INCLUDE_DIRS ${GLM_PATH})

find_path(GLM_PATH glm/glm.hpp)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

list(APPEND INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
set(LIBRARIES ${Vulkan_LIBRARIES} glfw)

target_link_libraries(vkTest PRIVATE ${LIBRARIES})
target_include_directories(vkTest PUBLIC ${INCLUDE_DIRS})
target_compile_definitions(vkTest PUBLIC ${COMPILE_DEFINITIONS})

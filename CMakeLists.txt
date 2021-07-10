CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
project(vkTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Og -g")

include_directories(".")

set(SOURCES Window.cc vkTest.cc validationTargets.cc dbgCallBacks.cc QueueFamilies.cc)
set(HEADERS Window.h common.h validationTargets.h dbgCallBacks.h QueueFamilies.h)
set(LIBRARIES vulkan glfw)

add_executable(vkTest ${SOURCES} ${HEADERS})
target_link_libraries(vkTest ${LIBRARIES})


add_executable(vkTestDbg ${SOURCES} ${HEADERS})
target_link_libraries(vkTestDbg ${LIBRARIES})
target_compile_definitions(vkTestDbg PUBLIC DEBUG)


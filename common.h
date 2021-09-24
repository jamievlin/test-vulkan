#pragma once

#if defined(_WIN32)
#include <iso646.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <algorithm>
#include <functional>

#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>
#include <string>
#include <cstring>

#include <vk_mem_alloc.h>


using std::optional;
using std::nullopt;

#include "utils.h"
#include "ErrorMessages.h"

#if defined(_WIN32) && defined(DEBUG)
#define PRINT_ERR_MSG(msg) std::cerr << "Error message: " << (msg) << std::endl;
#else
// already printed out in linux
#define PRINT_ERR_MSG(msg)
#endif

#if defined(DEBUG)
#define PRINT_ERROR_LINE() std::cerr << "Runtime error at " << __FILE__ << ":" << __LINE__ << std::endl
#else
#define PRINT_ERROR_LINE()
#endif

#define CHECK_VK_SUCCESS(fn, msg) if ((fn) != VK_SUCCESS) { \
    PRINT_ERROR_LINE();                                     \
    PRINT_ERR_MSG(msg);                                     \
    throw std::runtime_error(msg);                          \
    }

#define USE_HLSL 1
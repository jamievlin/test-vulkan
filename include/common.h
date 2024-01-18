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
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>
#include <string>
#include <cstring>

#include <vk_mem_alloc.h>

using std::nullopt;
using std::optional;

#include "utils.h"
#include "ErrorMessages.h"

#if defined(DEBUG)
#define PRINT_ERR_MSG(msg) std::cerr << "Error message: " << (msg) << std::endl;
#define PRINT_ERROR_LINE() \
    std::cerr << "Runtime error at " << __FILE__ << ":" << __LINE__ << std::endl;
#define PRINT_RET_CODE(err) std::cerr << "Return code: " << (err) << std::endl
#else
#define PRINT_ERR_MSG(msg)
#define PRINT_ERROR_LINE()
#define PRINT_RET_CODE(err)
#endif

#define CHECK_VK_SUCCESS(fn, msg) \
    { \
        VkResult __retcode = (fn); \
        if (__retcode != VK_SUCCESS) \
        { \
            PRINT_ERROR_LINE(); \
            PRINT_ERR_MSG(msg); \
            PRINT_RET_CODE(__retcode); \
            assert(__retcode == VK_SUCCESS); \
        } \
    }

#define USE_HLSL 1
#define VK_API_VERSION VK_API_VERSION_1_2

#define VEC4_ALIGN alignas(sizeof(glm::vec4))

constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

#define SHADOW_MAP_RESOLUTION 1024

#define CAST_UINT32(x) static_cast<uint32_t>(x)

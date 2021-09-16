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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

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


using std::optional;
using std::nullopt;

#include "utils.h"
#include "ErrorMessages.h"

#define CHECK_VK_SUCCESS(fn, msg) if ((fn) != VK_SUCCESS) { throw std::runtime_error(msg); }
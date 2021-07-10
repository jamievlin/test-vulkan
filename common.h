#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <algorithm>

#include <memory>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

using std::optional;
using std::nullopt;

#ifdef DEBUG
#define ENABLE_VALIDATION_LAYERS 1
#else
#define ENABLE_VALIDATION_LAYERS 0
#endif
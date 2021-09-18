//
// Created by Supakorn on 9/4/2021.
//

#pragma once
#include <cstdlib>
#include <vulkan/vulkan.h>
#define CREATE_STRUCT_ZERO(T, name) T name; memset(&name,0,sizeof(T));

// from vulkan website
#ifndef VK_MAKE_API_VERSION
#define VK_MAKE_API_VERSION(variant, major, minor, patch) \
    ((((uint32_t)(variant)) << 29) | (((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))
#endif

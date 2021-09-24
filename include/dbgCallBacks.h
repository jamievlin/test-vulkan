#pragma once

#if defined(ENABLE_VALIDATION_LAYERS)
#include "common.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
        VkDebugUtilsMessageTypeFlagsEXT msgType,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData
        );

#endif
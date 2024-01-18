#if defined(ENABLE_VALIDATION_LAYERS)

#include "dbgCallBacks.h"

VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void* pUserData
)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

#endif

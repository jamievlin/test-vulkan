//
// Created by Supakorn on 9/15/2021.
//

#pragma once
#include <string>

#define CHAR_CONSTEXPR constexpr char const*

namespace ErrorMessages
{
    CHAR_CONSTEXPR FAILED_CREATE_SWAP_CHAIN = "Cannot create swap chain!";
    CHAR_CONSTEXPR FAILED_CREATE_RENDER_PASS = "Cannot create render pass!";
    CHAR_CONSTEXPR FAILED_DRIVER_NOT_SUPPORT_IMGBUFFER = "Driver does not support Image buffer!";

    CHAR_CONSTEXPR FAILED_CREATE_QUEUE_FAMILY = "Cannot create queue family!";

    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_INSTANCE = "Cannot create Vulkan instance.";
    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_SURFACE = "Cannot create surface!";
    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_LOGICAL_DEV = "Cannot create Vulkan logical device.";

}
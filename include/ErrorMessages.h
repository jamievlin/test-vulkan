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

    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_CMD_BUFFER = "Cannot allocate command buffer!";
    CHAR_CONSTEXPR FAILED_CANNOT_BEGIN_CMD_BUFFER = "Cannot begin command buffer!";
    CHAR_CONSTEXPR FAILED_CANNOT_END_CMD_BUFFER = "Cannot end command buffer!";

    CHAR_CONSTEXPR FAILED_CANNOT_SUBMIT_QUEUE = "Cannot submit queue!";
    CHAR_CONSTEXPR FAILED_WAIT_IDLE = "Failed waiting for device idle!";

    CHAR_CONSTEXPR CREATE_GRAPHICS_PIPELINE_FAILED = "Cannot create graphics pipeline!";
    CHAR_CONSTEXPR CREATE_COMMAND_POOL_FAILED = "Cannot create command pool!";
    CHAR_CONSTEXPR CREATE_COMMAND_BUFFERS_FAILED = "Cannot create command buffers!";

    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_DESC_POOL = "Cannot create Descriptor pool!";

    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_ALLOCATOR = "Cannot create allocator!";
    CHAR_CONSTEXPR FAILED_CANNOT_CREATE_IMAGE = "Cannot create image!";

}
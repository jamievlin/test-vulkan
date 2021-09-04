//
// Created by jamie on 7/9/21.
//

#pragma once
#include "common.h"

class QueueFamilies
{
public:
    explicit QueueFamilies(VkPhysicalDevice const& dev, VkSurfaceKHR const& surf);

    [[nodiscard]]
    bool suitable() const;

    optional<uint32_t> graphicsFamily;
    optional<uint32_t> presentationFamily;
};



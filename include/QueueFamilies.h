//
// Created by jamie on 7/9/21.
//

#pragma once
#include "common.h"

class QueueFamilies
{
public:
    QueueFamilies() = default;
    ~QueueFamilies() = default;
    explicit QueueFamilies(VkPhysicalDevice const& dev, VkSurfaceKHR const& surf);

    [[nodiscard]]
    bool suitable() const;

    optional<uint32_t> graphicsFamily;
    optional<uint32_t> presentationFamily;
    optional<uint32_t> transferFamily;

    uint32_t transferQueueFamily();

    std::set<uint32_t> queuesForTransfer();
};

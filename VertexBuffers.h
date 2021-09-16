//
// Created by Supakorn on 9/15/2021.
//

#pragma once
#include "common.h"
#include "Vertex.h"
#include "Buffers.h"

namespace Buffers
{
    template<typename TVertex=Vertex>
    class VertexBuffer : public Buffer
    {
    public:
        VertexBuffer() = default;
        ~VertexBuffer() override = default;

        VertexBuffer(
                VkDevice* dev,
                VkPhysicalDevice const& physicalDev,
                std::vector<TVertex> const& vertices,
                optUint32Set const& usedQueues = nullopt

        ) : Buffer(dev,
                   vertices.size() * sizeof(TVertex),
                   physicalDev,
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   usedQueues)
        {
            // Memory requirements
            CHECK_VK_SUCCESS(copyData(vertices), "Cannot copy data!");
        }

        VertexBuffer(VertexBuffer const&) = delete;

        VertexBuffer& operator=(VertexBuffer const&) = delete;

        VkResult copyData(std::vector<TVertex> const& data)
        {
            void* dat = nullptr;
            auto ret = vkMapMemory(*getLogicalDev(), deviceMemory, 0, sizeof(TVertex) * data.size(), 0, &dat);
            memcpy(dat, data.data(), getSize());

            vkUnmapMemory(*getLogicalDev(), deviceMemory);

            return ret;
        }
    };
}
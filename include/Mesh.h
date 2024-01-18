//
// Created by Supakorn on 9/30/2021.
//

#pragma once
#include "common.h"
#include "Vertex.h"
#include "Buffers.h"
#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>

class Mesh : public AVkGraphicsBase
{
public:
    Buffers::Buffer buf;

    Mesh() = default;
    ~Mesh() = default;

    Mesh(
        VkDevice* logicalDev, VmaAllocator* allocator, VkPhysicalDevice* physDev,
        std::string const& objFile
    );

    [[nodiscard]]
    size_t idxOffset() const;

    [[nodiscard]]
    size_t idxCount() const;

    std::shared_ptr<Buffers::StagingBuffer> stagingBuffer(std::set<uint32_t> const& transferQueues);

    Mesh(Mesh const&) = delete;
    Mesh& operator=(Mesh const&) = delete;

    Mesh(Mesh&& mesh) noexcept;
    Mesh& operator=(Mesh&& mesh) noexcept;

private:
    std::vector<NVertex> verts;
    std::vector<uint32_t> indices;

    VmaAllocator* allocator = nullptr;
    VkPhysicalDevice* physDev = nullptr;
};

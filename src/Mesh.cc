//
// Created by Supakorn on 9/30/2021.
//

#include "Mesh.h"

Mesh::Mesh(VkDevice* logicalDev, VmaAllocator* allocator, VkPhysicalDevice* physDev, std::string const& objFile) : AVkGraphicsBase(logicalDev), allocator(allocator), physDev(physDev)
{
    std::vector<glm::vec3> rawVerts;
    std::vector<glm::vec3> rawNormals;

    std::ifstream objfile(objFile);
    assert(objfile.is_open());

    std::string buffer;

    while(getline(objfile, buffer))
    {
        if (buffer[0] == '#')
        {
            continue;
        }

        std::istringstream sstr(buffer);
        std::string mode;
        sstr >> mode;

        if (mode == "v")
        {
            std::string x, y, z;
            sstr >> x >> y >> z;
            rawVerts.emplace_back(std::stof(x), std::stof(y), std::stof(z));
        }
        else if (mode == "vn")
        {
            std::string x, y, z;
            sstr >> x >> y >> z;
            rawNormals.emplace_back(std::stof(x), std::stof(y), std::stof(z));
        }
        else if (mode == "f")
        {
            std::string tmpindex;
            uint32_t currIndex = CAST_UINT32(verts.size());
            int counter = 0;

            while (sstr >> tmpindex)
            {
                std::vector<std::string> contents;
                boost::split(contents, tmpindex, [](char c) { return c == '/'; });

                glm::vec3 vert = rawVerts[std::stoi(contents[0]) - 1];
                glm::vec3 norm(0.f);
                if (contents.size() >= 3)
                {
                    norm = rawNormals[std::stoi(contents[2]) - 1];
                }
                auto& it = verts.emplace_back();
                it.pos = vert;
                it.normal = norm;
                it.texCoord = {0.f, 0.f};
                counter++;
            }

            indices.push_back(currIndex);
            indices.push_back(currIndex + 1);
            indices.push_back(currIndex + 2);

            if (counter >= 4)
            {
                indices.push_back(currIndex);
                indices.push_back(currIndex + 2);
                indices.push_back(currIndex + 3);
            }
        }
    }
    auto idxSize = indices.size() * sizeof(uint32_t);
    auto vertSize = idxOffset();
    buf = Buffers::Buffer(
            getLogicalDevPtr(), allocator, *physDev, idxSize + vertSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

size_t Mesh::idxOffset() const
{
    return verts.size() * sizeof(NVertex);
}

size_t Mesh::idxCount() const
{
    return indices.size();
}

std::shared_ptr<Buffers::StagingBuffer> Mesh::stagingBuffer(std::set<uint32_t> const& transferQueues)
{
    auto vertSize = idxOffset();
    auto idxSize = indices.size() * sizeof(uint32_t);
    auto stg_ptr = std::make_shared<Buffers::StagingBuffer>(
            getLogicalDevPtr(), allocator, *physDev, vertSize + idxSize,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            transferQueues);

    stg_ptr->loadData({{verts.data(), 0, vertSize}, {indices.data(), vertSize, idxSize}});
    return stg_ptr;
}

Mesh::Mesh(Mesh&& mesh) noexcept:
        AVkGraphicsBase(std::move(mesh)),
        buf(std::move(mesh.buf)),
        verts(std::move(mesh.verts)),
        indices(std::move(mesh.indices)),
        allocator(std::move(mesh.allocator)),
        physDev(std::move(mesh.physDev))
{

}

Mesh& Mesh::operator=(Mesh&& mesh) noexcept
{
    buf = std::move(mesh.buf);
    verts = std::move(mesh.verts);
    indices = std::move(mesh.indices);

    allocator = std::move(mesh.allocator);
    physDev = std::move(mesh.physDev);
    AVkGraphicsBase::operator=(std::move(mesh));
    return *this;
}

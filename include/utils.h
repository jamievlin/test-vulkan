//
// Created by Supakorn on 9/4/2021.
//

#pragma once
#include <cstdlib>
#include <cstring>

#include <vulkan/vulkan.h>
#define CREATE_STRUCT_ZERO(T, name) T name; memset(&name,0,sizeof(T));

// from vulkan website
#ifndef VK_MAKE_API_VERSION
#define VK_MAKE_API_VERSION(variant, major, minor, patch) \
    ((((uint32_t)(variant)) << 29) | (((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))
#endif

#define DELETE_COPY_CONSTRUCTOR(clsname) clsname(clsname const&) = delete;
#define DELETE_COPY_ASSIGNOP(clsname) clsname& operator=(clsname const&) = delete;

#define DISALLOW_COPY(clsname) DELETE_COPY_CONSTRUCTOR(clsname); DELETE_COPY_ASSIGNOP(clsname);


class AVkGraphicsBase
{
public:
    AVkGraphicsBase() = default;
    explicit AVkGraphicsBase(VkDevice* logicalDev) : logicalDev(logicalDev) {}
    virtual ~AVkGraphicsBase() = default;

    AVkGraphicsBase(AVkGraphicsBase const&) = delete;
    AVkGraphicsBase& operator= (AVkGraphicsBase const&) = delete;

    AVkGraphicsBase(AVkGraphicsBase&& obj) noexcept : logicalDev(obj.logicalDev)
    {
        obj.logicalDev = nullptr;
    }

    AVkGraphicsBase& operator=(AVkGraphicsBase&& obj) noexcept
    {
        logicalDev = obj.logicalDev;
        obj.logicalDev = nullptr;

        return *this;
    }

    explicit operator bool() const
    {
        return initialized();
    }
protected:
    [[nodiscard]]
    bool initialized() const
    {
        return logicalDev != nullptr;
    }

    VkDevice& getLogicalDev()
    {
        return *logicalDev;
    }

    [[nodiscard]]
    VkDevice const& getLogicalDev() const
    {
        return *logicalDev;
    }

    VkDevice* getLogicalDevPtr()
    {
        return logicalDev;
    }

    [[nodiscard]]
    VkDevice const* getLogicalDevPtr() const
    {
        return logicalDev;
    }


private:
    VkDevice* logicalDev = nullptr;
};
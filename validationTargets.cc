#if defined(ENABLE_VALIDATION_LAYERS)

#include "validationTargets.h"

bool checkValidationSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::unordered_set<std::string> availLayers;
    for (auto const& layer : availableLayers)
    {
        availLayers.emplace(layer.layerName);
    }

    for (auto const& layerTest : validationLayers)
    {
        if (availLayers.find(std::string(layerTest)) == availLayers.end())
        {
            return false;
        }
#ifdef DEBUG
        std::cerr << "Validation layer " << layerTest << " is available.";
#endif
    }

    return true;
}
#endif

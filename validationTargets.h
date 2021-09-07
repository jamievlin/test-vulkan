#pragma once

#if defined(ENABLE_VALIDATION_LAYERS)
#include "common.h"

std::vector<char const*> const validationLayers = {
        "VK_LAYER_KHRONOS_validation"
        };

bool checkValidationSupport();
#endif
#pragma once
#include "common.h"

std::vector<char const*> const validationLayers = {
        "VK_LAYER_KHRONOS_validation"
        };

bool checkValidationSupport();
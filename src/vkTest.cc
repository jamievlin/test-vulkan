#include "common.h"
#include "Window.h"

#define TITLE "Vulkan"

int main()
{
    Window win1(1920, 1080, TITLE);
    return win1.mainLoop();
}
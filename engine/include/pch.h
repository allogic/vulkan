#ifndef PCH_H
#define PCH_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <math.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <macros.h>
#include <fwd.h>
#include <cfg.h>

#include <mutils.h>
#include <vector2.h>
#include <vector3.h>
#include <vector4.h>
#include <ivector2.h>
#include <ivector3.h>
#include <ivector4.h>
#include <quaternion.h>
#include <matrix4.h>

#include <camera.h>
#include <transform.h>

#include <fsutils.h>
#include <heap.h>
#include <vdb.h>
#include <player.h>
#include <renderer.h>
#include <renderpass.h>
#include <swapchain.h>
#include <vkutils.h>
#include <window.h>

#endif // PCH_H

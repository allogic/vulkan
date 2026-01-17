#include <engine/pch.h>

#define MAX_DELTA_TIME 0.1
#define MAX_PHYSICAL_DEVICES 0x10
#define MAX_QUEUE_FAMILY_PROPERTIES_COUNT 0xFF
#define MAX_EXTENSION_PROPERTIES_COUNT 0x400
#define MAX_SURFACE_FORMAT_COUNT 0xFF
#define MAX_PRESENT_MODE_COUNT 0xFF

static LRESULT window_native_message_proc(HWND window_handle, UINT window_message, WPARAM w_param, LPARAM l_param);

#ifdef BUILD_DEBUG
static VkBool32 window_vulkan_message_proc(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT const *callback_data, void *user_data);
#endif // BUILD_DEBUG

static void window_create_native(void);
static void window_create_instance(void);
static void window_create_surface(void);
static void window_create_device(void);
static void window_create_command_pool(void);

static void window_find_physical_device(void);
static void window_find_physical_device_queue_families(void);

static void window_check_physical_device_extensions(void);
static void window_check_surface_capabilities(void);

static void window_update_surface_capabilities(void);

static void window_destroy_native(void);
static void window_destroy_instance(void);
static void window_destroy_surface(void);
static void window_destroy_device(void);
static void window_destroy_command_pool(void);

window_t g_window = {0};

static char const *s_window_class = "VULKAN_ENGINE_WND_CLASS";

#ifdef BUILD_DEBUG
static char const *s_window_validation_layers[] = {
  "VK_LAYER_KHRONOS_validation",
};
#endif // BUILD_DEBUG

static char const *s_window_layer_extensions[] = {
  "VK_KHR_surface",
  "VK_KHR_win32_surface",
#ifdef BUILD_DEBUG
  "VK_EXT_debug_utils",
#endif // BUILD_DEBUG
};

static char const *s_window_device_extensions[] = {
  "VK_KHR_swapchain",
  "VK_EXT_descriptor_indexing",
};

void window_create(int32_t width, int32_t height, char const *title) {
  g_window = (window_t){
    .window_title = title,
    .window_width = width,
    .window_height = height,
    .primary_queue_index = -1,
    .present_queue_index = -1,
  };

  window_create_native();
  window_create_instance();
  window_create_surface();

  window_find_physical_device();
  window_find_physical_device_queue_families();

  window_check_physical_device_extensions();

  window_create_device();
  window_create_command_pool();

  window_check_surface_capabilities();

  window_update_surface_capabilities();
}
void window_run(void) {
  QueryPerformanceFrequency(&g_window.time_freq);
  QueryPerformanceCounter(&g_window.time_prev);

  while (g_window.is_window_running) {

    g_window.mouse_wheel_delta = 0;

    uint8_t keyboard_key_index = 0;
    uint8_t keyboard_key_count = KEYBOARD_KEY_COUNT;

    while (keyboard_key_index < keyboard_key_count) {

      if (g_window.keyboard_key_states[keyboard_key_index] == KEY_STATE_PRESSED) {
        g_window.keyboard_key_states[keyboard_key_index] = KEY_STATE_DOWN;
      } else if (g_window.keyboard_key_states[keyboard_key_index] == KEY_STATE_RELEASED) {
        g_window.keyboard_key_states[keyboard_key_index] = KEY_STATE_UP;
      }

      keyboard_key_index++;
    }

    uint8_t mouse_key_index = 0;
    uint8_t mouse_key_count = MOUSE_KEY_COUNT;

    while (mouse_key_index < mouse_key_count) {

      if (g_window.mouse_key_states[mouse_key_index] == KEY_STATE_PRESSED) {
        g_window.mouse_key_states[mouse_key_index] = KEY_STATE_DOWN;
      } else if (g_window.mouse_key_states[mouse_key_index] == KEY_STATE_RELEASED) {
        g_window.mouse_key_states[mouse_key_index] = KEY_STATE_UP;
      }

      mouse_key_index++;
    }

    while (PeekMessageA(&g_window.window_message, 0, 0, 0, PM_REMOVE)) {

      TranslateMessage(&g_window.window_message);
      DispatchMessageA(&g_window.window_message);
    }

    QueryPerformanceCounter(&g_window.time_curr);

    double time_freq = (double)g_window.time_freq.QuadPart;
    double time_prev = (double)g_window.time_prev.QuadPart;
    double time_curr = (double)g_window.time_curr.QuadPart;

    double delta_time = (time_curr - time_prev) / time_freq;

    if (delta_time > MAX_DELTA_TIME) {
      delta_time = MAX_DELTA_TIME;
    }

    g_window.delta_time = delta_time;
    g_window.time_prev = g_window.time_curr;

    g_window.time += delta_time;
    g_window.elapsed_time_since_fps_count_update += delta_time;

    g_window.fps_counter++;
    g_window.frame_index++;

    if (g_window.elapsed_time_since_fps_count_update > 1.0) {

      static char title_buffer[0x400] = {0};

      snprintf(title_buffer, sizeof(title_buffer), "%s %s.%s.%s (%s) - FPS: %d",
               g_window.window_title,
               VERSION_MAJOR,
               VERSION_MINOR,
               VERSION_PATCH,
               GIT_VERSION_HASH,
               g_window.fps_counter);

      SetWindowTextA(g_window.window_handle, title_buffer);

      g_window.elapsed_time_since_fps_count_update = 0.0;
      g_window.fps_counter = 0;
    }
  }
}
void window_destroy(void) {
  window_destroy_command_pool();
  window_destroy_device();
  window_destroy_surface();
  window_destroy_instance();
  window_destroy_native();
}

static LRESULT window_native_message_proc(HWND window_handle, UINT window_message, WPARAM w_param, LPARAM l_param) {
  window_t *window = (window_t *)GetWindowLongPtr(window_handle, GWLP_USERDATA);

  switch (window_message) {

    case WM_CREATE: {

      window->is_window_running = 1;

      break;
    }
    case WM_CLOSE: {

      window->is_window_running = 0;

      break;
    }

    case WM_NCCREATE: {

      SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *)l_param)->lpCreateParams);

      return TRUE;
    }
    case WM_NCDESTROY: {

      SetWindowLongPtr(window_handle, GWLP_USERDATA, 0);

      break;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {

      UINT scan_code = MapVirtualKeyA((UINT)w_param, MAPVK_VK_TO_VSC);
      UINT virtual_key = MapVirtualKeyExA(scan_code, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0));

      switch (virtual_key) {
        case KEYBOARD_KEY_LEFT_SHIFT:
          window->keyboard_key_states[KEYBOARD_KEY_LEFT_SHIFT] = ((window->keyboard_key_states[KEYBOARD_KEY_LEFT_SHIFT] == KEY_STATE_UP) || (window->keyboard_key_states[KEYBOARD_KEY_LEFT_SHIFT] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
        case KEYBOARD_KEY_RIGHT_SHIFT:
          window->keyboard_key_states[KEYBOARD_KEY_RIGHT_SHIFT] = ((window->keyboard_key_states[KEYBOARD_KEY_RIGHT_SHIFT] == KEY_STATE_UP) || (window->keyboard_key_states[KEYBOARD_KEY_RIGHT_SHIFT] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
        case KEYBOARD_KEY_LEFT_CONTROL:
          window->keyboard_key_states[KEYBOARD_KEY_LEFT_CONTROL] = ((window->keyboard_key_states[KEYBOARD_KEY_LEFT_CONTROL] == KEY_STATE_UP) || (window->keyboard_key_states[KEYBOARD_KEY_LEFT_CONTROL] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
        case KEYBOARD_KEY_RIGHT_CONTROL:
          window->keyboard_key_states[KEYBOARD_KEY_RIGHT_CONTROL] = ((window->keyboard_key_states[KEYBOARD_KEY_RIGHT_CONTROL] == KEY_STATE_UP) || (window->keyboard_key_states[KEYBOARD_KEY_RIGHT_CONTROL] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
        case KEYBOARD_KEY_LEFT_MENU:
          window->keyboard_key_states[KEYBOARD_KEY_LEFT_MENU] = ((window->keyboard_key_states[KEYBOARD_KEY_LEFT_MENU] == KEY_STATE_UP) || (window->keyboard_key_states[KEYBOARD_KEY_LEFT_MENU] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
        case KEYBOARD_KEY_RIGHT_MENU:
          window->keyboard_key_states[KEYBOARD_KEY_RIGHT_MENU] = ((window->keyboard_key_states[KEYBOARD_KEY_RIGHT_MENU] == KEY_STATE_UP) || (window->keyboard_key_states[KEYBOARD_KEY_RIGHT_MENU] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
        default:
          window->keyboard_key_states[virtual_key] = ((window->keyboard_key_states[virtual_key] == KEY_STATE_UP) || (window->keyboard_key_states[virtual_key] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;
          break;
      }

      break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP: {

      UINT scan_code = MapVirtualKeyA((UINT)w_param, MAPVK_VK_TO_VSC);
      UINT virtual_key = MapVirtualKeyExA(scan_code, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0));

      switch (virtual_key) {
        case KEYBOARD_KEY_LEFT_SHIFT:
          window->keyboard_key_states[KEYBOARD_KEY_LEFT_SHIFT] = ((window->keyboard_key_states[KEYBOARD_KEY_LEFT_SHIFT] == KEY_STATE_DOWN) || (window->keyboard_key_states[KEYBOARD_KEY_LEFT_SHIFT] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
        case KEYBOARD_KEY_RIGHT_SHIFT:
          window->keyboard_key_states[KEYBOARD_KEY_RIGHT_SHIFT] = ((window->keyboard_key_states[KEYBOARD_KEY_RIGHT_SHIFT] == KEY_STATE_DOWN) || (window->keyboard_key_states[KEYBOARD_KEY_RIGHT_SHIFT] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
        case KEYBOARD_KEY_LEFT_CONTROL:
          window->keyboard_key_states[KEYBOARD_KEY_LEFT_CONTROL] = ((window->keyboard_key_states[KEYBOARD_KEY_LEFT_CONTROL] == KEY_STATE_DOWN) || (window->keyboard_key_states[KEYBOARD_KEY_LEFT_CONTROL] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
        case KEYBOARD_KEY_RIGHT_CONTROL:
          window->keyboard_key_states[KEYBOARD_KEY_RIGHT_CONTROL] = ((window->keyboard_key_states[KEYBOARD_KEY_RIGHT_CONTROL] == KEY_STATE_DOWN) || (window->keyboard_key_states[KEYBOARD_KEY_RIGHT_CONTROL] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
        case KEYBOARD_KEY_LEFT_MENU:
          window->keyboard_key_states[KEYBOARD_KEY_LEFT_MENU] = ((window->keyboard_key_states[KEYBOARD_KEY_LEFT_MENU] == KEY_STATE_DOWN) || (window->keyboard_key_states[KEYBOARD_KEY_LEFT_MENU] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
        case KEYBOARD_KEY_RIGHT_MENU:
          window->keyboard_key_states[KEYBOARD_KEY_RIGHT_MENU] = ((window->keyboard_key_states[KEYBOARD_KEY_RIGHT_MENU] == KEY_STATE_DOWN) || (window->keyboard_key_states[KEYBOARD_KEY_RIGHT_MENU] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
        default:
          window->keyboard_key_states[virtual_key] = ((window->keyboard_key_states[virtual_key] == KEY_STATE_DOWN) || (window->keyboard_key_states[virtual_key] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;
          break;
      }

      break;
    }

    case WM_LBUTTONDOWN: {

      window->mouse_key_states[MOUSE_KEY_LEFT] = ((window->mouse_key_states[MOUSE_KEY_LEFT] == KEY_STATE_UP) || (window->mouse_key_states[MOUSE_KEY_LEFT] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;

      break;
    }
    case WM_LBUTTONUP: {

      window->mouse_key_states[MOUSE_KEY_LEFT] = ((window->mouse_key_states[MOUSE_KEY_LEFT] == KEY_STATE_DOWN) || (window->mouse_key_states[MOUSE_KEY_LEFT] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;

      break;
    }
    case WM_MBUTTONDOWN: {

      window->mouse_key_states[MOUSE_KEY_MIDDLE] = ((window->mouse_key_states[MOUSE_KEY_MIDDLE] == KEY_STATE_UP) || (window->mouse_key_states[MOUSE_KEY_MIDDLE] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;

      break;
    }
    case WM_MBUTTONUP: {

      window->mouse_key_states[MOUSE_KEY_MIDDLE] = ((window->mouse_key_states[MOUSE_KEY_MIDDLE] == KEY_STATE_DOWN) || (window->mouse_key_states[MOUSE_KEY_MIDDLE] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;

      break;
    }
    case WM_RBUTTONDOWN: {

      window->mouse_key_states[MOUSE_KEY_RIGHT] = ((window->mouse_key_states[MOUSE_KEY_RIGHT] == KEY_STATE_UP) || (window->mouse_key_states[MOUSE_KEY_RIGHT] == KEY_STATE_RELEASED)) ? KEY_STATE_PRESSED : KEY_STATE_DOWN;

      break;
    }
    case WM_RBUTTONUP: {

      window->mouse_key_states[MOUSE_KEY_RIGHT] = ((window->mouse_key_states[MOUSE_KEY_RIGHT] == KEY_STATE_DOWN) || (window->mouse_key_states[MOUSE_KEY_RIGHT] == KEY_STATE_PRESSED)) ? KEY_STATE_RELEASED : KEY_STATE_UP;

      break;
    }
    case WM_LBUTTONDBLCLK: {

      break;
    }
    case WM_MBUTTONDBLCLK: {

      break;
    }
    case WM_RBUTTONDBLCLK: {

      break;
    }

    case WM_MOUSEMOVE: {

      INT mouse_x = LOWORD(l_param);
      INT mouse_y = HIWORD(l_param);

      window->mouse_position_x = mouse_x;
      window->mouse_position_y = mouse_y;

      break;
    }
    case WM_MOUSEWHEEL: {

      window->mouse_wheel_delta = GET_WHEEL_DELTA_WPARAM(w_param);

      break;
    }

    default: {

      return DefWindowProcA(window_handle, window_message, w_param, l_param);
    }
  }

  return 1;
}

#ifdef BUILD_DEBUG
static VkBool32 window_vulkan_message_proc(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT const *callback_data, void *user_data) {
  printf("%s\n", callback_data->pMessage);

  return 0;
}
#endif // BUILD_DEBUG

static void window_create_native(void) {
  g_window.module_handle = GetModuleHandleA(0);

  WNDCLASSEX window_class_ex = {
    .cbSize = sizeof(WNDCLASSEX),
    .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
    .lpfnWndProc = window_native_message_proc,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = g_window.module_handle,
    .hIcon = LoadIconA(0, IDI_APPLICATION),
    .hCursor = LoadCursorA(0, IDC_ARROW),
    .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
    .lpszMenuName = 0,
    .lpszClassName = s_window_class,
    .hIconSm = LoadIconA(0, IDI_APPLICATION),
  };

  RegisterClassExA(&window_class_ex);

  INT screen_width = GetSystemMetrics(SM_CXSCREEN);
  INT screen_height = GetSystemMetrics(SM_CYSCREEN);
  INT window_position_x = (screen_width - g_window.window_width) / 2;
  INT window_position_y = (screen_height - g_window.window_height) / 2;

  g_window.window_handle = CreateWindowExA(
    0,
    s_window_class, g_window.window_title,
    WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
    window_position_x, window_position_y,
    g_window.window_width, g_window.window_height,
    0,
    0,
    g_window.module_handle,
    &g_window);

  ShowWindow(g_window.window_handle, SW_SHOW);
  UpdateWindow(g_window.window_handle);
}
static void window_create_instance(void) {
  VkApplicationInfo application_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "VULKAN_APPLICATION",
    .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
    .pEngineName = "VULKAN_ENGINE",
    .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
    .apiVersion = VK_API_VERSION_1_1,
  };

#ifdef BUILD_DEBUG
  VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = window_vulkan_message_proc,
  };
#endif // BUILD_DEBUG

  VkInstanceCreateInfo instance_create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &application_info,
    .enabledExtensionCount = ARRAY_COUNT(s_window_layer_extensions),
    .ppEnabledExtensionNames = s_window_layer_extensions,
#ifdef BUILD_DEBUG
    .pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_utils_messenger_create_info,
    .enabledLayerCount = ARRAY_COUNT(s_window_validation_layers),
    .ppEnabledLayerNames = s_window_validation_layers,
#endif // BUILD_DEBUG
  };

  VK_CHECK(vkCreateInstance(&instance_create_info, 0, &g_window.instance));

#ifdef BUILD_DEBUG
  g_window.create_debug_utils_messenger_proc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_window.instance, "vkCreateDebugUtilsMessengerEXT");
  g_window.destroy_debug_utils_messenger_proc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_window.instance, "vkDestroyDebugUtilsMessengerEXT");

  VK_CHECK(g_window.create_debug_utils_messenger_proc(g_window.instance, &debug_utils_messenger_create_info, 0, &g_window.debug_utils_messenger));
#endif // BUILD_DEBUG
}
static void window_create_surface(void) {
  VkWin32SurfaceCreateInfoKHR win32_surface_create_info = {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hwnd = g_window.window_handle,
    .hinstance = g_window.module_handle,
  };

  VK_CHECK(vkCreateWin32SurfaceKHR(g_window.instance, &win32_surface_create_info, 0, &g_window.surface));
}
static void window_create_device(void) {
  float queue_priority = 1.0F;

  VkDeviceQueueCreateInfo device_queue_create_infos[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = g_window.primary_queue_index,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    },
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = g_window.present_queue_index,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    },
  };

  VkPhysicalDeviceDescriptorIndexingFeatures physical_device_descriptor_indexing_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    .pNext = 0,
  };

  VkPhysicalDeviceFeatures2 physical_device_features_2 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &physical_device_descriptor_indexing_features,
  };

  vkGetPhysicalDeviceFeatures2(g_window.physical_device, &physical_device_features_2);

  if (physical_device_descriptor_indexing_features.runtimeDescriptorArray == 0) {
    // TODO: fallback to alternative techniques..
  }

  VkDeviceCreateInfo device_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = ARRAY_COUNT(device_queue_create_infos),
    .pQueueCreateInfos = device_queue_create_infos,
    .pEnabledFeatures = 0,
    .enabledExtensionCount = ARRAY_COUNT(s_window_device_extensions),
    .ppEnabledExtensionNames = s_window_device_extensions,
    .pNext = &physical_device_features_2,
#ifdef BUILD_DEBUG
    .enabledLayerCount = ARRAY_COUNT(s_window_validation_layers),
    .ppEnabledLayerNames = s_window_validation_layers,
#endif // BUILD_DEBUG
  };

  VK_CHECK(vkCreateDevice(g_window.physical_device, &device_create_info, 0, &g_window.device));

  vkGetDeviceQueue(g_window.device, g_window.primary_queue_index, 0, &g_window.primary_queue);
  vkGetDeviceQueue(g_window.device, g_window.present_queue_index, 0, &g_window.present_queue);
}
static void window_create_command_pool(void) {
  VkCommandPoolCreateInfo command_pool_create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = g_window.primary_queue_index,
  };

  VK_CHECK(vkCreateCommandPool(g_window.device, &command_pool_create_info, 0, &g_window.command_pool));
}

static void window_find_physical_device(void) {
  int32_t physical_device_index = 0;
  int32_t physical_device_count = 0;

  static VkPhysicalDevice physical_devices[MAX_PHYSICAL_DEVICES] = {0};

  VK_CHECK(vkEnumeratePhysicalDevices(g_window.instance, &physical_device_count, 0));
  VK_CHECK(vkEnumeratePhysicalDevices(g_window.instance, &physical_device_count, physical_devices));

  while (physical_device_index < physical_device_count) {

    VkPhysicalDevice physical_device = physical_devices[physical_device_index];

    vkGetPhysicalDeviceProperties(physical_device, &g_window.physical_device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &g_window.physical_device_features);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &g_window.physical_device_memory_properties);

    if (g_window.physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {

      if (g_window.physical_device_features.geometryShader &&
          g_window.physical_device_features.samplerAnisotropy) {

        g_window.physical_device = physical_device;

        break;
      }
    }

    physical_device_index++;
  }

#ifdef BUILD_DEBUG
  printf("Selected Physical Device\n");
  printf("  Physical Device Index %d\n", physical_device_index);
  printf("\n");
#endif // BUILD_DEBUG
}
static void window_find_physical_device_queue_families(void) {
  int32_t queue_family_property_index = 0;
  int32_t queue_family_property_count = 0;

  static VkQueueFamilyProperties queue_family_properties[MAX_QUEUE_FAMILY_PROPERTIES_COUNT] = {0};

  vkGetPhysicalDeviceQueueFamilyProperties(g_window.physical_device, &queue_family_property_count, 0);
  vkGetPhysicalDeviceQueueFamilyProperties(g_window.physical_device, &queue_family_property_count, queue_family_properties);

  while (queue_family_property_index < queue_family_property_count) {

    VkQueueFamilyProperties queue_family_property = queue_family_properties[queue_family_property_index];

    VkBool32 graphics_support = queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT;
    VkBool32 compute_support = queue_family_property.queueFlags & VK_QUEUE_COMPUTE_BIT;
    VkBool32 present_support = 0;

    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(g_window.physical_device, (uint32_t)queue_family_property_index, g_window.surface, &present_support));

    if (graphics_support && compute_support && (g_window.primary_queue_index == -1)) {

      g_window.primary_queue_index = queue_family_property_index;

    } else if (present_support && (g_window.present_queue_index == -1)) {

      g_window.present_queue_index = queue_family_property_index;
    }

    if ((g_window.primary_queue_index != -1) && (g_window.present_queue_index != -1)) {

      break;
    }

    queue_family_property_index++;
  }

#ifdef BUILD_DEBUG
  printf("Selected Physical Queues\n");
  printf("  Primary Queue Index %d\n", g_window.primary_queue_index);
  printf("  Present Queue Index %d\n", g_window.present_queue_index);
  printf("\n");
#endif // BUILD_DEBUG
}

static void window_check_physical_device_extensions(void) {
  int32_t available_device_extension_count = 0;

  VkExtensionProperties available_extension_properties[MAX_EXTENSION_PROPERTIES_COUNT] = {0};

  VK_CHECK(vkEnumerateDeviceExtensionProperties(g_window.physical_device, 0, &available_device_extension_count, 0));
  VK_CHECK(vkEnumerateDeviceExtensionProperties(g_window.physical_device, 0, &available_device_extension_count, available_extension_properties));

#ifdef BUILD_DEBUG
  printf("Device Extensions\n");
#endif // BUILD_DEBUG

  int32_t device_extension_index = 0;
  int32_t device_extension_count = ARRAY_COUNT(s_window_device_extensions);

  while (device_extension_index < device_extension_count) {

    int8_t device_extensions_available = 0;

    int32_t available_device_extension_index = 0;

    while (available_device_extension_index < available_device_extension_count) {

      if (strcmp(s_window_device_extensions[device_extension_index], available_extension_properties[available_device_extension_index].extensionName) == 0) {

#ifdef BUILD_DEBUG
        printf("  Found %s\n", s_window_device_extensions[device_extension_index]);
#endif // BUILD_DEBUG

        device_extensions_available = 1;

        break;
      }

      available_device_extension_index++;
    }

    if (device_extensions_available == 0) {

#ifdef BUILD_DEBUG
      printf("  Missing %s\n", s_window_device_extensions[device_extension_index]);
#endif // BUILD_DEBUG

      break;
    }

    device_extension_index++;
  }

#ifdef BUILD_DEBUG
  printf("\n");
#endif // BUILD_DEBUG
}
static void window_check_surface_capabilities(void) {
  int32_t surface_format_index = 0;
  int32_t surface_format_count = 0;

  VkSurfaceFormatKHR surface_formats[MAX_SURFACE_FORMAT_COUNT] = {0};

  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(g_window.physical_device, g_window.surface, &surface_format_count, 0));
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(g_window.physical_device, g_window.surface, &surface_format_count, surface_formats));

  int32_t present_mode_index = 0;
  int32_t present_mode_count = 0;

  VkPresentModeKHR present_modes[MAX_PRESENT_MODE_COUNT] = {0};

  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(g_window.physical_device, g_window.surface, &present_mode_count, 0));
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(g_window.physical_device, g_window.surface, &present_mode_count, present_modes));

  while (surface_format_index < surface_format_count) {

    VkSurfaceFormatKHR surface_format = surface_formats[surface_format_index];

    if ((surface_format.format == VK_FORMAT_B8G8R8A8_UNORM) && (surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {

      g_window.prefered_surface_color_format = surface_format;

      break;
    }

    surface_format_index++;
  }

  g_window.prefered_surface_depth_format = VK_FORMAT_D32_SFLOAT; // TODO: check for supported depth format..

  while (present_mode_index < present_mode_count) {

    VkPresentModeKHR present_mode = present_modes[present_mode_index];

    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {

      g_window.prefered_present_mode = present_mode;

      break;
    }

    present_mode_index++;
  }

#ifdef BUILD_DEBUG
  printf("Prefered Surface Color Format %d\n", g_window.prefered_surface_color_format.format);
  printf("Prefered Surface Depth Format %d\n", g_window.prefered_surface_depth_format);
  printf("Prefered Present Mode %d\n", g_window.prefered_present_mode);
  printf("\n");
#endif // BUILD_DEBUG
}

static void window_update_surface_capabilities(void) {
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_window.physical_device, g_window.surface, &g_window.surface_capabilities));

  g_window.window_width = g_window.surface_capabilities.currentExtent.width;
  g_window.window_height = g_window.surface_capabilities.currentExtent.height;
  g_window.min_image_count = g_window.surface_capabilities.minImageCount;
  g_window.max_image_count = g_window.surface_capabilities.maxImageCount;
  g_window.surface_transform = g_window.surface_capabilities.currentTransform;
}

static void window_destroy_native(void) {
  DestroyWindow(g_window.window_handle);

  UnregisterClassA(s_window_class, g_window.module_handle);
}
static void window_destroy_instance(void) {
#ifdef BUILD_DEBUG
  g_window.destroy_debug_utils_messenger_proc(g_window.instance, g_window.debug_utils_messenger, 0);
#endif // BUILD_DEBUG

  vkDestroyInstance(g_window.instance, 0);
}
static void window_destroy_surface(void) {
  vkDestroySurfaceKHR(g_window.instance, g_window.surface, 0);
}
static void window_destroy_device(void) {
  vkDestroyDevice(g_window.device, 0);
}
static void window_destroy_command_pool(void) {
  vkDestroyCommandPool(g_window.device, g_window.command_pool, 0);
}

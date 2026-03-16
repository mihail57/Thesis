
#include "renderer.h"

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <sstream>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>
#include <optional>
#include <fstream>
#include <cctype>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

#if !defined(_WIN32)
extern char** environ;
#endif

struct RendererState
{        
    VkAllocationCallbacks*   g_Allocator = nullptr;
    VkInstance               g_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice                 g_Device = VK_NULL_HANDLE;
    uint32_t                 g_QueueFamily = (uint32_t)-1;
    VkQueue                  g_Queue = VK_NULL_HANDLE;
    VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
    VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

    ImGui_ImplVulkanH_Window g_MainWindowData;
    uint32_t                 g_MinImageCount = 2;
    bool                     g_SwapChainRebuild = false;
    
    #ifdef _DEBUG
    #define APP_USE_VULKAN_DEBUG_REPORT
    VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
    #endif

    GLFWwindow*              window;
    ImVec4                   clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};



static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static void SetupVulkan(RendererState* state, ImVector<const char*> instance_extensions)
{
    VkResult err;

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(err);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, state->g_Allocator, &state->g_Instance);
        check_vk_result(err);

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(state->g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vkCreateDebugReportCallbackEXT(state->g_Instance, &debug_report_ci, state->g_Allocator, &state->g_DebugReport);
        check_vk_result(err);
#endif
    }

    // Select Physical Device (GPU)
    state->g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(state->g_Instance);
    IM_ASSERT(state->g_PhysicalDevice != VK_NULL_HANDLE);

    // Select graphics queue family
    state->g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(state->g_PhysicalDevice);
    IM_ASSERT(state->g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(state->g_PhysicalDevice, nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(state->g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = state->g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(state->g_PhysicalDevice, &create_info, state->g_Allocator, &state->g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(state->g_Device, state->g_QueueFamily, 0, &state->g_Queue);
    }

    // Create Descriptor Pool
    // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(state->g_Device, &pool_info, state->g_Allocator, &state->g_DescriptorPool);
        check_vk_result(err);
    }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(RendererState* state, VkSurfaceKHR surface, int width, int height)
{
    ImGui_ImplVulkanH_Window* wd = &state->g_MainWindowData;
    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(state->g_PhysicalDevice, state->g_QueueFamily, surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->Surface = surface;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(state->g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(state->g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(state->g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(state->g_Instance, state->g_PhysicalDevice, state->g_Device, wd, state->g_QueueFamily, state->g_Allocator, width, height, state->g_MinImageCount, 0);
}

static void CleanupVulkan(RendererState* state)
{
    vkDestroyDescriptorPool(state->g_Device, state->g_DescriptorPool, state->g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(state->g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(state->g_Instance, state->g_DebugReport, state->g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(state->g_Device, state->g_Allocator);
    vkDestroyInstance(state->g_Instance, state->g_Allocator);
}

static void CleanupVulkanWindow(RendererState* state)
{
    ImGui_ImplVulkanH_DestroyWindow(state->g_Instance, state->g_Device, &state->g_MainWindowData, state->g_Allocator);
    vkDestroySurfaceKHR(state->g_Instance, state->g_MainWindowData.Surface, state->g_Allocator);
}

static void FrameRender(RendererState* state, ImDrawData* draw_data)
{
    ImGui_ImplVulkanH_Window* wd = &state->g_MainWindowData;
    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(state->g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        state->g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(state->g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(state->g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(state->g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(state->g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(RendererState* state)
{
    ImGui_ImplVulkanH_Window* wd = &state->g_MainWindowData;
    if (state->g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(state->g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        state->g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}
    

static float ComputeInitialFontSize(GLFWwindow* window) {
    int window_w = 1280;
    int window_h = 800;
    if (window != nullptr)
        glfwGetWindowSize(window, &window_w, &window_h);

    const float physical_h = static_cast<float>(window_h);
    const float base_size = physical_h / 50.0f; // 800px physical height -> 16px font
    return std::clamp(base_size, 13.0f, 28.0f);
}

static std::optional<std::string> GetEnvironmentVariableValue(const char* name) {
#if defined(_WIN32)
    DWORD size = GetEnvironmentVariableA(name, nullptr, 0);
    if (size == 0)
        return std::nullopt;

    std::string value(size - 1, '\0');
    GetEnvironmentVariableA(name, &value[0], size);
    return value;
#else
    const size_t name_len = std::strlen(name);
    if (environ == nullptr)
        return std::nullopt;

    for (char** env = environ; *env != nullptr; ++env) {
        const char* entry = *env;
        if (std::strncmp(entry, name, name_len) == 0 && entry[name_len] == '=')
            return std::string(entry + name_len + 1);
    }
    return std::nullopt;
#endif
}

static std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static std::optional<bool> TryDetectLinuxThemeFromGtkConfig() {
#if defined(__linux__)
    std::vector<std::filesystem::path> config_roots;
    if (const auto xdg_config_home = GetEnvironmentVariableValue("XDG_CONFIG_HOME"))
        config_roots.emplace_back(*xdg_config_home);
    if (const auto home = GetEnvironmentVariableValue("HOME"))
        config_roots.emplace_back(std::filesystem::path(*home) / ".config");

    for (const auto& root : config_roots) {
        const std::filesystem::path gtk3 = root / "gtk-3.0" / "settings.ini";
        const std::filesystem::path gtk4 = root / "gtk-4.0" / "settings.ini";

        for (const auto& path : {gtk3, gtk4}) {
            std::ifstream in(path);
            if (!in.is_open())
                continue;

            std::string line;
            while (std::getline(in, line)) {
                const std::string lower = ToLowerCopy(line);

                if (lower.find("gtk-application-prefer-dark-theme=1") != std::string::npos)
                    return true;
                if (lower.find("gtk-application-prefer-dark-theme=0") != std::string::npos)
                    return false;

                if (lower.find("gtk-theme-name") != std::string::npos && lower.find("dark") != std::string::npos)
                    return true;
            }
        }
    }
#endif
    return std::nullopt;
}

static std::optional<bool> DetectSystemDarkTheme() {
#if defined(_WIN32)
    DWORD value = 1;
    DWORD value_size = sizeof(value);
    const LSTATUS status = RegGetValueA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        "AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &value_size
    );

    if (status == ERROR_SUCCESS)
        return value == 0;

    return std::nullopt;
#elif defined(__linux__)
    if (const auto gtk_theme_env = GetEnvironmentVariableValue("GTK_THEME")) {
        const std::string lower = ToLowerCopy(*gtk_theme_env);
        if (lower.find("dark") != std::string::npos)
            return true;
        return false;
    }

    return TryDetectLinuxThemeFromGtkConfig();
#else
    return std::nullopt;
#endif
}

static void ApplySystemThemeOrLightFallback() {
    const std::optional<bool> is_dark = DetectSystemDarkTheme();
    if (is_dark.has_value() && *is_dark)
        ImGui::StyleColorsDark();
    else
        ImGui::StyleColorsLight();
}

bool LoadImGuiSystemFontOnce(GLFWwindow* window) {
    static bool loaded = false;
    if (loaded)
        return true;

    ImGuiIO& io = ImGui::GetIO();

    std::vector<std::string> candidates;

#if defined(_WIN32)
    const auto windir_env = GetEnvironmentVariableValue("WINDIR");
    const std::string windir = windir_env.has_value() ? *windir_env : "C:\\Windows";

    candidates = {
        windir + "\\Fonts\\segoeui.ttf",
        windir + "\\Fonts\\arial.ttf",
        windir + "\\Fonts\\tahoma.ttf"
    };
#elif defined(__linux__)
    candidates = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf"
    };
#endif

    const float font_size = ComputeInitialFontSize(window);

    ImFont* loaded_font = nullptr;
    for (const std::string& path : candidates) {
        if (!std::filesystem::exists(path))
            continue;

        loaded_font = io.Fonts->AddFontFromFileTTF(path.c_str(), font_size);
        if (loaded_font != nullptr)
            break;
    }

    if (loaded_font == nullptr)
        loaded_font = io.Fonts->AddFontDefault();

    io.FontDefault = loaded_font;
    loaded = true;
    return loaded_font != nullptr;
}



void SetupImGui(GLFWwindow* window) {
    // Setup Dear ImGui style based on system preference when available.
    ApplySystemThemeOrLightFallback();
    LoadImGuiSystemFontOnce(window);
}


Renderer::Renderer() {
    state = new RendererState();

    glfwSetErrorCallback(glfw_error_callback);
#if defined(__linux__) && defined(GLFW_PLATFORM) && defined(GLFW_PLATFORM_X11)
    // In WSLg/Wayland server-side decorations may be unavailable; X11 usually provides standard frame/buttons.
    if (std::getenv("WSL_DISTRO_NAME") != nullptr || std::getenv("WSL_INTEROP") != nullptr) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }
#endif
    if (!glfwInit())
        state = nullptr;

    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    state->window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Интерактивный вывод типов", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        state = nullptr;
    }

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(state, extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(state->g_Instance, state->window, state->g_Allocator, &surface);
    check_vk_result(err);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(state->window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &state->g_MainWindowData;
    SetupVulkanWindow(state, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForVulkan(state->window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = state->g_Instance;
    init_info.PhysicalDevice = state->g_PhysicalDevice;
    init_info.Device = state->g_Device;
    init_info.QueueFamily = state->g_QueueFamily;
    init_info.Queue = state->g_Queue;
    init_info.PipelineCache = state->g_PipelineCache;
    init_info.DescriptorPool = state->g_DescriptorPool;
    init_info.MinImageCount = state->g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.Allocator = state->g_Allocator;
    init_info.PipelineInfoMain.RenderPass = wd->RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    SetupImGui(state->window);
}

Renderer::Renderer(Renderer&& other) noexcept : state(other.state) {
    other.state = nullptr;
}
Renderer& Renderer::operator=(Renderer&& other) noexcept {
    std::swap(state, other.state);
    return *this;
}

Renderer::~Renderer() {        
    auto err = vkDeviceWaitIdle(state->g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkanWindow(state);
    CleanupVulkan(state);

    glfwDestroyWindow(state->window);
    glfwTerminate();

    delete state;
}
bool Renderer::InitCheck() { return state != nullptr; }
bool Renderer::ShouldQuit() { return glfwWindowShouldClose(state->window); }

bool Renderer::BeforeFrame() {        
    glfwPollEvents();

    // Resize swap chain?
    int fb_width, fb_height;
    glfwGetFramebufferSize(state->window, &fb_width, &fb_height);
    if (fb_width > 0 && fb_height > 0 && (state->g_SwapChainRebuild || state->g_MainWindowData.Width != fb_width || state->g_MainWindowData.Height != fb_height))
    {
        ImGui_ImplVulkan_SetMinImageCount(state->g_MinImageCount);
        ImGui_ImplVulkanH_CreateOrResizeWindow(state->g_Instance, state->g_PhysicalDevice, state->g_Device, &state->g_MainWindowData, state->g_QueueFamily, state->g_Allocator, fb_width, fb_height, state->g_MinImageCount, 0);
        state->g_MainWindowData.FrameIndex = 0;
        state->g_SwapChainRebuild = false;
    }
    if (glfwGetWindowAttrib(state->window, GLFW_ICONIFIED) != 0)
    {
        ImGui_ImplGlfw_Sleep(10);
        return false;
    }

    // Font Reloading Logic. REDO
    // if (g_Style.FontDirty) {
    //     vkDeviceWaitIdle(state->g_Device);
    //     // ReloadFonts(io, main_scale);    
    //     // g_Style.FontDirty = false;
    // }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    return true;
}
void Renderer::AfterFrame() {        
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        state->g_MainWindowData.ClearValue.color.float32[0] = state->clear_color.x * state->clear_color.w;
        state->g_MainWindowData.ClearValue.color.float32[1] = state->clear_color.y * state->clear_color.w;
        state->g_MainWindowData.ClearValue.color.float32[2] = state->clear_color.z * state->clear_color.w;
        state->g_MainWindowData.ClearValue.color.float32[3] = state->clear_color.w;
        FrameRender(state, draw_data);
        FramePresent(state);
    }
}

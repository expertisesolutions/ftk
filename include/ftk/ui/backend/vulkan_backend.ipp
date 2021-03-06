///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_IPP
#define FTK_FTK_UI_BACKEND_VULKAN_IPP

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_core.h>

#include <ftk/ui/backend/vulkan/queues.hpp>
#include <ftk/ui/backend/khr_display_backend.hpp>
#include <ftk/ui/backend/xlib_surface_backend.hpp>

namespace ftk { namespace ui { namespace backend {

std::vector<unsigned int> get_graphics_family (VkPhysicalDevice physical_device)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  std::vector<unsigned int> graphic_families;
    
  unsigned int i = 0;
  for (const auto& queue_family : queue_families)
  {
    if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      graphic_families.push_back(i);
    ++i;
  }
  return graphic_families;
}

std::vector<unsigned int> get_presentation_family (VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  std::vector<unsigned int> highest_priority_presentation_families;
  std::vector<unsigned int> lower_priority_presentation_families;
  unsigned int i = 0;
  for (const auto& queue_family : queue_families)
  {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentSupport);
    if (queue_family.queueCount > 0 && presentSupport)
    {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) // let give priority to non graphic family
        lower_priority_presentation_families.push_back(i);
      else
        highest_priority_presentation_families.push_back(i);
    }
    ++i;
  }

  std::vector<unsigned int> presentation_families (highest_priority_presentation_families.begin()
                                                   , highest_priority_presentation_families.end());
  std::copy (lower_priority_presentation_families.begin()
             , lower_priority_presentation_families.end()
             , std::back_inserter(presentation_families));
  return presentation_families;
}

template <typename Loop>
typename xlib_surface_backend<Loop>::window xlib_surface_backend<Loop>::create_window
   (int width, int height, std::filesystem::path resource_path) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    window w;
    static_cast<typename base::window&>(w) = base::create_window (width, height, resource_path);

    {
      uint32_t count;
      vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
      std::vector<VkExtensionProperties> extensions(count);
      vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
      for (auto & extension : extensions) {
        std::cout << "extension: " << extension.extensionName << std::endl;
      }
    }
    {
      VkApplicationInfo ApplicationInfo;
      std::memset(&ApplicationInfo, 0, sizeof(ApplicationInfo));
      ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      ApplicationInfo.pNext = nullptr;
      ApplicationInfo.pApplicationName = "My Application";
      ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.pEngineName = "My Engine";
      ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

      std::array<const char*, 5> extensions
        (
         {  "VK_KHR_surface"
          , "VK_KHR_xlib_surface"
          , "VK_KHR_get_physical_device_properties2"
          , "VK_EXT_debug_report"
          , "VK_EXT_debug_utils"
        });
  
      VkInstanceCreateInfo cinfo;

      std::memset(&cinfo, 0, sizeof(cinfo));

      std::array<const char*, 1> layers({"VK_LAYER_KHRONOS_validation"});
  
      cinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      cinfo.pNext = NULL;
      cinfo.flags = 0;
      cinfo.pApplicationInfo = &ApplicationInfo;
      cinfo.enabledLayerCount = layers.size();
      cinfo.ppEnabledLayerNames = &layers[0];
      cinfo.enabledExtensionCount = extensions.size();;
      cinfo.ppEnabledExtensionNames = &extensions[0];

      vulkan_error_code r = from_result(vkCreateInstance(&cinfo, NULL, &w.instance));
      if (r != vulkan_error_code::success)
      {
        std::cout << "error? " << static_cast<int>(r) << std::endl;
        std::cout << "error? " << make_error_code(r).message() << std::endl;
        throw std::system_error (make_error_code (r));
      }
      std::cout << "VkCreateInstance OK" << std::endl;

      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, nullptr);

      assert(deviceCount != 0);

      std::cout << "There are " << deviceCount << " physical devices" << std::endl;
      
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, devices.data());

      w.physicalDevice = devices[0];

      VkPhysicalDeviceProperties physical_properties;
      vkGetPhysicalDeviceProperties (w.physicalDevice, &physical_properties);

      std::cout << "Device name "  << physical_properties.deviceName << std::endl;

      {
        std::cout << "get device extensions" << std::endl;
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, NULL);
        std::vector<VkExtensionProperties> properties(count);
        if (!properties.empty())
        {
          vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, &properties[0]);
          for (auto&& property : properties)
          {
            std::cout << "device extension: " << property.extensionName << std::endl;
          }
        }
      }
      
      VkXlibSurfaceCreateInfoKHR info = {};
      std::memset(&info, 0, sizeof(info));
      info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      info.dpy = w.x11_display;
      info.window = w.x11_window;

      r = from_result (vkCreateXlibSurfaceKHR(w.instance, &info, NULL, &w.surface));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
      }

    return w;
}
      
khr_display_backend::window khr_display_backend::create_window(int width, int height
  , std::filesystem::path resource_path) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;
    window w;
    {
      uint32_t count;
      vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
      std::vector<VkExtensionProperties> extensions(count);
      vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
      for (auto & extension : extensions) {
        std::cout << "extension: " << extension.extensionName << std::endl;
      }
    }
    {
      VkApplicationInfo ApplicationInfo;
      std::memset(&ApplicationInfo, 0, sizeof(ApplicationInfo));
      ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      ApplicationInfo.pNext = nullptr;
      ApplicationInfo.pApplicationName = "My Application";
      ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.pEngineName = "My Engine";
      ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

      std::array<const char*, 4> extensions
        (
         {  "VK_KHR_display"
          , "VK_KHR_surface"
          , "VK_EXT_debug_report"
          , "VK_EXT_debug_utils"
        });
      // /* "VK_KHR_get_display_properties2", "VK_KHR_get_physical_device_properties2", "VK_KHR_external_memory_capabilities", "VK_EXT_direct_mode_display", "VK_KHR_get_surface_capabilities2", "VK_KHR_external_fence_capabilities", "VK_KHR_external_semaphore_capabilities", "VK_KHR_device_group_creation", "VK_KHR_surface_protected_capabilities", "VK_EXT_display_surface_counter", */,
  
      VkInstanceCreateInfo cinfo;

      std::memset(&cinfo, 0, sizeof(cinfo));

      std::array<const char*, 1> layers({"VK_LAYER_KHRONOS_validation"});
  
      cinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      cinfo.pNext = NULL;
      cinfo.flags = 0;
      cinfo.pApplicationInfo = &ApplicationInfo;
      cinfo.enabledLayerCount = layers.size();
      cinfo.ppEnabledLayerNames = &layers[0];
      cinfo.enabledExtensionCount = extensions.size();;
      cinfo.ppEnabledExtensionNames = &extensions[0];

      vulkan_error_code r = from_result(vkCreateInstance(&cinfo, NULL, &w.instance));
      if (r != vulkan_error_code::success)
      {
        std::cout << "error? " << static_cast<int>(r) << std::endl;
        std::cout << "error? " << make_error_code(r).message() << std::endl;
        throw std::system_error (make_error_code (r));
      }
      std::cout << "VkCreateInstance OK" << std::endl;

      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, nullptr);

      assert(deviceCount != 0);

      std::cout << "There are " << deviceCount << " physical devices" << std::endl;
      
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, devices.data());

      w.physicalDevice = devices[0];

      {
        std::cout << "get device extensions" << std::endl;
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, NULL);
        std::vector<VkExtensionProperties> properties(count);
        if (!properties.empty())
        {
          vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, &properties[0]);
          for (auto&& property : properties)
          {
            std::cout << "device extension: " << property.extensionName << std::endl;
          }
        }
      }
      
      VkDisplayPropertiesKHR* display_properties = nullptr;
      {
        uint32_t count = 0;
        r = from_result(vkGetPhysicalDeviceDisplayPropertiesKHR (w.physicalDevice, &count, nullptr));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        std::cout << "count " << count << std::endl;

        display_properties = new VkDisplayPropertiesKHR[count];

        r = from_result(vkGetPhysicalDeviceDisplayPropertiesKHR (w.physicalDevice, &count, display_properties));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        for (unsigned int i = 0; i != count; ++i )
        {
          std::cout << "display name " << display_properties[i].displayName << std::endl;
        }
        std::cout << "transform supported " << (int)display_properties[0].supportedTransforms << std::endl;
        
        assert (count != 0);
      }

      VkDisplayModePropertiesKHR* display_mode_properties = nullptr;
      {
        uint32_t count = 0;        
        r = from_result(vkGetDisplayModePropertiesKHR (w.physicalDevice, display_properties[0].display, &count, nullptr));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        display_mode_properties = new VkDisplayModePropertiesKHR[count];
        std::cout << "display modes " << count << std::endl;

        r = from_result(vkGetDisplayModePropertiesKHR (w.physicalDevice, display_properties[0].display, &count, display_mode_properties));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        assert(count != 0);
      }

      VkDisplaySurfaceCreateInfoKHR info = {};

      info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
      info.displayMode = display_mode_properties[0].displayMode;
      info.imageExtent = display_mode_properties[0].parameters.visibleRegion;
      info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      // info.transform = VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
      info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
      
      r = from_result (vkCreateDisplayPlaneSurfaceKHR(w.instance, &info, NULL, &w.surface));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
    }
    return w;
  }
      
template <typename Loop, typename WindowingBase>
typename vulkan_backend<Loop, WindowingBase>::window vulkan_backend<Loop, WindowingBase>::create_window (int width, int height
  , std::filesystem::path resource_path) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    window_base wb = WindowingBase::create_window(width, height, resource_path);

    VkDevice device;
    VkRenderPass renderPass;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkCommandPool commandPool;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    VkFence executionFinishedFence;

    std::cout << "Creating vulkan surface " << std::endl;
    std::vector<unsigned int> graphic_families = get_graphics_family (wb.physicalDevice);
    std::vector<unsigned int> presentation_families = get_presentation_family (wb.physicalDevice, wb.surface);

    std::cout << "graphic families: " << graphic_families.size() << " presentation families " << presentation_families.size()
              << " highest priority " << presentation_families[0] << std::endl;
    vulkan::queues queues;
    {
      std::vector<VkDeviceQueueCreateInfo> queue_info;
      std::unique_ptr<float[]> queue_priorities;

      std::tie (queue_info, queue_priorities)
        = backend::vulkan::queues_create_queue_create_info (wb.physicalDevice, wb.surface);

      VkPhysicalDeviceFeatures deviceFeatures = {};
      deviceFeatures.fragmentStoresAndAtomics = true;

      // std::cout << "creating device" << std::endl;
      
      VkDeviceCreateInfo deviceCInfo = {};
      deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCInfo.pQueueCreateInfos = &queue_info[0];
      deviceCInfo.queueCreateInfoCount = queue_info.size();
      deviceCInfo.pEnabledFeatures = &deviceFeatures;
      std::array<const char*, 4> requiredDeviceExtensions
        ({"VK_KHR_swapchain"
          , VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
          , VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME
          , VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME

          /*, "VK_EXT_external_memory_dma_buf"*/
          /*  , "VK_KHR_external_memory_fd", "VK_KHR_external_memory"*/
        });
      deviceCInfo.enabledExtensionCount = requiredDeviceExtensions.size();
      deviceCInfo.ppEnabledExtensionNames = &requiredDeviceExtensions[0];
  
      auto r = from_result(vkCreateDevice(wb.physicalDevice, &deviceCInfo, nullptr, &device));
      if (r != vulkan_error_code::success)
      {
        std::cout << "failed creating device " << static_cast<int>(r) << std::endl;
        throw std::system_error (make_error_code (r));
      }

      std::cout << "created device" << std::endl;

      auto separated_queues = backend::vulkan::queues_create_queues (device, wb.physicalDevice, wb.surface);
      queues = backend::vulkan::queues {separated_queues[0], separated_queues[1], separated_queues[2]};

      VkSurfaceCapabilitiesKHR capabilities;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(wb.physicalDevice, wb.surface, &capabilities);

      std::cout << "cap w: " << capabilities.currentExtent.width << " h: " << capabilities.currentExtent.height << std::endl;
      
      uint32_t formatCount;
      r = from_result(vkGetPhysicalDeviceSurfaceFormatsKHR(wb.physicalDevice, wb.surface, &formatCount, nullptr));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(wb.physicalDevice, wb.surface, &formatCount, formats.data());
      }
      assert(formatCount != 0);
      uint32_t imageCount = 2;

      VkSwapchainCreateInfoKHR swapInfo = {};
      swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      swapInfo.surface = wb.surface;
      swapInfo.minImageCount = imageCount;
      swapInfo.imageFormat = formats[0].format;//VK_FORMAT_B8G8R8A8_UNORM;//format.format;
      swapInfo.imageColorSpace = formats[0].colorSpace;//VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;//surfaceFormat.colorSpace;
      swapInfo.imageExtent = capabilities.currentExtent;//VkExtent2D {gwa.width, gwa.height};
      swapInfo.imageArrayLayers = 1;
      swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
      swapInfo.clipped = VK_TRUE;
      swapInfo.oldSwapchain = VK_NULL_HANDLE;
      swapInfo.preTransform = capabilities.currentTransform;

      if (!queues.global_shared_families.empty()
          && (queues.global_graphic_families.empty()
              || queues.global_presentation_families.empty()))
      {
        std::cout << "we have a exclusive mode swapchain" << std::endl;
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      }
      else
      {
        std::cout << "we have a concurrent mode swapchain" << std::endl;
        std::vector<uint32_t> indices;
        auto families = backend::vulkan::queues_separate_queue_families (device, wb.physicalDevice, wb.surface);
        for (auto&& index : families[0])
          indices.push_back(index.index);
        for (auto&& index : families[1])
          indices.push_back(index.index);
        for (auto&& index : families[2])
          indices.push_back(index.index);
        swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapInfo.queueFamilyIndexCount = indices.size();
        swapInfo.pQueueFamilyIndices = &indices[0];
        queues.push_back_swapchain ({{families[0].begin(), families[0].end()}
                                     , {families[1].begin(), families[1].end()}
                                     , {families[2].begin(), families[2].end()}});
      }

      r = from_result(vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapChain));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      swapChainImageFormat = formats[0].format;
      swapChainExtent = capabilities.currentExtent;

      std::cout << "swapChainExtent.width " << swapChainExtent.width << " swapChainExtent.height " << swapChainExtent.height << std::endl;
      
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
      std::vector<VkImage> swapChainImages(imageCount);
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
      
      std::vector<VkImageView> swapChainImageViews(imageCount);
      for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create image views!");
        }
    
      }

      VkDeviceMemory depth_stencil_memory;
      VkImage depth_stencil_image;
      VkImageView depth_stencil_image_view;
      auto depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
      {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depth_format; //VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        //imageInfo.flags = 0; // Optional

        auto r = from_result(vkCreateImage(device, &imageInfo, nullptr, &depth_stencil_image));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code(r));


        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, depth_stencil_image, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(wb.physicalDevice, &memProperties);
        uint32_t memory_index;
        for (memory_index = 0; memory_index < memProperties.memoryTypeCount; memory_index++) {
          if ((memRequirements.memoryTypeBits & (1 << memory_index))
              && (memProperties.memoryTypes[memory_index].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            break;
        }
        assert (memory_index != memProperties.memoryTypeCount);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memory_index;

        r = from_result (vkAllocateMemory(device, &allocInfo, nullptr, &depth_stencil_memory));
        if (r != vulkan_error_code::success)
          throw std::system_error(make_error_code(r));

        vkBindImageMemory(device, depth_stencil_image, depth_stencil_memory, 0);
        
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depth_stencil_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depth_format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
       
        r = from_result (vkCreateImageView(device, &viewInfo, nullptr, &depth_stencil_image_view));
        if (r != vulkan_error_code::success)
          throw std::system_error(make_error_code (r));
      }

      VkAttachmentDescription depthAttachment = {};
      depthAttachment.format = depth_format;
      depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depthAttachmentRef = {};
      depthAttachmentRef.attachment = 1;
      depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      
      VkAttachmentDescription colorAttachment = {};
      colorAttachment.format = swapChainImageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VkAttachmentReference colorAttachmentRef = {};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  
      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;
      subpass.pDepthStencilAttachment = &depthAttachmentRef;

      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkSubpassDependency self_subpass_dependency = {};
      self_subpass_dependency.srcSubpass = 0; // self-dependency
      self_subpass_dependency.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      self_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      self_subpass_dependency.dstSubpass = 0; // self-dependency
      self_subpass_dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;//VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
      self_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
      self_subpass_dependency.dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;

      VkSubpassDependency storage_subpass_dependency = {};
      storage_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      storage_subpass_dependency.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      storage_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_HOST_BIT;
      storage_subpass_dependency.dstSubpass = 0;
      storage_subpass_dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      storage_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      storage_subpass_dependency.dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;

      VkSubpassDependency storage_end_subpass_dependency = {};
      storage_end_subpass_dependency.srcSubpass = 0;
      storage_end_subpass_dependency.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      storage_end_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      storage_end_subpass_dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
      storage_end_subpass_dependency.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      storage_end_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_HOST_BIT;
      storage_subpass_dependency.dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;
      
      std::array<VkSubpassDependency, 4> dependencies {dependency, self_subpass_dependency
                                                       , storage_subpass_dependency, storage_end_subpass_dependency};
      
      std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = attachments.size();
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      // renderPassInfo.dependencyCount = 1;
      // renderPassInfo.pDependencies = &dependency;
      renderPassInfo.dependencyCount = dependencies.size();
      renderPassInfo.pDependencies = &dependencies[0];
      
      if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
      }

      swapChainFramebuffers.resize(imageCount);
      for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] =
          {
           swapChainImageViews[i]
           , depth_stencil_image_view
          };
      
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = sizeof(attachments)/sizeof(attachments[0]);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create framebuffer!");
        }
      }

      VkCommandPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.queueFamilyIndex = /**graphicsFamilyIndex*/graphic_families[0];
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional

      if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
      }

      VkFenceCreateInfo fenceInfo = {};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = 0/*VK_FENCE_CREATE_SIGNALED_BIT*/;
      if (vkCreateFence (device, &fenceInfo, nullptr, &executionFinishedFence) != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphores!");
    }

    // VkCommandPool mt_buffer_pool;
    // {
    //   VkCommandPoolCreateInfo alloc_info {};
    //   alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //   alloc_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //   alloc_info.queueFamilyIndex = /**graphicsFamilyIndex*/graphic_families[0];

      
    // }

    window w {{wb}, {}, {{{}/*, graphicsQueue, presentQueue*/, {}, {}
              , swapChainImageFormat, swapChainExtent, device, wb.physicalDevice
              , renderPass, commandPool, &w.shader_loader}}, /**graphicsFamilyIndex*/static_cast<int>(graphic_families[0])
              , swapChainFramebuffers, swapChain
              , executionFinishedFence, {}/*mt_buffer_pool*/, std::move(queues)};
    w.shader_loader = {resource_path / "shader/vulkan", device};
    // for faster loading later
    return w;
}
      
} } }

#endif

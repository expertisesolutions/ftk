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

namespace ftk { namespace ui { namespace backend {

template <typename Loop, typename WindowingBase>
typename vulkan<Loop, WindowingBase>::window vulkan<Loop, WindowingBase>::create_window (int width, int height) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;
    window_base wb = WindowingBase::create_window(width, height);

    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue graphicsQueue, copy_buffer_queue, presentQueue;
    VkRenderPass renderPass;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkCommandPool commandPool;
    std::optional<unsigned int> graphicsFamilyIndex;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    VkFence executionFinishedFence;

    std::cout << "Creating vulkan surface " << std::endl;
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

      std::array<const char*, 14> extensions({"VK_KHR_display", "VK_KHR_surface", "VK_KHR_get_display_properties2", "VK_KHR_get_physical_device_properties2", "VK_KHR_external_memory_capabilities", "VK_EXT_direct_mode_display", "VK_KHR_get_surface_capabilities2", "VK_KHR_external_fence_capabilities", "VK_KHR_external_semaphore_capabilities", "VK_KHR_device_group_creation", "VK_KHR_surface_protected_capabilities", "VK_EXT_display_surface_counter", "VK_EXT_debug_report", "VK_EXT_debug_utils"});
  
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

      vulkan_error_code r = from_result(vkCreateInstance(&cinfo, NULL, &instance));
      if (r != vulkan_error_code::success)
      {
        std::cout << "error? " << static_cast<int>(r) << std::endl;
        std::cout << "error? " << make_error_code(r).message() << std::endl;
        throw std::system_error (make_error_code (r));
      }
      std::cout << "VkCreateInstance OK" << std::endl;

      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

      assert(deviceCount != 0);

      std::cout << "There are " << deviceCount << " physical devices" << std::endl;
      
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

      physicalDevice = devices[0];

      {
        std::cout << "get device extensions" << std::endl;
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties (physicalDevice, NULL, &count, NULL);
        std::vector<VkExtensionProperties> properties(count);
        if (!properties.empty())
        {
          vkEnumerateDeviceExtensionProperties (physicalDevice, NULL, &count, &properties[0]);
          for (auto&& property : properties)
          {
            std::cout << "device extension: " << property.extensionName << std::endl;
          }
        }
      }
      
      VkDisplayPropertiesKHR* display_properties = nullptr;
      {
        uint32_t count = 0;
        r = from_result(vkGetPhysicalDeviceDisplayPropertiesKHR (physicalDevice, &count, nullptr));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        std::cout << "count " << count << std::endl;

        display_properties = new VkDisplayPropertiesKHR[count];

        r = from_result(vkGetPhysicalDeviceDisplayPropertiesKHR (physicalDevice, &count, display_properties));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        for (int i = 0; i != count; ++i )
        {
          std::cout << "display name " << display_properties[i].displayName << std::endl;
        }
        std::cout << "transform supported " << (int)display_properties[0].supportedTransforms << std::endl;
        
        assert (count != 0);
      }

      VkDisplayModePropertiesKHR* display_mode_properties = nullptr;
      {
        uint32_t count = 0;        
        r = from_result(vkGetDisplayModePropertiesKHR (physicalDevice, display_properties[0].display, &count, nullptr));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        display_mode_properties = new VkDisplayModePropertiesKHR[count];
        std::cout << "display modes " << count << std::endl;

        r = from_result(vkGetDisplayModePropertiesKHR (physicalDevice, display_properties[0].display, &count, display_mode_properties));
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
      
      r = from_result (vkCreateDisplayPlaneSurfaceKHR(instance, &info, NULL, &surface));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
      
      // abort();
      // VkXlibSurfaceCreateInfoKHR info = {};
      // std::memset(&info, 0, sizeof(info));
      // info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      // info.dpy = wb.display;
      // info.window = wb.win;

      // r = from_result (vkCreateXlibSurfaceKHR(instance, &info, NULL, &surface));
      // if (r != vulkan_error_code::success)
      //   throw std::system_error (make_error_code (r));

      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

      std::optional<unsigned int> presentationFamilyIndex;
    
      int i = 0;
      for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          graphicsFamilyIndex = i;
        }

        {
          VkBool32 presentSupport = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
          if (queueFamily.queueCount > 0 && presentSupport) {
            presentationFamilyIndex = i;
          }
        }

        if (graphicsFamilyIndex && presentationFamilyIndex)
          break;
        i++;
      }

      float queuePriority = 1.0f;
      std::array<VkDeviceQueueCreateInfo, 2> queueInfo
      ({
        VkDeviceQueueCreateInfo {
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
          , NULL
          , 0
          , *graphicsFamilyIndex
          , 2
          , &queuePriority
        },
        VkDeviceQueueCreateInfo {
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
          , NULL
          , 0
          , *presentationFamilyIndex
          , 1
          , &queuePriority
        }
      });
      //assert(*graphicsFamilyIndex == *presentationFamilyIndex);

      VkPhysicalDeviceFeatures deviceFeatures = {};

      std::cout << "creating device" << std::endl;
      
      VkDeviceCreateInfo deviceCInfo = {};
      deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCInfo.pQueueCreateInfos = &queueInfo[0];
      deviceCInfo.queueCreateInfoCount = queueInfo.size();
      deviceCInfo.pEnabledFeatures = &deviceFeatures;
      std::array<const char*, 4> requiredDeviceExtensions({"VK_KHR_swapchain", "VK_EXT_external_memory_dma_buf"
                                                           , "VK_KHR_external_memory_fd", "VK_KHR_external_memory"});
      deviceCInfo.enabledExtensionCount = requiredDeviceExtensions.size();
      deviceCInfo.ppEnabledExtensionNames = &requiredDeviceExtensions[0];
  
      r = from_result(vkCreateDevice(physicalDevice, &deviceCInfo, nullptr, &device));
      if (r != vulkan_error_code::success)
      {
        std::cout << "failed creating device " << static_cast<int>(r) << std::endl;
        throw std::system_error (make_error_code (r));
      }

      std::cout << "created device" << std::endl;
      
      vkGetDeviceQueue(device, *graphicsFamilyIndex, 0, &graphicsQueue);
      vkGetDeviceQueue(device, *graphicsFamilyIndex, 1, &copy_buffer_queue);
      vkGetDeviceQueue(device, *presentationFamilyIndex, 0, &presentQueue);

      VkSurfaceCapabilitiesKHR capabilities;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

      std::cout << "cap w: " << capabilities.currentExtent.width << " h: " << capabilities.currentExtent.height << std::endl;
      
      uint32_t formatCount;
      r = from_result(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
      }
      assert(formatCount != 0);
      uint32_t imageCount = 2;

      // {
      //   // VkPhysicalDeviceImageDrmFormatModifierInfoEXT imageMod {};
      //   // VkPhysicalDeviceImageFormatInfo2 info2 {};
      //   // info2.sType = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
      //   // //info2.
      //   // info2.format = fotmats[0].format;
      //   // info2.type = VK_IMAGE_TYPE_2D;
      //   // info2.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
      //   // info2.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      //   // info2.flags = 0;
      //   // VkImageFormatProperties2 prop;
      //   // r = from_result(vkGetPhysicalDeviceImageFormatProperties2 (physicalDevice, &info2, &prop));
      //   // std::cout << " image format error " << static_cast<int>(r) << std::endl;
        
      //   VkDrmFormatModifierPropertiesListEXT drmModifiers {};
      //   drmModifiers.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
      //   //drmModifiers.pNext = &drmModifiers;
        
      //   VkFormatProperties2 formatProperties {};
      //   formatProperties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
      //   formatProperties.pNext = &drmModifiers;
        
      //   std::cout << "Going to get format properties" << std::endl;
      //   vkGetPhysicalDeviceFormatProperties2 (physicalDevice, /*formats[0].format*/VK_FORMAT_B8G8R8A8_UNORM, &formatProperties);
      //   // if (r != vulkan_error_code::success)
      //   //   throw std::system_error (make_error_code (r));

      //   std::cout << "There are " << drmModifiers.drmFormatModifierCount << " modifier formats" << std::endl;

      //   std::vector<VkDrmFormatModifierPropertiesEXT> modifiers {drmModifiers.drmFormatModifierCount};
      //   if (!modifiers.empty())
      //   {
      //     drmModifiers.pDrmFormatModifierProperties = &modifiers[0];

      //     vkGetPhysicalDeviceFormatProperties2 (physicalDevice, /*formats[0].format*/VK_FORMAT_B8G8R8A8_UNORM, &formatProperties);
      //     // if (r != vulkan_error_code::success)
      //     //   throw std::system_error (make_error_code (r));

          
      //   }        
      // }
      
      VkSwapchainCreateInfoKHR swapInfo = {};
      swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      swapInfo.surface = surface;
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

      std::vector<uint32_t> indices({*graphicsFamilyIndex, *presentationFamilyIndex});
      swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;//VK_SHARING_MODE_CONCURRENT;
      swapInfo.queueFamilyIndexCount = 2;
      swapInfo.pQueueFamilyIndices = &indices[0];
      swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      // swapInfo.queueFamilyIndexCount = 0;
      // swapInfo.pQueueFamilyIndices = nullptr;


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

      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = 1;
      renderPassInfo.pAttachments = &colorAttachment;
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      renderPassInfo.dependencyCount = 1;
      renderPassInfo.pDependencies = &dependency;
      
      if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
      }

      swapChainFramebuffers.resize(imageCount);
      for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
                                     swapChainImageViews[i]
        };
      
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
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
      poolInfo.queueFamilyIndex = *graphicsFamilyIndex;
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

      if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
      }

      VkFenceCreateInfo fenceInfo = {};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = 0/*VK_FENCE_CREATE_SIGNALED_BIT*/;
      if (vkCreateFence (device, &fenceInfo, nullptr, &executionFinishedFence) != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphores!");
    }

      // {
      //   VkDrmFormatModifierPropertiesListEXT drmModifiers {};
      //   drmModifiers.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
      //   drmModifiers.pNext = NULL;
        
      //   VkFormatProperties2 formatProperties {};
      //   formatProperties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
      //   formatProperties.pNext = &drmModifiers;
        
      //   std::cout << "Going to get format properties" << std::endl;
      //   vkGetPhysicalDeviceFormatProperties2 (physicalDevice, swapChainImageFormat, &formatProperties);
      //   // if (r != vulkan_error_code::success)
      //   //   throw std::system_error (make_error_code (r));

      //   std::cout << "There are " << drmModifiers.drmFormatModifierCount << " modifier formats" << std::endl;

      //   std::vector<VkDrmFormatModifierPropertiesEXT> modifiers {drmModifiers.drmFormatModifierCount};
      //   assert (!modifiers.empty());
      //   if (!modifiers.empty())
      //   {
      //     drmModifiers.pDrmFormatModifierProperties = &modifiers[0];

      //     vkGetPhysicalDeviceFormatProperties2 (physicalDevice, swapChainImageFormat, &formatProperties);
      //     // if (r != vulkan_error_code::success)
      //     //   throw std::system_error (make_error_code (r));

          
      //   }        
      // }
    
    window w {{wb}, {}, {{{}, graphicsQueue, presentQueue, {}, {}
          , swapChainImageFormat, swapChainExtent, device, physicalDevice
              , renderPass, commandPool, &w.shader_loader}}, *graphicsFamilyIndex
              , swapChainFramebuffers, presentQueue, swapChain
              , executionFinishedFence, copy_buffer_queue};
    w.shader_loader = {"../fastdraw/res/shader/vulkan", device};
    // for faster loading later
    w.shader_loader.load(fastdraw::output::vulkan::shader::image_vertex);
    w.shader_loader.load(fastdraw::output::vulkan::shader::image_frag);
    return w;
}
      
} } }

#endif

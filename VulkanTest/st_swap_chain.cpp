#include "st_swap_chain.h"
// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
namespace st {
    StSwapChain::StSwapChain(StDevice &deviceRef, VkExtent2D extent)
        : device{deviceRef}, windowExtent{extent} {
        init();
    }

    StSwapChain::StSwapChain(StDevice &deviceRef, VkExtent2D extent,std::shared_ptr<StSwapChain> previous)
        : device{ deviceRef }, windowExtent{ extent }, oldSwapChain{ previous } {
        init();

        oldSwapChain = nullptr;
    }

    void StSwapChain::init() {
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDepthResources();
        createFramebuffers();
        createSyncObjects();
    }

    StSwapChain::~StSwapChain() {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device.device(), imageView, nullptr);
        }
        swapChainImageViews.clear();
        if (swapChain != nullptr) {
            vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
            swapChain = nullptr;
        }
        for (int i = 0; i < depthImages.size(); i++) {
            vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
            vkDestroyImage(device.device(), depthImages[i], nullptr);
            vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
        }
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
        }
        vkDestroyRenderPass(device.device(), renderPass, nullptr);
        // cleanup synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device.device(), inFlightFences[i], nullptr);
        }
        for (auto& view : binSwapChainImageViews) {
            vkDestroyImageView(device.device(),view,nullptr);
        }
        binSwapChainImageViews.clear();
        for (auto& image : binSwapChainImages) {
            vkDestroyImage(device.device(),image,nullptr);
        }
        binSwapChainImages.clear();
        for (auto& mem : binSwapChainImageMemory) {
            vkFreeMemory(device.device(),mem,nullptr);
        }
        binSwapChainImageMemory.clear();

        //for (auto& view : texIdSwapChainImageViews) {
        //    vkDestroyImageView(device.device(),view,nullptr);
        //}
        //texIdSwapChainImageViews.clear();
        //for (auto& image : texIdSwapChainImages) {
        //    vkDestroyImage(device.device(),image,nullptr);
        //}
        //texIdSwapChainImages.clear();
        //for (auto& mem : texIdSwapChainImageMemory) {
        //    vkFreeMemory(device.device(),mem,nullptr);
        //}
        //texIdSwapChainImageMemory.clear();

    }
    VkResult StSwapChain::acquireNextImage(uint32_t *imageIndex) {
        vkWaitForFences(
            device.device(),
            1,
            &inFlightFences[currentFrame],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());
        VkResult result = vkAcquireNextImageKHR(
            device.device(),
            swapChain,
            std::numeric_limits<uint64_t>::max(),
            imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
            VK_NULL_HANDLE,
            imageIndex);
        return result;
    }
    VkResult StSwapChain::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex) {
        if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[*imageIndex] = inFlightFences[currentFrame];
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
        if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndex;
        auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return result;
    }
    void StSwapChain::createSwapChain() {
        SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device.surface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;      // Optional
            createInfo.pQueueFamilyIndices = nullptr;  // Optional
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE:oldSwapChain->swapChain;
        VkResult res = vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain);
        if ( res != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        // we only specified a minimum number of images in the swap chain, so the implementation is
        // allowed to create a swap chain with more. That's why we'll first query the final number of
        // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
        // retrieve the handles.
        vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());
        
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        binSwapChainImages.resize(imageCount);
        //texIdSwapChainImages.resize(imageCount);
        binSwapChainImageMemory.resize(imageCount);
        //texIdSwapChainImageMemory.resize(imageCount);
        for (int i = 0; i < imageCount; i++) {
            VkImageCreateInfo image{};
            image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image.imageType = VK_IMAGE_TYPE_2D;
            image.format = VK_FORMAT_R32_UINT;
            image.extent.width = swapChainExtent.width;
            image.extent.height = swapChainExtent.height;
            image.extent.depth = 1;
            image.mipLevels = 1;
            image.arrayLayers = 1;
            image.samples = VK_SAMPLE_COUNT_1_BIT;
            image.tiling = VK_IMAGE_TILING_OPTIMAL;
            image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

            VkMemoryAllocateInfo memAlloc{};
            VkMemoryRequirements memReqs;

            vkCreateImage(device.device(), &image, nullptr, &binSwapChainImages[i]);
            vkGetImageMemoryRequirements(device.device(), binSwapChainImages[i], &memReqs);
            memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAlloc.allocationSize = memReqs.size;
            memAlloc.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(device.device(), &memAlloc, nullptr, &binSwapChainImageMemory[i]);
            vkBindImageMemory(device.device(), binSwapChainImages[i], binSwapChainImageMemory[i], 0);
            
            //vkCreateImage(device.device(), &image, nullptr, &texIdSwapChainImages[i]);
            //vkGetImageMemoryRequirements(device.device(), texIdSwapChainImages[i], &memReqs);
            //memAlloc.allocationSize = memReqs.size;
            //memAlloc.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            //vkAllocateMemory(device.device(), &memAlloc, nullptr, &texIdSwapChainImageMemory[i]);
            //vkBindImageMemory(device.device(), texIdSwapChainImages[i], texIdSwapChainImageMemory[i], 0);
        }


    }
    void StSwapChain::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        binSwapChainImageViews.resize(binSwapChainImages.size());
        //texIdSwapChainImageViews.resize(texIdSwapChainImages.size());

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_TRUE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        
        vkCreateSampler(device.device(),&samplerInfo,nullptr,&binSampler);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
            viewInfo.image = binSwapChainImages[i];
            viewInfo.format = VK_FORMAT_R32_UINT;

            if(vkCreateImageView(device.device(),&viewInfo,nullptr,&binSwapChainImageViews[i])!=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = binSampler;
            imageInfo.imageView = binSwapChainImageViews[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            binBindDescriptorInfo.push_back(imageInfo);

            //viewInfo.image = texIdSwapChainImages[i];
            //if(vkCreateImageView(device.device(),&viewInfo,nullptr,&texIdSwapChainImageViews[i])!=
            //    VK_SUCCESS) {
            //    throw std::runtime_error("failed to create texture image view!");
            //}
        }
    }
    void StSwapChain::createRenderPass() {
        
        
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = getSwapChainImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentDescription binAttachment{};
        binAttachment.format = VK_FORMAT_R32_UINT;
        binAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        binAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        binAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        binAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        binAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        binAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        binAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //VkAttachmentDescription texIdAttachment{};
        //texIdAttachment.format = VK_FORMAT_R32_UINT;
        //texIdAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        //texIdAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        //texIdAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        //texIdAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        //texIdAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        //texIdAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //texIdAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference binAttachmentRef = {};
        binAttachmentRef.attachment = 1;
        binAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        //VkAttachmentReference texIdAttachmentRef = {};
        //texIdAttachmentRef.attachment = 2;
        //texIdAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 2;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        std::array<VkAttachmentReference,2> colorAttachmentRefs{colorAttachmentRef,binAttachmentRef};
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = colorAttachmentRefs.size();
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        VkSubpassDependency dependency = {};
        dependency.dstSubpass = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        std::array<VkAttachmentDescription, 3> attachments = {colorAttachment,binAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }
    void StSwapChain::createFramebuffers() {
        swapChainFramebuffers.resize(imageCount());
        for (size_t i = 0; i < imageCount(); i++) {
            std::array<VkImageView, 3> attachments = {swapChainImageViews[i],binSwapChainImageViews[i], depthImageViews[i]};
            VkExtent2D swapChainExtent = getSwapChainExtent();
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(
                device.device(),
                &framebufferInfo,
                nullptr,
                &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
        
    }
    void StSwapChain::createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        swapChainDepthFormat = depthFormat;
        VkExtent2D swapChainExtent = getSwapChainExtent();
        depthImages.resize(imageCount());
        depthImageMemorys.resize(imageCount());
        depthImageViews.resize(imageCount());
        for (int i = 0; i < depthImages.size(); i++) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = swapChainExtent.width;
            imageInfo.extent.height = swapChainExtent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = depthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.flags = 0;
            device.createImageWithInfo(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImages[i],
                depthImageMemorys[i]);
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = depthImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }
    void StSwapChain::createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    VkSurfaceFormatKHR StSwapChain::chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }
    VkPresentModeKHR StSwapChain::chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                std::cout << "Present mode: Mailbox" << std::endl;
                return availablePresentMode;
            }
        }
        // for (const auto &availablePresentMode : availablePresentModes) {
        //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        //     std::cout << "Present mode: Immediate" << std::endl;
        //     return availablePresentMode;
        //   }
        // }
        std::cout << "Present mode: V-Sync" << std::endl;
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D StSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }
    VkFormat StSwapChain::findDepthFormat() {
        return device.findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }
}  // namespace lve
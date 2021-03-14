/*
* Extended sample base class for ray tracing based samples
*
* Copyright (C) 2020-2021 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanRaytracingSample.h"

void VulkanRaytracingSample::enableExtensions()
{
	// Require Vulkan 1.1
	apiVersion = VK_API_VERSION_1_1;

	// Ray tracing related extensions required by this sample
	enabledDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

	// Required by VK_KHR_acceleration_structure
	enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	// Required for VK_KHR_ray_tracing_pipeline
	enabledDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	enabledDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

VulkanRaytracingSample::ScratchBuffer VulkanRaytracingSample::createScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratchBuffer{};
	// Buffer and memory
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &scratchBuffer.handle));
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, scratchBuffer.handle, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &scratchBuffer.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, scratchBuffer.handle, scratchBuffer.memory, 0));
	// Buffer device address
	VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{};
	bufferDeviceAddresInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAddresInfo.buffer = scratchBuffer.handle;
	scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAddresInfo);
	return scratchBuffer;
}

void VulkanRaytracingSample::deleteScratchBuffer(ScratchBuffer& scratchBuffer)
{
	if (scratchBuffer.memory != VK_NULL_HANDLE) {
		vkFreeMemory(vulkanDevice->logicalDevice, scratchBuffer.memory, nullptr);
	}
	if (scratchBuffer.handle != VK_NULL_HANDLE) {
		vkDestroyBuffer(vulkanDevice->logicalDevice, scratchBuffer.handle, nullptr);
	}
}

void VulkanRaytracingSample::createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	// Buffer and memory
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &accelerationStructure.buffer));
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, accelerationStructure.buffer, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &accelerationStructure.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, accelerationStructure.buffer, accelerationStructure.memory, 0));
	// Acceleration structure
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
	accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreate_info.buffer = accelerationStructure.buffer;
	accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
	accelerationStructureCreate_info.type = type;
	vkCreateAccelerationStructureKHR(vulkanDevice->logicalDevice, &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);
	// AS device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.handle;
	accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vulkanDevice->logicalDevice, &accelerationDeviceAddressInfo);
}

void VulkanRaytracingSample::deleteAccelerationStructure(AccelerationStructure& accelerationStructure)
{
	vkFreeMemory(device, accelerationStructure.memory, nullptr);
	vkDestroyBuffer(device, accelerationStructure.buffer, nullptr);
	vkDestroyAccelerationStructureKHR(device, accelerationStructure.handle, nullptr);
}

uint64_t VulkanRaytracingSample::getBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAI);
}

void VulkanRaytracingSample::createStorageImage(StorageImage& image, VkFormat format, VkExtent3D extent)
{
	// Release resources if image is to be recreated
	if (image.image != VK_NULL_HANDLE) {
		vkDestroyImageView(device, image.view, nullptr);
		vkDestroyImage(device, image.image, nullptr);
		vkFreeMemory(device, image.memory, nullptr);
		image = {};
	}
	image.device = device;

	VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = extent;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCI, nullptr, &image.image));

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, image.image, &memReqs);
	VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memReqs.size;
	memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &image.memory));
	VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, image.image, image.memory, 0));

	VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = format;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;
	colorImageView.image = image.image;
	VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &colorImageView, nullptr, &image.view));

	VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vks::tools::setImageLayout(cmdBuffer, image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	vulkanDevice->flushCommandBuffer(cmdBuffer, queue);
}

void VulkanRaytracingSample::deleteStorageImage(StorageImage &image)
{
	vkDestroyImageView(vulkanDevice->logicalDevice, image.view, nullptr);
	vkDestroyImage(vulkanDevice->logicalDevice, image.image, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, image.memory, nullptr);
}

void VulkanRaytracingSample::prepare()
{
	VulkanExampleBase::prepare();
	// Get properties and features
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &rayTracingPipelineProperties;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &accelerationStructureFeatures;
	vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
	// Get the function pointers required for ray tracing
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
}

VkStridedDeviceAddressRegionKHR VulkanRaytracingSample::getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount)
{
	const uint32_t handleSizeAligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
	stridedDeviceAddressRegionKHR.deviceAddress = getBufferDeviceAddress(buffer);
	stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
	stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;
	return stridedDeviceAddressRegionKHR;
}

void VulkanRaytracingSample::createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
{
	// Create buffer to hold all shader handles for the SBT
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		&shaderBindingTable, 
		rayTracingPipelineProperties.shaderGroupHandleSize * handleCount));
	// Get the strided address to be used when dispatching the rays
	shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStridedDeviceAddressRegion(shaderBindingTable.buffer, handleCount);
	// Map persistent 
	shaderBindingTable.map();
}

void VulkanRaytracingSample::StorageImage::destroy()
{
	vkDestroyImageView(device, view, nullptr);
	vkDestroyImage(device, image, nullptr);
	vkFreeMemory(device, memory, nullptr);
}
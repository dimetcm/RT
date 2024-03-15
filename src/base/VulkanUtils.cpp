#include "VulkanUtils.h"

#include <iostream>
#include <fstream>

namespace VulkanUtils
{

const bool errorModeSilent = false;

void FatalExit(const std::string& message, int32_t exitCode)
{
    if (!errorModeSilent)
    {
		MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);
	}
	
    std::cerr << message << "\n";
    exit(exitCode);
}

void FatalExit(const std::string& message, VkResult resultCode)
{
    FatalExit(message, static_cast<int32_t>(resultCode));   
}

std::string VkResultToString(VkResult result)
{
    switch (result)
	{
#define STR(r) case VK_ ##r: return #r
    STR(SUCCESS);
    STR(NOT_READY);
    STR(TIMEOUT);
    STR(EVENT_SET);
    STR(EVENT_RESET);
    STR(INCOMPLETE);
    STR(ERROR_OUT_OF_HOST_MEMORY);
    STR(ERROR_OUT_OF_DEVICE_MEMORY);
    STR(ERROR_INITIALIZATION_FAILED);
    STR(ERROR_DEVICE_LOST);
    STR(ERROR_MEMORY_MAP_FAILED);
    STR(ERROR_LAYER_NOT_PRESENT);
    STR(ERROR_EXTENSION_NOT_PRESENT);
    STR(ERROR_FEATURE_NOT_PRESENT);
    STR(ERROR_INCOMPATIBLE_DRIVER);
    STR(ERROR_TOO_MANY_OBJECTS);
    STR(ERROR_FORMAT_NOT_SUPPORTED);
    STR(ERROR_FRAGMENTED_POOL);
    STR(ERROR_UNKNOWN);
    STR(ERROR_OUT_OF_POOL_MEMORY);
    STR(ERROR_INVALID_EXTERNAL_HANDLE);
    STR(ERROR_FRAGMENTATION);
    STR(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
    STR(PIPELINE_COMPILE_REQUIRED);
    STR(ERROR_SURFACE_LOST_KHR);
    STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    STR(SUBOPTIMAL_KHR);
    STR(ERROR_OUT_OF_DATE_KHR);
    STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    STR(ERROR_VALIDATION_FAILED_EXT);
    STR(ERROR_INVALID_SHADER_NV);
    STR(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
    STR(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
    STR(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
    STR(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
    STR(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
    STR(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);
    STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
    STR(ERROR_NOT_PERMITTED_KHR);
    STR(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
    STR(THREAD_IDLE_KHR);
    STR(THREAD_DONE_KHR);
    STR(OPERATION_DEFERRED_KHR);
    STR(OPERATION_NOT_DEFERRED_KHR);
    STR(ERROR_COMPRESSION_EXHAUSTED_EXT);
    STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
	default: return "Unknown VkResult";
	}
}

std::string VkPhysicalDeviceTypeToString(VkPhysicalDeviceType type)
{
    switch (type)
    {
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
    STR(OTHER);
    STR(INTEGRATED_GPU);
    STR(DISCRETE_GPU);
    STR(VIRTUAL_GPU);
    STR(CPU);
#undef STR
    default: return "UNKNOWN_DEVICE_TYPE";
    }
}

VkPipelineShaderStageCreateInfo CreateShaderStage(VkDevice device, const std::string& fileName, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
    std::ifstream is(fileName.c_str(), std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        size_t size = is.tellg();
        is.seekg(0, std::ios::beg);
        char* shaderCode = new char[size];
        is.read(shaderCode, size);
        is.close();

        assert(size > 0);

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode);

        VK_CHECK_RESULT_MSG(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule),
            "Error: Could not create shader module \"" + fileName + "\"");

        delete[] shaderCode;

        shaderStage.module = shaderModule;
    }
    else
    {
        std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
    }

	shaderStage.pName = "main";
	assert(shaderStage.module != VK_NULL_HANDLE);
	return shaderStage;
}

void DestroyShaderStage(VkDevice device, VkPipelineShaderStageCreateInfo shaderStage)
{
    vkDestroyShaderModule(device, shaderStage.module, nullptr);
}

}
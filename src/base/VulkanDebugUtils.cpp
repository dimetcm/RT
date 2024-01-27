#include "VulkanDebugUtils.h"

namespace VulkanDebugUtils
{
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };

    void Setup(VkInstance instance)
    {
        vkCmdBeginDebugUtilsLabelEXT =reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>
            (vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
        vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>
            (vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
        vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>
            (vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
    }
}
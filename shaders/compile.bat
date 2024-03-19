"%VULKAN_SDK%\bin\glslc.exe" texture.vert -o texture.vert.spv
"%VULKAN_SDK%\bin\glslc.exe" texture.frag -o texture.frag.spv
"%VULKAN_SDK%\bin\glslc.exe" raytracing.comp -o raytracing.comp.spv
pause
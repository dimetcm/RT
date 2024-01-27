file(GLOB BASE_SRC "*.cpp" "*.h")
file(GLOB BASE_HEADERS "*.h")

add_library(base STATIC ${BASE_SRC})
target_link_libraries(base 
    PUBLIC
        ${Vulkan_LIBRARY}
        ${WINLIBS})
target_include_directories(base 
    PUBLIC 
        ${Vulkan_INCLUDE_DIRS})


file(GLOB BASE_SRC "*.cpp" "*.h")
file(GLOB BASE_HEADERS "*.h")

add_library(base STATIC ${BASE_SRC})
target_link_libraries(base ${Vulkan_LIBRARY} ${WINLIBS})

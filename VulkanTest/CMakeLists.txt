# CMakeList.txt : CMake project for VulkanTest, include source and define
# project specific logic here.
#





# Add source to this project's executable.
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable (VulkanTest "VulkanTest.cpp" "VulkanTest.h" "st_window.cpp" "st_app.cpp" "st_pipeline.cpp" "st_device.cpp" "st_swap_chain.cpp" "st_model.cpp" "st_renderer.cpp" "simple_render_system.cpp" "st_camera.cpp" "keyboard_movement_controller.cpp" "st_bsp_loader.cpp" "st_game_object.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VulkanTest PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
find_package(Vulkan REQUIRED)



add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/simple_shader.vert.spv"
    COMMAND $ENV{VK_SDK_PATH}/Bin/glslc.exe "shaders/simple_shader.vert" -o "${CMAKE_CURRENT_BINARY_DIR}/simple_shader.vert.spv"
    DEPENDS "shaders/simple_shader.vert"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Buiding simple vert shader"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/simple_shader.frag.spv"
    COMMAND $ENV{VK_SDK_PATH}/Bin/glslc.exe "shaders/simple_shader.frag" -o "${CMAKE_CURRENT_BINARY_DIR}/simple_shader.frag.spv" 
    DEPENDS "shaders/simple_shader.frag"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Buiding simple frag shader"
)

add_custom_target(FragShader DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/simple_shader.frag.spv")
add_custom_target(VertShader DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/simple_shader.vert.spv")

target_link_libraries(VulkanTest PRIVATE Vulkan::Vulkan ${PROJECT_SOURCE_DIR}/lib/glfw3.lib)

add_dependencies(VulkanTest VertShader FragShader)
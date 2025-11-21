# CMakeList.txt: D3D1_MESA_DEMO 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
# 将源代码添加到此项目的可执行文件（使用 WIN32 子系统以支持 WinMain 入口点）。
add_executable (D3D1_MESA_DEMO WIN32 "D3D1_MESA_DEMO.cpp" "D3D1_MESA_DEMO.h" "KMTInterceptor.h" "KMTInterceptor.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET D3D1_MESA_DEMO PROPERTY CXX_STANDARD 20)
endif()

# 添加包含目录
target_include_directories(D3D1_MESA_DEMO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 链接 Windows D3D10 运行时库
target_link_libraries(D3D1_MESA_DEMO PRIVATE d3d10 d3dcompiler dxgi ${CMAKE_CURRENT_SOURCE_DIR}/lib/detours.lib gdi32.lib user32.lib)

# 设置运行时输出目录
set_target_properties(D3D1_MESA_DEMO PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# TODO: 如有需要，请添加测试并安装目标。

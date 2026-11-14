cmake_minimum_required(VERSION 3.20)

project(market-analyzer C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# GLFW (system-independent, fetched for the project)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# Dear ImGui (C++ core library)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b-docking
)
FetchContent_MakeAvailable(imgui)

# cimgui (C wrapper for Dear ImGui)
FetchContent_Declare(
  cimgui
  GIT_REPOSITORY https://github.com/cimgui/cimgui.git
  GIT_TAG 1.91.9bdock
)
FetchContent_GetProperties(cimgui)
if(NOT cimgui_POPULATED)
  FetchContent_Populate(cimgui)
endif()

# cimgui expects Dear ImGui headers under its imgui/ subdirectory.
if(NOT EXISTS "${cimgui_SOURCE_DIR}/imgui/imgui.h")
  file(CREATE_LINK "${imgui_SOURCE_DIR}" "${cimgui_SOURCE_DIR}/imgui" SYMBOLIC)
endif()

# ImPlot (C++ plotting library for Dear ImGui)
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v0.17
)
FetchContent_MakeAvailable(implot)



find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

add_library(cimgui_lib STATIC
  ${cimgui_SOURCE_DIR}/cimgui.cpp
  ${cimgui_SOURCE_DIR}/cimgui_impl.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
  ${implot_SOURCE_DIR}/implot_demo.cpp
)

target_include_directories(cimgui_lib PUBLIC
  ${cimgui_SOURCE_DIR}
  ${cimgui_SOURCE_DIR}/imgui
  ${imgui_SOURCE_DIR}/backends
  ${implot_SOURCE_DIR}
)

target_compile_definitions(cimgui_lib PUBLIC
  IMGUI_DEFINE_MATH_OPERATORS
)

target_compile_definitions(cimgui_lib PUBLIC
  CIMGUI_USE_GLFW
  CIMGUI_USE_OPENGL3
  IMGUI_IMPL_API=extern\ \"C\"
)

target_link_libraries(cimgui_lib PUBLIC
  glfw
  OpenGL::GL
)

if(UNIX AND NOT APPLE)
  target_link_libraries(cimgui_lib PUBLIC dl pthread m)
endif()

add_library(alphavantage STATIC
  src/alphavantage/alphavantage.c
  src/alphavantage/av_time_series.c
  src/alphavantage/av_search.c
  src/alphavantage/av_market.c
  src/alphavantage/av_fundamentals.c
  src/alphavantage/av_utils.c
)

target_include_directories(alphavantage PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${JSONC_INCLUDE_DIRS}
)

target_link_libraries(alphavantage PUBLIC
  CURL::libcurl
  ${JSONC_LIBRARIES}
)

add_executable(market-analyzer
  src/main.c
  src/app/app.c
  src/windows/stock_selector.c
  src/windows/stock_data.c
  src/plot/plot_c.cpp
)

target_include_directories(market-analyzer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(market-analyzer PRIVATE
  cimgui_lib
  alphavantage
)

if(MSVC)
  target_compile_options(market-analyzer PRIVATE /W4)
else()
  target_compile_options(market-analyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_target(run
  COMMAND $<TARGET_FILE:market-analyzer>
  DEPENDS market-analyzer
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL
)

cmake_minimum_required(VERSION 3.20)
project(SPSC-Queue)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PARENT_DIR)

target_include_directories(${PROJECT_NAME} PRIVATE ${PARENT_DIR}/include)

include(${PARENT_DIR}/cmake/CompilerWarnings.cmake)
include(${PARENT_DIR}/cmake/Sanitizers.cmake)

myproject_set_project_warnings(${PROJECT_NAME} TRUE "X" "" "" "X")

add_compile_options(-pipe -fPIC)
myproject_enable_sanitizers(${PROJECT_NAME} TRUE TRUE TRUE FALSE TRUE)

# ------------------------------
# Install library
install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

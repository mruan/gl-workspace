cmake_minimum_required (VERSION 2.6)
project (GL_workspace)

set(CMAKE_CXX_FLAGS "-g -Wall")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)

find_package(PCL 1.3 REQUIRED COMPONENTS common io)

# Compile external dependencies
add_subdirectory(external)

include_directories(
  ${PCL_INCLUDE_DIRS}
  /usr/local/include/assimp/
  /usr/local/include/glfx/
  external/glfw-2.7.6/include/
  external/glm-0.9.4.0/
  external/glew-1.9.0/include/
  .)

link_directories(
  ${PCL_LIBRARY_DIRS}
  /usr/local/lib/
  )

add_definitions(${PCL_DEFINITIONS})

set(ALL_LIBS
  ${OPENGL_LIBRARY}
  ${GLUT_LIBRARY}
  ${OpenCV_LIBS}
  GLFW_276
  GLEW_190
)

add_definitions(
  -DTW_STATIC
  -DTW_NO_LIB_PRAGMA
  -DTW_NO_DIRECT3D
  -DGLEW_STATIC
  -D_CRT_SECURE_NO_WARNINGS)

add_subdirectory(src)

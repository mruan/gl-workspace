
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glfw.h>

#include "mesh.hpp"

#define MESH_FILE "/home/ming/Work/OpenGL/Workspace/base_test.dae"

int main(int argc, char* argv[])
{
  // Initialise GLFW
  if( !glfwInit() )
    {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return -1;
    }

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  Mesh andy;
  andy.LoadMesh(MESH_FILE);

  //  std::vector<
    //  andyUpdateBoneTransform(0, 

  return 0;
}

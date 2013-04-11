#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <GL/glew.h>

#include <GL/glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/objloader.hpp>

#include "camera.hpp"
#include "buffer_util.hpp"
#include "mesh.hpp"

#define WIN_HEIGHT 240
#define WIN_WIDTH  320

//#define TEXTURE_FILE "resource/uvmap.DDS"
#define TEXTURE_FILE "resource/male_base_uv.tga"
//#define OBJ_FILE     "/home/ming/Dropbox/base.dae"
#define OBJ_FILE      "resource/base.dae"
//#define OBJ_FILE     "resource/boblampclean.md5mesh"
#define BVH_FILE     "/home/ming/Dropbox/01_14t.bvh"

#define VERTEX_SHADER   "resource/TransVtxBone.shader"
#define FRAGMENT_SHADER "resource/TextFrag.shader"

glm::vec3 initPosition(0.0f, 0.0f, 10.0f);
glm::vec3 initTarget(0.0f, 0.0f, 0.0f);

Camera view_port(WIN_HEIGHT, WIN_WIDTH,
		 60.0f, 4.0f/3.0f,
		 0.3f, 3.0f,
		 initPosition, initTarget);

int initGLstuff(const char* win_title);
void checkSaveImage();

int main(void)
{
  //  int status;
  if (initGLstuff("Main") !=1)
    return -1;
  
  // Create and compile our GLSL program from the shaders
  GLuint programID = LoadShaders(VERTEX_SHADER, FRAGMENT_SHADER );
  
  // Get a handle for our "MVP" uniform
  GLuint MatrixID = glGetUniformLocation(programID, "MVP");
  
  if (MatrixID == INVALID_OGL_VALUE)
    {
      printf("Warning, unable to get the lacation of MVP\n");
    }

  // Load the texture
  // GLuint Texture = loadDDS(TEXTURE_FILE);
  // GLuint Texture = loadBMP_custom(TEXTURE_FILE);
  GLuint Texture = loadTGA_glfw(TEXTURE_FILE);

  // Get a handle for our "myTextureSampler" uniform
  GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
  
  Mesh andy;
  andy.LoadMesh(OBJ_FILE, BVH_FILE);
  andy.BindBones(programID);

  printf("\nStart the main loop\n");

  double startTime = glfwGetTime();

  do{
    
    //  checkSaveImage();

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
    // Use our shader
    glUseProgram(programID);

    // Compute the MVP matrix from keyboard and mouse input
    view_port.updateViewFromInput();
    glm::mat4 ModelMatrix = andy.GetGlobalTfInv();
    glm::mat4 MVP = view_port.getMVP(ModelMatrix);
		
    // Send our transformation to the currently bound shader, 
    // in the "MVP" uniform
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(TextureID, 0);
    
    double currentTime = glfwGetTime();
    float runningTime = (float) (currentTime - startTime);
    
    andy.BoneTransform(runningTime);
    andy.RenderModel();

    // Swap buffers
    glfwSwapBuffers();
    
  } // Check if the ESC key was pressed or the window was closed
  while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
	 glfwGetWindowParam( GLFW_OPENED ) );

  // Cleanup VBO and shader
  //  glDeleteBuffers(1, &vertexbuffer);
  //  glDeleteBuffers(1, &uvbuffer);
  glDeleteProgram(programID);
  glDeleteTextures(1, &TextureID);
  //  glDeleteVertexArrays(1, &VertexArrayID);
	
  // Close OpenGL window and terminate GLFW
  glfwTerminate();
  
  return 0;  
}









// Call this before clear screen or after rendering
void checkSaveImage()
{
  static int gCount =1;
  // Save one image per stroke
  static bool save_buffer_flag = true;
  if (glfwGetKey('S') == GLFW_PRESS)
    {
	if (save_buffer_flag)
	  {
	    bu::saveColorBuffer(gCount);
	    bu::saveDepthBufferToPCD(gCount);
	    ++gCount;
	    save_buffer_flag = false;
	  }
	return;
    }
  else
    save_buffer_flag = true;
  return;
}

int initGLstuff(const char* win_title)
{
  // Initialise GLFW
  if( !glfwInit() )
    {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return -1;
    }
  
  glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  // Open a window and create its OpenGL context
  if( !glfwOpenWindow(WIN_WIDTH, WIN_HEIGHT, 0,0,0,0, 32,0, GLFW_WINDOW ) )
    {
      fprintf( stderr, "Failed to open GLFW window\n");
      fprintf( stderr, "GPU is not 3.3 compatible. Try previous version of GL and GLSL \n" );
      glfwTerminate();
      return -1;
    }
  
  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }
  
  glfwSetWindowTitle(win_title );

  // Ensure we can capture the escape key being pressed below
  glfwEnable( GLFW_STICKY_KEYS );
  glfwSetMousePos(WIN_WIDTH/2, WIN_HEIGHT/2);
  
  // Dark blue background
  glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
  
  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS); 

  // Cull triangles which normal is not towards the camera
  glEnable(GL_CULL_FACE);

  return 1;
}

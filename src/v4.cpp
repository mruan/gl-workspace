#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <unistd.h> // for usleep

#include <GL/glew.h>
#include <GL/glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

#include "shader.hpp"
#include "camera.hpp"
#include "buffer_util.hpp"
#include "mesh.hpp"
#include "common/texture.hpp"

#define WIN_WIDTH   800
#define WIN_HEIGHT  600

#define TEXTURE_FILE "/home/ming/Work/OpenGL/Workspace/walk_textured.tga"
#define OBJ_FILE     "/home/ming/Work/OpenGL/Workspace/dae/walk_textured.dae"
#define SKELETON_BVH "/home/ming/Work/OpenGL/Workspace/bvh/bvh_male1.bvh"

#define ANIMATION_BVH "/home/ming/Work/OpenGL/Workspace/bvh/03_01t.bvh"

#define SHADER_FILE   "/home/ming/Work/OpenGL/Workspace/opengl-workspace/resource/shader.glsl"

glm::vec3 initPosition(10.0f, 0.0f, 0.0f);
glm::vec3 initTarget(0.0f, 0.0f, 0.0f);
glm::vec3 initUp(0.0f, 0.0f, 1.0);

Camera view_port(WIN_HEIGHT, WIN_WIDTH,
		90.0f, 4.0f/3.0f,
		0.3f, 10.0f,
		initPosition, initUp, initTarget);

int initGLstuff(const char* win_title);

bool updateframe(void)
{
	static bool space_flag = true;
	if (glfwGetKey(GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		if(!space_flag)
		{
			space_flag = true;
		}
		else
			return false;
	}
	else
		space_flag = false;

	return space_flag;
}

int main(void)
{
	//  int opengl stuff;
	if (initGLstuff("Main") !=1)
		return -1;

	// Load shader
	Shader shader(SHADER_FILE);
	if (!shader.Init())
	{
		printf("Error initializing the shading technique\n");
		return -1;
	}
	else
		shader.Enable();

	// Load texture
	GLuint Texture = loadTGA_glfw(TEXTURE_FILE);
	shader.SetColorTextureUnit(Texture);

	// Load Model
	Mesh andy;
	andy.LoadColladaMesh(OBJ_FILE);

	// Load animation from BVH
	BvhLoader animationLoader;
	std::vector<unsigned int> bone2ch;
	printf("Load the driving source for the rig\n");
	animationLoader.LoadBvhMotion(ANIMATION_BVH, andy.GetSkeleton(), bone2ch, 1);
	const BvhAnimation& anim = animationLoader.getAnimation();

	printf("\nStart the main loop\n");
	unsigned int frameIdx = 0;
	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Enable shader program
		shader.Enable();

		// Fill the transform with the latest animation
		if(updateframe())
		{
			printf("\n[[[frame=%u]]]\n", frameIdx);
//			andy.UpdateBoneWithTr(anim.GetRootTraj(frameIdx),anim.GetFrame(frameIdx),bone2ch);
			andy.UpdateBoneTfDisc(frameIdx);
			frameIdx++;
		}
		// Set the bone transform so that the shader knows how to render
		for (uint i = 0 ; i < andy.GetNumTfs() ; i++)
			shader.SetBoneTransform(i, andy.GetBoneTf(i));

		// Compute the MVP matrix from keyboard and mouse input
		view_port.updateViewFromInput();
		glm::mat4 WorldMatrix = glm::mat4(1.0f);
		glm::mat4 WVP = view_port.getMVP(WorldMatrix);
		shader.SetWVP(WVP);

		// Render Model
		andy.RenderModel();

		// Swap buffers
		glfwSwapBuffers();

		//    break;
	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
			glfwGetWindowParam( GLFW_OPENED ) );

	// Cleanup VBO and shader
	//  glDeleteBuffers(1, &vertexbuffer);
	//  glDeleteBuffers(1, &uvbuffer);
	//  glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
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

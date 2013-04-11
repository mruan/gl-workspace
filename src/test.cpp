#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "BvhLoader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void ptrRefTest(int*& ap, int*& bp)
{
  int* cp = ap;
  ap = bp;
  bp = cp;
}

void fileio()
{
  //  double num;
  char* buffer = new char[32];
  //  assert(argc ==2);
  fscanf(stdin, " %s", buffer);
  printf("1st string is: %s (end)\n", buffer);
  fscanf(stdin, " %s", buffer);
  printf("2nd string is: %s (end)\n", buffer);
  //  size_t n;
  //  getline(&buffer, &n, stdin);
  //  printf("then: %s (end)\n", buffer);

  delete buffer;
}

#define SKELETON_PATH "/home/ming/Work/OpenGL/Workspace/bvh/bvh_male1.bvh"
#define MOTION_PATH "/home/ming/Work/OpenGL/Workspace/bvh/03_01.bvh"

#define PI_PER_DEG 3.1415927f/180.0f
int main()
{

	glm::mat4 RotMat  = glm::mat4(1.0f);
	RotMat = glm::rotate(RotMat, 90.0f, glm::vec3(0.0f, 0.0f, 1.0f));

	for(int i=0; i< 4; i++)
	{
		for (int j=0; j< 4; j++)
			printf("%8.5f ", RotMat[j][i]);

		printf("\n");
	}
}
/*
int main(int argc, char* argv[])
{
  BvhLoader skel_loader;
  BvhLoader anim_loader;

  std::vector<unsigned int> dict;
  printf("Loading the rig\n");
  skel_loader.LoadBvhSkeleton(SKELETON_PATH);

  printf("Loading the driving bvh\n");
  anim_loader.LoadBvhMotion(MOTION_PATH, skel_loader.getSkeleton(), dict, 8);
  return 0;
}
*/
/*
int main(int argc, char* argv[])
{
  int a = 0, b=1;
  int* ap= &a;
  int* bp= &b;

  printf("Before swap: %d %d\n", *ap, *bp);
  ptrRefTest(ap, bp);
  printf("After swap: %d %d\n", *ap, *bp);
  return 0;
}
*/

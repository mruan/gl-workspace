
#include <Importer.hpp>
#include <scene.h>
#include <postprocess.h>
#include <quaternion.h>
#include <stdio.h>

#define ANIM_FILE "/home/ming/Dropbox/01_14t.bvh"

int main()
{
  /*
  const aiScene* pScene;

  Assimp::Importer ip;

  pScene = ip.ReadFile(ANIM_FILE, aiProcess_ValidateDataStructure);

  if (pScene == NULL)
    {
      printf("Error parsing '%s': '%s'\n", ANIM_FILE, ip.GetErrorString());
      return -1;
    }

  // check the animation
  unsigned int nAnim = pScene->mNumAnimations;
  
  printf("%d animations loaded\n", nAnim);
  for(unsigned int i=0; i< nAnim; i++)
    {
      aiAnimation* pAnim = pScene->mAnimations[i];

      printf("Name: %s\n", pAnim->mName.C_Str());
      printf("DU: %lf sec, %d bone channels, %d mesh channels, TPC=%lf\n", 
	     pAnim->mDuration, pAnim->mNumChannels, pAnim->mNumMeshChannels, pAnim->mTicksPerSecond);

      for (unsigned int j = 0 ; j < pAnim->mNumChannels ; j++) 
	{
	  const aiNodeAnim* pNodeAnim = pAnim->mChannels[j];
	  
	  printf("%-2d NodeName=%s\n", j+1, pNodeAnim->mNodeName.data);
	}
    }
  */



  aiMatrix4x4 m1;
  aiMatrix3x3 m2;
  aiQuaterniont<float> q(90.0f, 0.0f, 0.0f);

  printf("Q: %f %f %f %f\n", q.w, q.x, q.y, q.z);

  m2 = q.GetMatrix();
  printf("%4f %4f %4f\n%4f %4f %4f\n %4f %4f %4f\n", m2.a1, m2.a2, m2.a3, m2.b1, m2.b2, m2.b3, m2.c1, m2.c2, m2.c3);

  printf("%f %f %f %f\n", m1.a1, m1.a2, m1.a3, m1.a4);
  printf("%f %f %f %f\n", m1.b1, m1.b2, m1.b3, m1.b4);
  printf("%f %f %f %f\n", m1.c1, m1.c2, m1.c3, m1.c4);
  printf("%f %f %f %f\n", m1.d1, m1.d2, m1.d3, m1.d4);


  return 0;
}

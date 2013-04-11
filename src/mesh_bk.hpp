#ifndef MESH_H
#define MESH_H

#include <string.h>
#include <vector>
#include <map>
#include <GL/glew.h>

#include <glm/glm.hpp>

// Assimp libraries
#include <Importer.hpp>
#include <matrix4x4.h>
#include <scene.h>
#include <postprocess.h>

#define INVALID_MATERIAL  0xFFFFFFFF
#ifndef INVALID_OGL_VALUE
#define INVALID_OGL_VALUE 0xFFFFFFFF
#endif
#define NUM_BONES_PER_VERTEX 4

typedef unsigned int uint;
class Mesh
{
public:

  Mesh();

  ~Mesh();

  bool LoadMesh(const char* path);
  void BindBones(GLuint programID);

  void RenderModel();

  void UpdateBoneTfDisc(uint frame_index, std::vector<glm::mat4>& Tfs,
			uint anim_index = 0);

  void UpdateBoneTfCont(float TimeInSeconds, std::vector<glm::mat4>& Tfs,
			uint anim_index = 0);

  glm::mat4 GetGlobalTfInv()
  {
    return _globalInvTf;
  }

  template <typename T>
  static void pprintMat(T mat, int size, std::string name)
  {
    printf("\n%s\n", name.c_str());
    for(int i=0; i< size; i++)
      {
      for (int j=0; j< size; j++)
	printf("%8.5f ", mat[j][i]);
      
      printf("\n");
      }
  }

private:

  struct MeshEntry
  {
    MeshEntry()
      :numIdx(0),baseVtx(0),baseIdx(0){}

    unsigned int numIdx;
    unsigned int baseVtx;
    unsigned int baseIdx;
    //    unsigned int MaterialIndex;
  };

  struct BoneTf
  {
    glm::mat4 Offset;
    glm::mat4 FinalTf;

    BoneTf()
    {
      Offset  = glm::mat4(0.0f);
      FinalTf = glm::mat4(1.0f);
    }
  };

  struct VtxBoneInfo
  {
    unsigned int ID[NUM_BONES_PER_VERTEX];
    float  W[NUM_BONES_PER_VERTEX];

    VtxBoneInfo()
    {
      Reset();
    }
    
    void Reset(){memset(ID, 0, sizeof(ID)); memset(W,  0, sizeof(W));} 

    void AddBoneData(unsigned int BoneID, float weight)
    {
      for(unsigned int i=0; i< NUM_BONES_PER_VERTEX; i++)
	{
	  if (W[i] == 0.0f)
	    {
	      ID[i] = BoneID;
	      W[i] = weight;
	      return;
	  }
	}
      
      //      printf("Error, more than 4 weights per vertex\n");
      weight /= NUM_BONES_PER_VERTEX;
      for(unsigned int i=0; i< NUM_BONES_PER_VERTEX; i++)
	{
	  W[i] += weight;
      	}
      return;
      // should never arrive here (More bones than we have space for
      
      assert(0);
    }
  };

  enum VB_TYPES {
    INDEX_BUFFER,
    POS_VB,
    NORMAL_VB,
    TEXCOORD_VB,
    BONE_VB,
    NUM_VBs      
  };

private:

  /************ Initialization functions **************/
  void Clear();
  bool InitFromScene(const aiScene* pScene, const char* Filename);
  bool InitMesh(unsigned int Index, const aiMesh* paiMesh,
		std::vector<float>& pos, std::vector<float>& nrm,
		std::vector<float>& tex, std::vector<VtxBoneInfo>& bones,
		std::vector<unsigned int>& idx);

  void LoadBones(unsigned int Index, const aiMesh* pMesh,
		 std::vector<VtxBoneInfo>& bones);

  //  bool InitMaterials(const aiScene* pScene, const char* Filename);
  //  void ClearAllTexture();

  /************ Animation Functions *************/
  void ReadNodeHeirarchyDisc(unsigned int frame_index, 
			     const aiNode* pNode, 
			     const glm::mat4& ParentTf,
			     unsigned int anim_index);

  void ReadNodeHeirarchyCont(float AnimationTime, 
			     const aiNode* pNode, 
			     const glm::mat4& ParentTf,
			     unsigned int anim_index);

  const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, 
				 const std::string& NodeName);

  void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
  void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
  void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
  uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
  uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
  uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);

  /************ Properties ***********/
  GLuint _VAO;
  GLuint _Buffers[NUM_VBs];

  std::vector<MeshEntry> _entries;

  std::vector<std::map<std::string, const aiNodeAnim*> > _channels;
  //  std::vector<Texture*> _Textures;

  std::map<std::string, unsigned int> _b2i_map;
  unsigned int _numBone;

  std::vector<BoneTf> _boneTfs;

  glm::mat4 _globalInvTf;

  const aiScene* _pSceneMesh;
  //  const aiScene* _pSceneAnim;
  Assimp::Importer _ipMesh;
  //  Assimp::Importer _ipAnim;

};

#endif

/*
struct Vertex
{
  float px, py, pz;
  float tu, tv;
  float nx, ny, nz;

  Vertex(float x,float y,float z,
	 float u,float v,
	 float nx, float ny,float nz)
    :px(x),py(y),pz(z),
     tu(u),tv(v),
     nx(nx),ny(ny),nz(nz){}
};
*/

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

#include "BvhLoader.hpp"
#include "math_utils.hpp"

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

  bool LoadColladaMesh(const char* path);

  const BvhSkeleton& GetSkeleton(){return _bvhSkel;};

  void BindBones(GLuint programID);

  void RenderModel();

  void UpdateBoneWithTr(
		  const Triplet& offset,
		  const std::vector<Triplet>& frame,
		  const std::vector<uint>& bone2Chnl);

  // This function can be called directly if the root doesn't move
  void UpdateBoneHeirarchy(
		  uint idx, const std::vector<Triplet>& frame,
		  const std::vector<uint>& bone2Chnl, const glm::mat4& parentTf);

  void UpdateBoneTfDisc(uint frame_index);

  void ReadNodeHeirarchyDisc(
		  	  unsigned int frame_index,
		  	  const aiNode* pNode,
		  	  const glm::mat4& ParentTf);

  glm::mat4& GetGlobalTfInv()
  {
    return _globalInvTf;
  }

  const uint GetNumTfs(){return _numBone;};
  glm::mat4& GetBoneTf(int i){ return _boneTfs.at(i).FinalTf;};
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

  bool InitMesh(
		  unsigned int Index, const aiMesh* paiMesh,
		  std::vector<float>& pos, std::vector<float>& nrm,
		  std::vector<float>& tex, std::vector<VtxBoneInfo>& bones,
		  std::vector<unsigned int>& idx);

  void LoadBones(
		  unsigned int Index, const aiMesh* pMesh,
		  std::vector<VtxBoneInfo>& bones);

  //  bool InitMaterials(const aiScene* pScene, const char* Filename);
  //  void ClearAllTexture();

  void BuildSkelFromAnim(std::map<std::string, const aiNodeAnim*>& _channels);

  int ReadSkelHeirarchy(const aiNode* pNode, unsigned int parentIdx,
		  std::map<std::string, const aiNodeAnim*>& _channels);
  /************ Properties ***********/
  GLuint _VAO;
  GLuint _Buffers[NUM_VBs];

  unsigned int _numBone;

  std::vector<MeshEntry> _entries;

  std::map<std::string, unsigned int> _bone2TfIdx;

  std::vector<BoneTf> _boneTfs;

  glm::mat4 _globalInvTf;

  std::map<std::string, const aiNodeAnim*> _channels;

  BvhSkeleton _bvhSkel;
  const aiScene* _pSceneMesh;
  Assimp::Importer _ipMesh;
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

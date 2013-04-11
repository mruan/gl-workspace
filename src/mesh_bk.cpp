#include <assert.h>
#include <stdio.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.hpp"

//Row major to column major
void CopyaiMat(const aiMatrix4x4& from, glm::mat4 &to) 
{
  to[0][0] = from.a1; to[1][0] = from.a2;
  to[2][0] = from.a3; to[3][0] = from.a4;
  to[0][1] = from.b1; to[1][1] = from.b2;
  to[2][1] = from.b3; to[3][1] = from.b4;
  to[0][2] = from.c1; to[1][2] = from.c2;
  to[2][2] = from.c3; to[3][2] = from.c4;
  to[0][3] = from.d1; to[1][3] = from.d2;
  to[2][3] = from.d3; to[3][3] = from.d4;
}

/**************************
Mesh::Class
**************************/
Mesh::Mesh()
{
  _VAO = 0;
  memset(_Buffers, 0, sizeof(_Buffers));
  _numBone = 0;
  _pSceneMesh = NULL;
}

Mesh::~Mesh()
{
  Clear();
}

void Mesh::Clear()
{
  /*
   for (unsigned int i = 0 ; i < _textures.size() ; i++) 
     {
       if(_textures[i])
	 {
	   delete _texture[i];
	   _textures[i] = 0;
	 }
     }
  */
    if (_Buffers[0] != 0)
      glDeleteBuffers(sizeof(_Buffers)/sizeof(_Buffers[0]), _Buffers);
       
    if (_VAO != 0) 
      {
        glDeleteVertexArrays(1, &_VAO);
        _VAO = 0;
      }
}

bool Mesh::LoadMesh(const char* path)
{
  // Do a clean load
  Clear();

  // Create the VAO
  glGenVertexArrays(1, &_VAO);
  glBindVertexArray(_VAO);
  
  // Create the buffers for the vertices:
  glGenBuffers(sizeof(_Buffers)/sizeof(_Buffers[0]), _Buffers);

  bool ret = false;

  int flag = aiProcess_Triangulate | aiProcess_GenSmoothNormals;// | aiProcess_FlipUV;
  _ipMesh.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
  _pSceneMesh = _ipMesh.ReadFile(path, flag);

  if (_pSceneMesh)
    {
      CopyaiMat(_pSceneMesh->mRootNode->mTransformation, _globalInvTf);
      _globalInvTf = glm::affineInverse(_globalInvTf);

      pprintMat(_globalInvTf, 4, "_globalInvTf");

      ret = InitFromScene(_pSceneMesh, path);
      printf("Loaded Mesh file '%s'\n", path);
    }
  else
    printf("Error parsing file '%s': '%s'\n", path, _ipMesh.GetErrorString());


  // Make sure the VAO is not changed from outside
  glBindVertexArray(0);
  return ret;
}

bool Mesh::InitFromScene(const aiScene* pScene, const char* filename)
{
  _entries.resize(pScene->mNumMeshes);
  _channels.resize(pScene->mNumAnimations);
  //_textures.resize(pScene->mNumMaterials);

  std::vector<float> pos, nrm, tex;
  std::vector<VtxBoneInfo> bones;
  std::vector<unsigned int> idx;
  
  unsigned int NumVertices = 0;
  unsigned int NumIndices  = 0;

  // Count the number of vertices and indices
  for (unsigned int i=0; i< _entries.size(); i++)
    {
      //      _entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
      _entries[i].numIdx = pScene->mMeshes[i]->mNumFaces*3;
      _entries[i].baseVtx= NumVertices;
      _entries[i].baseIdx= NumIndices;

      printf("entry %d with %d indices\n", i, _entries[i].numIdx);

      NumVertices += pScene->mMeshes[i]->mNumVertices;
      NumIndices  += _entries[i].numIdx;
    }

  // reserve space for faster push_back()
  pos.reserve(3*NumVertices);
  nrm.reserve(3*NumVertices);
  tex.reserve(2*NumVertices);
  bones.resize(NumVertices);
  //  idx.reserve(NumIndices);

  // Initialize the meshes one at time
  for(unsigned int i=0; i< _entries.size(); i++)
    InitMesh(i, pScene->mMeshes[i], pos, nrm, tex, bones, idx);

  // Initialize the anims
  printf("%d animations loaded\n", pScene->mNumAnimations);
  for(unsigned int i=0; i< _channels.size(); i++)
    {
      aiAnimation* pAnim = pScene->mAnimations[i];
      printf("Name: %s, %d bones\n", pAnim->mName.C_Str(), pAnim->mNumChannels);
      for(unsigned int j=0; j< pAnim->mNumChannels; j++)
	{
	  _channels[i][std::string(pAnim->mChannels[j]->mNodeName.data)] = pAnim->mChannels[j];
	  printf("%-2d NodeName: %15s ", j+1, pAnim->mChannels[j]->mNodeName.data);
	  printf("nS:%3d nT:%3d nR:%3d\n", pAnim->mChannels[j]->mNumScalingKeys,
		 pAnim->mChannels[j]->mNumPositionKeys,pAnim->mChannels[j]->mNumRotationKeys);
	}
    }
  // TODO: init material here

  // Generate and populate the buffers with vertex attributes and indices
  glBindBuffer(GL_ARRAY_BUFFER, _Buffers[POS_VB]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*pos.size(), &pos[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, _Buffers[TEXCOORD_VB]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*tex.size(), &tex[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, _Buffers[NORMAL_VB]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*nrm.size(), &nrm[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer(GL_ARRAY_BUFFER, _Buffers[BONE_VB]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(bones[0])*bones.size(), &bones[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(3); // ID
  glVertexAttribIPointer(3, NUM_BONES_PER_VERTEX, GL_INT, sizeof(VtxBoneInfo), 0);
  glEnableVertexAttribArray(4); // Weights
  //  const int offset = sizeof(float)*NUM_BONES_PER_VERTEX;
  glVertexAttribPointer(4, NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(VtxBoneInfo), 
		        (const void*) 16);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _Buffers[INDEX_BUFFER]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*idx.size(), &idx[0], GL_STATIC_DRAW); 

  return (glGetError() == GL_NO_ERROR);
}

bool Mesh::InitMesh(unsigned int Index, const aiMesh* paiMesh,
		    std::vector<float>& pos, 
		    std::vector<float>& nrm,
		    std::vector<float>& tex,
		    std::vector<VtxBoneInfo>& bones,
		    std::vector<unsigned int>& idx)
{
  const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

  for(unsigned int i=0; i< paiMesh->mNumVertices; i++)
    {
      const aiVector3D* pPos = &(paiMesh->mVertices[i]);
      const aiVector3D* pNor = &(paiMesh->mNormals[i]);
      const aiVector3D* pTex = paiMesh->HasTextureCoords(0)?
	&(paiMesh->mTextureCoords[0][i]): &zero3D;

      pos.push_back(pPos->x); pos.push_back(pPos->y); pos.push_back(pPos->z);
      nrm.push_back(pNor->x); nrm.push_back(pNor->y); nrm.push_back(pNor->z);
      tex.push_back(pTex->x); tex.push_back(pTex->y);
    }

  if (paiMesh->HasBones())
    LoadBones(Index, paiMesh, bones);
  else
    printf("Mesh does not contain bones\n");

  /*
  printf("Vertices (size=%u)\n", vtx.size());
  for(unsigned int i=0; i<vtx.size(); i++)
    printf("%6.4f %6.4f %6.4f\n", vtx[i].px, vtx[i].py, vtx[i].pz);
  */

  for(unsigned int i=0; i<paiMesh->mNumFaces; i++)
    {
      aiFace& f = paiMesh->mFaces[i];
      assert(f.mNumIndices ==3);
      idx.push_back(f.mIndices[0]);
      idx.push_back(f.mIndices[1]);
      idx.push_back(f.mIndices[2]);
    }
  return true;
}

void Mesh::LoadBones(unsigned int Index, const aiMesh* pMesh,
		     std::vector<VtxBoneInfo>& bones)
{
  printf("Mesh %d contains %d bones\n", Index, pMesh->mNumBones);
  for (unsigned int i=0; i < pMesh->mNumBones; i++)
    {
      unsigned int BoneIndex =0;
      std::string BoneName(pMesh->mBones[i]->mName.data);

      if(_b2i_map.find(BoneName)== _b2i_map.end())
	{
	  // Allocate an index for a new bone
	  BoneIndex = _numBone;
	  _numBone++;
	  BoneTf bt;
	  _boneTfs.push_back(bt);
	  CopyaiMat(pMesh->mBones[i]->mOffsetMatrix,
		    _boneTfs[BoneIndex].Offset);
	  _b2i_map[BoneName] = BoneIndex;
	}
      else
	BoneIndex = _b2i_map[BoneName];

      printf("%-2d BoneName: %15s, nweights=%d\n", i+1, BoneName.c_str(), pMesh->mBones[i]->mNumWeights);
      for (unsigned int j=0; j< pMesh->mBones[i]->mNumWeights; j++)
	{
	  unsigned int Vid = _entries[Index].baseVtx + pMesh->mBones[i]->mWeights[j].mVertexId;
	  float weight = pMesh->mBones[i]->mWeights[j].mWeight;
	  //  printf("vid = %u, weight = %f\n", Vid, weight);
	  bones[Vid].AddBoneData(BoneIndex, weight);
	}
    }

  printf("_numBone = %d\n", _numBone);
  if(_b2i_map.find(std::string("Hips"))==_b2i_map.end())
    {
      BoneTf bt;
      _boneTfs.push_back(bt);
      _b2i_map["Hips"] = _numBone++;
      printf("%-2d BoneName: Hips, nweights=0\n", _numBone);
    }
}

void Mesh::RenderModel()
{
  glBindVertexArray(_VAO);

  for(unsigned int i=0; i< _entries.size(); i++)
    {
      //      glDrawElements(GL_TRIANGLES, _entries[i].numIdx, GL_UNSIGNED_INT, 0);
      
      glDrawElementsBaseVertex(GL_TRIANGLES, 
                               _entries[i].numIdx, 
			       GL_UNSIGNED_INT,
		       	       (void*)(sizeof(unsigned int) * _entries[i].baseIdx), 
			       _entries[i].baseVtx);

    }

  glBindVertexArray(0);
}

/**********************************************************************************
Snapshot mode, give animation and frame indices directly
 *********************************************************************************/
void Mesh::UpdateBoneTfDisc(uint frame_index, 
			    std::vector<glm::mat4>& Tfs,
			    uint anim_index)
{
  aiNode* pNode =  _pSceneMesh->mRootNode;
  std::string nodeName (pNode->mName.data);
  while (_b2i_map.find(nodeName) == _b2i_map.end())
    {
      pNode = pNode->mChildren[0];
      //      printf("Root Node: %s\n", pNode->mName.data);
      nodeName = std::string(pNode->mName.data);
    }
  
  glm::mat4 identity = glm::mat4(1.0f);
  ReadNodeHeirarchyDisc(frame_index, pNode, identity, anim_index);
      
  Tfs.resize(_numBone);

  for (unsigned int i=0; i< _numBone; i++)
    {
      Tfs[i] = _boneTfs[i].FinalTf;
    }
}

void Mesh::ReadNodeHeirarchyDisc(uint frame_index,
				 const aiNode* pNode,
				 const glm::mat4& ParentTf,
				 uint anim_index)
{
  std::string NodeName(pNode->mName.data);
  glm::mat4 NodeTf;
  CopyaiMat(pNode->mTransformation, NodeTf);

  if(_b2i_map.find(NodeName) == _b2i_map.end())
    {
      printf("Node: %15s\n", NodeName.c_str());
      return;  // bone does not exist
    }
  const aiNodeAnim* pNodeAnim = _channels[anim_index][NodeName];

  // assume all transform types share the same amount of keys
  frame_index = frame_index % pNodeAnim->mNumPositionKeys; 
  const aiVector3D&   sc = pNodeAnim->mScalingKeys[frame_index].mValue;
  const aiQuaternion& qt = pNodeAnim->mRotationKeys[frame_index].mValue;
  const aiVector3D&   tr = pNodeAnim->mPositionKeys[frame_index].mValue;

  glm::mat4 ScMat=glm::scale(glm::mat4(1.0f),
			     glm::vec3(sc.x, sc.y, sc.z));

  glm::mat4 RotMat;
  CopyaiMat(aiMatrix4x4(qt.GetMatrix()), RotMat);

  glm::mat4 TranMat = glm::translate(glm::mat4(1.0f),
				     glm::vec3(tr.x, tr.y, tr.z));

  NodeTf = TranMat * RotMat * ScMat;

  glm::mat4 GlobalTf = ParentTf * NodeTf;
  unsigned int BoneIdx = _b2i_map[NodeName];
  _boneTfs[BoneIdx].FinalTf= _globalInvTf * GlobalTf * _boneTfs[BoneIdx].Offset;

  // For debugging: print joint position in 3D:
  printf("%-15s %7.4f %7.4f %7.4f\n", pNode->mName.data, 
	 _boneTfs[BoneIdx].FinalTf[3][0], _boneTfs[BoneIdx].FinalTf[3][1], _boneTfs[BoneIdx].FinalTf[3][2]);

  // recursively update the children bones
  for(unsigned int i=0; i< pNode->mNumChildren; i++)
    ReadNodeHeirarchyDisc(frame_index, pNode->mChildren[i], GlobalTf, anim_index);
}

/**********************************************************************************
Animating the model
(Animation mode, with linear interpolations)
**********************************************************************************/
void Mesh::UpdateBoneTfCont(float TimeInSeconds, 
			    std::vector<glm::mat4>& Tfs,
			    unsigned int anim_index)
{
  float TPS = (float)(_pSceneMesh->mAnimations[0]->mTicksPerSecond);
  float TimeInTicks = TimeInSeconds * TPS;
  
  float AnimTime = fmod(TimeInTicks, (float)_pSceneMesh->mAnimations[0]->mDuration);
      
  // Find the first non-meta data node
  aiNode* pNode =  _pSceneMesh->mRootNode;
  std::string nodeName (pNode->mName.data);
  while (_b2i_map.find(nodeName) == _b2i_map.end())
    {
      pNode = pNode->mChildren[0];
      //      printf("Root Node: %s\n", pNode->mName.data);
      nodeName = std::string(pNode->mName.data);
    }
  
  glm::mat4 identity = glm::mat4(1.0f);
  ReadNodeHeirarchyCont(AnimTime, pNode, identity, anim_index);
      
  Tfs.resize(_numBone);

  for (unsigned int i=0; i< _numBone; i++)
    {
      Tfs[i] = _boneTfs[i].FinalTf;
    }
}

void Mesh::ReadNodeHeirarchyCont(float AnimationTime, 
				 const aiNode* pNode, 
				 const glm::mat4& ParentTf,
				 unsigned int anim_index)
{
  std::string NodeName(pNode->mName.data);
  glm::mat4 NodeTf;
  CopyaiMat(pNode->mTransformation, NodeTf);

  // NOTE: the skeleton must use the same naming convention used by the .bvh files
  if(_b2i_map.find(NodeName) == _b2i_map.end())
    {
      printf("Node: %15s\n", NodeName.c_str());
      return;  // bone does not exist
    }
  //  const aiNodeAnim* pNodeAnim=FindNodeAnim(_pSceneMesh->mAnimations[anim_index],NodeName);
  const aiNodeAnim* pNodeAnim = _channels[anim_index][NodeName];

  if(pNodeAnim)
    {
      // Interpolate scaling and generate scaling transformation matrix
      aiVector3D sc;
      CalcInterpolatedScaling(sc, AnimationTime, pNodeAnim);
      glm::mat4 ScMat=glm::scale(glm::mat4(1.0f),
				 glm::vec3(sc.x, sc.y, sc.z));

      //     printf("Node name: %s: Sc: %f %f %f\n", NodeName.c_str(), sc.x, sc.y, sc.z);

      aiQuaternion quat;
      CalcInterpolatedRotation(quat, AnimationTime, pNodeAnim);
      glm::mat4 RotMat;
      CopyaiMat(aiMatrix4x4(quat.GetMatrix()), RotMat);
      
      aiVector3D tran;
      CalcInterpolatedPosition(tran, AnimationTime, pNodeAnim);
      glm::mat4 TranMat = glm::translate(glm::mat4(1.0f),
					 glm::vec3(tran.x, tran.y, tran.z));

      NodeTf = TranMat*RotMat*ScMat;
    }
  
  glm::mat4 GlobalTf = ParentTf * NodeTf;
  unsigned int BoneIdx = _b2i_map[NodeName];
  _boneTfs[BoneIdx].FinalTf= _globalInvTf * GlobalTf * _boneTfs[BoneIdx].Offset;

  for(unsigned int i=0; i< pNode->mNumChildren; i++)
    {
      ReadNodeHeirarchyCont(AnimationTime, pNode->mChildren[i], GlobalTf, anim_index);
    }
}

const aiNodeAnim* Mesh::FindNodeAnim(const aiAnimation* pAnimation, 
				     const std::string& NodeName)
{
  for(unsigned int i=0; i< pAnimation->mNumChannels; i++)
    {
      const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

      //      printf("C: %s <> N: %s\n", pNodeAnim->mNodeName.data, NodeName.c_str());
      if(std::string(pNodeAnim->mNodeName.data) == NodeName)
	{
	  //	  printf("Anim Node name: %s\n", NodeName.c_str());
	  return pNodeAnim;
	}
    }
  return NULL;
}

uint Mesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{    
    for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }
    
    assert(0);

    return 0;
}


uint Mesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }
    
    assert(0);

    return 0;
}


uint Mesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);
    
    for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }
    
    assert(0);

    return 0;
}


void Mesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }
            
    uint PositionIndex = FindPosition(AnimationTime, pNodeAnim);
    uint NextPositionIndex = (PositionIndex + 1);
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime 
			    - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    Factor = Factor>0.0f? (Factor<1.0f? Factor: 1.0f) : 0.0f;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}


void Mesh::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }
    
    uint RotationIndex = FindRotation(AnimationTime, pNodeAnim);
    uint NextRotationIndex = (RotationIndex + 1);
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime 
			    - pNodeAnim->mRotationKeys[RotationIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    Factor = Factor>0.0f? (Factor<1.0f? Factor: 1.0f) : 0.0f;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;    
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out = Out.Normalize();
}


void Mesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
    uint NextScalingIndex = (ScalingIndex + 1);
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime 
			    - pNodeAnim->mScalingKeys[ScalingIndex].mTime);

    float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    Factor = Factor>0.0f? (Factor<1.0f? Factor: 1.0f) : 0.0f;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}
/*
void Mesh::ClearAllTexture()
{
  for(unsigned int i=0; i<_Textures.size(); i++)
    {
      if (_Textures[i] != NULL)
	{
	  delete _Texture[i];
	  _Texture[i] = NULL;
	}
    }
}
*/

#include <assert.h>
#include <stdio.h>

#include <glm/gtc/quaternion.hpp>
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

bool Mesh::LoadColladaMesh(const char* path)
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
	//_channels.resize(pScene->mNumAnimations);

	aiAnimation* pAnim = pScene->mAnimations[0];
	printf("Name: %s, %d bones\n", pAnim->mName.C_Str(), pAnim->mNumChannels);

	for(unsigned int j=0; j< pAnim->mNumChannels; j++)
	{
		_channels[std::string(pAnim->mChannels[j]->mNodeName.data)] = pAnim->mChannels[j];
		printf("%-2d NodeName: %15s ", j+1, pAnim->mChannels[j]->mNodeName.data);
		printf("nS:%3d nT:%3d nR:%3d\n", pAnim->mChannels[j]->mNumScalingKeys,
				pAnim->mChannels[j]->mNumPositionKeys,pAnim->mChannels[j]->mNumRotationKeys);
	}

	// Build a skeleton heirarchy from the animation
	BuildSkelFromAnim(_channels);

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

		if(_bone2TfIdx.find(BoneName)== _bone2TfIdx.end())
		{
			// Allocate an index for a new bone
			BoneIndex = _numBone;
			_numBone++;
			BoneTf bt;
			_boneTfs.push_back(bt);
			CopyaiMat(pMesh->mBones[i]->mOffsetMatrix,
					_boneTfs[BoneIndex].Offset);
			_bone2TfIdx[BoneName] = BoneIndex;
		}
		else
			BoneIndex = _bone2TfIdx[BoneName];

		printf("%-2d BoneName: %15s, nweights=%d %f %f %f\n", i+1, BoneName.c_str(), pMesh->mBones[i]->mNumWeights,
				_boneTfs[BoneIndex].Offset[3][0], _boneTfs[BoneIndex].Offset[3][1],_boneTfs[BoneIndex].Offset[3][2]);
		for (unsigned int j=0; j< pMesh->mBones[i]->mNumWeights; j++)
		{
			unsigned int Vid = _entries[Index].baseVtx + pMesh->mBones[i]->mWeights[j].mVertexId;
			float weight = pMesh->mBones[i]->mWeights[j].mWeight;
			//  printf("vid = %u, weight = %f\n", Vid, weight);
			bones[Vid].AddBoneData(BoneIndex, weight);
		}
	}

	printf("_numBone = %d\n", _numBone);
	if(_bone2TfIdx.find(std::string("Hips"))==_bone2TfIdx.end())
	{
		BoneTf bt;
		_boneTfs.push_back(bt);
		_bone2TfIdx["Hips"] = _numBone++;
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

void Mesh::BuildSkelFromAnim(std::map<std::string, const aiNodeAnim*>& channels)
{
	aiNode* pNode =  _pSceneMesh->mRootNode;
	std::string nodeName (pNode->mName.data);
	while (_bone2TfIdx.find(nodeName) == _bone2TfIdx.end())
	{
		pNode = pNode->mChildren[0];
//      printf("Root Node: %s\n", pNode->mName.data);
		nodeName = std::string(pNode->mName.data);
	}

	// Build a skeleton from the root
	_bvhSkel.reserve(channels.size());
	ReadSkelHeirarchy(pNode, -1, channels);
}

int Mesh::ReadSkelHeirarchy(const aiNode* pNode, unsigned int parentIdx,
		std::map<std::string, const aiNodeAnim*>& channels)
{
	BoneNode newBone(parentIdx, pNode->mName.data);
	const aiNodeAnim* pNodeAnim = channels[newBone.name];
	const aiVector3D& tr = pNodeAnim->mPositionKeys[0].mValue;

	// Add the offset (well, probably should've been called something else)
	newBone.offset.x = tr.x;
	newBone.offset.y = tr.y;
	newBone.offset.z = tr.z;

	// Add the new node into the skeleton
	int myIdx = _bvhSkel.mBones();
	_bvhSkel.addBone(newBone);

//	printf("%2d Node %-15s added to the skeleton\n", myIdx, _bvhSkel[myIdx].name.c_str());
	std::vector<int>& myChildren = _bvhSkel[myIdx].childIdx;
	myChildren.reserve(pNode->mNumChildren);
	for(unsigned int i=0; i< pNode->mNumChildren; i++)
		myChildren.push_back(ReadSkelHeirarchy(pNode->mChildren[i], myIdx, channels));

	return myIdx;
}

void Mesh::UpdateBoneTfDisc(uint frame_index)
{
	aiNode* pNode =  _pSceneMesh->mRootNode;
	std::string nodeName (pNode->mName.data);
	while (_bone2TfIdx.find(nodeName) == _bone2TfIdx.end())
	{
		pNode = pNode->mChildren[0];
		//      printf("Root Node: %s\n", pNode->mName.data);
		nodeName = std::string(pNode->mName.data);
	}

	glm::mat4 identity = glm::mat4(1.0f);
	ReadNodeHeirarchyDisc(frame_index, pNode, identity);
}

void Mesh::ReadNodeHeirarchyDisc(uint frame_index,
		const aiNode* pNode,
		const glm::mat4& ParentTf)
{
	std::string NodeName(pNode->mName.data);
	glm::mat4 NodeTf;
	CopyaiMat(pNode->mTransformation, NodeTf);

	if(_bone2TfIdx.find(NodeName) == _bone2TfIdx.end())
	{
		printf("Node: %15s\n", NodeName.c_str());
		return;  // bone does not exist
	}

	const aiNodeAnim* pNodeAnim = _channels[NodeName];

	// assume all transform types share the same amount of keys
	frame_index = frame_index % pNodeAnim->mNumPositionKeys;
//	const aiVector3D&   sc = pNodeAnim->mScalingKeys[frame_index].mValue;
	const aiQuaternion& qt = pNodeAnim->mRotationKeys[frame_index].mValue;
	const aiVector3D&   tr = pNodeAnim->mPositionKeys[frame_index].mValue;

//	glm::mat4 ScMat=glm::scale(glm::mat4(1.0f),
//			glm::vec3(sc.x, sc.y, sc.z));

	glm::mat4 RotMat;
	CopyaiMat(aiMatrix4x4(qt.GetMatrix()), RotMat);

//	glm::quat Qt = glm::quat_cast(RotMat);
	Triplet euler = rot2eularZYX(RotMat);

	glm::mat4 TranMat = glm::translate(glm::mat4(1.0f),
			glm::vec3(tr.x, tr.y, tr.z));

//	NodeTf = TranMat * RotMat;// * ScMat;

	glm::mat4 GlobalTf = ParentTf * TranMat * RotMat;
	unsigned int BoneIdx = _bone2TfIdx[NodeName];
	_boneTfs[BoneIdx].FinalTf= _globalInvTf * GlobalTf * _boneTfs[BoneIdx].Offset;

//	pprintMat(_boneTfs[BoneIdx].FinalTf, 4, NodeName);
//	pprintMat(TranMat, 4, "tran");
//	pprintMat(RotMat, 4, "rot");

	// For debugging: print Qt and tr:
	printf("Name: %-13s, Tr: %8.5f %8.5f %8.5f, Euler:%8.5f %8.5f %8.5f\n",
			NodeName.c_str(), tr.x, tr.y, tr.z, euler.x, euler.y, euler.z);

	// For debugging: print joint position in 3D:
//	printf("%-15s %7.4f %7.4f %7.4f\n", pNode->mName.data,
//			_boneTfs[BoneIdx].FinalTf[3][0], _boneTfs[BoneIdx].FinalTf[3][1], _boneTfs[BoneIdx].FinalTf[3][2]);

	// recursively update the children bones
	for(unsigned int i=0; i< pNode->mNumChildren; i++)
		ReadNodeHeirarchyDisc(frame_index, pNode->mChildren[i], GlobalTf);
}

void Mesh::UpdateBoneWithTr(
		const Triplet& offset, const std::vector<Triplet>& frame,
		const std::vector<uint>& bone2Chnl)
{
	// Update the root motion
	const float scale = 0.07f;
	glm::mat4 TranMat = glm::translate(glm::mat4(1.0f),
			scale*glm::vec3(offset.x, offset.y, offset.z));

	const Triplet& eular = frame[bone2Chnl[0]];
	glm::mat4 RotMat  = glm::mat4(1.0f);

	// GLM uses degrees unless GLM_FORCE_RADIANS is defined.
	RotMat = glm::rotate(RotMat, eular.z, glm::vec3(0.0f, 0.0f, 1.0f));
	RotMat = glm::rotate(RotMat, eular.y, glm::vec3(0.0f, 1.0f, 0.0f));
	RotMat = glm::rotate(RotMat, eular.x, glm::vec3(1.0f, 0.0f, 0.0f));
//	pprintMat(RotMat, 3, "rotation");

	glm::mat4 GlobalTf = TranMat * RotMat;
	unsigned int BoneIdx = _bone2TfIdx[_bvhSkel[0].name];

	_boneTfs[BoneIdx].FinalTf = _globalInvTf * GlobalTf * _boneTfs[BoneIdx].Offset;

	pprintMat(_boneTfs[BoneIdx].FinalTf, 4, _bvhSkel[0].name);
	pprintMat(TranMat, 4, "tran");
	pprintMat(RotMat, 4, "rot");

	// For debugging: print Qt and tr:
	printf("Name: %-13s, Tr: %8.5f %8.5f %8.5f, Euler: %8.5f %8.5f %8.5f\n",
			_bvhSkel[0].name.c_str(), scale*offset.x, scale*offset.y, scale*offset.z,
			eular.z, eular.y, eular.x);
	// Then propagate down the tree
	for(unsigned int i=0; i< _bvhSkel[0].childIdx.size(); i++)
		UpdateBoneHeirarchy(_bvhSkel[0].childIdx[i], frame, bone2Chnl, GlobalTf);
}

// This function can be called directly if the root doesn't move
void Mesh::UpdateBoneHeirarchy(
		uint idx, const std::vector<Triplet>& frame,
		const std::vector<uint>& bone2Chnl, const glm::mat4& parentTf)
{
	const Triplet& offset = _bvhSkel[idx].offset;
	glm::mat4 TranMat = glm::translate(glm::mat4(1.0f), glm::vec3(offset.x, offset.y, offset.z));

	const Triplet& eular = frame[bone2Chnl[idx]];
	glm::mat4 RotMat  = glm::mat4(1.0f);
	RotMat = glm::rotate(RotMat, eular.z, glm::vec3(0.0f, 0.0f, 1.0f));
	RotMat = glm::rotate(RotMat, eular.y, glm::vec3(0.0f, 1.0f, 0.0f));
	RotMat = glm::rotate(RotMat, eular.x, glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 GlobalTf = parentTf * TranMat * RotMat;
	unsigned int BoneIdx = _bone2TfIdx[_bvhSkel[0].name];

	_boneTfs[BoneIdx].FinalTf= _globalInvTf * GlobalTf * _boneTfs[BoneIdx].Offset;

	pprintMat(_boneTfs[BoneIdx].FinalTf, 4, _bvhSkel[idx].name);
	pprintMat(TranMat, 4, "tran");
	pprintMat(RotMat, 4, "rot");

	printf("Name: %-13s, Tr: %8.5f %8.5f %8.5f, Euler: %8.5f %8.5f %8.5f\n",
			_bvhSkel[idx].name.c_str(),
			offset.x, offset.y, offset.z, eular.z, eular.y, eular.x);

	for(unsigned int i=0; i< _bvhSkel[idx].childIdx.size(); i++)
		UpdateBoneHeirarchy(_bvhSkel[idx].childIdx[i], frame, bone2Chnl, GlobalTf);
}

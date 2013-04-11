#include <Importer.hpp>
#include <matrix4x4.h>
//#include <vector3.h>
#include <vector>
#include <map>

#include <scene.h>
#include <stdio.h>

using namespace std;

aiMatrix4x4 globalInvTf;
vector<aiMatrix4x4> baseTfs;
map<string, int> name2idx;
//vector<pair<string, aiMatrix4x4> > worldTfs;
// Printing helper functions
void dump_4x4Tf(const aiMatrix4x4& mat)
{
	printf("%8.5f %8.5f %8.5f %8.5f\n", mat.a1, mat.a2, mat.a3, mat.a4);
	printf("%8.5f %8.5f %8.5f %8.5f\n", mat.b1, mat.b2, mat.b3, mat.b4);
	printf("%8.5f %8.5f %8.5f %8.5f\n", mat.c1, mat.c2, mat.c3, mat.c4);
	printf("%8.5f %8.5f %8.5f %8.5f\n", mat.d1, mat.d2, mat.d3, mat.d4);
}


void dump_ScQtTr(const aiVector3D& sc, const aiQuaternion& qt, const aiVector3D& tr)
{
	printf("sc: %4.2f %4.2f %4.2f qt: %7.4f %7.4f %7.4f %7.4f tr: %7.4f %7.4f %7.4f\n",
			sc.x, sc.y, sc.z, qt.w, qt.x, qt.y, qt.z, tr.x, tr.y, tr.z);
//	printf("%7.4f %7.4f %7.4f\n", tr.x, tr.y, tr.z);
}

void dump_QuatLoc(const aiMatrix4x4& mat)
{
	aiVector3D sc;
	aiVector3D tr;
	aiQuaternion qt;
	mat.Decompose(sc, qt, tr);
	dump_ScQtTr(sc, qt, tr);
}

// Dump two separate parts that involve bone transformation
void dump_meshInfo(const aiMesh* pMesh)
{
	baseTfs.reserve(pMesh->mNumBones);
	for (unsigned int i=0; i< pMesh->mNumBones; i++)
	{
		string boneName(pMesh->mBones[i]->mName.data);
		// add new offset into baseTfs
		baseTfs.push_back(pMesh->mBones[i]->mOffsetMatrix);
		// also build up the look up table
		name2idx[boneName] = i;

		printf("[%-2d]%-13s ", i, pMesh->mBones[i]->mName.data);
		dump_QuatLoc(pMesh->mBones[i]->mOffsetMatrix);
//		dump_4x4Tf(pMesh->mBones[i]->mOffsetMatrix);
	}
	printf("\n");
}

void ReadNodeHeirarchy(const aiNode* pNode, const aiAnimation* pAnim,
						  const aiMatrix4x4& piTf, int frameIdx)
{
	std::string NodeName(pNode->mName.data);
	if( name2idx.find(NodeName) == name2idx.end())
		return;

	int channelIdx = name2idx[NodeName];

	const aiNodeAnim* pNodeAnim = pAnim->mChannels[channelIdx];

	int numRt = pAnim->mChannels[channelIdx]->mNumRotationKeys;
	int numSc = pAnim->mChannels[channelIdx]->mNumScalingKeys;
	int numTr = pAnim->mChannels[channelIdx]->mNumPositionKeys;

	const aiVector3D&   sc = pNodeAnim->mScalingKeys[frameIdx%numSc].mValue;
	const aiQuaternion& qt = pNodeAnim->mRotationKeys[frameIdx%numRt].mValue;
	const aiVector3D&   tr = pNodeAnim->mPositionKeys[frameIdx%numTr].mValue;

	aiMatrix4x4 ScMat;
	aiMatrix4x4::Scaling(sc, ScMat);

	aiMatrix4x4 QtMat(qt.GetMatrix());

	aiMatrix4x4 TrMat;
	aiMatrix4x4::Translation(tr, TrMat);

	// This is what is passed to the children:
	aiMatrix4x4 jointTf = TrMat * QtMat;
	aiMatrix4x4 globalTf = piTf * jointTf;//* TrMat * QtMat * ScMat;

	// This is what we see in world frame
//	aiMatrix4x4 worldTf = globalInvTf * globalTf * baseTfs[channelIdx];

	printf("[%2d]%-13s ", channelIdx, NodeName.c_str());
	dump_QuatLoc(globalTf);

	// Recursively build the graph
	for(unsigned int i=0; i< pNode->mNumChildren; i++)
	{
		ReadNodeHeirarchy(pNode->mChildren[i], pAnim, globalTf, frameIdx);
	}
}

void dump_animGlobalInfo(const aiScene* pScene)
{
	// sometimes the root node isn't the right root skeletal node...
	aiNode* pNode = pScene->mRootNode;
	std::string nodeName(pNode->mName.data);

	while(name2idx.find(nodeName) == name2idx.end())
	{
		pNode = pNode->mChildren[0];
		nodeName = string(pNode->mName.data);
	}

	const aiAnimation* pAnim = pScene->mAnimations[0];
	aiMatrix4x4 I(1.0f, 0.0f, 0.0f, 0.0f,
				  0.0f, 1.0f, 0.0f, 0.0f,
				  0.0f, 0.0f, 1.0f, 0.0f,
				  0.0f, 0.0f, 0.0f, 1.0f);


	// for all frames:
	for(unsigned int j=0; j<pAnim->mChannels[0]->mNumRotationKeys; j++)
	{
		printf("\n[[Frame %2d]]] \n", j);
		ReadNodeHeirarchy(pNode, pAnim, I, j);
	}
}

void dump_animInfo(const aiScene* pScene)
{
	// for each joint
	const aiAnimation* pAnim = pScene->mAnimations[0];
	for(unsigned int j=0; j<pAnim->mNumChannels; j++)
	{
		const aiNodeAnim* pNodeAnim = pAnim->mChannels[j];
		printf("Node: %-s\n", pNodeAnim->mNodeName.data);

		// for each frame...
		int numKey = pAnim->mChannels[0]->mNumRotationKeys;
		int numSc  = pAnim->mChannels[0]->mNumScalingKeys;
		int numTr  = pAnim->mChannels[0]->mNumScalingKeys;
		for(int k=0; k <numKey; k++)
		{
			const aiVector3D&   sc = pNodeAnim->mScalingKeys[k%numSc].mValue;
			const aiQuaternion& qt = pNodeAnim->mRotationKeys[k].mValue;
			const aiVector3D&   tr = pNodeAnim->mPositionKeys[k%numTr].mValue;
			dump_ScQtTr(sc, qt, tr);
		}
	}
}

// I need a function to dump the information once I load the scene
void dump_scene(const aiScene* pScene)
{
	// dump _globalInvTf
	printf("_globalInvTf\n");
	globalInvTf = pScene->mRootNode->mTransformation.Inverse();
	dump_4x4Tf(pScene->mRootNode->mTransformation);

	// I want to dump skeleton and animation
	const aiMesh* pMesh = pScene->mMeshes[0];
	if (pMesh->HasBones())
		dump_meshInfo(pMesh);

	if (pScene->HasAnimations())
		dump_animGlobalInfo(pScene);
}

int main(int argc, char* argv[])
{
	Assimp::Importer ip;
	const aiScene* pScene = ip.ReadFile(argv[1], 0x8);

	if (pScene ==0)
	{
		printf("Error, cannot load file %s\n", argv[1]);
		return -1;
	}
	dump_scene(pScene);
	return 0;
}



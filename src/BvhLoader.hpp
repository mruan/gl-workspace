
#ifndef BVHLOADER_HPP
#define BVHLOADER_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#include "math_utils.hpp"

class BoneNode
{
	// Note: I don't care about the exact offset, because
	// the definition for a particular skeleton/mesh is defined in dae file
public:
	BoneNode(const BoneNode& bn)
	:parentIdx(bn.parentIdx),name(bn.name),offset(bn.offset),childIdx(bn.childIdx){};
	BoneNode(int idx, std::string bn):parentIdx(idx),name(bn){};
	BoneNode(int idx, const char* bn):parentIdx(idx),name(bn){};

	int parentIdx;
	std::string name;
	Triplet offset;
	std::vector<int> childIdx;
};

class BvhSkeleton
{
	friend class BvhLoader;

public:
    BoneNode& operator [](int i){return _skeleton[i];};
	bool ParseSkeleton(FILE*& fd);

	int mBones(){return _skeleton.size();};
	void addBone(BoneNode& b){_skeleton.push_back(b);};
	void reserve(int n){_skeleton.reserve(n);};

private:
	int ParseNode(FILE*& fd, const int parentIdx, char* buffer);
	Triplet ReadOffset(FILE*&, char*);
	int GetNumChannels(FILE*&, char*);

	std::vector<BoneNode> _skeleton;
};

class BvhAnimation
{
	friend class BvhLoader;
public:
	BvhAnimation(unsigned int skip_rate=1):_skip_frame(skip_rate){};

	void ParseAnimation(FILE*& fd, const int numJoints, std::vector<bool>& mask);

	const Triplet& GetRootTraj(size_t i)const {return _root_traj[i % _root_traj.size()];};
	const std::vector<Triplet>& GetFrame(size_t i)const {return _frames[i % _frames.size()];};
private:
	unsigned int _skip_frame;
	float _sample_rate;
	std::map<std::string, int> _boneDict;
	std::vector<Triplet> _root_traj;
	std::vector<std::vector<Triplet> > _frames;
};

class BvhLoader
{
public:
	//  ~BvhLoader();
//	bool LoadBvhSkeleton(const char* filepath);
	bool LoadBvhMotion(
			const char* filepath, const BvhSkeleton& ref_skel,
			std::vector<unsigned int>& dict, int skip=1);

	const BvhAnimation& getAnimation(){return _anim;};
	const BvhSkeleton& getSkeleton(){return _skel;};

private:
	void MakeMasknMap(
			std::vector<bool>& mask, const std::vector<BoneNode>& ref_skel,
			std::vector<unsigned int>& dict);

	BvhAnimation _anim;
	BvhSkeleton  _skel;
};

#endif

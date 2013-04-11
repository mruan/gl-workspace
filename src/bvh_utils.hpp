
#ifndef BVH_UTILS
#define BVH_UTILS

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map

#include "math_utils.hpp"

class BoneNode
{
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
public:
  BoneNode& operator [](int i){return _skeleton[i];};
  bool LoadfromFD(FILE*& fd);
  
  int mBones(){return _skeleton.size();};
  void addBone(BoneNode& b){_skeleton.push_back(b);};
  void reserve(int n){_skeleton.reserve(n);};
  
private:
  int ParseNode(FILE*& fd, const int parentIdx, char* buffer);
  Triplet ReadOffset(FILE*&, char*);
  int GetNumChannels(FILE*&, char*);
  
  std::vector<BoneNode> _skeleton;
};

class BvhFrame
{
  BvhFrame(int numJoints){joint_angles.researve(numJoints);};
public:
  Triplet root_offset;
  std::vector<Triplet> joint_angles;
};

class BvhAnimation
{
public:
  BvhAnimation(unsigned int skip_rate=1):_skip_frame(skip_rate){};
  
  void ParseAnimation(FILE*& fd);
  
  void WriteAnimation(FILE*& fd, std::vector<int>& dict);
  //  const Triplet& GetRootTraj(size_t i)const {return _root_traj[i % _root_traj.size()];};
  //  const std::vector<Triplet>& GetFrame(size_t i)const {return _frames[i % _frames.size()];};
private:
  std::vector<BvhFrame> _frames;
};

class BvhLoader
{
public:
  BvhLoader();

  void LoadnDump(FILE*& load_fd, FILE*& dump_fd, const BvhSkeleton* ref_skel=0); 
private:

  void MakeDict(const BvhSkeleton& ref_skel);
  unsigned int _skip_frame;
  unsigned int _downsample_rate;
  std::vector<int> _boneDict;
  BvhSkeleton _skel;
}
